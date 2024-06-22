
#ifndef _fs_h_
#define _fs_h_

#define FS_TYPE_FAT   1
#define FS_TYPE_DEV   2
#define FS_TYPE_RFS   3

typedef struct {
  dev_t *dev;
  
  u16_t BytesPerSector;
  u8_t  SectorsPerCluster;
  u8_t  FatCopyNumber;
  u32_t FatStartSector;
  u32_t RootStartSector;
  u32_t DataStartSector;
  u16_t MaxRootEntries;
  u32_t FirstSector;
  u32_t NbSector;
} fs_fat_context_t;

typedef struct {
  dev_t *dev;
} fs_rfs_context_t;

typedef union
{
  fs_fat_context_t fat;
  fs_rfs_context_t rfs;  
} fs_context_t;

typedef struct {
  fs_context_t *fs;  // fs_fat_context_t
  u8_t  Attributes;
  u16_t FirstCluster;
  u16_t CurrentCluster;
  u32_t CurrentSector;
  u32_t FileSize;
  u32_t CurrentByte;
  u16_t ByteInSector;
  u8_t SectorInCluster;
  u8_t buf[512];
} fs_fat_file_context_t;

typedef struct {
  fs_context_t *fs;  // fs_devfs_context_t
  dev_t *dev;
  int flags;
  u32_t CurrentDev;
  off_t lseek;
} fs_devfs_file_context_t;
  
typedef struct {
  fs_context_t *fs;  // fs_rfs_context_t
  dev_t *dev;
  int fd;
  int flags;
  off_t lseek;
  u32_t FileSize;
} fs_rfs_file_context_t;
  
typedef union
{
  fs_context_t *ctx;
  fs_fat_file_context_t fat;
  fs_devfs_file_context_t devfs;
  fs_rfs_file_context_t rfs;
} fs_file_context_t;

typedef struct fs_operations {
  int (*open)(fs_file_context_t *ctx, const char *pathname, int flags);
  int (*close)(fs_file_context_t *ctx);
  size_t (*read)(fs_file_context_t *ctx, void *buf, size_t count);
  size_t (*write)(fs_file_context_t *ctx, const void *buf, size_t count);
  off_t (*lseek)(fs_file_context_t *ctx, off_t offset, int whence);
  int (*getdents)(fs_file_context_t *ctx, struct dirent *dirp, unsigned int count);
  int (*ioctl)(fs_file_context_t *ctx, pid_t pid, int request, mem_va_t ptr);
  int (*mkdir)(fs_context_t *ctx, const char *pathname, mode_t mode);
} fs_operations_t;

extern const fs_operations_t _fs_fat_fsops;
extern const fs_operations_t _fs_devfs_fsops;
extern const fs_operations_t _fs_rfs_fsops;

extern int fat_mount(fs_context_t *ctx);
extern int rfs_mount(fs_context_t *ctx);

#endif /* _fs_h_ */
