//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/kernel - RFS file system
//

#include <stdint.h>
#include <types.h>
#include <stddef.h>
#include <fcntl.h>
#include <dirent.h>
#include <syscall.h>
#include <sys/stat.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "dev.h"
#include "mem.h"
#include "vfs.h"
#include "msg.h"
#include "debug.h"

#include "sys/rfs.h"

// private data for struct vfs (.vfs_data)
typedef u32_t rnode_t;

struct rfs {
  // device
  dev_t *dev;
  // root rnode
  rnode_t root;
  // list of vnodes
  struct vnode *pvn;
};

// check struct vfs private data storage is enough for RFS - if not increase K_MAX_VFS_PRIVATE_LEN
extern char size_check_rfs[(signed)K_MAX_VFS_PRIVATE_LEN-(signed)sizeof(struct rfs)];

// private data for struct vnode (.v_data)
struct rnode {
  rnode_t rnode;         // unique ID for rnode
  off_t   lseek;
  u32_t   FileSize;
};

// check struct rnode private data storage is enough for RFS - if not increase K_MAX_VN_PRIVATE_LEN
extern char size_check_rnode[(signed)K_MAX_VN_PRIVATE_LEN-(signed)sizeof(struct rnode)];

/*
static void _dump_msg(u8_t *buf, int len) {
  int i;
  for (i = 0; i < len; i++) K_PRINTF(2, "%Xh ", buf[i]);
  K_PRINTF(2, "\n");
}
*/

const struct vnodeops _rfs_vnodeops;

////////////////////////////////////////////////////////////////////////////////
// functions
////////////////////////////////////////////////////////////////////////////////
static u8_t _check_msg(u8_t *msg)
{
  if (*msg++ != 0x55) return 0;
  u8_t len = *msg++;
  u8_t sum = 0;

  while (len--) {
    sum += *msg++;
  }

  if (sum != *msg++) return 0;
  if (*msg++ != 0xAA) return 0;

  return 1;
}



////////////////////////////////////////////////////////////////////////////////
// vnode/rnode management
////////////////////////////////////////////////////////////////////////////////
// find a vnode with matching rnode
static struct vnode *_find_vnode(struct rfs *prfs, rnode_t rnode)
{
  struct vnode *pvn = prfs->pvn;

  while (pvn) {
    struct rnode *prn = (struct rnode *)&(pvn->v_data[0]);

    if (prn->rnode == rnode) {
      return pvn;
    }

    pvn = pvn->next;
  }

  // not found
  return 0;
}

static struct vnode *_allocate_vnode(struct vfs *pvfs, rnode_t rnode)
{
  struct vnode *pvn = vfs_vnode_alloc();

  if (!pvn) {
    return 0;
  }

  struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);

  // init vnode
  pvn->v_count = 0;
  pvn->v_op    = &_rfs_vnodeops;
  pvn->v_vfsp  = pvfs;

  struct rnode *prn = (struct rnode *)&(pvn->v_data[0]);

  prn->rnode    = rnode;
  prn->lseek    = 0;
  prn->FileSize = 0;

  // add vnode to FS vnode list
  pvn->next = prfs->pvn;
  prfs->pvn = pvn;

  return pvn;
}

////////////////////////////////////////////////////////////////////////////////
// rfs operations
////////////////////////////////////////////////////////////////////////////////
const struct vfsops _rfs_vfs_op;

int rfs_mount (struct vfs *pvfs, dev_t *pdev)
{
  message_t msg;

  K_PRINTF(2, "rfs: probing connection ...\n");
  
  // open device
  msg.type = DEV_OPEN;
  msg.body.dev_open.handle = pdev->handle;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // send reset command
  rfs_creset_t creset = RFS_CRESET();
  
  msg.type = DEV_WRITE;
  msg.body.dev_write.handle = pdev->handle;
  msg.body.dev_write.src = creset;
  msg.body.dev_write.count = sizeof(creset);
 
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // get reset command return
  rfs_areset_t areset;  
  
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = areset;
  msg.body.dev_read.count = sizeof(areset);
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  //_dump_msg(_buf, 5);

  if (msg.body.s32 == sizeof(areset) && _check_msg((u8_t *)&areset)) {
    // initialize vfs
    pvfs->vfs_op           = (struct vfsops *)&_rfs_vfs_op;
    pvfs->vfs_vnodecovered = 0;
    pvfs->vfs_flag         = 0;  // TODO

    // initialize private data
    struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);
    prfs->dev = pdev;
    prfs->root = RFS_ARESET_ROOT(areset);
    prfs->pvn = 0;
    
    K_PRINTF(2, "rfs: connected; root is %u\n", prfs->root);
    return 0;
  } else {
    K_PRINTF(2, "rfs: failed to connect (%i)\n", msg.body.s32);
    return -1;
  }    
}

static int _rfs_unmount (struct vfs *pvfs)
{
  // TODO
  return -1;
}

static int _rfs_root (struct vfs *pvfs, struct vnode **ppv)
{
  struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);

  struct vnode *pvroot = _find_vnode(prfs, prfs->root);

  if (pvroot) {
    // exists
    VN_HOLD(pvroot);
    *ppv = pvroot;
    return 0;
  }

  pvroot = _allocate_vnode(pvfs, prfs->root);

  if (!pvroot) {
    // out of ressource
    *ppv = 0;
    return -ENOMEM;
  }

  pvroot->v_type = VDIR;
  VN_HOLD(pvroot);
  *ppv = pvroot;
  return 0;
}

const struct vfsops _rfs_vfs_op = {
  .vfs_mount   = rfs_mount,
  .vfs_unmount = _rfs_unmount,
  .vfs_root    = _rfs_root
};

////////////////////////////////////////////////////////////////////////////////
// rnode operations
////////////////////////////////////////////////////////////////////////////////
static int _rnode_open(struct vnode *pvn, int flags, pid_t pid)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);
  struct rnode *prn = (struct rnode *)&(pvn->v_data[0]);

  dev_t *pdev = prfs->dev;
  
  message_t msg;

  // send open command
  rfs_copen_t copen = RFS_COPEN(prn->rnode, flags, pid);
  
  msg.type = DEV_WRITE;
  msg.body.dev_write.handle = pdev->handle;
  msg.body.dev_write.src = &copen;
  msg.body.dev_write.count = sizeof(copen);
 
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // get open command return
  rfs_aopen_t aopen;
  
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = &aopen;
  msg.body.dev_read.count = sizeof(aopen);
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);
 
  if (msg.body.s32 == sizeof(aopen) && _check_msg((u8_t *)&aopen) && RFS_AOPEN_STATUS(aopen) == 1) {

    prn->lseek = 0;

    if (flags != O_DIRECTORY) {
      prn->FileSize = RFS_AOPEN_SIZE(aopen);
    } else {
      prn->FileSize = 0;
    }
    
    K_PRINTF(3, "rfs: open rnode %u, %u bytes\n", prn->rnode, prn->FileSize);

    return 0;
  }
  
  /* couldn't open file */
  return -1;  
}

static int _rnode_close(struct vnode *pvn, pid_t pid)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);
  struct rnode *prn = (struct rnode *)&(pvn->v_data[0]);

  dev_t *pdev = prfs->dev;
  
  message_t msg;

  // send close command
  rfs_cclose_t cclose = RFS_CCLOSE(prn->rnode);
    
  msg.type = DEV_WRITE;
  msg.body.dev_write.handle = pdev->handle;
  msg.body.dev_write.src = &cclose;
  msg.body.dev_write.count = sizeof(cclose);
 
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // get open command return
  rfs_cclose_t aclose;
  
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = &aclose;
  msg.body.dev_read.count = sizeof(aclose);
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  if (msg.body.s32 == sizeof(aclose) && _check_msg((u8_t *)&aclose)) {
    if (RFS_ACLOSE_STATUS(aclose) == 1) {
      K_PRINTF(3, "rfs: close rnode %u\n", prn->rnode);

      return 0;
    } else {
      K_PRINTF(2, "rfs: error while closing rnode %u\n", prn->rnode);
      return -1;  
    }
  } else {
    K_PRINTF(2, "rfs: warning: bad frame\n");
  }
  
  /* couldn't close file */
  return -1;  
}

static size_t _rnode_read (struct vnode *pvn, void *buf, size_t count, pid_t pid)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);
  struct rnode *prn = (struct rnode *)&(pvn->v_data[0]);

  dev_t *pdev = prfs->dev;
  
  message_t msg;
  u8_t *dst = (u8_t *)buf;
  size_t done = 0;

  while (done < count) {
    u8_t len = (count - done) > 64 ? 64 : count - done;
    
    // send read command
    rfs_cread_t cread = RFS_CREAD(len, prn->rnode, prn->lseek);
      
    msg.type = DEV_WRITE;
    msg.body.dev_write.handle = pdev->handle;
    msg.body.dev_write.src = &cread;
    msg.body.dev_write.count = sizeof(cread);
  
    sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

    // get read command return
    u8_t buf[64+6];

    msg.type = DEV_READ;
    msg.body.dev_read.handle = pdev->handle;
    msg.body.dev_read.dst = buf;
    msg.body.dev_read.count = 6+len;
  
    sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

    //_dump_msg(_buf, 5+len);

    u8_t blen = RFS_AREAD_BLEN(buf);
    
    if (msg.body.s32 == 6+len && _check_msg(buf) && blen >= 0) {
      // valid frame
      if (blen == 0) {
	// end of file
	break;
      }
      
      u8_t *src = RFS_AREAD_BUF(buf);

      prn->lseek += blen;
      
      while (blen--) {
	*dst++ = *src++;
	done++;
      }
    } else {
      K_PRINTF(2, "rfs: warning: bad frame\n");
      break;
    }
  }
  
  return done;
}
  
static size_t _rnode_write (struct vnode *pvn, const void *buf, size_t count, pid_t pid)
{
  return 0;
}

static off_t _rnode_lseek (struct vnode *pvn, off_t offset, int whence)
{
  struct rnode *prn = (struct rnode *)&(pvn->v_data[0]);

  if (whence == SEEK_END) {
    if (offset > prn->FileSize) return -1;

    offset = prn->FileSize - offset;
  }

  if (whence == SEEK_CUR) {
    if ((prn->lseek + offset) > prn->FileSize) return -1;

    offset = prn->lseek + offset;
  }

  prn->lseek = offset;
  
  return 0;
}

static int _rnode_getdents (struct vnode *pvn, char *buf, unsigned int count)
{
  struct dirent *dirp = (struct dirent *)buf;
  
  struct vfs *pvfs = pvn->v_vfsp;
  struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);
  struct rnode *prn = (struct rnode *)&(pvn->v_data[0]);

  if (count > 64) {
    K_PRINTF(2, "rfs: getdents: error: bad count\n");
    return -1;
  }
  
  dev_t *pdev = prfs->dev;

  message_t msg;

  // send getdents command
  rfs_cgetdents_t cgetdents = RFS_CGETDENTS(prn->rnode, prn->lseek++);
  
  msg.type = DEV_WRITE;

  msg.body.dev_write.handle = pdev->handle;
  msg.body.dev_write.src = &cgetdents;
  msg.body.dev_write.count = sizeof(cgetdents);
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // get getdents answer
  rfs_agetdents_t agetdents;

  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = &agetdents;
  msg.body.dev_read.count = sizeof(agetdents);
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // answer format:
  // 0: 55h
  // 1: len=5+count
  // 2: type (1:file, 2:directory)
  // 3+: name[count] (null terminated string)
  // : checksum
  // : AAh

  //_dump_msg(_buf, 5+count);

  if (msg.body.s32 == sizeof(agetdents) && _check_msg((u8_t *)&agetdents)) {
    // valid frame
    if (RFS_AGETDENTS_BUF(agetdents)[0]) {
      RFS_AGETDENTS_BUF(agetdents)[count-offsetof(struct dirent, d_name)-1] = 0;  // limit max length

      int len = strlen((char *)RFS_AGETDENTS_BUF(agetdents));

      dirp->d_type = 0;

      if (RFS_AGETDENTS_TYPE(agetdents) == 1) dirp->d_type = DT_REG;
      if (RFS_AGETDENTS_TYPE(agetdents) == 2) dirp->d_type = DT_DIR;
      
      dirp->d_size = offsetof(struct dirent, d_name) + len + 1;
      strcpy(dirp->d_name, (char *)RFS_AGETDENTS_BUF(agetdents));
      
      return offsetof(struct dirent, d_name) + len + 1;
    }

    // end of directory
    return 0;
  }

  K_PRINTF(2, "rfs: getdents: warning: bad frame\n");
  return -1;
}

static int _rnode_ioctl (struct vnode *pvn, int request, mem_va_t ptr, pid_t pid)
//int rfs_ioctl(fs_file_context_t *ctx, pid_t pid, int request, mem_va_t ptr)
{
  return -1;
}

static int _rnode_mkdir (struct vnode *pvn, char *nm, pid_t pid)
//static int _rfs_mkdir(fs_context_t *ctx, const char *pathname, mode_t mode)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);
  struct rnode *prn = (struct rnode *)&(pvn->v_data[0]);

  dev_t *pdev = prfs->dev;

  message_t msg;

  int len = strlen(nm);

  if (len > RFS_MAX_NAME_LEN) {
    return ENAMETOOLONG;
  }

  // send mkdir command
  rfs_cmkdir_t cmkdir = RFS_CMKDIR(prn->rnode, 0 /* TODO flags */, pid, nm);
  strcpy(RFS_CMKDIR_NAME(cmkdir), nm);
  
  msg.type = DEV_WRITE;
  msg.body.dev_write.handle = pdev->handle;
  msg.body.dev_write.src = &cmkdir;
  msg.body.dev_write.count = sizeof(cmkdir);
 
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // get mkdir command return
  rfs_amkdir_t amkdir;
  
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = &amkdir;
  msg.body.dev_read.count = sizeof(amkdir);
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  if (msg.body.s32 == sizeof(amkdir) && _check_msg((u8_t *)&amkdir) && RFS_AMKDIR_ERRNO(amkdir) == 0) {
    K_PRINTF(3, "rfs: mkdir '%s'\n", nm);

    return 0;
  }
  
  /* couldn't open file */
  return RFS_AMKDIR_ERRNO(amkdir);  
}

static int _rnode_lookup(struct vnode *pvn, char *nm, struct vnode **ppv, pid_t pid)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);
  struct rnode *prn = (struct rnode *)&(pvn->v_data[0]);

  dev_t *pdev = prfs->dev;
  
  message_t msg;
  int len = strlen(nm);

  if (len > RFS_MAX_NAME_LEN) {
    return -ENAMETOOLONG;
  }

  // send open command
  rfs_clookup_t clookup = RFS_CLOOKUP(prn->rnode, pid, nm);

  msg.type = DEV_WRITE;
  msg.body.dev_write.handle = pdev->handle;
  msg.body.dev_write.src = &clookup;
  msg.body.dev_write.count = sizeof(clookup);
 
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // get lookup command return
  rfs_alookup_t alookup;
  
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = &alookup;
  msg.body.dev_read.count = sizeof(alookup);
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  rnode_t rn = RFS_ALOOKUP_RNODE(alookup);
  
  if (msg.body.s32 != sizeof(alookup) || !_check_msg((u8_t *)&alookup) || !rn) {
    /* couldn't lookup file */
    return -1;  
  }

  *ppv = _find_vnode(prfs, rn);

  if (*ppv) {
    // already exists
    VN_HOLD(*ppv);

    return 0;
  }
   
  *ppv = _allocate_vnode(pvfs, rn);

  if (!*ppv) {
    // out of resource
    return -ENOMEM;
  }

  VN_HOLD(*ppv);
  (*ppv)->v_vfsmountedhere = 0;
  (*ppv)->v_type = RFS_ALOOKUP_TYPE(alookup);

  prn = (struct rnode *)&((*ppv)->v_data[0]);
  
  prn->rnode    = rn;
  prn->FileSize = 0;
  prn->lseek    = 0;
 
  K_PRINTF(3, "rfs: lookup '%s': rnode %u, type %u\n", nm, prn->rnode, (*ppv)->v_type);

  return 0;
}

static int _rnode_inactive (struct vnode *pvn)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);

  // first in list is root directory, it would be remove when unmounting
  //  so 1st vnode is not tested
  struct vnode *pvn_list = prfs->pvn;

  while (pvn_list) {
    if (pvn_list->next == pvn) {
      pvn_list->next = pvn->next;

      pvn->next = 0;
      vfs_vnode_free(pvn);

      return 0;
    }

    pvn_list = pvn_list->next;
  }
  
  K_PRINTF(2, "rfs: inactive failed !\n");
  
  return 0;
}

const struct vnodeops _rfs_vnodeops = {
  .vn_open     = _rnode_open,
  .vn_close    = _rnode_close,
  .vn_read     = _rnode_read,
  .vn_write    = _rnode_write,
  .vn_lseek    = _rnode_lseek,
  .vn_ioctl    = _rnode_ioctl,
  //  int (*vn_select)();
  //  int (*vn_getattr)();
  //  int (*vn_setattr)();
  //  int (*vn_access)();
  .vn_lookup   = _rnode_lookup,
  //  int (*vn_create)();
  //  int (*vn_remove)();
  //  int (*vn_link)();
  //  int (*vn_rename)();
  .vn_mkdir    = _rnode_mkdir,
  //  int (*vn_rmdir)();
  .vn_getdents = _rnode_getdents,
  //  int (*vn_symlink)();
  //  int (*vn_readlink)();
  //  int (*vn_fsync)();
  .vn_inactive = _rnode_inactive
  //  int (*vn_bmap)();
  //  int (*vn_strategy)();
  //  int (*vn_bread)();
  //  int (*vn_brelse)();
};

