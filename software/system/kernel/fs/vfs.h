
#ifndef _vfs_h_
#define _vfs_h_

#define FS_TYPE_FAT   1
#define FS_TYPE_DEV   2
#define FS_TYPE_RFS   3

struct vfs {
  struct vnode *vfs_vnodecovered;  // where this file system were mounted on
  //u16_t flags;
  struct vfsops *vfs_ops;          // FS operations
  void *vfs_data;                  // FS specific data
};


struct vfsops {
  u32_t (*vfs_mount)(struct vfs *pvfs, dev_t *pdev);
  int (*vfs_root)(struct vfs *pvfs, struct vnode **ppvnode);
};

/*
int
(*vfs_unmount)();
int
(*vfs_root)();
int
(*vfs_statfs)();
int
(*vfs_sync)();
int
(*vfs_fid)();
int
(*vfs_vget)();
*/

#endif /* _vfs_h_ */
