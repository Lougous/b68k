//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/kernel - devfs file system
//

#include <fcntl.h>
#include <types.h>
#include <dirent.h>
#include <stddef.h>
#include <syscall.h>
#include <sys/stat.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "debug.h"
#include "dev.h"
#include "mem.h"
#include "vfs.h"
#include "msg.h"


// private data for struct vfs (.vfs_data)
struct devfs {
  // root rnode
  struct vnode *pvnroot;
  // list of vnodes
  struct vnode *pvn;
};

// check struct vfs private data storage is enough for DEVFS - if not increase K_MAX_VFS_PRIVATE_LEN
extern char size_check_devfs[(signed)K_MAX_VFS_PRIVATE_LEN-(signed)sizeof(struct devfs)];

// private data for struct vnode (.v_data)
struct devnode {
  int flags;
  struct dev *pdev;
  off_t lseek;
};

// check struct devnode private data storage is enough for DEVFS - if not increase K_MAX_VN_PRIVATE_LEN
extern char size_check_devnode[(signed)K_MAX_VN_PRIVATE_LEN-(signed)sizeof(struct devnode)];

const struct vfsops _devfs_vfs_op;
const struct vnodeops _devfs_vnodeops;

////////////////////////////////////////////////////////////////////////////////
// vnode/rnode management
////////////////////////////////////////////////////////////////////////////////
// find a vnode with matching pdev
static struct vnode *_find_vnode(struct devfs *pdevfs, struct dev *pdev)
{
  struct vnode *pvn = pdevfs->pvn;

  while (pvn) {
    struct devnode *pdn = (struct devnode *)&(pvn->v_data[0]);

    if (pdn->pdev == pdev) {
      return pvn;
    }

    pvn = pvn->next;
  }

  // not found
  return 0;
}

static struct vnode *_allocate_vnode(struct vfs *pvfs, struct dev *pdev)
{
  struct vnode *pvn = vfs_vnode_alloc();

  if (!pvn) {
    return 0;
  }

  struct devfs *pdevfs = (struct devfs *)&(pvfs->vfs_data[0]);

  // init vnode
  pvn->v_count = 0;
  pvn->v_op    = &_devfs_vnodeops;
  pvn->v_vfsp  = pvfs;

  struct devnode *pdn = (struct devnode *)&(pvn->v_data[0]);

  pdn->pdev   = pdev;
  pdn->lseek  = 0;
  pdn->flags  = 0;

  // add vnode to FS vnode list
  pvn->next   = pdevfs->pvn;
  pdevfs->pvn = pvn;

  return pvn;
}

////////////////////////////////////////////////////////////////////////////////
// devfs operations
////////////////////////////////////////////////////////////////////////////////
int devfs_mount (struct vfs *pvfs, struct dev *pdev)
{
  K_PRINTF(2, "devfs: mounting\n");
  
  // initialize vfs
  pvfs->vfs_op           = (struct vfsops *)&_devfs_vfs_op;
  pvfs->vfs_vnodecovered = 0;
  pvfs->vfs_flag         = 0;  // TODO

  // no private data

  // allocate and setup root vnode
  struct vnode *pvn = vfs_vnode_alloc();

  if (!pvn) {
    return -ENOMEM;
  }

  pvn->v_op    = &_devfs_vnodeops;
  pvn->v_vfsp  = pvfs;
  pvn->v_type  = VDIR;
  VN_HOLD(pvn);

  struct devnode *pdn = (struct devnode *)&(pvn->v_data[0]);

  pdn->flags = 0;  // TODO
  pdn->lseek = 0;
  pdn->pdev  = 0;

  // add vnode in list
  struct devfs *pdevfs = (struct devfs *)&(pvfs->vfs_data[0]);
  pdevfs->pvn = pvn;
  pdevfs->pvnroot = pvn;

  return 0;
}

static int _devfs_unmount (struct vfs *pvfs)
{
  // TODO
  return -1;
}

static int _devfs_root (struct vfs *pvfs, struct vnode **ppv)
{
  struct devfs *pdevfs = (struct devfs *)&(pvfs->vfs_data[0]);
  *ppv = pdevfs->pvnroot;
  VN_HOLD(*ppv);
  return 0;
}

const struct vfsops _devfs_vfs_op = {
  .vfs_mount   = devfs_mount,
  .vfs_unmount = _devfs_unmount,
  .vfs_root    = _devfs_root
};

////////////////////////////////////////////////////////////////////////////////
// devnode operations
////////////////////////////////////////////////////////////////////////////////
static int _vn_open(struct vnode *pvn, int flags, pid_t pid)
{
  struct devnode *pdn = (struct devnode *)&(pvn->v_data[0]);
  struct dev *pdev = pdn->pdev;

  if (pvn->v_type == VDIR) {
    pdn->lseek = 0;
    return 0;
  }
  
  // pdev should be valid here if vnode properly returned by lookup
  message_t msg;
  msg.type = DEV_OPEN;
  msg.body.dev_open.handle = pdev->handle;
  msg.body.dev_open.flags = flags;
 
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  return msg.body.s32;
}

static int _vn_close(struct vnode *pvn, pid_t pid)
{
  if (pvn->v_type == VDIR) {
    return 0;
  }
  
  struct devnode *pdn = (struct devnode *)&(pvn->v_data[0]);
  struct dev *pdev = pdn->pdev;

  // pdev should be valid here if vnode properly returned by lookup
  message_t msg;
  msg.type = DEV_CLOSE;  
  msg.body.dev_close.handle = pdev->handle;

  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  return msg.body.u32;
}

static size_t _vn_read (struct vnode *pvn, void *buf, size_t count, pid_t pid)
{
  if (pvn->v_type == VDIR) {
    return -EINVAL;
  }
  
  struct devnode *pdn = (struct devnode *)&(pvn->v_data[0]);
  struct dev *pdev = pdn->pdev;
  
  // pdev should be valid here if vnode properly returned by lookup
  message_t msg;
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.src_seek = pdn->lseek;
  msg.body.dev_read.dst = buf;
  msg.body.dev_read.count = count;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);
  
  pdn->lseek += msg.body.s32;
  
  return msg.body.s32;
}

static size_t _vn_write (struct vnode *pvn, const void *buf, size_t count, pid_t pid)
{
  if (pvn->v_type == VDIR) {
    return -EINVAL;
  }
  
  struct devnode *pdn = (struct devnode *)&(pvn->v_data[0]);
  struct dev *pdev = pdn->pdev;

  // pdev should be valid here if vnode properly returned by lookup
  message_t msg;
  msg.type = DEV_WRITE;
  msg.body.dev_write.handle = pdev->handle;
  msg.body.dev_write.src = (void *)buf;
  msg.body.dev_write.dst_seek = pdn->lseek;
  msg.body.dev_write.count = count;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);
  
  pdn->lseek += msg.body.s32;
  
  return msg.body.s32;
}

static int _vn_ioctl (struct vnode *pvn, int request, mem_va_t ptr, pid_t pid)
//int devfs_ioctl(fs_file_context_t *ctx, pid_t pid, int request, mem_va_t ptr)
{
  if (pvn->v_type == VDIR) {
    return -ENOTTY;
  }
  
  struct devnode *pdn = (struct devnode *)&(pvn->v_data[0]);
  struct dev *pdev = pdn->pdev;

  // pdev should be valid here if vnode properly returned by lookup
  message_t msg;
  msg.type = DEV_IOCTL;
  msg.body.dev_ioctl.handle  = pdev->handle;
  msg.body.dev_ioctl.pid     = pid;  // TODO
  msg.body.dev_ioctl.request = request;
  msg.body.dev_ioctl.va_ptr  = (void *)ptr;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);
  
  return msg.body.s32;
}

static off_t _vn_lseek(struct vnode *pvn, off_t offset, int whence)
{
  if (pvn->v_type == VDIR) {
    return -1;
  }
  
  struct devnode *pdn = (struct devnode *)&(pvn->v_data[0]);

  if (whence == SEEK_END) {
    // cannot seek from end
    return -EINVAL;
  }

  if (whence == SEEK_CUR) {
    offset = pdn->lseek + offset;
  }

  pdn->lseek = offset;
  
  return 0;
}

static int _vn_getdents (struct vnode *pvn, char *buf, unsigned int count, pid_t pid)
{
  K_PRINTF(3, "devfs: getdents: node %Xh\n", pvn);
  
  // lookup possible in root only
  if (pvn->v_type != VDIR) {
    K_PRINTF(3, "devfs: getdents: error: ENOTDIR\n");
    return -ENOTDIR;
  }

  struct devnode *pdn = (struct devnode *)&(pvn->v_data[0]);

  char *ndev = dev_name(pdn->lseek);

  if (ndev) {
    int size = offsetof(struct dirent, d_name) + strlen(ndev) + 1;
    struct dirent *dirp = (struct dirent *)buf;

    if (size <= count) { 
      dirp->d_size = size;

      struct dev *pdev = dev_get(ndev);
      dirp->d_type = pdev->attr.attr_type == DEV_ATTR_CHAR ? DT_CHR : DT_BLK;

      strcpy(dirp->d_name, ndev);

      pdn->lseek++;

      K_PRINTF(3, "devfs: getdents: %s, %u\n", dirp->d_name, size);
      return size;
    }
  }
  
  K_PRINTF(3, "devfs: getdents: end of directory\n");
  return 0;
}

static int _vn_mkdir (struct vnode *pvn, char *nm, pid_t pid)
{
  return EROFS;
}

int _vn_lookup(struct vnode *pvn, char *nm, struct vnode **ppv, pid_t pid)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct vnode *root = (struct vnode *)pvfs->vfs_data[0];

  // lookup possible in root only
  if (pvn != root) {
    return -ENOTDIR;
  }

  int len = strlen(nm);

  if (len > K_MAX_FILENAME_LEN) {
    return -ENAMETOOLONG;
  }

  struct dev *pdev = dev_get(nm);
  
  if (!pdev) {
    return -ENOENT;
  }

  // vnode already exists ? check vnode list
  struct devfs *pdevfs = (struct devfs *)&(pvfs->vfs_data[0]);
  *ppv = _find_vnode(pdevfs, pdev);

  if (!*ppv) {
    // need to create new vnode
    *ppv = _allocate_vnode(pvfs, pdev);

    if (!*ppv) {
      // out of resource
      return -ENOMEM;
    }
  }

  VN_HOLD(*ppv);
  (*ppv)->v_vfsmountedhere = 0;
  (*ppv)->v_type = pdev->attr.attr_type == DEV_ATTR_CHAR ? VCHR : VBLK;
  
  struct devnode *pdn = (struct devnode *)&(pvn->v_data[0]);

  pdn->flags = 0;  // TODO
  pdn->lseek = 0;
  pdn->pdev  = pdev;

  return 0;
}

static int _vn_inactive (struct vnode *pvn)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct devfs *pdevfs = (struct devfs *)&(pvfs->vfs_data[0]);

  // first in list is root directory, it would be remove when unmounting
  //  so 1st vnode is not tested
  struct vnode *pvn_list = pdevfs->pvn;

  if (pvn == pvn_list) {
    // remove 1st
    pdevfs->pvn = pvn->next;
    vfs_vnode_free(pvn);
    return 0;
  }

  while (pvn_list) {
    if (pvn_list->next == pvn) {
      pvn_list->next = pvn->next;

      pvn->next = 0;
      vfs_vnode_free(pvn);

      return 0;
    }

    pvn_list = pvn_list->next;
  }
  
  K_PRINTF(2, "devfs: inactive failed !\n");
  
  return 0;
}

static int _vn_getattr (struct vnode *pvn, struct stat *pto)
{
  struct devnode *pdn = (struct devnode *)&(pvn->v_data[0]);
  pto->st_rdev = pdn->pdev;
  return -1;
}


const struct vnodeops _devfs_vnodeops = {
  .vn_open     = _vn_open,
  .vn_close    = _vn_close,
  .vn_read     = _vn_read,
  .vn_write    = _vn_write,
  .vn_lseek    = _vn_lseek,
  .vn_ioctl    = _vn_ioctl,
  //  int (*vn_select)();
  .vn_getattr  = _vn_getattr,
  //  int (*vn_setattr)();
  //  int (*vn_access)();
  .vn_lookup   = _vn_lookup,
  //  int (*vn_create)();
  //  int (*vn_remove)();
  //  int (*vn_link)();
  //  int (*vn_rename)();
  .vn_mkdir    = _vn_mkdir,
  //  int (*vn_rmdir)();
  .vn_getdents = _vn_getdents,
  //  int (*vn_symlink)();
  //  int (*vn_readlink)();
  //  int (*vn_fsync)();
  .vn_inactive = _vn_inactive
  //  int (*vn_bmap)();
  //  int (*vn_strategy)();
  //  int (*vn_bread)();
  //  int (*vn_brelse)();
};

