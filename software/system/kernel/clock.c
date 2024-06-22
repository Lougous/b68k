
#include <stdio.h>
#include <stddef.h>
#include <types.h>
#include <syscall.h>

#include "config.h"
#include "irq.h"
#include "mem.h"
#include "proc.h"
#include "lock.h"
#include "msg.h"
#include "debug.h"

static void _irq_timer_handler(void);

struct _clock_proc_t {
  u32_t wake_time_ms;
};

struct _clock_proc_t  _clock_proc_table[K_PROC_COUNT];

u32_t k_time_ms;

u32_t clock_now()
{
  return k_time_ms;
}

void clock_task ()
{
  int p;
  
  K_PRINTF(2, "clock: starting task\n");

  for (p = 0; p < K_PROC_COUNT; p++) {
    _clock_proc_table[p].wake_time_ms = 0;
  }

  k_time_ms = 0;
  
  {
    u16_t lbkp = k_lock();

    /* enable timer interrupt */
    irq_register_interrupt(IRQ_CLOCK, _irq_timer_handler);
    irq_enable_interrupt(IRQ_CLOCK);  // timer

    k_unlock(lbkp);
  }

  while (1) {
    static message_t msg;

    pid_t from = receive(ANY, &msg);

    // this receive may be interrupted by signals
    if (from >= 0) {
      // proceed with message
      switch(msg.type) {
      case SLEEP:
	{
	  // disable interruptions to prevent race condition with IRQ_CLOCK handler
	  u16_t lbkp = k_lock();
	  _clock_proc_table[from].wake_time_ms = k_time_ms + (msg.body.u32 / K_TIMER_PERIOD_MS)*K_TIMER_PERIOD_MS;
	  k_unlock(lbkp);
	}
	break;

      default:
	K_PRINTF(2, "clock: error: unknown command %Xh from %i\n", msg.type, from);
	break;
      }
    } else {
      
    }
  }
}

static void _irq_timer_handler(void)
{
  // update time
  k_time_ms += K_TIMER_PERIOD_MS;

  // check processes
  int p;
  
  for (p = 0; p < K_PROC_COUNT; p++) {
    if (_clock_proc_table[p].wake_time_ms == k_time_ms) {
      // time to wake up
      proc_sig(p, SIGALRM);
    }
  }
}


