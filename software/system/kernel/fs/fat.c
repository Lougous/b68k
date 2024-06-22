
#include <types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <syscall.h>
#include <sys/stat.h>
#include <errno.h>

#include "config.h"
#include "dev.h"
#include "mem.h"
#include "fs.h"
#include "debug.h"
#include "msg.h"

#define _FAT_SECTOR_NONE   0xFFFF  // TODO

#define _FAT_DIRENT_ATTR_READ_ONLY_MASK     0x01
#define _FAT_DIRENT_ATTR_HIDDEN_MASK        0x02
#define _FAT_DIRENT_ATTR_SYSTEM_FILE_MASK   0x04
#define _FAT_DIRENT_ATTR_DIRECTORY_MASK     0x10
#define _FAT_DIRENT_ATTR_ARCHIVE_MASK       0x20

#define _FAT_DIRENT_ATTR_VOLUME_LABEL       0x08
#define _FAT_DIRENT_ATTR_LFN                0x0F

#define _FAT_FREE_ENTRY_TAG                 0xE5

static u8_t _buf[512];
static message_t _msg;

int fat_mount(fs_context_t *ctx) {
  dev_t *pdev = ctx->fat.dev;
  u32_t i;

  //  K_PRINTF(2, "FirstSector       : %u\n", ctx->fat.FirstSector);

  // read partition boot record
  _msg.type = DEV_READ;
  _msg.body.dev_read.handle = pdev->handle;
  _msg.body.dev_read.src_seek = ctx->fat.FirstSector*512;
  _msg.body.dev_read.dst = &_buf[0];
  _msg.body.dev_read.count = 512;
  
  sendreceive(pdev->drv, &_msg, O_SEND | O_RECV);
  
  if (_msg.body.s32 != 512) return -4;
  
  printf("partition label   : ");
  for (i = 0x2B; i < 0x2b+11; i++) putchar(_buf[i]);
  putchar('\n');
  
  ctx->fat.BytesPerSector    = _buf[0xB] + (_buf[0xC] << 8);
  ctx->fat.SectorsPerCluster = _buf[0xD];
  ctx->fat.FatCopyNumber     = _buf[0x10];
  ctx->fat.MaxRootEntries    = _buf[0x11] + (_buf[0x12] << 8);

  if (ctx->fat.BytesPerSector != 512) {
    printf("unexpecting sector size (%u, expected 512)\n", ctx->fat.BytesPerSector);
    return -1;
  }
  
  //K_PRINTF(3, "BytesPerSector    : %u\n", ctx->fat.BytesPerSector);
  //K_PRINTF(3, "SectorsPerCluster : %u\n", ctx->fat.SectorsPerCluster);
  //K_PRINTF(3, "FatCopyNumber     : %u\n", ctx->fat.FatCopyNumber);
  
  // Start + # of Reserved Sectors
  ctx->fat.FatStartSector = ctx->fat.FirstSector + (_buf[0xE] + (_buf[0xF] << 8));

  // Start + # of Reserved + (# of Sectors Per FAT * 2)
  ctx->fat.RootStartSector = ctx->fat.FatStartSector + ctx->fat.FatCopyNumber * (_buf[0x16] + (_buf[0x17] << 8));

  // FatRootStartSector + ((Maximum Root Directory Entries * 32) / Bytes per Sector)
  ctx->fat.DataStartSector = ctx->fat.RootStartSector + (32*ctx->fat.MaxRootEntries)/512;

  //K_PRINTF(3, "RootStartSector   : %u\n", ctx->fat.RootStartSector);
  //K_PRINTF(3, "DataStartSector   : %u\n", ctx->fat.DataStartSector);
  
  return 0;
}

// return 1 when a / separated part is found in pathname
static int _extract_name(const char *pathname, int *len)
{
  const char *c = pathname;
  
  for ( ; *c && (*c != '/'); c++ );

  *len = (int)(c - pathname);

  if (*c == '/') return 1;

  return 0;
}

//
static char _to_upper(char c)
{
  // TODO special characters
  if ((c >= 'a') && (c <= 'z')) return c + ('A' - 'a');
  return c;
}

// convert name in 8.3 format
static char *_to_fat_name(char *src, int len, char *dst)
{
  off_t i;

  for (i = 0; i < 11; i++) dst[i] = ' ';

  i = 0;
  
  while (*src && (i < 11) && len) {
    if (*src == '.') {
      i = 8;
    } else {
      dst[i++] = _to_upper(*src);
    }

    src++;
    len--;
  }

  dst[11] = 0;

  return dst;
}


// get next cluster in FAT
static u16_t _fat_next_cluster(fs_fat_file_context_t *ctx, u16_t from)
{
  dev_t *pdev = ctx->fs->fat.dev;
  u16_t to;

  _msg.type = DEV_READ;
  _msg.body.dev_read.handle = pdev->handle;
  _msg.body.dev_read.src_seek = (ctx->fs->fat.FatStartSector + (from*2)/512)*512;
  _msg.body.dev_read.dst = &ctx->buf[0];
  _msg.body.dev_read.count = 512;
  
  sendreceive(pdev->drv, &_msg, O_SEND | O_RECV);

  //TODO: check return value

  ctx->CurrentSector = 0;  // invalid
 
  to = ctx->buf[(from*2) & 0x1ff] + (ctx->buf[((from*2) & 0x1ff) + 1] << 8);

  return to;
}
  

// search for name through root directory
// return 1 when found
static int _fat_point_to_root(fs_fat_file_context_t *ctx, const char *name, int len)
{
  dev_t *pdev = ctx->fs->fat.dev;
  char name83[12];
  u16_t dent;
  u32_t seek = ctx->fs->fat.RootStartSector*512;
  
  for (dent = 0; dent < ctx->fs->fat.MaxRootEntries; dent++)
  {
    u16_t entry = dent & 0xf;
    
    if (!entry) {
      // read a sector every 16 entries
      // (up to 16 entries per sector)
      ctx->CurrentSector = 0;  // invalid
      
      _msg.type = DEV_READ;
      _msg.body.dev_read.handle = pdev->handle;
      _msg.body.dev_read.src_seek = seek;
      _msg.body.dev_read.dst = &ctx->buf[0];
      _msg.body.dev_read.count = 512;
  
      sendreceive(pdev->drv, &_msg, O_SEND | O_RECV);
      
      if (_msg.body.s32 != 512) return 0;

      seek += 512;
    }

    if ((ctx->buf[32*entry] != _FAT_FREE_ENTRY_TAG) &&
	(ctx->buf[32*entry+11] != _FAT_DIRENT_ATTR_VOLUME_LABEL) &&
	(ctx->buf[32*entry+11] != _FAT_DIRENT_ATTR_LFN)) {

      //printf("_fat_point_to_root: '%s' - '%s'\n",
      //     _to_fat_name((char *)name, len, &name83[0]),
      //     (char *)&ctx->buf[32*entry]);
      
      if (strncmp(_to_fat_name((char *)name, len, &name83[0]), (char *)&ctx->buf[32*entry], 11) == 0) {
	// name match
	ctx->Attributes      = ctx->buf[32*entry+0x0B];
	ctx->CurrentCluster  = ctx->buf[32*entry+0x1A] + (ctx->buf[32*entry+0x1B] << 8);
	ctx->FirstCluster    = ctx->CurrentCluster;
	ctx->ByteInSector    = 0;
	ctx->SectorInCluster = 0;
	ctx->FileSize        = ctx->buf[32*entry+0x1c] + (ctx->buf[32*entry+0x1d] << 8) +
	  (ctx->buf[32*entry+0x1e] << 16) + (ctx->buf[32*entry+0x1f] << 24);
	//K_PRINTF(2, "_fat_point_to_root: %s (%u)\n", name, ctx->CurrentCluster);
	return 1;
      }
    }

    if (ctx->buf[32*entry] == 0) {
      // end of directory catalogue
      return 0;
    }
  }
  
  return 0;
}


// search for name through current directory
// return 1 when found
static int _fat_point_to(fs_fat_file_context_t *ctx, const char *name, int len)
{
  dev_t *pdev = ctx->fs->fat.dev;
  u16_t cluster;
  char name83[11];

  for (cluster = ctx->CurrentCluster; ; cluster = _fat_next_cluster(ctx, cluster))
  {
    u32_t ssec;
    u32_t sector = ctx->fs->fat.DataStartSector + (cluster - 2) * ctx->fs->fat.SectorsPerCluster;

    for (ssec = 0; ssec < ctx->fs->fat.SectorsPerCluster; ssec++, sector++) {
      int entry;
      
      // read a directory entry sector
      ctx->CurrentSector = 0;  // invalid
      
      _msg.type = DEV_READ;
      _msg.body.dev_read.handle = pdev->handle;
      _msg.body.dev_read.src_seek = sector*512;
      _msg.body.dev_read.dst = &ctx->buf[0];
      _msg.body.dev_read.count = 512;
  
      sendreceive(pdev->drv, &_msg, O_SEND | O_RECV);
      
      if (_msg.body.s32 != 512) return 0;

      ctx->CurrentSector = sector;

      //printf("_fat_point_to: read sector %u @%x\n", sector, (u32_t)&ctx->buf[0]);
      
      // up to 16 entries per sector
      for (entry = 0; entry < 16; entry++) {
	if ((ctx->buf[32*entry] != _FAT_FREE_ENTRY_TAG) &&
	    (ctx->buf[32*entry+11] != _FAT_DIRENT_ATTR_VOLUME_LABEL) &&
	    (ctx->buf[32*entry+11] != _FAT_DIRENT_ATTR_LFN)) {
	  
	  //printf("_fat_point_to: '%s' - '%s'\n",
	  //     _to_fat_name((char *)name, len, &name83[0]),
	  //     (char *)&ctx->buf[32*entry]);
      
	  if (strncmp(_to_fat_name((char *)name, len, &name83[0]), (char *)&ctx->buf[32*entry], 11) == 0) {
	    // name match
	    ctx->Attributes      = ctx->buf[32*entry+0x0B];
	    ctx->CurrentCluster  = ctx->buf[32*entry+0x1A] + (ctx->buf[32*entry+0x1B] << 8);
	    ctx->FirstCluster    = ctx->CurrentCluster;
	    ctx->ByteInSector    = 0;
	    ctx->SectorInCluster = 0;
	    ctx->FileSize        = ctx->buf[32*entry+0x1c] + (ctx->buf[32*entry+0x1d] << 8) +
	      (ctx->buf[32*entry+0x1e] << 16) + (ctx->buf[32*entry+0x1f] << 24);
	    //K_PRINTF(2, "_fat_point_to: %s (%u)\n", name, ctx->CurrentCluster);
	    return 1;
	  }

	  if (ctx->buf[32*entry] == 0) {
	    // end of directory catalogue
	    return 0;
	  }
	}
      }
    }
  }
  
  return 0;
}

int fat_open(fs_file_context_t *ctx, const char *pathname, int flags)
{
  off_t len;

  ctx->fat.ByteInSector    = 0;
  ctx->fat.SectorInCluster = 0;
  ctx->fat.CurrentCluster  = 0;
  ctx->fat.CurrentSector   = 0;
  ctx->fat.Attributes      = 0;
  ctx->fat.CurrentByte     = 0;
  ctx->fat.FileSize        = 0;
  
  // remove heading /
  while (*pathname == '/') pathname++;

  if (! *pathname) {
    // open root directory
    ctx->fat.Attributes = _FAT_DIRENT_ATTR_DIRECTORY_MASK;

    //K_PRINTF(2, "fat_open: /\n");
    return 0;
  }

  _extract_name(pathname, &len);

  //K_PRINTF(2, "fat_open: %s - %i\n", pathname, len);
 
  if (!_fat_point_to_root(&ctx->fat, pathname, len)) {
    // failed to enter root directory or point to file
    // TODO: create file when write mode
    return -1;
  }
  
  pathname += len;

  //K_PRINTF(2, "fat_open: %s\n", pathname);
 
  // remove heading /
  while (*pathname == '/') pathname++;

  while (*pathname) {
    //K_PRINTF(2, "fat_open: %s\n", pathname);
 
    _extract_name(pathname, &len);
    
    //K_PRINTF(2, "fat_open: %s - %i\n", pathname, len);
 
    if (!_fat_point_to(&ctx->fat, pathname, len)) {
      // failed to enter directory or point to file
      // TODO: create file when write mode
      //K_PRINTF(2, "failed to enter directory or point to file\n");
      return -1;
    }

    // continue with next tree level
    pathname += len;

    // remove heading /
    while (*pathname == '/') pathname++;
  }

  //printf("ATTR : %x  %i\n", ctx->fat.Attributes, flags);

  if ((ctx->fat.Attributes & _FAT_DIRENT_ATTR_DIRECTORY_MASK) && (flags & O_DIRECTORY)) {
    // directory open
    return 0;
  } else if (!(ctx->fat.Attributes & _FAT_DIRENT_ATTR_DIRECTORY_MASK) && !(flags & O_DIRECTORY)) {
    // file open
    return 0;
  }
  
  // flags mismatch
  return -1;  
}

int fat_close(fs_file_context_t *ctx)
{
  return 0;
}

size_t fat_read(fs_file_context_t *ctx, void *buf, size_t count)
{
  dev_t *pdev = ctx->fat.fs->fat.dev;
  size_t read = 0;

  //printf("-> fat_read\n");
  
  if (!(ctx->fat.Attributes & _FAT_DIRENT_ATTR_DIRECTORY_MASK) &&
      ((ctx->fat.CurrentByte + count) >= ctx->fat.FileSize)) {
    // would reach end of file
    //printf("EOF\n");
    count = ctx->fat.FileSize - ctx->fat.CurrentByte;
  }

  while (count) {
    u32_t sector = ctx->fat.fs->fat.DataStartSector + (ctx->fat.CurrentCluster - 2) * ctx->fat.fs->fat.SectorsPerCluster + ctx->fat.SectorInCluster;
    
    if (ctx->fat.CurrentSector != sector) {
      // need to read sector
      ctx->fat.CurrentSector = 0;

      _msg.type = DEV_READ;
      _msg.body.dev_read.handle = pdev->handle;
      _msg.body.dev_read.src_seek = sector*512;
      _msg.body.dev_read.dst = &ctx->fat.buf[0];
      _msg.body.dev_read.count = 512;
 
      sendreceive(pdev->drv, &_msg, O_SEND | O_RECV);
      
      if (_msg.body.s32 != 512) {
	//printf("<- fat_read -1\n");
	return -1;
      }
      
      ctx->fat.CurrentSector = sector;
    }

    size_t bsize;
    
    if (count <= (ctx->fat.fs->fat.BytesPerSector - ctx->fat.ByteInSector)) {
      bsize = count;
    } else {
      bsize = ctx->fat.fs->fat.BytesPerSector - ctx->fat.ByteInSector;
    }

    memcpy(buf, &ctx->fat.buf[ctx->fat.ByteInSector], bsize);

    read += bsize;
    count -= bsize;
    buf = (void *)((char *)buf + bsize);
    ctx->fat.CurrentByte += bsize;

    ctx->fat.ByteInSector += bsize;

    if (ctx->fat.ByteInSector >= ctx->fat.fs->fat.BytesPerSector) {
      // end of sector
      ctx->fat.ByteInSector = 0;
      ctx->fat.CurrentSector = 0;
      ctx->fat.SectorInCluster++;

      // end of cluster
      if (ctx->fat.SectorInCluster >= ctx->fat.fs->fat.SectorsPerCluster) {
	ctx->fat.SectorInCluster = 0;
	ctx->fat.CurrentCluster  = _fat_next_cluster((fs_fat_file_context_t *)ctx, ctx->fat.CurrentCluster);
      }
    }
  }
    
  //printf("<- fat_read %d\n", read);
  
  return read;
}
  
size_t fat_write(fs_file_context_t *ctx, const void *buf, size_t count)
{
  //dev_t *pdev = ctx->fat.fs->fat.dev;
  return 0;
}

off_t fat_lseek(fs_file_context_t *ctx, off_t offset, int whence)
{
  if (whence == SEEK_END) {
    if (offset > ctx->fat.FileSize) return -1;

    offset = ctx->fat.FileSize - offset;
  }

  if (whence == SEEK_CUR) {
    if ((ctx->fat.CurrentByte + offset) > ctx->fat.FileSize) return -1;

    offset = ctx->fat.CurrentByte + offset;
  }

  // rewind
  ctx->fat.ByteInSector    = 0;
  ctx->fat.SectorInCluster = 0;
  ctx->fat.CurrentCluster  = ctx->fat.FirstCluster;

  size_t ssize = ctx->fat.fs->fat.BytesPerSector;
    
  while (offset) {
    //u32_t sector = ctx->fat.fs->fat.DataStartSector + (ctx->fat.CurrentCluster - 2) * ctx->fat.fs->fat.SectorsPerCluster + ctx->fat.SectorInCluster;

    if (offset < ssize) {
      // last sector
      ctx->fat.ByteInSector = offset;
      offset = 0;
    } else {
      // go to next sector
      if (ctx->fat.SectorInCluster == (ctx->fat.fs->fat.SectorsPerCluster - 1)) {
	// go to next cluster
	ctx->fat.SectorInCluster = 0;

	ctx->fat.CurrentCluster = _fat_next_cluster((fs_fat_file_context_t *)ctx, ctx->fat.CurrentCluster);
      } else {
	ctx->fat.SectorInCluster++;
      }

      offset -= ssize;
    }
  }
  
  return 0;
}

#define _FAT_DIRECTORY_ENTRY_SIZE  32
#define _FAT_DIRENT_SIZE  (offsetof(struct dirent, d_name) + 8 + 1 + 3 + 1)

// convert a 11 chars string as found in directory entry to regular string name
char *_to_string_name(char *dst, char *src) {
  int b;
  char *d = dst;

  for (b = 0; b < 8; b++) {
    if (src[b] == ' ') break;
    *d++ = src[b];
  }

  if (src[8] != ' ') {
    // any extension
    *d++ = '.';

    for (b = 8; b < 11; b++) {
      if (src[b] == ' ') break;
      *d++ = src[b];
    }
  }

  *d = 0;

  //printf("_to_string_name: %s\n", dst);
   
  return dst;
}
  
u8_t *_fat_get_root_entry(fs_file_context_t *ctx)
{
  dev_t *pdev = ctx->fat.fs->fat.dev;

  // 16 by sector, 512 bytes sectors
  // use SectorInCluster as a sector counter from RootStartSector
  u32_t sector = ctx->fat.fs->fat.RootStartSector + ctx->fat.SectorInCluster;
  //u16_t entry = ctx->fat.ByteInSector / _FAT_DIRECTORY_ENTRY_SIZE;
  u8_t *char_entry = &ctx->fat.buf[ctx->fat.ByteInSector];

  //printf("_fat_get_root_entry: %i\n", entry);
  
  if (sector != ctx->fat.CurrentSector) {
    //printf("_fat_get_root_entry: read sector %u\n", sector);
    ctx->fat.CurrentSector = 0;
    
    _msg.type = DEV_READ;
    _msg.body.dev_read.handle = pdev->handle;
    _msg.body.dev_read.src_seek = sector*512;
    _msg.body.dev_read.dst = &ctx->fat.buf[0];
    _msg.body.dev_read.count = 512;
    
    sendreceive(pdev->drv, &_msg, O_SEND | O_RECV);
    
    if (_msg.body.s32 != 512) return 0;
      
    ctx->fat.CurrentSector = sector;
    //printf("_fat_get_root_entry: read OK at %x\n", (u32_t)&ctx->fat.buf[0]);
  }

  ctx->fat.ByteInSector += _FAT_DIRECTORY_ENTRY_SIZE;

  if (ctx->fat.ByteInSector >= 512) {
    // end of sector
    ctx->fat.ByteInSector = 0;
    ctx->fat.SectorInCluster++;
  }

  return char_entry;
}
      
    
int fat_getdents(fs_file_context_t *ctx, struct dirent *dirp, unsigned int count)
{
  int len = 0;
  char de_buf[32];
  
  // need actual directory
  if (!(ctx->fat.Attributes & _FAT_DIRENT_ATTR_DIRECTORY_MASK)) return -1;

  // while enough space in buffer
  while (count >= _FAT_DIRENT_SIZE) {
    char *dentry;
    
    if (ctx->fat.CurrentCluster == 0) {
      // root directory
      dentry = (char *)_fat_get_root_entry(ctx);

      //printf("fat_getdents: entry %x\n", (u32_t)dentry);
    } else {
      // other directories
      size_t read = fat_read(ctx, &de_buf[0], _FAT_DIRECTORY_ENTRY_SIZE);

      //printf("read: %u\n", read);
      
      if (read != _FAT_DIRECTORY_ENTRY_SIZE) {
	return -1;
      }

      dentry = &de_buf[0];
    }

    if (
	(dentry[0] != _FAT_FREE_ENTRY_TAG) &&
	(dentry[11] != _FAT_DIRENT_ATTR_LFN) &&
	(dentry[11] != _FAT_DIRENT_ATTR_VOLUME_LABEL)) {
      // ignore free locations, long file names and volume labels
      if (dentry[0] == 0) {
	// end of catalogue
	return len;
      }
    
      dirp->d_size = _FAT_DIRENT_SIZE;
    
      if (dentry[11] & _FAT_DIRENT_ATTR_DIRECTORY_MASK) {
	dirp->d_type = DT_DIR;
      } else {
	dirp->d_type = DT_REG;
      }

      _to_string_name(&dirp->d_name[0], dentry);

      dirp->d_size = _FAT_DIRENT_SIZE;  // TODO
    
      // next entry
      dirp = (struct dirent *)((void *)dirp + _FAT_DIRENT_SIZE);
      len += _FAT_DIRENT_SIZE;
      count -= _FAT_DIRENT_SIZE;
    }
  }
  
  return len;
}
  
static int _fat_ioctl(fs_file_context_t *ctx, pid_t pid, int request, mem_va_t ptr)
{
  return -1;
}

static int _fat_mkdir(fs_context_t *ctx, const char *pathname, mode_t mode)
{
  return EROFS;
}

const struct fs_operations _fs_fat_fsops = {
  .open      = fat_open,
  .close     = fat_close,
  .read      = fat_read,
  .write     = fat_write,
  .lseek     = fat_lseek,
  .getdents  = fat_getdents,
  .ioctl     = _fat_ioctl,
  .mkdir     = _fat_mkdir
};

