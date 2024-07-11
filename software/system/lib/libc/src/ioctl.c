//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc - ioctl
//

#include <types.h>
#include <stddef.h>
#include <syscall.h>
#include <sys/ioctl.h>

#include "trap.h"

int ioctl(int fd, int request, void *pargs)
{
  // use message to vfs task
  volatile message_t msg = {
    .type               = IOCTL,
    .body.ioctl.fd      = fd,
    .body.ioctl.request = request,
    .body.ioctl.ptr     = pargs
  };

  u32_t rval;

  SENDRECEIVE(msg, 3, rval);

  if (rval != 3) {
    return -1;
  }
  
  return msg.body.s32;
}

