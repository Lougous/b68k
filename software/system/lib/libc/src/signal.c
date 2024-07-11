//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc - process signaling
//

#include <types.h>   // TODO: <sys/types.h>
#include <signal.h>

#include <stddef.h>
#include <syscall.h>

#include "trap.h"

int kill(pid_t pid, int sig)
{
  // use message to system task
  volatile message_t msg = { .type = KILL,
			     .body.kill.pid = pid,
			     .body.kill.sig = sig  };

  u32_t rval;

  SENDRECEIVE(msg, 0, rval);

  if (rval != 0) {
    return -1;
  }
  
  return msg.body.s32;
}
