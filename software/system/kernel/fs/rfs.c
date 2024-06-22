
#include <types.h>
#include <stddef.h>
#include <fcntl.h>
#include <dirent.h>
#include <syscall.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "dev.h"
#include "mem.h"
#include "fs.h"
#include "msg.h"
#include "debug.h"

u8_t _buf[256];

/*
static void _dump_msg(u8_t *buf, int len) {
  int i;
  for (i = 0; i < len; i++) K_PRINTF(2, "%Xh ", buf[i]);
  K_PRINTF(2, "\n");
}
*/

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
  
int rfs_mount(fs_context_t *ctx) {
  dev_t *pdev = ctx->rfs.dev;
  message_t msg;

  K_PRINTF(2, "rfs: probing connection ...\n");
  
  // open device
  msg.type = DEV_OPEN;
  msg.body.dev_open.handle = pdev->handle;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // send reset command
  _buf[0] = 0x55;
  _buf[1] = 1;  // len
  _buf[2] = 'I';
  _buf[3] = 'I';  // checksum
  _buf[4] = 0xAA;
  
  msg.type = DEV_WRITE;
  msg.body.dev_write.handle = pdev->handle;
  msg.body.dev_write.src = _buf;
  msg.body.dev_write.count = 5;
 
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // get reset command return
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = _buf;
  msg.body.dev_read.count = 5;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  //_dump_msg(_buf, 5);

  if (msg.body.s32 == 5 && _check_msg(_buf)) {
    K_PRINTF(2, "rfs: connected\n");
    return 0;
  } else {
    K_PRINTF(2, "rfs: failed to connect (%i)\n", msg.body.s32);
    return -1;
  }    
}

static u8_t _sum_name(u8_t *name)
{
  u8_t sum = 0;

  while (*name) {
    sum += *name++;
  }

  return sum;
}

int rfs_open(fs_file_context_t *ctx, const char *pathname, int flags)
{
  dev_t *pdev = ctx->rfs.fs->rfs.dev;
  message_t msg;
  int len = strlen(pathname);

  // send open command
  _buf[0] = 0x55;
  _buf[1] = 2 + len;      // len
  _buf[2] = 'O';          // open command
  _buf[3] = (char)flags;  // flags
  strcpy((char *)&_buf[4], pathname);
  _buf[4+len] = 'O' + (char)flags + _sum_name((u8_t *)pathname);  // checksum
  _buf[5+len] = 0xAA;
  
  msg.type = DEV_WRITE;
  msg.body.dev_write.handle = pdev->handle;
  msg.body.dev_write.src = _buf;
  msg.body.dev_write.count = 6+strlen(pathname);
 
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // get open command return
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = _buf;
  msg.body.dev_read.count = 10;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // answer
  // 0: 55h
  // 1: len=6 
  // 2: status 1=OK
  // 3: fd (if OK)
  // 4: file size (LSB)
  // 5: file size
  // 6: file size
  // 7: file size (MSB)
  // 8: checksum
  // 9: AAh
  if (msg.body.s32 == 10 && _check_msg(_buf) && _buf[2] == 1) {
    ctx->rfs.dev   = ctx->rfs.fs->rfs.dev;
    ctx->rfs.fd    = _buf[3];
    ctx->rfs.flags = flags;
    ctx->rfs.lseek = 0;

    if (flags != O_DIRECTORY) {
      ctx->rfs.FileSize =
	((u32_t)_buf[4]) +
	(((u32_t)_buf[5]) << 8) +
	(((u32_t)_buf[6]) << 16) +
	(((u32_t)_buf[7]) << 24);
    } else {
      ctx->rfs.FileSize = 0;
    }
    
    K_PRINTF(3, "rfs: open %u '%s', %u bytes\n", ctx->rfs.fd, pathname, ctx->rfs.FileSize);

    return 0;
  }
  
  /* couldn't open file */
  return -1;  
}

int rfs_close(fs_file_context_t *ctx)
{
  dev_t *pdev = ctx->rfs.fs->rfs.dev;
  message_t msg;

  // send close command
  _buf[0] = 0x55;
  _buf[1] = 2;            // len
  _buf[2] = 'C';          // close command
  _buf[3] = ctx->rfs.fd;  // fd
  _buf[4] = 'C' + ctx->rfs.fd;  // checksum
  _buf[5] = 0xAA;
  
  msg.type = DEV_WRITE;
  msg.body.dev_write.handle = pdev->handle;
  msg.body.dev_write.src = _buf;
  msg.body.dev_write.count = 6;
 
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // get open command return
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = _buf;
  msg.body.dev_read.count = 5;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // answer
  // 0: 55h
  // 1: len=1
  // 2: status 1=OK
  // 3: checksum
  // 4: AAh
  if (msg.body.s32 == 5 && _check_msg(_buf)) {
    if (_buf[2] == 1) {
      K_PRINTF(3, "rfs: close %u\n", ctx->rfs.fd);

      return 0;
    } else {
      K_PRINTF(2, "rfs: error while closing %u\n", ctx->rfs.fd);
      return -1;  
    }
  } else {
    K_PRINTF(2, "rfs: warning: bad frame\n");
  }
  
  /* couldn't close file */
  return -1;  
}

static u8_t _sum(u8_t *buf, u16_t len)
{
  u8_t sum = 0;

  while (len--) {
    sum += *buf++;
  }

  return sum;
}

size_t rfs_read(fs_file_context_t *ctx, void *buf, size_t count)
{
  dev_t *pdev = ctx->rfs.fs->rfs.dev;
  message_t msg;
  u8_t *dst = (u8_t *)buf;
  size_t done = 0;

  while (done < count) {
    u8_t len = (count - done) > 64 ? 64 : count - done;
    
    // send read command
    _buf[0] = 0x55;
    _buf[1] = 7;
    _buf[2] = 'R';
    _buf[3] = ctx->rfs.fd;
    _buf[4] = len;
    _buf[5] = (unsigned char)(ctx->rfs.lseek & 0xff);
    _buf[6] = (unsigned char)((ctx->rfs.lseek >> 8) & 0xff);
    _buf[7] = (unsigned char)((ctx->rfs.lseek >> 16) & 0xff);
    _buf[8] = (unsigned char)((ctx->rfs.lseek >> 24) & 0xff);
    _buf[9] = _sum(_buf + 2, 7);  // checksum
    _buf[10] = 0xAA;
  
    msg.type = DEV_WRITE;
    msg.body.dev_write.handle = pdev->handle;
    msg.body.dev_write.src = _buf;
    msg.body.dev_write.count = 11;
  
    sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

    // get read command return
    msg.type = DEV_READ;
    msg.body.dev_read.handle = pdev->handle;
    msg.body.dev_read.dst = _buf;
    msg.body.dev_read.count = 5+len;
  
    sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

    // answer
    // 0: 55h
    // 1: len
    // 2: actually used length in buffer, 0xFF if access failed
    // 3+: buf[count]
    // : checksum
    // : AAh

    //_dump_msg(_buf, 5+len);

    char blen = _buf[2];
    
    if (msg.body.s32 == 5+len && _check_msg(_buf) && blen >= 0) {
      // valid frame
      if (blen == 0) {
	// end of file
	break;
      }
      
      u8_t *src = &_buf[3];

      ctx->rfs.lseek += blen;
      
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
  
size_t rfs_write(fs_file_context_t *ctx, const void *buf, size_t count)
{
  return 0;
}

off_t rfs_lseek(fs_file_context_t *ctx, off_t offset, int whence)
{
  if (whence == SEEK_END) {
    if (offset > ctx->rfs.FileSize) return -1;

    offset = ctx->rfs.FileSize - offset;
  }

  if (whence == SEEK_CUR) {
    if ((ctx->rfs.lseek + offset) > ctx->rfs.FileSize) return -1;

    offset = ctx->rfs.lseek + offset;
  }

  ctx->rfs.lseek = offset;
  
  return 0;
}

int rfs_getdents(fs_file_context_t *ctx, struct dirent *dirp, unsigned int count)
{
  dev_t *pdev = ctx->rfs.fs->rfs.dev;
  message_t msg;

  // send getdents command
  _buf[0] = 0x55;
  _buf[1] = 3;
  _buf[2] = 'D';
  _buf[3] = ctx->rfs.fd;
  _buf[4] = count;
  _buf[5] = _sum(_buf + 2, 3);  // checksum
  _buf[6] = 0xAA;
  
  msg.type = DEV_WRITE;

  msg.body.dev_write.handle = pdev->handle;
  msg.body.dev_write.src = _buf;
  msg.body.dev_write.count = 7;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // get getdents answer
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = _buf;
  msg.body.dev_read.count = 5+count;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // answer format:
  // 0: 55h
  // 1: len=5+count
  // 2: type (1:file, 2:directory)
  // 3+: name[count] (null terminated string)
  // : checksum
  // : AAh

  //_dump_msg(_buf, 5+count);

  if (msg.body.s32 == 5+count && _check_msg(_buf)) {
    // valid frame
    if (_buf[3]) {
      _buf[3+count-offsetof(struct dirent, d_name)-1] = 0;  // limit max length

      int len = strlen((char *)&_buf[3]);

      dirp->d_type = 0;

      if (_buf[2] == 1) dirp->d_type = DT_REG;
      if (_buf[2] == 2) dirp->d_type = DT_DIR;
      
      dirp->d_size = offsetof(struct dirent, d_name) + len + 1;
      strcpy(dirp->d_name, (char *)&_buf[3]);
      
      return offsetof(struct dirent, d_name) + len + 1;
    }

    // end of directory
    return 0;
  }

  K_PRINTF(2, "rfs: getdents: warning: bad frame\n");
  return -1;
}

int rfs_ioctl(fs_file_context_t *ctx, pid_t pid, int request, mem_va_t ptr)
{
  return -1;
}

static int _rfs_mkdir(fs_context_t *ctx, const char *pathname, mode_t mode)
{
  dev_t *pdev = ctx->rfs.dev;
  message_t msg;

  int len = strlen(pathname);

  // send open command
  _buf[0] = 0x55;
  _buf[1] = 3 + len;            // len
  _buf[2] = 'M';                // mkdir command
  _buf[3] = (char)mode;         // mode
  _buf[4] = (char)(mode >> 8);  // mode
  strcpy((char *)&_buf[5], pathname);
  _buf[5+len] = 'M' + (char)mode + (char)(mode >> 8) + _sum_name((u8_t *)pathname);  // checksum
  _buf[6+len] = 0xAA;
  
  msg.type = DEV_WRITE;
  msg.body.dev_write.handle = pdev->handle;
  msg.body.dev_write.src = _buf;
  msg.body.dev_write.count = 7+strlen(pathname);
 
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // get open command return
  msg.type = DEV_READ;
  msg.body.dev_read.handle = pdev->handle;
  msg.body.dev_read.dst = _buf;
  msg.body.dev_read.count = 10;
  
  sendreceive(pdev->drv, &msg, O_SEND | O_RECV);

  // answer
  // 0: 55h
  // 1: len=2 
  // 2: status 1=OK
  // 3: errno (if KO)
  // 8: checksum
  // 9: AAh
  if (msg.body.s32 == 6 && _check_msg(_buf) && _buf[2] == 1) {
    K_PRINTF(3, "rfs: mkdir '%s'\n", pathname);

    return 0;
  }
  
  /* couldn't open file */
  return _buf[3];  
}


const struct fs_operations _fs_rfs_fsops = {
  .open      = rfs_open,
  .close     = rfs_close,
  .read      = rfs_read,
  .write     = rfs_write,
  .lseek     = rfs_lseek,
  .getdents  = rfs_getdents,
  .ioctl     = rfs_ioctl,
  .mkdir     = _rfs_mkdir
};

