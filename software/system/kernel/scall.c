
#include <types.h>
#include <stddef.h>
#include <syscall.h>

#include "config.h"
#include "msg.h"
#include "mem.h"
#include "proc.h"
#include "debug.h"
#include "b68k.h"

u32_t k_scall (u16_t ops, u16_t dst, message_t *msg)
{
  message_t *pa_msg = (message_t *)va_to_pa(proc_current(), (mem_va_t)msg, sizeof(message_t));

  // messages must be 32-bits aligned
  if (pa_msg && ((3 & (mem_va_t)msg) == 0)) {
    return sendreceive(dst, pa_msg, O_SEND | O_RECV);
  }
  
  return -1;
}
