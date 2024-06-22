
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
#include "dev.h"
#include "mem.h"
#include "fs.h"
#include "msg.h"
#include "debug.h"

static message_t _msg;

int devfs_open(fs_file_context_t *ctx, const char *pathname, int flags)
{
  // remove heading /
  while (*pathname == '/') pathname++;

  ctx->devfs.flags      = flags;
  ctx->devfs.CurrentDev = 0;
  ctx->devfs.dev        = 0;
  ctx->devfs.lseek      = 0;

  if (*pathname == 0) {
    if (flags == O_DIRECTORY) {
      // read devfs root directory
      return 0;
    } else {
      return -1;
    }
  }
  
  dev_t *pdev = dev_get(pathname);

  if (!pdev) return -1;
  
  ctx->devfs.dev = pdev;
 
  _msg.type = DEV_OPEN;
  _msg.body.dev_open.handle = pdev->handle;
  _msg.body.dev_open.flags = flags;
 
  sendreceive(pdev->drv, &_msg, O_SEND | O_RECV);

  return _msg.body.s32;
}

int devfs_close(fs_file_context_t *ctx)
{
  dev_t *pdev = ctx->devfs.dev;

  if (pdev) {
    _msg.type = DEV_CLOSE;  
    _msg.body.dev_close.handle = pdev->handle;

    sendreceive(pdev->drv, &_msg, O_SEND | O_RECV);

    return _msg.body.u32;
  } else {
    return 0;
  }
}

size_t devfs_read(fs_file_context_t *ctx, void *buf, size_t count)
{
  dev_t *pdev = ctx->devfs.dev;
  
  if (pdev) {
    _msg.type = DEV_READ;
    _msg.body.dev_read.handle = pdev->handle;
    _msg.body.dev_read.src_seek = ctx->devfs.lseek;
    _msg.body.dev_read.dst = buf;
    _msg.body.dev_read.count = count;
 
    sendreceive(pdev->drv, &_msg, O_SEND | O_RECV);

    ctx->devfs.lseek += _msg.body.s32;

    return _msg.body.s32;
  } else {
    return -1;
  }
}

size_t devfs_write(fs_file_context_t *ctx, const void *buf, size_t count)
{
  dev_t *pdev = ctx->devfs.dev;
  
  if (pdev) {
    _msg.type = DEV_WRITE;
    _msg.body.dev_write.handle = pdev->handle;
    _msg.body.dev_write.src = (void *)buf;
    _msg.body.dev_write.dst_seek = ctx->devfs.lseek;
    _msg.body.dev_write.count = count;
 
    sendreceive(pdev->drv, &_msg, O_SEND | O_RECV);

    ctx->devfs.lseek += _msg.body.s32;

    return _msg.body.s32;
  } else {
    return -1;
  }
}

int devfs_ioctl(fs_file_context_t *ctx, pid_t pid, int request, mem_va_t ptr)
{
  dev_t *pdev = ctx->devfs.dev;
  
  if (pdev) {
    _msg.type = DEV_IOCTL;
    _msg.body.dev_ioctl.handle  = pdev->handle;
    _msg.body.dev_ioctl.pid     = pid;  // TODO
    _msg.body.dev_ioctl.request = request;
    _msg.body.dev_ioctl.va_ptr  = (void *)ptr;
 
    sendreceive(pdev->drv, &_msg, O_SEND | O_RECV);

    return _msg.body.s32;
  } else {
    return -1;
  }
}

off_t devfs_lseek(fs_file_context_t *ctx, off_t offset, int whence)
{
  if (whence == SEEK_END) {
    // cannot seek from end
    return -1;
  }

  if (whence == SEEK_CUR) {
    offset = ctx->devfs.lseek + offset;
  }

  ctx->devfs.lseek = offset;
  
  return 0;
}

int devfs_getdents(fs_file_context_t *ctx, struct dirent *dirp, unsigned int count)
{
  // need actual directory
  if (ctx->devfs.dev != 0 || ctx->devfs.flags != O_DIRECTORY) {
    return -1;
  }

  char *ndev = dev_name(ctx->devfs.CurrentDev);

  if (ndev) {
    int size = offsetof(struct dirent, d_name) + strlen(ndev) + 1;

    if (size <= count) { 
      dirp->d_size = size;
      dirp->d_type = DT_CHR;  // TODO

      strcpy(dirp->d_name, ndev);

      ctx->devfs.CurrentDev++;

      return size;
    }
  }
  
  return 0;
}

static int _devfs_mkdir(fs_context_t *ctx, const char *pathname, mode_t mode)
{
  return EROFS;
}

const struct fs_operations _fs_devfs_fsops = {
  .open      = devfs_open,
  .close     = devfs_close,
  .read      = devfs_read,
  .write     = devfs_write,
  .lseek     = devfs_lseek,
  .getdents  = devfs_getdents,
  .ioctl     = devfs_ioctl,
  .mkdir     = _devfs_mkdir
};

