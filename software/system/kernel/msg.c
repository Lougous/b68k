
#include <stddef.h>
#include <types.h>
#include <syscall.h>
#include <string.h>

#include "mem.h"
#include "proc.h"
#include "msg.h"
#include "config.h"
#include "debug.h"

int sendreceive_vfs_open (message_t *msg, const char *pathname, int flags)
{
  //  msg->src  = proc_current();
  msg->type = OPEN;
  msg->body.open.pathname = (char *)pathname;
  msg->body.open.len = strlen(pathname);
  msg->body.open.flags = flags;

  pid_t vfs_pid = proc_get_pid("vfs");

  sendreceive(vfs_pid, msg, O_SEND | O_RECV);

  return msg->body.s32;
}

int sendreceive_vfs_close(message_t *msg, int fd)
{
  //  msg->src  = proc_current();
  msg->type = CLOSE;
  msg->body.close.fd = fd;

  pid_t vfs_pid = proc_get_pid("vfs");

  sendreceive(vfs_pid, msg, O_SEND | O_RECV);

  return msg->body.s32;
}

ssize_t sendreceive_vfs_read(message_t *msg, int fd, void *buf, size_t count)
{
  pid_t vfs_pid = proc_get_pid("vfs");
  size_t done = 0;
  u8_t *cbuf = buf;
  
  while (done < count) {
    msg->type = READ;
    msg->body.read.fd = fd;
    msg->body.read.buf = cbuf;
    msg->body.read.count = count - done;

    sendreceive(vfs_pid, msg, O_SEND | O_RECV);

    if (msg->body.s32 <= 0) {
      break;
    }
    
    done += msg->body.s32;
    cbuf += msg->body.s32;
  }

  return done;
}

off_t sendreceive_vfs_lseek (message_t *msg, int fildes, off_t offset, int whence)
{
  //  msg->src  = proc_current();
  msg->type = LSEEK;
  msg->body.lseek.fd = fildes;
  msg->body.lseek.offset = offset;
  msg->body.lseek.whence = whence;

  pid_t vfs_pid = proc_get_pid("vfs");

  sendreceive(vfs_pid, msg, O_SEND | O_RECV);

  return msg->body.s32;
}

int sendreceive_vfs_getdents (message_t *msg, unsigned int fd, struct dirent *dirp, unsigned int count)
{
  //  msg->src  = proc_current();
  msg->type = GETDENTS;
  msg->body.getdents.fd = fd;
  msg->body.getdents.dirp = dirp;
  msg->body.getdents.count = count;

  pid_t vfs_pid = proc_get_pid("vfs");

  sendreceive(vfs_pid, msg, O_SEND | O_RECV);

  return msg->body.s32;
}

