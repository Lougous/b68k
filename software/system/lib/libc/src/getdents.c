//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc - directory listing
//

#include <types.h>
#include <stddef.h>
#include <syscall.h>
#include <dirent.h>

#include "trap.h"

int getdents (unsigned int fd, struct dirent *dirp, unsigned int count)
{
  message_t msg = { .type = GETDENTS,
		    .body.getdents.fd = fd,
		    .body.getdents.dirp = dirp,
		    .body.getdents.count = count };
  u32_t rval;

  SENDRECEIVE(msg, 3, rval);

  if (rval != 3) {
    return -1;
  }
  
  return msg.body.s32;
}

