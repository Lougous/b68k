//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/kernel - VFS task (virtual file system)
//

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
static void _vfs_mount (pid_t from, message_t *msg);
static void _vfs_open (pid_t from, message_t *msg);
static void _vfs_getdents (pid_t from, message_t *msg);
static void _vfs_close (pid_t from, message_t *msg);
static void _vfs_read (pid_t from, message_t *msg);
static void _vfs_write (pid_t from, message_t *msg);
static void _vfs_lseek (pid_t from, message_t *msg);
static void _vfs_fork (pid_t from, message_t *msg);
static void _vfs_chdir (pid_t from, message_t *msg);
static void _vfs_ioctl (pid_t from, message_t *msg);
static void _vfs_getcwd (pid_t from, message_t *msg);
static void _vfs_mkdir (pid_t from, message_t *msg);
static void _vfs_kill (pid_t from, message_t *msg);

#define MASK 0x1f

const message_handler_pfc_t _vfs_handlers[MASK+1] = {
  [MASK & MOUNT]    = (message_handler_pfc_t)_vfs_mount,
  [MASK & OPEN]     = (message_handler_pfc_t)_vfs_open,
  [MASK & GETDENTS] = (message_handler_pfc_t)_vfs_getdents,
  [MASK & CLOSE]    = (message_handler_pfc_t)_vfs_close,
  [MASK & READ]     = (message_handler_pfc_t)_vfs_read,
  [MASK & WRITE]    = (message_handler_pfc_t)_vfs_write,
  [MASK & LSEEK]    = (message_handler_pfc_t)_vfs_lseek,
  [MASK & VFS_FORK] = (message_handler_pfc_t)_vfs_fork,
  [MASK & CHDIR]    = (message_handler_pfc_t)_vfs_chdir,
  [MASK & IOCTL]    = (message_handler_pfc_t)_vfs_ioctl,
  [MASK & GETCWD]   = (message_handler_pfc_t)_vfs_getcwd,
  [MASK & MKDIR]    = (message_handler_pfc_t)_vfs_mkdir,
  [MASK & VFS_KILL] = (message_handler_pfc_t)_vfs_kill
};


  /*
#define MOUNT     0x0101
#define OPEN      0x0102
#define GETDENTS  0x0103
#define CLOSE     0x0104
#define READ      0x0105
#define WRITE     0x0106
#define LSEEK     0x0107
#define VFS_FORK  0x0108  // reserved to kernel
#define CHDIR     0x0109
#define IOCTL     0x0110
#define GETCWD    0x0111
#define MKDIR     0x0112
#define VFS_KILL  0x0113  // reserved to kernel
  */

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

    if (((msg.type & ~MASK) == 0x100) && (_vfs_handlers[msg.type & MASK])) {
      _vfs_handlers[msg.type & MASK](from, &msg);
    } else {
      // illegal message
      // TODO: kill process ? send signal ?
      K_PRINTF(2, "vfs: error: unknown command %Xh from %i\n", msg.type, from);
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

static void _vfs_mount (pid_t from, message_t *msg)
{
  message_t resp = { .body.u32 = 0 };  // OK
  
  /* allowed for kernel tasks only */
  // TODO : allow for root user
  if (proc_get_uid(from) != PROC_UID_KERNEL) {
    resp.body.u32 = -EPERM;
  }

  //_msg_out.body.u32 = _vfs_mount(msg.body.mount.dev,
  //				 msg.body.mount.path_to,
  //				 msg.body.mount.type);
  char *dev = msg->body.mount.dev;
  const char *path_to = msg->body.mount.path_to;
  char *type = msg->body.mount.type;
  struct _vfs_mount_t *pmnt = _vfs_alloc_mount();
  dev_t *pdev = dev_get(dev);

  // find free mount point
  if (pmnt == 0) {
    resp.body.u32 = -ENOMEM;  // out of resources
  }
  
  if (pdev == 0) {
    // not a registered device
    if (dev && (strcmp(dev, "devfs") == 0) &&
	type && (strcmp(type, "devfs") == 0)) {
      // devfs
      pmnt->path = (char *)path_to;  // TODO strdup
      pmnt->type = FS_TYPE_DEV;
      pmnt->ops  = &_fs_devfs_fsops;

      K_PRINTF(2, "vfs: mounting devfs to %s\n", path_to);      
    } else {
      resp.body.u32 = -ENODEV;  // unknown device
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

	K_PRINTF(2, "vfs: mounting %s, type fat\n", dev);
	resp.body.u32 = fat_mount(&pmnt->ctx);
      }
    else if (strcmp(type, "rfs") == 0) {
      // RFS
      pmnt->path = (char *)path_to;  // TODO strdup
      pmnt->type = FS_TYPE_RFS;
      pmnt->ops  = &_fs_rfs_fsops;
      pmnt->ctx.rfs.dev = pdev;
      
      K_PRINTF(2, "vfs: mounting %s, type rfs\n", dev);
      resp.body.u32 = rfs_mount(&pmnt->ctx);
    } else {
      resp.body.u32 = -EINVAL;  // bad type
    }
  }

  send(from, &resp);
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

static int _open (pid_t pid, const char *pathname, int flags)
{
  u32_t mnt;
  struct _vfs_mount_t *pmnt = 0;
  int ret;
  int fd = _vfs_alloc_fd(pid);


  if (fd < 0) {
    return -ENFILE;
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
    return -ENOENT;  // no match
  }
  
  pfd->oflags   = flags;
  pfd->ops      = pmnt->ops;
  pfd->fctx.ctx = &pmnt->ctx;

  ret = pmnt->ops->open(&pfd->fctx, pathname + strlen(pmnt->path), flags);

  if (ret < 0) {
    _vfs_free_fd(pid, fd);
    return ret;
  }
  
  K_PRINTF(3, "vfs: open %s (%i) by %u\n", pathname, fd, pid);

  return fd;
}

static void _vfs_open (pid_t from, message_t *msg)
{
  message_t resp;
  //K_PRINTF(2, "vfs: OPEN: %Xh\n", (mem_va_t)msg.body.open.pathname);
	
  int len = msg->body.open.len;
  mem_pa_t pathname = va_to_pa(from, (mem_va_t)msg->body.open.pathname, len);
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
    
    resp.body.s32 = _open (from,
			   (char *)fullname,
			   msg->body.open.flags);
  } else {
    resp.body.s32 = -EFAULT;
  }

  send(from, &resp);
}

static void _vfs_getdents (pid_t from, message_t *msg)
{
  message_t resp;

  /* limit buffer size to limit service duration */
  unsigned int count = msg->body.getdents.count;
  count = count > K_MAX_DIRENT_LEN ? K_MAX_DIRENT_LEN : count;
  
  mem_pa_t buf = va_to_pa(from, (mem_va_t)msg->body.getdents.dirp, count);
  
  if (buf) {
    struct _vfs_vnode_t *pfd = _vfs_vnode_from_fd(from, msg->body.getdents.fd);

    if (pfd) {
      if (pfd->oflags == O_DIRECTORY) {
	//printf("vfs_getdents ->\n");
	int i =  pfd->ops->getdents(&pfd->fctx, (struct dirent *)buf, count);
	//printf("vfs_getdents <- %i\n", i);
	resp.body.s32 = i;
      } else {
	resp.body.s32 = -ENOTDIR;
      }
    } else {
      resp.body.s32 = -EBADF;
    }

  } else {
    resp.body.s32 = -EACCESS;
  }
  
  send(from, &resp);
}

static int _close(pid_t pid, int fd)
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

  return -EBADF;
}

static void _vfs_close (pid_t from, message_t *msg)
{
  message_t resp;
  resp.body.s32 = _close(from, msg->body.close.fd);
  send(from, &resp);
}

static void _vfs_kill (pid_t from, message_t *msg)
{
  /* allowed for kernel tasks only */
  if (proc_get_uid(from) == PROC_UID_KERNEL) {
    pid_t pid = msg->body.u32;
    int fd;
    
    /* close any file for killed process */
    for (fd = 0; fd < K_PROC_FD_COUNT; fd++) {
      if (_vfs_proc_table[pid].fd_table[fd]) {
	_close(pid, fd);
	//_vfs_proc_table[pid].fd_table[fd] = 0;
      }
    }
    
    /* default current directory */
    _vfs_proc_table[pid].cdir[0] = '/';
    _vfs_proc_table[pid].cdir[1] = 0;
    
  }

  /* acknowledge (no argument) */
  send(from, &_msg_out);
}

static void _vfs_read (pid_t from, message_t *msg)
{
  message_t resp;
  
  /* limit buffer size to limit service duration */
  unsigned int count = msg->body.read.count;
  count = count > K_MAX_READWRITE_LEN ? K_MAX_READWRITE_LEN : count;
  
  mem_pa_t buf = va_to_pa(from, (mem_va_t)msg->body.read.buf, count);
  
  if (buf) {
    struct _vfs_vnode_t *pfd = _vfs_vnode_from_fd(from, msg->body.read.fd);

    if (pfd) {
      if (pfd->oflags != O_DIRECTORY) {
	resp.body.s32 = pfd->ops->read(&pfd->fctx, (char *)buf, count);
      } else {
	resp.body.s32 = -EISDIR;
      }
    } else {
      resp.body.s32 = -EBADF;
    }
  
  } else {
    resp.body.s32 = -EACCESS;
  }
  
  send(from, &resp);
}

static void _vfs_write (pid_t from, message_t *msg)
{
  message_t resp;
  
  /* limit buffer size to limit service duration */
  unsigned int count = msg->body.write.count;
  count = count > K_MAX_READWRITE_LEN ? K_MAX_READWRITE_LEN : count;
	
  mem_pa_t buf = va_to_pa(from, (mem_va_t)msg->body.write.buf, count);

  if (buf) {
    struct _vfs_vnode_t *pfd = _vfs_vnode_from_fd(from, msg->body.write.fd);

    if (pfd) {
      if (pfd->oflags != O_DIRECTORY) {
	resp.body.s32 = pfd->ops->write(&pfd->fctx, (char *)buf, count);
      } else {
	resp.body.s32 = -EISDIR;
      }
    } else {
      resp.body.s32 = -EBADF;
    }

  } else {
    resp.body.s32 = -EACCESS;
  }

  send(from, &resp);
}

static void _vfs_lseek (pid_t from, message_t *msg)
{
  message_t resp;
  struct _vfs_vnode_t *pfd = _vfs_vnode_from_fd(from, msg->body.lseek.fd);

  if (! pfd) {
    resp.body.s32 = -EBADF;
  } else {
    resp.body.s32 = pfd->ops->lseek(&pfd->fctx, msg->body.lseek.offset, msg->body.lseek.whence);
  }
  
  send(from, &resp);
}

static void _vfs_ioctl (pid_t from, message_t *msg)
{
  message_t resp;
  struct _vfs_vnode_t *pfd = _vfs_vnode_from_fd(from, msg->body.ioctl.fd);

  if (! pfd) {
    resp.body.s32 = -EBADF;
  } else {
    resp.body.s32 = pfd->ops->ioctl(&pfd->fctx, from, msg->body.ioctl.request, (mem_va_t)msg->body.ioctl.ptr);
  }
  
  send(from, &resp);
}

static void _vfs_chdir (pid_t from, message_t *msg)
{
  message_t resp;
  u16_t len = msg->body.chdir.len;
	
  if (! len) {
    resp.body.s32 = -EINVAL;
    goto vfs_chdir_exit;
  }
  
  mem_pa_t path = va_to_pa(from, (mem_va_t)msg->body.chdir.path, len);

  if (! path) {
    // bad address
    K_PRINTF(2, "VFS: chdir: bad address %Xh\n", (u32_t)msg->body.chdir.path);
    resp.body.s32 = -EFAULT;
    goto vfs_chdir_exit;
  }
    
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
    int fd = _open(from, fullname, O_DIRECTORY);
    
    K_PRINTF(2, "VFS: chdir: try %s\n", fullname);
    
    if (fd >= 0) {
      // good !
      _close(from, fd);
      
      strcpy(_vfs_proc_table[from].cdir, fullname);
      
      resp.body.s32 = 0;
    } else {
      K_PRINTF(2, "VFS: chdir: no dir %s\n", s);
      resp.body.s32 = -ENOTDIR;
    }
  } else {
    K_PRINTF(2, "VFS: chdir: bad string (len=%i)\n", len);
    resp.body.s32 = -EINVAL;
  }

 vfs_chdir_exit:
  send(from, &resp);
}

static void _vfs_getcwd (pid_t from, message_t *msg)
{
  message_t resp;
  u16_t size = msg->body.getcwd.size;
  
  mem_pa_t buf = va_to_pa(from, (mem_va_t)msg->body.getcwd.buf, size);
  
  if (! buf) {
    _msg_out.body.s32 = -EFAULT;
  } else if (size > strlen(_vfs_proc_table[from].cdir)) {
    strcpy((char *)buf, _vfs_proc_table[from].cdir);
    _msg_out.body.s32 = 0;
  } else {
    _msg_out.body.s32 = -ERANGE;
  }
  
  send(from, &resp);
}

static void _vfs_mkdir (pid_t pid, message_t *msg)
{
  message_t resp;
  mkdir_msg_body_t *msg_mkdir = &msg->body.mkdir;

  // check with extra byte for terminating null char
  mem_pa_t pathname = va_to_pa(pid, (mem_va_t)msg_mkdir->path, msg_mkdir->len+1);

  if (! pathname) {
    resp.body.s32 = -EFAULT;
    goto vfs_mkdir_exit;
  }

  if (((char *)pathname)[msg_mkdir->len]) {
    // must terminate with a null char
    resp.body.s32 = -EINVAL;
    goto vfs_mkdir_exit;
  }
  
  u32_t mnt;
  struct _vfs_mount_t *pmnt = 0;
  
  // find mount point this file belongs to
  for (mnt = 0; mnt < K_MOUNT_COUNT; mnt++) {
    if (_vfs_mounts[mnt].type &&
	(strncmp(_vfs_mounts[mnt].path, (const char *)pathname, strlen(_vfs_mounts[mnt].path)) == 0)) {
      pmnt = &_vfs_mounts[mnt];
    }    
  }

  if (pmnt == 0) {
    resp.body.s32 = -ENOENT;  // no match
    goto vfs_mkdir_exit;
  }
  
  resp.body.s32 = pmnt->ops->mkdir(&pmnt->ctx, (const char *)pathname + strlen(pmnt->path), msg_mkdir->flags);

  K_PRINTF(3, "vfs: mkdir %s by %u\n", pathname, pid);

 vfs_mkdir_exit:
  send(pid, &resp);
}

static void _vfs_fork (pid_t from, message_t *msg)
{
  /* allowed for kernel tasks only */
  if (proc_get_uid(from) == PROC_UID_KERNEL) {
    pid_t ppid = msg->body.vfs_fork.parent;
    pid_t child = msg->body.vfs_fork.child;
    
    /* copy links to file descriptors */
    memcpy((void *)&_vfs_proc_table[child], (void *)&_vfs_proc_table[ppid], sizeof(_vfs_proc_table[child]));
    
    K_PRINTF(3, "vfs: FORK: %i:%s %i:%s\n", ppid, _vfs_proc_table[ppid].cdir, child, _vfs_proc_table[child].cdir);

    /* increment open counter */
    int fd;

    for (fd = 0; fd < K_PROC_FD_COUNT; fd++) {
      struct _vfs_vnode_t *pfd = _vfs_proc_table[ppid].fd_table[fd];
      
      if (pfd) {
	pfd->count++;
      }
    }
    
    /* acknowledge (no argument) */
    send(from, &_msg_out);
  }
  // TODO : else
}
