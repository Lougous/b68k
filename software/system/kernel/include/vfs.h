//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//

#ifndef _vfs_h_
#define _vfs_h_


// temporary !
struct stat {
  dev_t *st_rdev;
};


struct vfs {
  struct vfs *vfs_next;            /* next vfs in list */
  const struct vfsops *vfs_op;           /* operations on vfs */
  struct vnode *vfs_vnodecovered;  /* vnode we cover */
  int vfs_flag;                    /* flags */
  //int vfs_bsize;                   /* native block size */
  u32_t vfs_data[(K_MAX_VFS_PRIVATE_LEN+3)/4];             /* private data */
};

struct vfsops {
  int (*vfs_mount)(struct vfs *pvfs, dev_t *pdev);
  int (*vfs_unmount)(struct vfs *pvfs);
  int (*vfs_root)(struct vfs *pvfs, struct vnode **ppv);
  //  int (*vfs_statfs)();
  //  int (*vfs_sync)();
  //  int (*vfs_fid)();
  //  int (*vfs_vget)();
};

enum vtype {
  VNON,
  VREG,
  VDIR,
  VBLK,
  VCHR,
  //  VLNK,
  //  VSOCK,
  VBAD
};

struct vnode {
  struct vnode *next;        /* next vnode in list */
  
  //  u16_t v_flag;     /* vnode flags */
  u16_t v_count;    /* reference count */
  //  u16_t v_shlockc;  /* # of shared locks */
  //  u16_t v_exlockc;  /* # of exclusive locks */

  struct vfs *v_vfsmountedhere;  /* covering vfs */
  const struct vnodeops *v_op;         /* vnode operations */

  //  union {
  //    struct socket *v_Socket;    /* unix ipc */
  //    struct stdata *v_Stream;    /* stream */
  //  };
  
  struct vfs *v_vfsp;  /* vfs we are in */
  enum vtype v_type;   /* vnode type */
  u32_t v_data[(K_MAX_VN_PRIVATE_LEN+3)/4];             /* private data */
};

struct vnodeops {
  int (*vn_open)(struct vnode *pvn, int flags, pid_t pid);
  int (*vn_close)(struct vnode *pvn, pid_t pid);
  size_t (*vn_read)(struct vnode *pvn, void *buf, size_t count, pid_t pid);
  size_t (*vn_write)(struct vnode *pvn, const void *buf, size_t count, pid_t pid);
  off_t (*vn_lseek)(struct vnode *pvn, off_t offset, int whence);
  int (*vn_ioctl)(struct vnode *pvn, int request, mem_va_t ptr, pid_t pid);
  //  int (*vn_select)();
  int (*vn_getattr)(struct vnode *pvn, struct stat *pto);
  //  int (*vn_setattr)();
  //  int (*vn_access)();
  int (*vn_lookup)(struct vnode *pv, char *nm, struct vnode **ppv, pid_t pid);
  //  int (*vn_create)();
  //  int (*vn_remove)();
  //  int (*vn_link)();
  //  int (*vn_rename)();
  int (*vn_mkdir)(struct vnode *pv, char *nm, pid_t pid);
  //  int (*vn_rmdir)();
  int (*vn_getdents)(struct vnode *pv, char *buf, unsigned int, pid_t pid);
  //  int (*vn_symlink)();
  //  int (*vn_readlink)();
  //  int (*vn_fsync)();
  int (*vn_inactive)(struct vnode *pvn);
  //  int (*vn_bmap)();
  //  int (*vn_strategy)();
  //  int (*vn_bread)();
  //  int (*vn_brelse)();
};

inline void VN_HOLD(struct vnode *pvn)
{
  K_PRINTF(3, "     VN_HOLD %Xh %u\n", pvn, pvn->v_count);
  pvn->v_count++;
}

inline void VN_RELE(struct vnode *pvn)
{
  K_PRINTF(3, "     VN_RELE %Xh %u\n", pvn, pvn->v_count);
  if (pvn->v_count) pvn->v_count--;

  if (! pvn->v_count) pvn->v_op->vn_inactive(pvn);
}


extern struct vnode *vfs_vnode_alloc();
extern void vfs_vnode_free(struct vnode *pvn);

#endif /* _vfs_h_ */
