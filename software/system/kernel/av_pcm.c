
#include <stddef.h>
#include <types.h>
#include <syscall.h>

#include "b68k.h"

#include "dev.h"
#include "mem.h"
#include "proc.h"
#include "msg.h"
#include "config.h"
#include "debug.h"

void av_pcm_task ()
{
  // inits

  // create device
  dev_register_char("pcm", proc_current(), NULL);
  
  // schedule a 1 ms period interrupt job

  // main message processing loop
  static message_t msg_out;

  while (1) {
    static message_t msg;

    pid_t from = receive(ANY, &msg);

    msg_out.type = 0;

    // this receive may be interrupted by signals
    if (from >= 0) {
      // proceed with message
      switch(msg.type) {
      case DEV_OPEN:
	{
	  /* todo: check permissions ? */
	  msg_out.body.s32 = 0;
	  send(from, &msg_out);
	}
	break;
	
      case DEV_CLOSE:
	{
	  msg_out.body.s32 = 0;
	  send(from, &msg_out);
	}
	break;
	
      case DEV_WRITE:
	{
	  if ((B68K_AV_FLEX->cfgsts & B68K_AV_FLEX_CFGSTS_APU_PCM_HFULL_MASK) == 0) {
	    /* less than half full, can store at least 128 more bytes (64 left-right samples) */
	    u16_t *pc = msg.body.dev_write.src;
	    s32_t count = msg.body.dev_write.count & ~1;

	    if (count < 0) count = 0;
	    if (count > 128) count = 128;

	    u16_t to_go = count;

	    while (to_go > 0) {
	      u16_t lr = *pc++;
	      B68K_AV_FLEX->apu0 = lr;
	      B68K_AV_FLEX->apu1 = lr >> 8;
	      
	      to_go -= 2;
	    }

	    msg_out.body.s32 = count;
	  } else {
	    /* at least half full, nothing stored yet */
	    //K_PRINTF(2, "av_pcm: full !\n");
	    msg_out.body.s32 = 0;
	  }
	  
	  send(from, &msg_out);
	}
	break;

      case DEV_READ:
	{
	  /* nothing to read */
	  msg_out.body.s32 = 0;
	  send(from, &msg_out);
	}  
	break;
	
      default:
	K_PRINTF(2, "av_pcm: error: unknown command %Xh from %i\n", msg.type, from);
	break;
      }
    }
  }
}
