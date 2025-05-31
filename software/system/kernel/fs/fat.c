//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/kernel - FAT file system
//

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
#include "debug.h"
#include "dev.h"
#include "mem.h"
#include "vfs.h"
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

typedef u32_t fnode_t;  // start sector of the file/directory

// private data for struct vfs (.vfs_data)
struct fat {
  dev_t *dev;
  
  u16_t BytesPerSector;
  u8_t  SectorsPerCluster;
  u8_t  FatCopyNumber;
  u32_t FatStartSector;
  fnode_t RootStartSector;
  u32_t DataStartSector;
  u16_t MaxRootEntries;
  u32_t FirstSector;
  u32_t NbSector;

  // list of vnodes
  struct vnode *pvn;
};

// check struct vfs private data storage is enough for FAT - if not increase K_MAX_VFS_PRIVATE_LEN
extern char size_check_fat[(signed)K_MAX_VFS_PRIVATE_LEN-(signed)sizeof(struct fat)];

// private data for struct vnode (.v_data)
struct fnode {
  fnode_t fnode;

  // file info
  u8_t  Attributes;
  u16_t FirstCluster;
  u32_t FileSize;

  // current position in the file
  u16_t CurrentCluster;
  u8_t  CurrentSectorInCluster;  // in range 0 to SectorsPerCluster
  u32_t CurrentByte;             // from start of file (fseek)
  u16_t CurrentByteInSector;     // in range 0 to BytesPerSector-1

  // buffer cache. invalid if buf_sector = 0
  u32_t buf_sector;
  u8_t buf[512];
};

// check struct fnode private data storage is enough for FAT - if not increase K_MAX_VN_PRIVATE_LEN
extern char size_check_fnode[(signed)K_MAX_VN_PRIVATE_LEN-(signed)sizeof(struct fnode)];

const struct vnodeops _fat_vnodeops;

////////////////////////////////////////////////////////////////////////////////
// functions
////////////////////////////////////////////////////////////////////////////////
static u16_t _read_u16_le (u8_t *pc)
{
  return pc[0] + (((u16_t)pc[1]) << 8);
}

////////////////////////////////////////////////////////////////////////////////
// vnode/fnode management
////////////////////////////////////////////////////////////////////////////////
// find a vnode with matching fnode
static struct vnode *_find_vnode(struct fat *pfat, fnode_t fnode)
{
  struct vnode *pvn = pfat->pvn;

  while (pvn) {
    struct fnode *pfn = (struct fnode *)&(pvn->v_data[0]);

    if (pfn->fnode == fnode) {
      return pvn;
    }

    pvn = pvn->next;
  }

  // not found
  return 0;
}

static struct vnode *_allocate_vnode(struct vfs *pvfs, fnode_t fnode)
{
  struct vnode *pvn = vfs_vnode_alloc();

  if (!pvn) {
    return 0;
  }

  struct fat *pfat = (struct fat *)&(pvfs->vfs_data[0]);

  // init vnode
  pvn->v_count = 0;
  pvn->v_op    = &_fat_vnodeops;
  pvn->v_vfsp  = pvfs;

  struct fnode *pfn = (struct fnode *)&(pvn->v_data[0]);

  pfn->fnode    = fnode;
  pfn->Attributes      = 0;
  pfn->FirstCluster  = 0;
  pfn->FileSize        = 0;
  
  pfn->CurrentCluster  = 0;
  pfn->CurrentByteInSector    = 0;
  pfn->CurrentSectorInCluster = 0;
  pfn->CurrentByte     = 0;

  pfn->buf_sector   = 0;

  // add vnode to FS vnode list
  pvn->next = pfat->pvn;
  pfat->pvn = pvn;

  return pvn;
}


////////////////////////////////////////////////////////////////////////////////
// fat operations
////////////////////////////////////////////////////////////////////////////////
const struct vfsops _fat_vfs_op;

static int _fat_root (struct vfs *pvfs, struct vnode **ppv)
{
  struct fat *pfat = (struct fat *)&(pvfs->vfs_data[0]);

  struct vnode *pvroot = _find_vnode(pfat, pfat->RootStartSector);

  K_PRINTF(3, "fat: root: found %Xh, fnode=%u\n", pvroot, pfat->RootStartSector);

  if (pvroot) {
    // exists
    VN_HOLD(pvroot);
    *ppv = pvroot;
    return 0;
  }

  pvroot = _allocate_vnode(pvfs, pfat->RootStartSector);
  K_PRINTF(3, "fat: root: allocated %Xh\n", pvroot);

  if (!pvroot) {
    // out of ressource
    *ppv = 0;
    return -ENOMEM;
  }

  pvroot->v_type = VDIR;
  struct fnode *pfn = (struct fnode *)&(pvroot->v_data[0]);
  pfn->Attributes = _FAT_DIRENT_ATTR_DIRECTORY_MASK;
  
  VN_HOLD(pvroot);
  *ppv = pvroot;
  return 0;
}

int fat_mount (struct vfs *pvfs, dev_t *pdev)
{
  message_t msg;

  //int fat_mount(fs_context_t *ctx) {
  u32_t i;


  //  K_PRINTF(2, "FirstSector       : %u\n", ctx->fat.FirstSector);

  // read partition boot record
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.src_seek = pdev->attr.partition.sector_start*512;
  msg.body.dev_read.dst = &_buf[0];
  msg.body.dev_read.count = 512;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);
  
  if (msg.body.s32 != 512) return -4;
  
  K_PRINTF(3, "fat: partition label   : ");
  
  for (i = 0x2B; i < 0x2b+11; i++) putchar(_buf[i]);
  putchar('\n');

  pvfs->vfs_op           = (struct vfsops *)&_fat_vfs_op;
  pvfs->vfs_vnodecovered = 0;
  pvfs->vfs_flag         = 0;  // TODO

  struct fat *pfat = (struct fat *)&(pvfs->vfs_data[0]);
  pfat->dev = pdev;
  pfat->pvn = NULL;  // empty fnode list yet


  pfat->FirstSector = pdev->attr.partition.sector_start;
  pfat->NbSector    = pdev->attr.partition.sector_cnt;
  pfat->BytesPerSector    = _buf[0xB] + (_buf[0xC] << 8);
  pfat->SectorsPerCluster = _buf[0xD];
  pfat->FatCopyNumber     = _buf[0x10];
  pfat->MaxRootEntries    = _buf[0x11] + (_buf[0x12] << 8);

  if (pfat->BytesPerSector != 512) {
    K_PRINTF(3, "fat: unexpecting sector size (%u, expected 512)\n", pfat->BytesPerSector);
    return -1;
  }
  
  K_PRINTF(3, "fat: BytesPerSector    : %u\n", pfat->BytesPerSector);
  K_PRINTF(3, "fat: SectorsPerCluster : %u\n", pfat->SectorsPerCluster);
  K_PRINTF(3, "fat: FatCopyNumber     : %u\n", pfat->FatCopyNumber);
  
  // Start + # of Reserved Sectors
  pfat->FatStartSector = pfat->FirstSector + (_buf[0xE] + (_buf[0xF] << 8));

  // Start + # of Reserved + (# of Sectors Per FAT * 2)
  pfat->RootStartSector = pfat->FatStartSector + pfat->FatCopyNumber * (_buf[0x16] + (_buf[0x17] << 8));

  // FatRootStartSector + ((Maximum Root Directory Entries * 32) / Bytes per Sector)
  pfat->DataStartSector = pfat->RootStartSector + (32*pfat->MaxRootEntries)/512;

  K_PRINTF(3, "fat: RootStartSector   : %u\n", pfat->RootStartSector);
  K_PRINTF(3, "fat: DataStartSector   : %u\n", pfat->DataStartSector);

  // add vnode for root
  return _fat_root(pvfs, &(pfat->pvn));
}

static int _fat_unmount (struct vfs *pvfs)
{
  // TODO
  return -1;
}

const struct vfsops _fat_vfs_op = {
  .vfs_mount   = fat_mount,
  .vfs_unmount = _fat_unmount,
  .vfs_root    = _fat_root
};

////////////////////////////////////////////////////////////////////////////////
// fnode operations
////////////////////////////////////////////////////////////////////////////////
//
static char _to_upper(char c)
{
  // TODO special characters
  if ((c >= 'a') && (c <= 'z')) return c + ('A' - 'a');
  return c;
}

// convert name in 8.3 format
static char *_to_fat_name(char *src, char *dst)
{
  off_t i;

  for (i = 0; i < 11; i++) dst[i] = ' ';

  i = 0;
  
  while (*src && (i < 11)) {
    if (*src == '.') {
      i = 8;
    } else {
      dst[i++] = _to_upper(*src);
    }

    src++;

  }

  dst[11] = 0;

  return dst;
}


// get next cluster in FAT
static u16_t _fat_next_cluster(struct vnode *pvn, u16_t from)
{
  struct fat *pfat = (struct fat *)&(pvn->v_vfsp->vfs_data[0]);
  struct fnode *pfn = (struct fnode *)&(pvn->v_data[0]);
  
  dev_t *pdev = pfat->dev;
  u16_t to;
  message_t msg;

  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.src_seek = (pfat->FatStartSector + (from*2)/512)*512;
  msg.body.dev_read.dst = &pfn->buf[0];
  msg.body.dev_read.count = 512;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  //TODO: check return value

  pfn->buf_sector = 0;  // invalid
 
  to = pfn->buf[(from*2) & 0x1ff] + (pfn->buf[((from*2) & 0x1ff) + 1] << 8);

  return to;
}
  

// search for name through root directory
static int _fnode_lookup_root(struct vnode *pvn, char *nm, struct vnode **ppv, pid_t pid)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct fat *pfat = (struct fat *)&(pvn->v_vfsp->vfs_data[0]);
  struct fnode *pfn = (struct fnode *)&(pvn->v_data[0]);
  
  dev_t *pdev = pfat->dev;
  char name83[12];
  u16_t dent;
  u32_t seek = pfat->RootStartSector*512;
  message_t msg;
 
  for (dent = 0; dent < pfat->MaxRootEntries; dent++)
  {
    u16_t entry = dent & 0xf;
    
    if (!entry) {
      // read a sector every 16 entries
      // (up to 16 entries per sector)
      pfn->buf_sector = 0;  // invalid
      
      msg.type = DEV_READ;
      msg.body.dev_read.handle = pdev->handle;
      msg.body.dev_read.src_seek = seek;
      msg.body.dev_read.dst = &pfn->buf[0];
      msg.body.dev_read.count = 512;
  
      sendreceive(pdev->drv, &msg, O_SEND | O_RECV);
      
      if (msg.body.s32 != 512) return -1;

      seek += 512;
    }

    if ((pfn->buf[32*entry] != _FAT_FREE_ENTRY_TAG) &&
	(pfn->buf[32*entry+11] != _FAT_DIRENT_ATTR_VOLUME_LABEL) &&
	(pfn->buf[32*entry+11] != _FAT_DIRENT_ATTR_LFN)) {

      //printf("_fat_point_to_root: '%s' - '%s'\n",
      //     _to_fat_name((char *)name, len, &name83[0]),
      //     (char *)&ctx->buf[32*entry]);
      
      if (strncmp(_to_fat_name((char *)nm, &name83[0]), (char *)&pfn->buf[32*entry], 11) == 0) {
	// name match
	u32_t start_cluster = pfn->buf[32*entry+0x1A] + (pfn->buf[32*entry+0x1B] << 8);
	fnode_t sector = pfat->DataStartSector + (start_cluster - 2) * pfat->SectorsPerCluster;

	*ppv = _find_vnode(pfat, sector);

	if (*ppv) {
	  // already exists
	  VN_HOLD(*ppv);

	  return 0;
	}
   
	*ppv = _allocate_vnode(pvfs, sector);

	if (!*ppv) {
	  // out of resource
	  return -ENOMEM;
	}

	struct fnode *pfn_found = (struct fnode *)&((*ppv)->v_data[0]);

	pfn_found->Attributes      = pfn->buf[32*entry+0x0B];
	pfn_found->CurrentCluster  = start_cluster;
	pfn_found->FirstCluster    = start_cluster;
	pfn_found->FileSize        = pfn->buf[32*entry+0x1c] + (pfn->buf[32*entry+0x1d] << 8) +
	  (pfn->buf[32*entry+0x1e] << 16) + (pfn->buf[32*entry+0x1f] << 24);

	VN_HOLD(*ppv);
	(*ppv)->v_vfsmountedhere = 0;
	(*ppv)->v_type = pfn_found->Attributes & _FAT_DIRENT_ATTR_DIRECTORY_MASK ? VDIR : VREG;

	K_PRINTF(3, "fat: lookup '%s': fnode %Xh, type %u\n", nm, sector, (*ppv)->v_type);

	return 0;
      }
    }

    if (pfn->buf[32*entry] == 0) {
      // end of directory catalogue
      return 0;
    }
  }
  
  return 0;
}


// search for name through current directory
static int _fnode_lookup(struct vnode *pvn, char *nm, struct vnode **ppv, pid_t pid)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct fat *pfat = (struct fat *)&(pvn->v_vfsp->vfs_data[0]);
  struct fnode *pfn = (struct fnode *)&(pvn->v_data[0]);

  K_PRINTF(3, "fat: lookup '%s'\n", nm);
  
  if (pfn->fnode == pfat->RootStartSector) {
    // root directory
    return _fnode_lookup_root(pvn, nm, ppv, pid);
  }
  
  dev_t *pdev = pfat->dev;
  u16_t cluster;
  char name83[11];
  message_t msg;

  for (cluster = pfn->CurrentCluster; ; cluster = _fat_next_cluster(pvn, cluster))
  {
    u32_t ssec;
    u32_t sector = pfat->DataStartSector + (cluster - 2) * pfat->SectorsPerCluster;

    for (ssec = 0; ssec < pfat->SectorsPerCluster; ssec++, sector++) {
      int entry;
      
      // read a directory entry sector
      pfn->buf_sector = 0;  // invalid
      
      msg.type = DEV_READ;
      msg.body.dev_read.handle = pdev->handle;
      msg.body.dev_read.src_seek = sector*512;
      msg.body.dev_read.dst = &pfn->buf[0];
      msg.body.dev_read.count = 512;
  
      sendreceive(pdev->drv, &msg, O_SEND | O_RECV);
      
      if (msg.body.s32 != 512) return -1;

      pfn->buf_sector = sector;

      //printf("_fat_point_to: read sector %u @%x\n", sector, (u32_t)&pfn->buf[0]);
      
      // up to 16 entries per sector
      for (entry = 0; entry < 16; entry++) {
	if ((pfn->buf[32*entry] != _FAT_FREE_ENTRY_TAG) &&
	    (pfn->buf[32*entry+11] != _FAT_DIRENT_ATTR_VOLUME_LABEL) &&
	    (pfn->buf[32*entry+11] != _FAT_DIRENT_ATTR_LFN)) {
	  
	  //printf("_fat_point_to: '%s' - '%s'\n",
	  //     _to_fat_name((char *)name, len, &name83[0]),
	  //     (char *)&pfn->buf[32*entry]);
      
	  if (strncmp(_to_fat_name((char *)nm, &name83[0]), (char *)&pfn->buf[32*entry], 11) == 0) {
	    // name match
	    u32_t start_cluster = pfn->buf[32*entry+0x1A] + (pfn->buf[32*entry+0x1B] << 8);
	    fnode_t sector = pfat->DataStartSector + (start_cluster - 2) * pfat->SectorsPerCluster;

	    *ppv = _find_vnode(pfat, sector);

	    if (*ppv) {
	      // already exists
	      VN_HOLD(*ppv);

	      return 0;
	    }
   
	    *ppv = _allocate_vnode(pvfs, sector);

	    if (!*ppv) {
	      // out of resource
	      return -ENOMEM;
	    }

	    struct fnode *pfn_found = (struct fnode *)&((*ppv)->v_data[0]);

	    pfn_found->Attributes      = pfn->buf[32*entry+0x0B];
	    pfn_found->FirstCluster    = start_cluster;
	    pfn_found->FileSize        = pfn->buf[32*entry+0x1c] + (pfn->buf[32*entry+0x1d] << 8) +
	      (pfn->buf[32*entry+0x1e] << 16) + (pfn->buf[32*entry+0x1f] << 24);

	    VN_HOLD(*ppv);
	    (*ppv)->v_vfsmountedhere = 0;
	    (*ppv)->v_type = pfn_found->Attributes & _FAT_DIRENT_ATTR_DIRECTORY_MASK ? VDIR : VREG;

	    K_PRINTF(3, "fat: lookup '%s': fnode %Xh, type %u\n", nm, sector, (*ppv)->v_type);

	    return 0;
	  }

	  if (pfn->buf[32*entry] == 0) {
	    // end of directory catalogue
	    return -1;
	  }
	}
      }
    }
  }
  
  return -1;
}

static int _fnode_open(struct vnode *pvn, int flags, pid_t pid)
//int fat_open(fs_file_context_t *ctx, const char *pathname, int flags)
{
  struct fnode *pfn = (struct fnode *)&(pvn->v_data[0]);

  K_PRINTF(3, "fat: open: Attributes %Xh\n", pfn->Attributes);

  // rewind
  pfn->CurrentCluster         = pfn->FirstCluster;
  pfn->CurrentByteInSector    = 0;
  pfn->CurrentSectorInCluster = 0;
  pfn->CurrentByte            = 0;
  
  if ((pfn->Attributes & _FAT_DIRENT_ATTR_DIRECTORY_MASK) && (flags & O_DIRECTORY)) {
    // directory open
    return 0;
  } else if (!(pfn->Attributes & _FAT_DIRENT_ATTR_DIRECTORY_MASK) && !(flags & O_DIRECTORY)) {
    // file open
    return 0;
  }
  
  // flags mismatch
  return -1;  
}

static int _fnode_close(struct vnode *pvn, pid_t pid)
{
  return 0;
}

static size_t _fnode_read (struct vnode *pvn, void *buf, size_t count, pid_t pid)
{
  message_t msg;
  struct fat *pfat = (struct fat *)&(pvn->v_vfsp->vfs_data[0]);
  struct fnode *pfn = (struct fnode *)&(pvn->v_data[0]);
  dev_t *pdev = pfat->dev;
  size_t read = 0;

  //printf("-> fat_read\n");
  
  if (!(pfn->Attributes & _FAT_DIRENT_ATTR_DIRECTORY_MASK) &&
      ((pfn->CurrentByte + count) >= pfn->FileSize)) {
    // would reach end of file
    //printf("EOF\n");
    count = pfn->FileSize - pfn->CurrentByte;
  }

  while (count) {
    u32_t sector = pfat->DataStartSector + (pfn->CurrentCluster - 2) * pfat->SectorsPerCluster + pfn->CurrentSectorInCluster;
    
    if (pfn->buf_sector != sector) {
      // need to read sector
      pfn->buf_sector = 0;

      msg.type = DEV_READ;
      msg.body.dev_read.handle = pdev->handle;
      msg.body.dev_read.src_seek = sector*512;
      msg.body.dev_read.dst = &pfn->buf[0];
      msg.body.dev_read.count = 512;
 
      sendreceive(pdev->drv, &msg, O_SEND | O_RECV);
      
      if (msg.body.s32 != 512) {
	//printf("<- fat_read -1\n");
	return -1;
      }
      
      pfn->buf_sector = sector;
    }

    size_t bsize;
    
    if (count <= (pfat->BytesPerSector - pfn->CurrentByteInSector)) {
      bsize = count;
    } else {
      bsize = pfat->BytesPerSector - pfn->CurrentByteInSector;
    }

    memcpy(buf, &pfn->buf[pfn->CurrentByteInSector], bsize);

    read += bsize;
    count -= bsize;
    buf = (void *)((char *)buf + bsize);
    pfn->CurrentByte += bsize;

    pfn->CurrentByteInSector += bsize;

    if (pfn->CurrentByteInSector >= pfat->BytesPerSector) {
      // end of sector
      pfn->CurrentByteInSector = 0;
      pfn->buf_sector = 0;
      pfn->CurrentSectorInCluster++;

      // end of cluster
      if (pfn->CurrentSectorInCluster >= pfat->SectorsPerCluster) {
	pfn->CurrentSectorInCluster = 0;
	pfn->CurrentCluster  = _fat_next_cluster(pvn, pfn->CurrentCluster);
      }
    }
  }
    
  //printf("<- fat_read %d\n", read);
  
  return read;
}
  
static size_t _fnode_write (struct vnode *pvn, const void *buf, size_t count, pid_t pid)
{
  return 0;
}

static off_t _fnode_lseek (struct vnode *pvn, off_t offset, int whence)
{
  struct fat *pfat = (struct fat *)&(pvn->v_vfsp->vfs_data[0]);
  struct fnode *pfn = (struct fnode *)&(pvn->v_data[0]);

  if (whence == SEEK_END) {
    if (offset > pfn->FileSize) return -1;

    offset = pfn->FileSize - offset;
  }

  if (whence == SEEK_CUR) {
    if ((pfn->CurrentByte + offset) > pfn->FileSize) return -1;

    offset = pfn->CurrentByte + offset;
  }

  // rewind
  pfn->CurrentByteInSector    = 0;
  pfn->CurrentSectorInCluster = 0;
  pfn->CurrentCluster  = pfn->FirstCluster;

  size_t ssize = pfat->BytesPerSector;
    
  while (offset) {
    if (offset < ssize) {
      // last sector
      pfn->CurrentByteInSector = offset;
      offset = 0;
    } else {
      // go to next sector
      if (pfn->CurrentSectorInCluster == (pfat->SectorsPerCluster - 1)) {
	// go to next cluster
	pfn->CurrentSectorInCluster = 0;

	pfn->CurrentCluster = _fat_next_cluster(pvn, pfn->CurrentCluster);
      } else {
	pfn->CurrentSectorInCluster++;
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
  
static u8_t *_fat_get_root_entry(struct vnode *pvn)
{
  message_t msg;
  struct fat *pfat = (struct fat *)&(pvn->v_vfsp->vfs_data[0]);
  struct fnode *pfn = (struct fnode *)&(pvn->v_data[0]);
  dev_t *pdev = pfat->dev;

  K_PRINTF(3, "fat: _fat_get_root_entry vfsp = %Xh\n", pvn->v_vfsp);

  // 16 by sector, 512 bytes sectors
  // use CurrentSectorInCluster as a sector counter from RootStartSector
  u32_t sector = pfat->RootStartSector + pfn->CurrentSectorInCluster;
  //u16_t entry = pfn->CurrentByteInSector / _FAT_DIRECTORY_ENTRY_SIZE;
  u8_t *char_entry = &pfn->buf[pfn->CurrentByteInSector];

  //printf("_fat_get_root_entry: %i\n", entry);
  
  if (sector != pfn->buf_sector) {
    //printf("_fat_get_root_entry: read sector %u\n", sector);
    pfn->buf_sector = 0;
    
    msg.type = DEV_READ;
    msg.body.dev_read.handle = pdev->handle;
    msg.body.dev_read.src_seek = sector*512;
    msg.body.dev_read.dst = &pfn->buf[0];
    msg.body.dev_read.count = 512;
    
    sendreceive(pdev->drv, &msg, O_SEND | O_RECV);
    
    if (msg.body.s32 != 512) return 0;
      
    pfn->buf_sector = sector;
    //printf("_fat_get_root_entry: read OK at %x\n", (u32_t)&pfn->buf[0]);
  }

  pfn->CurrentByteInSector += _FAT_DIRECTORY_ENTRY_SIZE;

  if (pfn->CurrentByteInSector >= 512) {
    // end of sector
    pfn->CurrentByteInSector = 0;
    pfn->CurrentSectorInCluster++;
  }

  return char_entry;
}
      
    
static int _fnode_getdents (struct vnode *pvn, char *buf, unsigned int count, pid_t pid)
//int fat_getdents(fs_file_context_t *ctx, struct dirent *dirp, unsigned int count)
{
  int len = 0;
  struct fnode *pfn = (struct fnode *)&(pvn->v_data[0]);
  char de_buf[32];
  struct dirent *dirp = (struct dirent *)buf;

  K_PRINTF(3, "fat: getdents\n");
  
  // need actual directory
  if (!(pfn->Attributes & _FAT_DIRENT_ATTR_DIRECTORY_MASK)) return -1;

  // while enough space in buffer
  while (count >= _FAT_DIRENT_SIZE) {
    char *dentry;
    
    if (pfn->CurrentCluster == 0) {
      // root directory
      dentry = (char *)_fat_get_root_entry(pvn);

    } else {
      // other directories
      size_t read = _fnode_read(pvn, &de_buf[0], _FAT_DIRECTORY_ENTRY_SIZE, pid);

      //printf("read: %u\n", read);
      
      if (read != _FAT_DIRECTORY_ENTRY_SIZE) {
	return -1;
      }

      dentry = &de_buf[0];
    }


    K_PRINTF(3, "fat_getdents: entry %X (%X %X) \n", (u32_t)dentry, dentry[0], dentry[11]);

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
  
static int _fnode_ioctl (struct vnode *pvn, int request, mem_va_t ptr, pid_t pid)
{
  return -1;
}

static int _fnode_mkdir (struct vnode *pvn, char *nm, pid_t pid)
{
  return -EROFS;
}

static int _fnode_inactive (struct vnode *pvn)
{
  struct vfs *pvfs = pvn->v_vfsp;
  struct fat *pfat = (struct fat *)&(pvfs->vfs_data[0]);

  // assumes list not empty (at least root vnode should be present)
  struct vnode *pvn_list = pfat->pvn;

  if (pvn == pvn_list) {
    // remove 1st
    pfat->pvn = pvn->next;
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
  
  K_PRINTF(2, "fat: inactive failed !\n");
  
  return 0;
}

static int _fnode_getattr (struct vnode *pvn, struct stat *pto)
{
  // TODO
  return -1;
}


const struct vnodeops _fat_vnodeops = {
  .vn_open     = _fnode_open,
  .vn_close    = _fnode_close,
  .vn_read     = _fnode_read,
  .vn_write    = _fnode_write,
  .vn_lseek    = _fnode_lseek,
  .vn_ioctl    = _fnode_ioctl,
  //  int (*vn_select)();
  .vn_getattr  = _fnode_getattr,
  //  int (*vn_setattr)();
  //  int (*vn_access)();
  .vn_lookup   = _fnode_lookup,
  //  int (*vn_create)();
  //  int (*vn_remove)();
  //  int (*vn_link)();
  //  int (*vn_rename)();
  .vn_mkdir    = _fnode_mkdir,
  //  int (*vn_rmdir)();
  .vn_getdents = _fnode_getdents,
  //  int (*vn_symlink)();
  //  int (*vn_readlink)();
  //  int (*vn_fsync)();
  .vn_inactive = _fnode_inactive
  //  int (*vn_bmap)();
  //  int (*vn_strategy)();
  //  int (*vn_bread)();
  //  int (*vn_brelse)();
};

