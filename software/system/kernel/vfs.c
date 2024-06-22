
#include <types.h>
#include <stddef.h>
#include <fcntl.h>
#include <dirent.h>
#include <syscall.h>
#include <errno.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "mem.h"
#include "proc.h"
#include "dev.h"
#include "fs.h"
#include "msg.h"
#include "lock.h"
#include "debug.h"

/* mount points */
struct _vfs_mount_t {
  char *path;
  u8_t type;
  const fs_operations_t *ops;
  fs_context_t ctx;
};

struct _vfs_mount_t _vfs_mounts[K_MOUNT_COUNT];

/* vnodes */
struct _vfs_vnode_t {
  struct _vfs_vnode_t *next;
  int oflags;
  u16_t count;
  const fs_operations_t *ops;
  fs_file_context_t fctx;
};

struct _vfs_vnode_t _vfs_vnode_table[K_VNODE_COUNT];

#define _K_VNODE_NULL  ((struct _vfs_vnode_t *) 0)

/* vnode to root directory */
#define _K_VNODE_ROOT  0

struct _vfs_vnode_t *_vfs_free_vnode_list;

/* processes data */
struct _vfs_proc_t {
  //root directory
  // current directory
  char cdir[K_MAX_DIRNAME_LEN];
  // file descriptors
  struct _vfs_vnode_t *fd_table[K_PROC_FD_COUNT];
};

struct _vfs_proc_t _vfs_proc_table[K_PROC_COUNT];

// prototypes
static u32_t _vfs_mount (char *dev, const char *path_to, char *type);
static int _vfs_open (pid_t pid, const char *pathname, int flags);
static int _vfs_getdents (pid_t pid, unsigned int fd, struct dirent *dirp, unsigned int count);
static int _vfs_close(pid_t pid, int fd);
static ssize_t _vfs_read(pid_t pid, int fd, void *buf, size_t count);
static ssize_t _vfs_write(pid_t pid, int fd, void *buf, size_t count);
static off_t _vfs_lseek (pid_t pid, int fildes, off_t offset, int whence);
static int _vfs_ioctl(pid_t pid, int fd, int request, mem_va_t ptr);
static int _vfs_mkdir(pid_t pid, mkdir_msg_body_t *msg);
  
static message_t _msg_out;

void vfs_task (void)
{
  u32_t mnt;
  u32_t fd;
  pid_t pid;

  K_PRINTF(3, "vfs: starting task\n");

  /* initialization */
  for (mnt = 0; mnt < K_MOUNT_COUNT; mnt++) {
    _vfs_mounts[mnt].type = 0;
  }

  for (pid = 0; pid < K_PROC_COUNT; pid++) {
    _vfs_proc_table[pid].cdir[0] = '/';
    _vfs_proc_table[pid].cdir[1] = 0;
    
    memset(_vfs_proc_table[pid].fd_table, 0, sizeof(_vfs_proc_table[pid].fd_table));
  }
  
  memset(_vfs_vnode_table, 0, sizeof(_vfs_vnode_table));
  
  for (fd = 0; fd < K_VNODE_COUNT-1; fd++) {
    _vfs_vnode_table[fd].next = &_vfs_vnode_table[fd+1];
  }

  _vfs_vnode_table[fd].next = 0;  /* last */

  _vfs_free_vnode_list = &_vfs_vnode_table[0];

  _msg_out.type = 0;
  
  while (1) {
    static message_t msg;
    
    pid_t from = receive(ANY, &msg);

    switch (msg.type) {
    case KILL:
      /* allowed for kernel tasks only */
      if (proc_get_uid(from) == PROC_UID_KERNEL) {
	pid_t pid = msg.body.u32;
	int fd;

	/* close any file for killed process */
	for (fd = 0; fd < K_PROC_FD_COUNT; fd++) {
	  if (_vfs_proc_table[pid].fd_table[fd]) {
	    _vfs_close(pid, fd);
	    //_vfs_proc_table[pid].fd_table[fd] = 0;
	  }
	}

	/* default current directory */
	_vfs_proc_table[pid].cdir[0] = '/';
	_vfs_proc_table[pid].cdir[1] = 0;

	/* acknowledge (no argument) */
	send(from, &_msg_out);
      }
      break;

    case VFS_FORK:
      /* allowed for kernel tasks only */
      if (proc_get_uid(from) == PROC_UID_KERNEL) {
	pid_t ppid = msg.body.vfs_fork.parent;
	pid_t child = msg.body.vfs_fork.child;

	/* copy links to file descriptors */
	memcpy((void *)&_vfs_proc_table[child], (void *)&_vfs_proc_table[ppid], sizeof(_vfs_proc_table[child]));
	
	int fd;
	
	K_PRINTF(3, "vfs: FORK: %i:%s %i:%s\n", ppid, _vfs_proc_table[ppid].cdir, child, _vfs_proc_table[child].cdir);

	/* increment open counter */
	for (fd = 0; fd < K_PROC_FD_COUNT; fd++) {
	  struct _vfs_vnode_t *pfd = _vfs_proc_table[ppid].fd_table[fd];
	  
	  if (pfd) {
	    pfd->count++;
	  }
	}

	/* acknowledge (no argument) */
	send(from, &_msg_out);
      }
      break;

    case MOUNT:
      /* allowed for kernel tasks only */
      // TODO : allow for root user
      if (proc_get_uid(from) == PROC_UID_KERNEL) {
	_msg_out.body.u32 = _vfs_mount(msg.body.mount.dev,
				       msg.body.mount.path_to,
				       msg.body.mount.type);
	
	send(from, &_msg_out);
      }
      break;

    case OPEN:
      {
	//K_PRINTF(2, "vfs: OPEN: %Xh\n", (mem_va_t)msg.body.open.pathname);
	
	int len = msg.body.open.len;
	mem_pa_t pathname = va_to_pa(from, (mem_va_t)msg.body.open.pathname, len);
	static char fullname[K_MAX_DIRNAME_LEN+K_MAX_FILENAME_LEN+2];
	char *pdst = fullname;
	char *psrc = _vfs_proc_table[from].cdir;

	K_PRINTF(3, "vfs: PID-%i OPEN: [%s]/[%s]\n", from, (char *)_vfs_proc_table[from].cdir, (char *)pathname);

	if (pathname) {
	  if (len && ((char *)pathname)[0] != '/') {
	    /* relative path, concat current directory with path */
	    while (*psrc) {
	      *pdst++ = *psrc++;
	    }

	    *pdst++ = '/';
	  }

	  psrc = (char *)pathname;
	  
	  while (len--) {
	    *pdst++ = *psrc++;
	  }

	  *pdst = 0;

	  K_PRINTF(3, "vfs: OPEN: '%s'\n", (char *)fullname);
	  
	  _msg_out.body.s32 = _vfs_open (from,
					 (char *)fullname,
					 msg.body.open.flags);
	} else {
	  _msg_out.body.s32 = -1;
	}
		  
	send(from, &_msg_out);
      }
      break;

    case CLOSE:
      _msg_out.body.s32 = _vfs_close(from,
				     msg.body.close.fd);
      send(from, &_msg_out);
      break;

    case READ:
      {
	/* limit buffer size to limit service duration */
	unsigned int count = msg.body.read.count;
	count = count > K_MAX_READWRITE_LEN ? K_MAX_READWRITE_LEN : count;
	
	mem_pa_t buf = va_to_pa(from, (mem_va_t)msg.body.read.buf, count);

	if (buf) {
	  _msg_out.body.s32 = _vfs_read (from,
					 msg.body.read.fd,
					 (char *)buf,
					 count);
	  
	} else {
	  _msg_out.body.s32 = -1;
	}

	send(from, &_msg_out);
      }
      break;

    case WRITE:
      {
	/* limit buffer size to limit service duration */
	unsigned int count = msg.body.write.count;
	count = count > K_MAX_READWRITE_LEN ? K_MAX_READWRITE_LEN : count;
	
	mem_pa_t buf = va_to_pa(from, (mem_va_t)msg.body.write.buf, count);

	if (buf) {
	  _msg_out.body.s32 = _vfs_write (from,
					  msg.body.write.fd,
					  (char *)buf,
					  count);
	} else {
	  _msg_out.body.s32 = -1;
	}

	send(from, &_msg_out);
      }
      break;

    case IOCTL:
      {
	_msg_out.body.s32 = _vfs_ioctl(from, msg.body.ioctl.fd, msg.body.ioctl.request, (mem_va_t)msg.body.ioctl.ptr);

	send(from, &_msg_out);
      }
      break;
      
    case LSEEK:
      _msg_out.body.s32 = _vfs_lseek (from,
				      msg.body.lseek.fd,
				      msg.body.lseek.offset,
				      msg.body.lseek.whence);
      send(from, &_msg_out);
      break;

    case GETDENTS:
      {
	/* limit buffer size to limit service duration */
	unsigned int count = msg.body.getdents.count;
	count = count > K_MAX_DIRENT_LEN ? K_MAX_DIRENT_LEN : count;
	
	mem_pa_t buf = va_to_pa(from, (mem_va_t)msg.body.getdents.dirp, count);

	if (buf) {
	  _msg_out.body.s32 = _vfs_getdents (from,
					     msg.body.getdents.fd,
					     (struct dirent *)buf,
					     count);
	} else {
	  _msg_out.body.s32 = -1;
	}
	  
	send(from, &_msg_out);
      }
      break;

    case CHDIR:
      {
	u16_t len = msg.body.chdir.len;
	
	mem_pa_t path = va_to_pa(from, (mem_va_t)msg.body.chdir.path, len);

	_msg_out.body.s32 = -1;  // default: error

	if (len && path) {
	  char *s = (char *)path;
	  len = strnlen(s, len);

	  static char fullname[K_MAX_DIRNAME_LEN+K_MAX_DIRNAME_LEN+2];
	  u16_t clen;

	  if (s[0] == '/') {
	    // absolute path
	    clen = 0;
	  } else {
	    // relative path
	    clen = strlen(_vfs_proc_table[from].cdir);
	    strcpy(fullname, _vfs_proc_table[from].cdir);
	    fullname[clen++] = '/';
	  }

	  if (s[len] == 0 && (len+clen) < K_MAX_DIRNAME_LEN) {
	    // a valid null terminated string
	    strcpy(fullname+clen, s);
	    
	    // try to open it
	    int fd = _vfs_open(from, fullname, O_DIRECTORY);
	  
	    K_PRINTF(2, "VFS: chdir: try %s\n", fullname);
	    
	    if (fd >= 0) {
	      // good !
	      _vfs_close(from, fd);
	      
	      strcpy(_vfs_proc_table[from].cdir, fullname);

	      _msg_out.body.s32 = 0;
	    } else {
	      K_PRINTF(2, "VFS: chdir: no dir %s\n", s);
	    }
	  } else {
	    K_PRINTF(2, "VFS: chdir: bad string (len=%i)\n", len);
	  }
	} else {
	  K_PRINTF(2, "VFS: chdir: bad address %Xh\n", (u32_t)msg.body.chdir.path);
	}


	send(from, &_msg_out);
      }
      break;
      
    case GETCWD:
      {
	u16_t size = msg.body.getcwd.size;
	
	mem_pa_t buf = va_to_pa(from, (mem_va_t)msg.body.getcwd.buf, size);

	_msg_out.body.s32 = -1;  // default: error
	
	if (buf && size > strlen(_vfs_proc_table[from].cdir)) {
	  strcpy((char *)buf, _vfs_proc_table[from].cdir);
	  
	  _msg_out.body.s32 = 0;
	}

	send(from, &_msg_out);
      }
      break;

    case MKDIR:
      {
	_msg_out.body.s32 = _vfs_mkdir(from, &msg.body.mkdir);
	send(from, &_msg_out);
      }
      break;
      
    default:
      // illegal message
      // send back error message ?
      K_PRINTF(2, "vfs: error: unknown command %Xh from %i\n", msg.type, from);
      break;
    }

  }
}

static struct _vfs_mount_t *_vfs_alloc_mount ()
{
  u32_t mnt;

  for (mnt = 0; mnt < K_MOUNT_COUNT; mnt++) {
    if (_vfs_mounts[mnt].type == 0)
      return &_vfs_mounts[mnt];
  }

  // out of memory/resource
  return 0;
}

static void _vfs_free_mount (struct _vfs_mount_t *mnt)
{
  // TODO free(mnt->path);
  mnt->type = 0;
  mnt->ops  = 0;

  return;
}
  
static int _vfs_alloc_fd (pid_t pid)
{
  int fd;
  struct _vfs_vnode_t *pfd = _vfs_free_vnode_list;

  if (pfd) {
    for (fd = 0; fd < K_PROC_FD_COUNT; fd++) {
      if (_vfs_proc_table[pid].fd_table[fd] == 0) {
	/* found free FD for process */
	_vfs_proc_table[pid].fd_table[fd] = pfd;
	pfd->count = 1;
	_vfs_free_vnode_list = _vfs_free_vnode_list->next;
	return fd;
      }
    }
  }
  
  /* out of process resource */
  return -1;
}

static int _vfs_free_fd (pid_t pid, int fd)
{
  struct _vfs_vnode_t *pfd = _vfs_proc_table[pid].fd_table[fd];

  
  if (pfd) {
    pfd->count--;
    
    if (pfd->count == 0) {
      /* no process left, free FD */
      pfd->next = _vfs_free_vnode_list;
      _vfs_free_vnode_list = pfd;
    }

    _vfs_proc_table[pid].fd_table[fd] = 0;

    return 0;
  }
  
  return -1;
}

static u32_t _vfs_mount (char *dev, const char *path_to, char *type)
{
  struct _vfs_mount_t *pmnt = _vfs_alloc_mount();
  dev_t *pdev = dev_get(dev);

  // find free mount point
  if (pmnt == 0) return -1;  // out of resources  
  if (pdev == 0) {
    // not a registered device
    if (dev && (strcmp(dev, "devfs") == 0) &&
	type && (strcmp(type, "devfs") == 0)) {
      // devfs
      pmnt->path = (char *)path_to;  // TODO strdup
      pmnt->type = FS_TYPE_DEV;
      pmnt->ops  = &_fs_devfs_fsops;

      printf("mounting devfs to %s\n", path_to);      
    } else {
      return -2;  // unknown device
    }
  } else {
    // registered device
    if (
	(pdev->attr.attr_type == DEV_ATTR_DISK_PARTITION) &&
	((type == 0 && pdev->attr.partition.type == 6) ||
	 (type && (strcmp(type, "fat") == 0)))
	)
      {
	// FAT partition
	pmnt->path = (char *)path_to;  // TODO strdup
	pmnt->type = FS_TYPE_FAT;
	pmnt->ops  = &_fs_fat_fsops;
	pmnt->ctx.fat.dev         = pdev;
	pmnt->ctx.fat.FirstSector = pdev->attr.partition.sector_start;
	pmnt->ctx.fat.NbSector    = pdev->attr.partition.sector_cnt;

	printf("mounting %s, type fat\n", dev);
	return fat_mount(&pmnt->ctx);
      }
    else if (strcmp(type, "rfs") == 0) {
      // RFS
      pmnt->path = (char *)path_to;  // TODO strdup
      pmnt->type = FS_TYPE_RFS;
      pmnt->ops  = &_fs_rfs_fsops;
      pmnt->ctx.rfs.dev = pdev;
      
      printf("mounting %s, type rfs\n", dev);
      return rfs_mount(&pmnt->ctx);
    } else {
      return -4;  // bad type
    }
  }

  return 0;
}

u32_t vfs_umount (const char *path_to)
{
  u32_t mnt;
  struct _vfs_mount_t *pmnt = 0;

  for (mnt = 0; mnt < K_MOUNT_COUNT; mnt++) {
    
    if (_vfs_mounts[mnt].type && (strcmp(_vfs_mounts[mnt].path, path_to) == 0)) {
      pmnt = &_vfs_mounts[mnt];
      break;
    }    
  }

  if (pmnt) {
    // TODO: check open files ...
    _vfs_free_mount(pmnt);
    return 0;
  }
  else {
    return -1;
  }
}

static struct _vfs_vnode_t *_vfs_vnode_from_fd(pid_t pid, unsigned int fd)
{
  if (fd < K_PROC_FD_COUNT) {
    return _vfs_proc_table[pid].fd_table[fd];
  }

  return 0;
}

static int _vfs_open (pid_t pid, const char *pathname, int flags)
{
  u32_t mnt;
  struct _vfs_mount_t *pmnt = 0;
  int ret;
  int fd = _vfs_alloc_fd(pid);


  if (fd < 0) {
    return -1;
  }
  
  struct _vfs_vnode_t *pfd = _vfs_vnode_from_fd(pid, fd);
  
  // find mount point this file belongs to
  for (mnt = 0; mnt < K_MOUNT_COUNT; mnt++) {

    //if (_vfs_mounts[mnt].type) printf("%s = %s ?\n", _vfs_mounts[mnt].path, pathname);
  
    
    if (_vfs_mounts[mnt].type &&
	(strncmp(_vfs_mounts[mnt].path, pathname, strlen(_vfs_mounts[mnt].path)) == 0)) {
      pmnt = &_vfs_mounts[mnt];
    }    
  }

  if (pmnt == 0) {
    _vfs_free_fd(pid, fd);
    return -1;  // no match
  }
  
  pfd->oflags   = flags;
  pfd->ops      = pmnt->ops;
  pfd->fctx.ctx = &pmnt->ctx;

  ret = pmnt->ops->open(&pfd->fctx, pathname + strlen(pmnt->path), flags);

  if (ret < 0) {
    _vfs_free_fd(pid, fd);
    return -1;
  }
  
  K_PRINTF(3, "vfs: open %s (%i) by %u\n", pathname, fd, pid);

  return fd;
}

static int _vfs_getdents (pid_t pid, unsigned int fd, struct dirent *dirp, unsigned int count)
{
  struct _vfs_vnode_t *pfd = _vfs_vnode_from_fd(pid, fd);

  if (pfd) {
    if (pfd->oflags == O_DIRECTORY) {
      //printf("vfs_getdents ->\n");
      int i =  pfd->ops->getdents(&pfd->fctx, dirp, count);
      //printf("vfs_getdents <- %i\n", i);
      return i;
    } else {
      return -1;
    }
  } else {
    return -1;
  }

  return 0;
 }

static int _vfs_close(pid_t pid, int fd)
{
  struct _vfs_vnode_t *pfd = _vfs_vnode_from_fd(pid, fd);

  // TODO: flush/sync ?
  if (pfd) {
    int i =  pfd->ops->close(&pfd->fctx);

    if (i >= 0) {
      _vfs_free_fd(pid, fd);

      K_PRINTF(3, "vfs: close %u by %u\n", fd, pid);
    }
    
    return i;
  }

  return -1;
}

static ssize_t _vfs_read(pid_t pid, int fd, void *buf, size_t count)
{
  struct _vfs_vnode_t *pfd = _vfs_vnode_from_fd(pid, fd);

  if (pfd) {
    if (pfd->oflags != O_DIRECTORY) {
      return pfd->ops->read(&pfd->fctx, buf, count);
    } else {
      return -1;
    }
  } else {
    return -1;
  }
  
  return 0;
}

static ssize_t _vfs_write(pid_t pid, int fd, void *buf, size_t count)
{
  struct _vfs_vnode_t *pfd = _vfs_vnode_from_fd(pid, fd);

  if (pfd) {
    if (pfd->oflags != O_DIRECTORY) {
      return pfd->ops->write(&pfd->fctx, buf, count);
    } else {
      return -1;
    }
  } else {
    return -1;
  }
  return 0;
}

static off_t _vfs_lseek (pid_t pid, int fildes, off_t offset, int whence)
{
  struct _vfs_vnode_t *pfd = _vfs_vnode_from_fd(pid, fildes);

  if (! pfd) {
    return -1;
  }
  
  return pfd->ops->lseek(&pfd->fctx, offset, whence);
}

static int _vfs_ioctl(pid_t pid, int fd, int request, mem_va_t ptr)
{
  struct _vfs_vnode_t *pfd = _vfs_vnode_from_fd(pid, fd);

  if (! pfd) {
    return -1;
  }

  return pfd->ops->ioctl(&pfd->fctx, pid, request, ptr);
}

static int _vfs_mkdir(pid_t pid, mkdir_msg_body_t *msg_mkdir)
{
  // check with extra byte for terminating null char
  mem_pa_t pathname = va_to_pa(pid, (mem_va_t)msg_mkdir->path, msg_mkdir->len+1);

  if (! pathname) {
    return EFAULT;
  }

  if (((char *)pathname)[msg_mkdir->len]) {
    // must terminate with a null char
    return EINVAL;
  }
  
  u32_t mnt;
  struct _vfs_mount_t *pmnt = 0;
  int ret;
  
  // find mount point this file belongs to
  for (mnt = 0; mnt < K_MOUNT_COUNT; mnt++) {

    //if (_vfs_mounts[mnt].type) printf("%s = %s ?\n", _vfs_mounts[mnt].path, pathname);
  
    
    if (_vfs_mounts[mnt].type &&
	(strncmp(_vfs_mounts[mnt].path, (const char *)pathname, strlen(_vfs_mounts[mnt].path)) == 0)) {
      pmnt = &_vfs_mounts[mnt];
    }    
  }

  if (pmnt == 0) {
    return ENOENT;  // no match
  }
  
  ret = pmnt->ops->mkdir(&pmnt->ctx, (const char *)pathname + strlen(pmnt->path), msg_mkdir->flags);

  K_PRINTF(3, "vfs: mkdir %s by %u\n", pathname, pid);

  return ret;
}
