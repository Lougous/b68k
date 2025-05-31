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
#include "debug.h"
#include "dev.h"
#include "mem.h"
#include "vfs.h"
#include "msg.h"

#include "sys/rfs.h"

// private data for struct vfs (.vfs_data)
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


static void _dump_msg(u8_t *buf, int len) {
  int i;
  for (i = 0; i < len; i++) K_PRINTF(2, "%Xh ", buf[i]);
  K_PRINTF(2, "\n");
}


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

static u8_t _receive_msg(dev_t *pdev, u8_t *buf)
{
  message_t msg;
  
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = buf;
  msg.body.dev_read.count = 2;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  if (msg.body.s32 != 2) return 0;
  if (buf[0] != 0x55) return 0;
  
  u32_t len = buf[1];

  if (len > (RFS_MAX_READ + 2)) return 0;
  
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = buf+2;
  msg.body.dev_read.count = len + 2;

  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  if (msg.body.s32 != (len + 2)) return 0;

  return _check_msg(buf);
}

static u8_t _chksum(u8_t *msg, u8_t len)
{
  u8_t sum = 0;
  
  while (len--) {
    sum += *msg++;
  }

  return sum;
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
  rfs_creset_t creset = { 0x55, 0x1, 'I', 'I', 0xAA };
  
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
    prfs->root = rfs_read_u32le(areset + 3);
    prfs->pvn = 0;
    
    K_PRINTF(2, "rfs: connected; root is %Xh\n", prfs->root);
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
  rfs_copen_t copen;
  copen[0] = 0x55;
  copen[1] = RFS_PAYLOAD_LEN(sizeof(rfs_copen_t));
  copen[2] = 'O';
  (void)rfs_u32le_at(copen + 3, prn->rnode);
  copen[7] = flags;
  copen[8] = pid;
  copen[9] = _chksum(copen + 2, RFS_PAYLOAD_LEN(sizeof(rfs_copen_t)));
  copen[10] = 0xAA;
 
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
 
  if (msg.body.s32 == sizeof(aopen) && _check_msg((u8_t *)&aopen) && aopen[3] == 1) {

    prn->lseek = 0;

    if (flags != O_DIRECTORY) {
      prn->FileSize = rfs_read_u32le(aopen + 4);
    } else {
      prn->FileSize = 0;
    }
    
    K_PRINTF(3, "rfs: open rnode %Xh, %u bytes\n", prn->rnode, prn->FileSize);

    return 0;
  }
  
  /* couldn't open file */
  K_PRINTF(3, "rfs: open failed\n");

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
  rfs_cclose_t cclose;

  cclose[0] = 0x55;
  cclose[1] = RFS_PAYLOAD_LEN(sizeof(rfs_cclose_t));
  cclose[2] = 'C';
  (void)rfs_u32le_at(cclose + 3, prn->rnode);
  cclose[7] = _chksum(cclose + 2, RFS_PAYLOAD_LEN(sizeof(rfs_cclose_t)));
  cclose[8] = 0xAA;

  msg.type = DEV_WRITE;
  msg.body.dev_write.handle = pdev->handle;
  msg.body.dev_write.src = cclose;
  msg.body.dev_write.count = sizeof(cclose);
 
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // get open command return
  rfs_aclose_t aclose;
  
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = aclose;
  msg.body.dev_read.count = sizeof(aclose);
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  if (msg.body.s32 == sizeof(aclose) && _check_msg(aclose)) {
    if (aclose[3] == 1) {
      K_PRINTF(3, "rfs: close rnode %Xh\n", prn->rnode);

      return 0;
    } else {
      K_PRINTF(2, "rfs: error while closing rnode %Xh\n", prn->rnode);
      return -1;  
    }
  } else {
    K_PRINTF(2, "rfs: aclose: bad frame\n");
    _dump_msg(aclose, sizeof(aclose)) ;
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

  if ((prn->lseek + count) > prn->FileSize) {
    // will stop at end of file
    count = prn->FileSize - prn->lseek;
  }

  while (done < count) {
    u8_t len = (count - done) > 64 ? 64 : count - done;
    
    // send read command
    rfs_cread_t cread;

    cread[0] = 0x55;
    cread[1] = RFS_PAYLOAD_LEN(sizeof(rfs_cread_t));
    cread[2] = 'R';
    cread[3] = len;
    (void)rfs_u32le_at(cread + 4, prn->rnode);
    (void)rfs_u32le_at(cread + 8, prn->lseek);
    cread[12] = _chksum(cread + 2, RFS_PAYLOAD_LEN(sizeof(rfs_cread_t)));
    cread[13] = 0xAA;
    
    msg.type = DEV_WRITE;
    msg.body.dev_write.handle = pdev->handle;
    msg.body.dev_write.src = &cread;
    msg.body.dev_write.count = sizeof(cread);
  
    sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

    // get read command return
    u8_t buf[RFS_MAX_READ+sizeof(rfs_aread_ko_t)];

    // header + CID + status
    if (_receive_msg(pdev, buf) == 0) {
      K_PRINTF(2, "rfs: aread: bad frame\n");
      break;
    }
      
    if (buf[3] == RFS_AREAD_KO) {
      // valid message but status KO
      K_PRINTF(2, "rfs: warning: read KO\n");
      break;
    }
   
    u8_t blen = buf[1] - 2;  // - CID and status byte
    
    if (blen == 0) {
      // end of file
      break;
    }
      
    u8_t *src = buf + 4;

    prn->lseek += blen;
    done += blen;
      
    while (blen--) {
      *dst++ = *src++;
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

static int _rnode_getdents (struct vnode *pvn, char *buf, unsigned int count, pid_t pid)
{
  struct dirent *dirp = (struct dirent *)buf;
  
  struct vfs *pvfs = pvn->v_vfsp;
  struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);
  struct rnode *prn = (struct rnode *)&(pvn->v_data[0]);

  dev_t *pdev = prfs->dev;

  message_t msg;

  // send getdents command
  rfs_cgetdents_t cgetdents;
  cgetdents[0] = 0x55;
  cgetdents[1] = RFS_PAYLOAD_LEN(sizeof(rfs_cgetdents_t));
  cgetdents[2] = 'D';
  (void)rfs_u32le_at(cgetdents + 3, prn->rnode);
  cgetdents[7] = prn->lseek++;
  cgetdents[8] = _chksum(cgetdents + 2, RFS_PAYLOAD_LEN(sizeof(rfs_cgetdents_t)));
  cgetdents[9] = 0xAA;
  
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
  // 2: 'd'
  // 3: type (1:file, 2:directory)
  // 4+: name[count] (null terminated string)
  // : checksum
  // : AAh

  //_dump_msg(_buf, 5+count);

  if (msg.body.s32 != sizeof(agetdents) || !_check_msg(agetdents)) {
    K_PRINTF(2, "rfs: getdents: warning: bad frame\n");
    return -1;
  }

  //_dump_msg(agetdents, sizeof(agetdents));
  
  // valid frame
  if (agetdents[4] == 0) {
    // empty string : end of directory
    return 0;
  }

  // ensure null char at end of string
  agetdents[4+RFS_MAX_NAME_LEN-1] = 0;

  int len = strlen((char *)agetdents + 4);

  // enough space ?
  u16_t d_size = offsetof(struct dirent, d_name) + len + 1;
  
  if (count < d_size) {
    // does not fit
    return -EINVAL;
  }

  // fill dirent structure
  dirp->d_size = d_size;
  dirp->d_type = DT_UNKNOWN;
  if (agetdents[3] == RFS_TYPE_UNK) dirp->d_type = DT_UNKNOWN;
  if (agetdents[3] == RFS_TYPE_REG) dirp->d_type = DT_REG;
  if (agetdents[3] == RFS_TYPE_DIR) dirp->d_type = DT_DIR;
  
  strcpy(dirp->d_name, (char *)agetdents + 4);
      
  return offsetof(struct dirent, d_name) + len + 1;
}

static int _rnode_ioctl (struct vnode *pvn, int request, mem_va_t ptr, pid_t pid)
{
  return -1;
}

static int _rnode_mkdir (struct vnode *pvn, char *nm, pid_t pid)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);
  struct rnode *prn = (struct rnode *)&(pvn->v_data[0]);

  dev_t *pdev = prfs->dev;

  message_t msg;

  u16_t len = strlen(nm);

  if (len > (RFS_MAX_NAME_LEN - 1)) {
    return -ENAMETOOLONG;
  }

  // send mkdir command
  rfs_cmkdir_t cmkdir;  // = RFS_CMKDIR(prn->rnode, 0 /* TODO flags */, pid, nm);
  cmkdir[0] = 0x55;
  cmkdir[1] = RFS_PAYLOAD_LEN(sizeof(rfs_cmkdir_t));
  cmkdir[2] = 'M';
  (void)rfs_u32le_at(cmkdir + 3, prn->rnode);
  cmkdir[7] = 0;    // TODO flags
  cmkdir[8] = pid;
  strcpy((char *)cmkdir + 9, nm);
  memset(cmkdir + 9 + len, 0, RFS_MAX_NAME_LEN - len);
  cmkdir[9+RFS_MAX_NAME_LEN] = _chksum(cmkdir + 2, RFS_PAYLOAD_LEN(sizeof(rfs_cmkdir_t)));
  cmkdir[10+RFS_MAX_NAME_LEN] = 0xAA;
  
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

  if (msg.body.s32 == sizeof(amkdir) && _check_msg(amkdir) && amkdir[3] == 0) {
    K_PRINTF(3, "rfs: mkdir '%s'\n", nm);

    return 0;
  }
  
  // couldn't open file
  // TODO translate error code
  return -(int)amkdir[3];
}

static int _rnode_lookup(struct vnode *pvn, char *nm, struct vnode **ppv, pid_t pid)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);
  struct rnode *prn = (struct rnode *)&(pvn->v_data[0]);

  dev_t *pdev = prfs->dev;
  
  message_t msg;
  int len = strlen(nm);

  if (len > (RFS_MAX_NAME_LEN - 1)) {
    return -ENAMETOOLONG;
  }

  // send open command
  rfs_clookup_t clookup;  // = RFS_CLOOKUP(prn->rnode, pid, nm);
  clookup[0] = 0x55;
  clookup[1] = RFS_PAYLOAD_LEN(sizeof(rfs_clookup_t));
  clookup[2] = 'L';
  (void)rfs_u32le_at(clookup + 3, prn->rnode);
  clookup[7] = pid;
  strcpy((char *)clookup + 8, nm);
  memset(clookup + 8 + len, 0, RFS_MAX_NAME_LEN - len);
  clookup[8+RFS_MAX_NAME_LEN] = _chksum(clookup + 2, RFS_PAYLOAD_LEN(sizeof(rfs_clookup_t)));
  clookup[9+RFS_MAX_NAME_LEN] = 0xAA;

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

  rnode_t rn = rfs_read_u32le(alookup + 3);
  u8_t type = alookup[7];
  
  if (msg.body.s32 != sizeof(alookup) || !_check_msg((u8_t *)&alookup) || !type) {
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
  (*ppv)->v_type = type == RFS_TYPE_DIR ? VDIR : VREG;

  prn = (struct rnode *)&((*ppv)->v_data[0]);
  
  prn->rnode    = rn;
  prn->FileSize = 0;
  prn->lseek    = 0;
 
  K_PRINTF(3, "rfs: lookup '%s': rnode %Xh, type %u\n", nm, prn->rnode, (*ppv)->v_type);

  return 0;
}

static int _rnode_inactive (struct vnode *pvn)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct rfs *prfs = (struct rfs *)&(pvfs->vfs_data[0]);

  // assumes list not empty (at least root vnode should be present)
  struct vnode *pvn_list = prfs->pvn;

  if (pvn == pvn_list) {
    // remove 1st
    prfs->pvn = pvn->next;
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
  
  K_PRINTF(2, "rfs: inactive failed !\n");
  
  return 0;
}

static int _rnode_getattr (struct vnode *pvn, struct stat *pto)
{
  // TODO
  return -1;
}


const struct vnodeops _rfs_vnodeops = {
  .vn_open     = _rnode_open,
  .vn_close    = _rnode_close,
  .vn_read     = _rnode_read,
  .vn_write    = _rnode_write,
  .vn_lseek    = _rnode_lseek,
  .vn_ioctl    = _rnode_ioctl,
  //  int (*vn_select)();
  .vn_getattr  = _rnode_getattr,
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

