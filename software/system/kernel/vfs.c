//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/kernel - VFS task (virtual file system)
//
// based on : Vnodes: An Architecture for Multiple File System Types in Sun UNIX
//            S.R. Kleiman / Sun Microsystems
//

#include <stdint.h>
#include <types.h>
#include <stddef.h>
#include <fcntl.h>
#include <dirent.h>
#include <syscall.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "debug.h"
#include "mem.h"
#include "proc.h"
#include "dev.h"
#include "vfs.h"
#include "msg.h"
#include "lock.h"

// mount points
struct vfs _vfs_vfs_table[K_VFS_COUNT];

#define _K_VFS_NULL  ((struct vfs *) 0)

struct vfs *_vfs_free_vfs_list;
struct vfs *_vfs_used_vfs_list;

// list of managed file systems
struct fs_type {
  const char *name;
  int16_t (* mount)(struct vfs *pvfs, dev_t dev);
};

extern int16_t rfs_mount(struct vfs *pvfs, dev_t dev);
extern int16_t devfs_mount(struct vfs *pvfs, dev_t dev);
extern int16_t fat_mount(struct vfs *pvfs, dev_t dev);

static const struct fs_type _vfs_fs_types[] = {
  { .name = "devfs", .mount = devfs_mount },
  { .name = "rfs", .mount = rfs_mount },
  { .name = "fat", .mount = fat_mount }
};

// vnodes pool
struct vnode _vfs_vnode_table[K_VNODE_COUNT];

#define _K_VNODE_NULL  ((struct vnode *) 0)

struct vnode *_vfs_free_vnode_list;

struct vnode *_vfs_vnroot;

/* processes data */
struct _vfs_proc_t {
  //root directory
  // current directory
  struct vnode *cdir;
  // file descriptors (index: file descriptor)
  struct vnode *fd_table[K_PROC_FD_COUNT];
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
  [MASK & MKDIR]    = (message_handler_pfc_t)_vfs_mkdir,
  [MASK & VFS_KILL] = (message_handler_pfc_t)_vfs_kill
};


void vfs_task (void)
{
  u32_t mnt;
  u32_t fd;
  pid_t pid;

  K_PRINTF(3, "vfs: starting task\n");

  // init mounted vfs list
  for (mnt = 0; mnt < K_VFS_COUNT-1; mnt++) {
    _vfs_vfs_table[mnt].vfs_next = &_vfs_vfs_table[mnt+1];
  }

  _vfs_vfs_table[mnt].vfs_next = _K_VFS_NULL;  // last

  _vfs_free_vfs_list = &_vfs_vfs_table[0];
  _vfs_used_vfs_list = _K_VFS_NULL;

  // init processes context
  for (pid = 0; pid < K_PROC_COUNT; pid++) {
    _vfs_proc_table[pid].cdir = _K_VNODE_NULL;
    
    memset(_vfs_proc_table[pid].fd_table, 0, sizeof(_vfs_proc_table[pid].fd_table));
  }

  // vnodes
  memset(_vfs_vnode_table, 0, sizeof(_vfs_vnode_table));
  
  for (fd = 0; fd < K_VNODE_COUNT-1; fd++) {
    // setup linked list
    _vfs_vnode_table[fd].next = &_vfs_vnode_table[fd+1];
  }

  _vfs_vnode_table[fd].next = _K_VNODE_NULL;  // last

  _vfs_free_vnode_list = &_vfs_vnode_table[0];

  _vfs_vnroot = _K_VNODE_NULL;
  
  // main loop
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

static int _vfs_alloc_fd (pid_t pid)
{
  int fd;

  for (fd = 0; fd < K_PROC_FD_COUNT; fd++) {
    if (_vfs_proc_table[pid].fd_table[fd] == 0) {
      /* found free FD for process */
      return fd;
    }
  }
  
  /* out of process resource */
  return -1;
}

static int _vfs_free_fd (pid_t pid, int fd)
{
  _vfs_proc_table[pid].fd_table[fd] = 0;

  return 0;
}

char *_trim_slash(char *str)
{
  while (*str && (*str != '/')) str++;
  while (*str && (*str == '/')) str++;

  return str;
}

// len of first path element in name
u16_t _subpathlen (char *path)
{
  char *at = path;

  while (*at && *at != '/') at++;

  return (u16_t)(at - path);
}
  
int _lookuppn (char *nm, struct vnode **ppv, pid_t pid)
{
  struct vnode *pvn;
  int ret;

  K_PRINTF(3, "vfs: PID-%d: vn_lookup: %s\n", pid, nm);
  
  if (nm[0] == '/') {
    // absolute path
    nm = _trim_slash(nm);
    pvn = _vfs_vnroot;
  } else {
    // relative path
    pvn = _vfs_proc_table[pid].cdir;

    // no current directory
    if (!pvn) {
      pvn = _vfs_vnroot;
    }
  }

  if (!pvn) {
    // likely no root FS mounted yet
    return -ENOENT;
  }

  VN_HOLD(pvn);
  *ppv = pvn;

  while (*nm) {
    K_PRINTF(3, "vfs: PID-%d: vn_lookup: %s\n", pid, nm);
    struct vfs *mounted = pvn->v_vfsmountedhere;

    if (mounted) {
      K_PRINTF(3, "vfs: PID-%d: vn_lookup: change VFS to %Xh\n", pid, mounted);
      K_PRINTF(3, "vfs: PID-%d: vn_lookup: vfs_root %Xh\n", pid, (u32_t)mounted->vfs_op->vfs_root);
      (void)mounted->vfs_op->vfs_root(mounted, ppv);
      VN_RELE(pvn);
      pvn = *ppv;
    }

    // split first path element
    u16_t elen = _subpathlen(nm);
    char cbak = nm[elen];
    nm[elen] = 0;
    
    K_PRINTF(3, "vfs:   path elem: %s (%u)\n", nm, elen);
    ret = pvn->v_op->vn_lookup(pvn, nm, ppv, pid);
    VN_RELE(pvn);
    pvn = *ppv;

    nm[elen] = cbak;
    
    if (ret) return ret;

    // next in name tree
    nm += elen;
    nm = _trim_slash(nm);
  }

  // last vnode mount point ?
  struct vfs *mounted = pvn->v_vfsmountedhere;

  if (mounted) {
    K_PRINTF(3, "vfs: PID-%d: vn_lookup: change VFS to %Xh\n", pid, mounted);
    K_PRINTF(3, "vfs: PID-%d: vn_lookup: vfs_root %Xh\n", pid, (u32_t)mounted->vfs_op->vfs_root);
    (void)mounted->vfs_op->vfs_root(mounted, ppv);
    VN_RELE(pvn);
  }
  
  return 0;
}

#define MOUNT_DEBUG  3

static void _vfs_mount (pid_t pid, message_t *msg)
{
  message_t resp = { .body.u32 = 0 };  // OK
  
  if ((proc_get_uid(pid) != PROC_UID_KERNEL) && (proc_get_uid(pid) != PROC_UID_ROOT)) {
    K_PRINTF(MOUNT_DEBUG, "vfs: PID-%d: vfs_mount: EPERM\n", pid);
    resp.body.u32 = -EPERM;
  }

  // check parameters
  char *path_to = (char *)va_to_pa_str(pid, (mem_va_t)msg->body.mount.path_to, K_MAX_DIRNAME_LEN);
  char *type = (char *)va_to_pa_str(pid, (mem_va_t)msg->body.mount.type, 8);
  char *dev = (char *)va_to_pa_str(pid, (mem_va_t)msg->body.mount.dev, K_MAX_DIRNAME_LEN);

  if ((! path_to) || (! type) || (! dev)) {
    K_PRINTF(MOUNT_DEBUG, "vfs: PID-%d: vfs_mount: EFAULT\n", pid);
    resp.body.u32 = -EFAULT;   // memory fault
    goto _vfs_mount_exit;
  }

  K_PRINTF(MOUNT_DEBUG, "vfs: PID-%d: vfs_mount: %s to %s (%s)\n", pid, dev, path_to, type);

  // get a free VFS
  struct vfs *pvfs = _vfs_free_vfs_list;
  
  if (! pvfs) {
    K_PRINTF(MOUNT_DEBUG, "vfs: PID-%d: vfs_mount: ENOMEM\n", pid);
    resp.body.u32 = -ENOMEM;   // out of resources
    goto _vfs_mount_exit;
  }

  // get device to mount
  dev_t dev_id;

  if (proc_get_uid(pid) == PROC_UID_KERNEL) {
    // kernel specifies device name (root filesystem device cannot be specified as /dev/...)
    struct dev *pdev = dev_get(dev);
    dev_id = pdev->dev_id;
  } else {
    // user specifies special file path (block device)
    // get vnode for device
    struct vnode *pvn_dev;
    resp.body.u32 = _lookuppn(dev, &pvn_dev, pid);

    if (resp.body.u32) {
      K_PRINTF(MOUNT_DEBUG, "vfs: PID-%d: vfs_mount: path_to EACCES\n", pid);
      resp.body.u32 = -EACCES;
      goto _vfs_mount_exit;
    }

    if (pvn_dev->v_type != VBLK) {
      K_PRINTF(MOUNT_DEBUG, "vfs: PID-%d: vfs_mount: path_to ENOTBLK\n", pid);
      resp.body.u32 = -ENOTBLK;
      goto _vfs_mount_exit;
    }

    // get struct dev
    struct stat stats;
    pvn_dev->v_op->vn_getattr(pvn_dev, &stats);

    dev_id = stats.st_rdev;
  }

  int16_t (* pmount)(struct vfs *pvfs, dev_t dev) = 0;

  if (type) {
    int16_t ft;

    for (ft = 0; ft < (sizeof(_vfs_fs_types)/sizeof(struct fs_type)); ft++) {
      if (strcmp(type, _vfs_fs_types[ft].name) == 0) {
	pmount = _vfs_fs_types[ft].mount;
      }
    }
  }

  K_PRINTF(MOUNT_DEBUG, "vfs: PID-%d: vfs_mount: device is %Xh\n", pid, dev_id);

  // unknown type of FS
  if (! pmount) {
    K_PRINTF(MOUNT_DEBUG, "vfs: PID-%d: vfs_mount: ENODEV\n", pid);
    resp.body.u32 = -ENODEV;   // filesystemtype not available in the kernel
    goto _vfs_mount_exit;
  }

  // mount FS
  if (pmount(pvfs, dev_id)) {
    K_PRINTF(MOUNT_DEBUG, "vfs: PID-%d: vfs_mount: EINVAL (VFS mount)\n", pid);
    resp.body.u32 = -EINVAL;   // invalid FS
    goto _vfs_mount_exit;
  }

  // get root vnode
  struct vnode *pvn_root;
  (void)pvfs->vfs_op->vfs_root(pvfs, &pvn_root);

  if (!pvn_root) {
    pvfs->vfs_op->vfs_unmount(pvfs);
    // TODO: might be other cause
    K_PRINTF(MOUNT_DEBUG, "vfs: PID-%d: vfs_mount: EINVAL (root vnode)\n", pid);
    resp.body.u32 = -EINVAL;   // invalid FS
    goto _vfs_mount_exit;
  }

  // target directory
  if (strcmp(path_to, "/") == 0) {
    // mount root
    if (_vfs_used_vfs_list) {
      // root already mounted
      VN_RELE(pvn_root);
      pvfs->vfs_op->vfs_unmount(pvfs);
      K_PRINTF(MOUNT_DEBUG, "vfs: PID-%d: vfs_mount: EBUSY (root)\n", pid);
      resp.body.u32 = -EBUSY;
      goto _vfs_mount_exit;
    }

    pvfs->vfs_vnodecovered = _K_VNODE_NULL;
    _vfs_vnroot = pvn_root;

    goto _vfs_mount_success;
  }

  // get vnode for directory to mount at
  struct vnode *pvn_at;
  resp.body.u32 = _lookuppn(path_to, &pvn_at, pid);

  if (resp.body.u32) {
    VN_RELE(pvn_root);
    pvfs->vfs_op->vfs_unmount(pvfs);
    K_PRINTF(MOUNT_DEBUG, "vfs: PID-%d: vfs_mount: EACCES (path_to)\n", pid);
    resp.body.u32 = -EACCES;
    goto _vfs_mount_exit;
  }

  if (pvn_at->v_type != VDIR) {
    VN_RELE(pvn_root);
    VN_RELE(pvn_at);
    pvfs->vfs_op->vfs_unmount(pvfs);
    resp.body.u32 = -ENOTDIR;
    goto _vfs_mount_exit;
  }

  pvfs->vfs_vnodecovered = pvn_at;

 _vfs_mount_success:
  pvn_at->v_vfsmountedhere = pvfs;
  _vfs_free_vfs_list = _vfs_free_vfs_list->vfs_next;
  pvfs->vfs_next = _vfs_used_vfs_list;
  _vfs_used_vfs_list = pvfs;

  K_PRINTF(MOUNT_DEBUG, "vfs: PID-%d: mounted %s at %s %Xh (%s)\n", pid, dev, path_to, pvfs, type);

 _vfs_mount_exit:
  send(pid, &resp);
}

/*
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
*/

static struct vnode *_vfs_vnode_from_fd(pid_t pid, unsigned int fd)
{
  if (fd < K_PROC_FD_COUNT) {
    return _vfs_proc_table[pid].fd_table[fd];
  }

  return _K_VNODE_NULL;
}

static void _vfs_open (pid_t pid, message_t *msg)
{
  message_t resp;
  int16_t len = msg->body.open.len;
  mem_pa_t pathname = va_to_pa(pid, (mem_va_t)msg->body.open.pathname, len);

  K_PRINTF(3, "vfs: PID-%i OPEN: %s\n", pid, pathname);

  if (! len) {
    K_PRINTF(3, "vfs: PID-%i OPEN: ENOENT\n", pid);
    resp.body.s32 = -ENOENT;
    goto _vfs_open_exit;
  }
  
  if (! pathname) {
    K_PRINTF(3, "vfs: PID-%i OPEN: EFAULT\n", pid);
    resp.body.s32 = -EFAULT;
    goto _vfs_open_exit;
  }

  char *path = (char *)pathname;

  if (path[len]) {
    K_PRINTF(3, "vfs: PID-%i OPEN: ENOENT\n", pid);
    resp.body.s32 = -EINVAL;
    goto _vfs_open_exit;
  }
  
  struct vnode *pvn;
  resp.body.s32 = _lookuppn(path, &pvn, pid);

  if (resp.body.s32) {
    K_PRINTF(3, "vfs: PID-%i OPEN: error %i\n", pid, resp.body.s32);
    goto _vfs_open_exit;
  }
    
  int fd = _vfs_alloc_fd(pid);

  if (fd < 0) {
    K_PRINTF(3, "vfs: PID-%i OPEN: EMFILE\n", pid);
    VN_RELE(pvn);
    resp.body.s32 = -EMFILE;
    goto _vfs_open_exit;
  }
  
  resp.body.s32 = pvn->v_op->vn_open(pvn, msg->body.open.flags, pid);
  
  if (resp.body.s32) {
    K_PRINTF(3, "vfs: PID-%i OPEN: %i\n", pid, resp.body.s32);
    VN_RELE(pvn);
    _vfs_free_fd(pid, fd);
    goto _vfs_open_exit;
  }

  // success
  _vfs_proc_table[pid].fd_table[fd] = pvn;
  
  K_PRINTF(3, "vfs: PID-%i OPEN: fd %i, node %Xh\n", pid, fd, pvn);
  resp.body.s32 = fd;

 _vfs_open_exit:
  send(pid, &resp);
}

static void _vfs_getdents (pid_t pid, message_t *msg)
{
  message_t resp;

  K_PRINTF(3, "vfs: PID-%i GETDENTS: fd %i\n", pid, msg->body.getdents.fd);

  /* limit buffer size to limit service duration */
  unsigned int count = msg->body.getdents.count;
  count = count > K_MAX_DIRENT_LEN ? K_MAX_DIRENT_LEN : count;
  
  mem_pa_t buf = va_to_pa(pid, (mem_va_t)msg->body.getdents.dirp, count);
  
  if (! buf) {
    resp.body.s32 = -EACCES;
    goto _vfs_getdents_exit;
  }

  struct vnode *pvn = _vfs_vnode_from_fd(pid, msg->body.getdents.fd);

  K_PRINTF(3, "vfs: PID-%i GETDENTS: node %Xh\n", pid, (u32_t)pvn);

  if (! pvn) {
    resp.body.s32 = -EBADF;
    goto _vfs_getdents_exit;
  }

  if (pvn->v_type != VDIR) {
    resp.body.s32 = -ENOTDIR;
    goto _vfs_getdents_exit;
  }

  // TODO: check directory is open

  resp.body.s32 = pvn->v_op->vn_getdents(pvn, (char *)buf, count, pid);

  K_PRINTF(3, "vfs: PID-%i GETDENTS: %i\n", pid, resp.body.s32);

 _vfs_getdents_exit:
  send(pid, &resp);
}

/*
static int _close(pid_t pid, int fd)
{

  resp.body.s32 = pvn->ops->close(pvn, pid);

  _vfs_free_fd(pid, fd);
  VN_RELE(pvn);
  
}
*/

static void _vfs_close (pid_t pid, message_t *msg)
{
  message_t resp;
  int fd = msg->body.close.fd;

  // TODO: flush/sync ?
  struct vnode *pvn = _vfs_vnode_from_fd(pid, fd);

  K_PRINTF(3, "vfs: PID-%i CLOSE fd %i, node %Xh\n", pid, fd, (u32_t)pvn);

  if (! pvn) {
    resp.body.s32 = -EBADF;
    goto _vfs_close_exit;
  }

  resp.body.s32 = pvn->v_op->vn_close(pvn, pid);

  VN_RELE(pvn);
  _vfs_free_fd(pid, fd);

 _vfs_close_exit:
  send(pid, &resp);
}

static void _vfs_kill (pid_t pid, message_t *msg)
{
  message_t resp;
  
  K_PRINTF(3, "vfs: PID-%i KILL PID %i\n", pid, msg->body.u32);

  // allowed for kernel tasks only: actual user kill syscall is to send through system task
  if (proc_get_uid(pid) == PROC_UID_KERNEL) {
    pid_t pid = msg->body.u32;
    int fd;
    
    /* close any file for killed process */
    K_PRINTF(3, "     closing files\n");
    
    for (fd = 0; fd < K_PROC_FD_COUNT; fd++) {
      struct vnode *pvn = _vfs_vnode_from_fd(pid, fd);
      
      if (pvn) {
	(void)pvn->v_op->vn_close(pvn, pid);
	VN_RELE(pvn);
	_vfs_free_fd(pid, fd);
      }
    }
    
    // default current directory
    K_PRINTF(3, "     closing current directory\n");
    
    if (_vfs_proc_table[pid].cdir) {
      VN_RELE(_vfs_proc_table[pid].cdir);
      _vfs_proc_table[pid].cdir = _K_VNODE_NULL;
    }
  }

  /* acknowledge (no argument) */
  send(pid, &resp);
}

static void _vfs_read (pid_t pid, message_t *msg)
{
  message_t resp;
  
  /* limit buffer size to limit service duration */
  unsigned int count = msg->body.read.count;
  count = count > K_MAX_READWRITE_LEN ? K_MAX_READWRITE_LEN : count;
  
  mem_pa_t buf = va_to_pa(pid, (mem_va_t)msg->body.read.buf, count);
  int fd = msg->body.read.fd;
  
  if (buf) {
    struct vnode *pvn = _vfs_vnode_from_fd(pid, fd);

    //if (fd || !pid) K_PRINTF(3, "vfs: PID-%i READ: fd %i, node %Xh, count %u\n", pid, fd, (u32_t)pvn, count);

    if (pvn) {
      if (pvn->v_type == VDIR) {
	K_PRINTF(3, "vfs: PID-%i READ: EISDIR\n", pid);
	resp.body.s32 = -EISDIR;
      } else {
	resp.body.s32 = pvn->v_op->vn_read(pvn, (char *)buf, count, pid);
	//if (fd || !pid) K_PRINTF(3, "vfs: PID-%i READ: %u bytes\n", pid, resp.body.s32);
      }
    } else {
      K_PRINTF(3, "vfs: PID-%i READ: EBADF\n", pid);
      resp.body.s32 = -EBADF;
    }
  
  } else {
    K_PRINTF(3, "vfs: PID-%i READ: EACCESS\n", pid);
    resp.body.s32 = -EACCES;
  }
  
  send(pid, &resp);
}

static void _vfs_write (pid_t pid, message_t *msg)
{
  message_t resp;
  
  /* limit buffer size to limit service duration */
  unsigned int count = msg->body.write.count;
  count = count > K_MAX_READWRITE_LEN ? K_MAX_READWRITE_LEN : count;
	
  mem_pa_t buf = va_to_pa(pid, (mem_va_t)msg->body.write.buf, count);
  int fd = msg->body.write.fd;

  if (buf) {
    struct vnode *pvn = _vfs_vnode_from_fd(pid, fd);

    if (pvn) {
      if (pvn->v_type == VDIR) {
	resp.body.s32 = -EISDIR;
      } else {
	resp.body.s32 = pvn->v_op->vn_write(pvn, (char *)buf, count, pid);
      }
    } else {
      resp.body.s32 = -EBADF;
    }

  } else {
    resp.body.s32 = -EACCES;
  }

  send(pid, &resp);
}

static void _vfs_lseek (pid_t pid, message_t *msg)
{
  message_t resp;
  struct vnode *pvn = _vfs_vnode_from_fd(pid, msg->body.lseek.fd);

  if (! pvn) {
    resp.body.s32 = -EBADF;
  } else {
    resp.body.s32 = pvn->v_op->vn_lseek(pvn, msg->body.lseek.offset, msg->body.lseek.whence);
  }
  
  send(pid, &resp);
}

static void _vfs_ioctl (pid_t pid, message_t *msg)
{
  message_t resp;
  struct vnode *pvn = _vfs_vnode_from_fd(pid, msg->body.ioctl.fd);

  if (! pvn) {
    resp.body.s32 = -EBADF;
  } else {
    resp.body.s32 = pvn->v_op->vn_ioctl(pvn, msg->body.ioctl.request, (mem_va_t)msg->body.ioctl.ptr, pid);
  }
  
  send(pid, &resp);
}

static void _vfs_chdir (pid_t pid, message_t *msg)
{
  message_t resp;
  u16_t len = msg->body.chdir.len;  // includes terminating null character
	
  K_PRINTF(3, "vfs: PID-%i CHDIR: %Xh (%u)\n", pid, (mem_va_t)msg->body.chdir.path, len);

  if ((len <= 0) || (len > K_MAX_DIRNAME_LEN)) {
    K_PRINTF(3, "vfs: PID-%i CHDIR: EINVAL\n", pid);
    resp.body.s32 = -EINVAL;
    goto _vfs_chdir_exit;
  }
  
  char *path = (char *)va_to_pa(pid, (mem_va_t)msg->body.chdir.path, len);

  if (! path) {
    // bad address
    K_PRINTF(3, "vfs: PID-%i CHDIR: EFAULT\n", pid);
    resp.body.s32 = -EFAULT;
    goto _vfs_chdir_exit;
  }
    
  if (path[len-1]) {
    K_PRINTF(3, "vfs: PID-%i CHDIR: EINVAL\n", pid);
    resp.body.s32 = -EINVAL;
    goto _vfs_chdir_exit;
  }
  
  struct vnode *pvn;
  resp.body.s32 = _lookuppn(path, &pvn, pid);

  if (resp.body.s32) {
    K_PRINTF(3, "vfs: PID-%i CHDIR: %u\n", pid, resp.body.s32);
    goto _vfs_chdir_exit;
  }
    
  if (pvn->v_type != VDIR) {
    K_PRINTF(3, "vfs: PID-%i CHDIR: ENOTDIR\n", pid);
    resp.body.s32 = -ENOTDIR;
    VN_RELE(pvn);
    goto _vfs_chdir_exit;
  }

  K_PRINTF(3, "vfs: PID-%i CHDIR: %s\n", pid, path);
  _vfs_proc_table[pid].cdir = pvn;

 _vfs_chdir_exit:
  send(pid, &resp);
}

static void _vfs_mkdir (pid_t pid, message_t *msg)
{
  message_t resp;
  mkdir_msg_body_t *msg_mkdir = &msg->body.mkdir;
  int16_t len = msg_mkdir->len;

  K_PRINTF(3, "vfs: PID-%i MKDIR: %Xh\n", pid, (mem_va_t)msg_mkdir->path);

  if (! len) {
    K_PRINTF(3, "vfs: PID-%i MKDIR: ENOENT\n", pid);
    resp.body.s32 = -ENOENT;
    goto _vfs_mkdir_exit;
  }
  
  char *pa_path = (char *)va_to_pa(pid, (mem_va_t)msg_mkdir->path, msg_mkdir->len+1);

  if (! pa_path) {
    K_PRINTF(3, "vfs: PID-%i MKDIR: EFAULT\n", pid);
    resp.body.s32 = -EFAULT;
    goto _vfs_mkdir_exit;
  }

  int16_t ptr = len;
  
  // remove tailing /s
  while (ptr && (pa_path[ptr-1] == '/')) ptr--;

  if (! ptr) {
    // only /s in name
    K_PRINTF(3, "vfs: PID-%i MKDIR: ENOENT\n", pid);
    resp.body.s32 = -ENOENT;
    goto _vfs_mkdir_exit;
  }
    
  // split full path in path + name to create
  ptr = len - 1;
  
  while (ptr && (pa_path[ptr] != '/')) ptr--;

  char *basename = pa_path + ptr;
  
  if (pa_path[ptr] == '/') {
    basename++;
  }

  K_PRINTF(3, "vfs: PID-%I MKDIR: basename = %s\n", pid, basename);

  struct vnode *pvn;

  // not very clean to alterate user's memory space but avoids a copy
  char bak = pa_path[len];
  pa_path[len] = 0;

  resp.body.s32 = _lookuppn(pa_path, &pvn, pid);

  pa_path[len] = bak;

  if (resp.body.s32) {
    K_PRINTF(3, "vfs: PID-%i MKDIR: %i\n", pid, resp.body.s32);
    goto _vfs_mkdir_exit;
  }
    
  if (pvn->v_type != VDIR) {
    K_PRINTF(3, "vfs: PID-%i MKDIR: ENOTDIR\n", pid);
    VN_RELE(pvn);
    resp.body.s32 = -ENOTDIR;
    goto _vfs_mkdir_exit;
  }
   
  resp.body.s32 = pvn->v_op->vn_mkdir(pvn, basename, pid);

  VN_RELE(pvn);

  if (resp.body.s32) {
    K_PRINTF(3, "vfs: PID-%i MKDIR: v_op->mkdir %i\n", pid, resp.body.s32);
    goto _vfs_mkdir_exit;
  }

  // success
  K_PRINTF(3, "vfs: PID-%i MKDIR: %s\n", pid, basename);
  
 _vfs_mkdir_exit:
  send(pid, &resp);
}

static void _vfs_fork (pid_t pid, message_t *msg)
{
  message_t resp;

  /* allowed for kernel tasks only */
  if (proc_get_uid(pid) == PROC_UID_KERNEL) {
    pid_t ppid = msg->body.vfs_fork.parent;
    pid_t child = msg->body.vfs_fork.child;
    
    /* copy links to file descriptors */
    memcpy((void *)&_vfs_proc_table[child], (void *)&_vfs_proc_table[ppid], sizeof(_vfs_proc_table[child]));
    
    K_PRINTF(3, "vfs: PID-%i FORK: %i to %i\n", pid, ppid, child);

    /* increment open counter */
    int fd;

    for (fd = 0; fd < K_PROC_FD_COUNT; fd++) {
      struct vnode *pvn = _vfs_proc_table[ppid].fd_table[fd];

      if (pvn) VN_HOLD(pvn);
    }

    /* current directory */
    if (_vfs_proc_table[child].cdir) {
      VN_HOLD(_vfs_proc_table[child].cdir);
    }
    
    /* acknowledge (no argument) */
    send(pid, &resp);
  }
  // TODO : else
}

// vnodes allocation
struct vnode *vfs_vnode_alloc()
{
  struct vnode *pvn = _vfs_free_vnode_list;

  if (pvn) {
    // remove from free list
    _vfs_free_vnode_list = pvn->next;
  }

  // get alone
  pvn->next = _K_VNODE_NULL;
  
  return pvn;
}

void vfs_vnode_free(struct vnode *pvn)
{
  pvn->next = _vfs_free_vnode_list;
  _vfs_free_vnode_list = pvn;
}
