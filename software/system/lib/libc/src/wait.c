//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc - wait
//

#include <types.h>
#include <sys/wait.h>
#include <stddef.h>
#include <syscall.h>

#include "trap.h"

pid_t wait(int *wstatus)
{
  // message to system task
  volatile message_t msg = { .type = WAIT };

  u32_t rval;

  SENDRECEIVE(msg, 0, rval);

  if (rval != 0) {
    return -1;
  }
  
  return msg.body.s32;
}
