
#include <stddef.h>
#include <string.h>
#include <types.h>
#include <syscall.h>
#include <errno.h>
#include <sys/opl2.h>

#include "b68k.h"

#include "dev.h"
#include "mem.h"
#include "proc.h"
#include "msg.h"
#include "config.h"
#include "debug.h"

// context
static pid_t _pid;
static mem_pa_t _rb_pa;  // opl2_ring_buffer_t *
typedef enum { REGISTER, DELAY1, DELAY2_1, DELAY2_2, DATA } dro_state_t;
static u8_t _state;
static u8_t _esc;
static u16_t _delay;
static u8_t _reg;


// local ring buffer for startup shime
static opl2_ring_buffer_t _ring_buffer;

#define FREQ  0x240  // A
#define OCT   4

static const u8_t _beep[] = {
  // operator 
  0x20, 0x21,
  0x40, 0x00,
  0x60, 0xF0,
  0x80, 0x0F,
  
  0x23, 0x21,
  0x43, 0x00,
  0x63, 0xF0,
  0x83, 0x0F,

  // channel
  0xC0, 0x01,  // sum

  // set freq and key on
  0xA0, FREQ & 0xff,
  0xB0, 0x20 | (OCT << 2) | (FREQ >> 8),

  // pause
  0, 200,

  // kef off
  0xB0, 0x00,

  // revert to default values
  0x20, 0x00,
  0x40, 0x00,
  0x60, 0x00,
  0x80, 0x00,
  0x23, 0x00,
  0x43, 0x00,
  0x63, 0x00,
  0x83, 0x00,
  0xC0, 0x00,
  0xA0, 0x00  
};
  

/* ASM */
extern void k_loop (u16_t iter);
extern void k_loop0 (void);

static void OPL2_WRITE_REGISTER(u8_t reg, u8_t c)
{
  B68K_AV_OPL2->opl2_regad_sts = reg;
  k_loop0();
  B68K_AV_OPL2->opl2_regdt = c;
  k_loop(15);
}

void __attribute__ ((interrupt)) opl2_interrupt (void)
{
  // acknowledge interrupt
  OPL2_WRITE_REGISTER(0x4, 0x81);  // timer control
  
  if (_delay) {
    // still delay to wait for
    _delay--;
  } else {
    // ensure process is still valid for audio operations
    if (_pid != PROC_PID_NONE && proc_get_state(_pid) & PROC_STATE_ACTIVE_MASK) {
      volatile opl2_ring_buffer_t *pbr = (opl2_ring_buffer_t *)_rb_pa;
      u8_t paused = 0;
    
      while ((! paused) && (pbr->rptr != pbr->wptr)) {
      
	u8_t c = pbr->buf[pbr->rptr++];

	switch (_state) {
	case REGISTER:
	  if (_esc) {
	    _esc = 0;

	    _reg = c;
	    _state = DATA;
	  } else {
	    switch (c) {
	    case 0:
	      // Delay. The single data byte following should be incremented by
	      // one, and is then the delay in milliseconds. 
	      _state = DELAY1;
	      break;
	    case 1:
	      // Delay. Same as above only there are two bytes following, in the
	      // form of a UINT16LE integer. 
	      _state = DELAY2_1;
	      break;
	    case 2:
	      // Switch to "low" OPL chip (#0) => ignore
	      break;
	    case 3:
	      // Switch to "high" OPL chip (#1) => ignore
	      break;
	    case 4:
	      // Escape - the next two bytes are normal register/value pairs even
	      // though the register might be 0x00-0x04
	      _esc = 1;
	      break;
	    default:
	      _reg   = c;
	      _state = DATA;
	      break;
	    }
	  }
	  break;

	case DELAY1:
	  _delay = ((u16_t)c) + 1;
	  paused = 1;
	  _state = REGISTER;
	  break;

	case DELAY2_1:
	  _delay = c;
	  _state = DELAY2_2;
	  break;

	case DELAY2_2:
	  _delay = _delay + (((u16_t)c) << 8) + 1;
	  paused = 1;
	  _state = REGISTER;
	  break;

	case DATA:
	  OPL2_WRITE_REGISTER(_reg, c);
	  _state = REGISTER;
	  break;
	}
      
      }

      if (!paused) {
	// buffer empty, check again on next interrupt
	_delay = 0;
      } else {
	_delay--;
      }
    } else {
      // not valid
      // TODO: turn keys off ?
    }
  }
}

void av_opl2_task ()
{
  // inits
  memcpy(_ring_buffer.buf, _beep, sizeof(_beep));
  _ring_buffer.rptr = 0;
  _ring_buffer.wptr = sizeof(_beep);
  
  _rb_pa = (mem_pa_t) &_ring_buffer;
  _pid   = proc_current();
  _state = REGISTER;
  _esc   = 0;
  _delay = 0;

  // create device
  dev_register_char("opl2", proc_current(), NULL);
  
  // setup interrupt handler
  B68K_IRQ_VECTOR[B68K_AV_IRQ] = (u32_t)opl2_interrupt;

  // for startup shime 
  OPL2_WRITE_REGISTER(0x2, 256-12);  // timer 1, 80us x 12 = 960 us
  OPL2_WRITE_REGISTER(0x4, 0x01);  // timer control: enable timer 1

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
	  /* input: char list with address (1 byte) / data (1 byte) couples */
	  // TODO: virtual => physical ?
	  u8_t *pc = msg.body.dev_write.src;
	  s32_t count = msg.body.dev_write.count & ~1;

	  while (count -= 2) {
	    u8_t reg = *pc++;
	    u8_t dt  = *pc++;
	    OPL2_WRITE_REGISTER(reg, dt);
	  }
  
	  msg_out.body.s32 = count;
	  
	  send(from, &msg_out);
	}
	break;

      case DEV_READ:
	{
	  /* TODO */
	  msg_out.body.s32 = 0;
	  send(from, &msg_out);
	}  
	break;

      case DEV_IOCTL:
	{
	  if (msg.body.dev_ioctl.request == OPL2_IOCTL_SETBUF) {
	    mem_pa_t rb = va_to_pa(msg.body.dev_ioctl.pid,
				   (mem_va_t)msg.body.dev_ioctl.va_ptr,
				   sizeof(opl2_ring_buffer_t));

	    if (rb) {
	      // use this ring buffer
	      {
		u16_t lbkp = k_lock();
	      
		_pid   = msg.body.dev_ioctl.pid;
		_rb_pa = rb;
		_state = REGISTER;
		_esc   = 0;
		_delay = 0;

		k_unlock(lbkp);
	      }
	    
	      msg_out.body.s32 = 0;
	    } else {
	      msg_out.body.s32 = EFAULT;
	    }
	  } else {
	    msg_out.body.s32 = EINVAL;
	    K_PRINTF(3, "tty: error: unknown IOCTL %Xh from %i\n", msg.body.dev_ioctl.request, from);
	  }

	  send(from, &msg_out);
	}
	break;
	
      default:
	K_PRINTF(2, "av_opl2: error: unknown command %Xh from %i\n", msg.type, from);
	break;
      }
    }
  }
}
