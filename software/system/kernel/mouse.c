// PS/2 mouse driver for MFP
//
// because mouse inactivity or disconnect cannot be differentiated, a reset
// command is sent when no data is received during a timeout period (see
// K_MOUSE_TIMOUT_MS)

#include <string.h>
#include <types.h>
#include <syscall.h>
#include <errno.h>

#include "irq.h"
#include "b68k.h"
#include "config.h"
#include "lock.h"
#include "mem.h"
#include "proc.h"
#include "msg.h"
#include "dev.h"
#include "debug.h"

// screen/character sizes
#define SSZX  640
#define SSZY  480

#define K_MOUSE_TIMOUT_MS  2000

// prototypes
static void _mouse_interrupt (void);
static void _mouse_send (u8_t par, u8_t val);

// mouse globals
pid_t _mouse_pid;

typedef enum { MS_SELF_TEST,
	       MS_ERROR,
	       MS_MOUSE_ID,
	       MS_ACK_SAMPLE_SET,
	       MS_ACK_SAMPLE_RATE,
	       MS_ACK,
	       MS_BYTE1,
	       MS_BYTE2,
	       MS_BYTE3
} ms_state_t;

ms_state_t _ms_state;

struct {
  u16_t x, y;
  u16_t buttons;

} _ms_nfo;

volatile u8_t _ms_init_done;
volatile u8_t _ms_updated;

void mouse_task ()
{
  K_PRINTF(2, "mouse: starting task\n");
  
  // globals init
  _mouse_pid = proc_current();
  _ms_state = MS_SELF_TEST;
  _ms_nfo.x = 0;
  _ms_nfo.y = 0;
  _ms_nfo.buttons = 0;
  _ms_init_done = 0;
  _ms_updated = 0;

  // register char device
  {
    u16_t lbkp = k_lock();

    dev_register_char("mouse", _mouse_pid, 0);

    k_unlock(lbkp);
  }
  
  // register hooks/interrupts
  irq_register_interrupt(IRQ_MOUSE, _mouse_interrupt);
  irq_enable_interrupt(IRQ_MOUSE);
  
  // main loop
  static message_t msg_out;
  pid_t clock_pid = proc_get_pid("clock");

  // post 1st mon event
  msg_out.type     = SLEEP;
  msg_out.body.u32 = K_MOUSE_TIMOUT_MS;
  send(clock_pid, &msg_out);
  
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
	  // no write op
	  msg_out.body.s32 = 0;
	  send(from, &msg_out);
	}
	break;

      case DEV_READ:
	{
	  int len = msg.body.dev_read.count > 6 ? 6 : msg.body.dev_read.count;
	  u8_t *dst = msg.body.dev_read.dst;
	  u8_t *src = (u8_t *)&_ms_nfo;

	  msg_out.body.s32 = len;

	  while (len--) {
	    *dst++ = *src++;
	  }

	  send(from, &msg_out);
	}  
	break;

      case DEV_IOCTL:
	{
	  // TODO: set speed for exemple
	  msg_out.body.s32 = EINVAL;
	  send(from, &msg_out);
	}
	break;
	
      default:
	K_PRINTF(2, "tty: error: unknown command %Xh from %i\n", msg.type, from);
	break;
      }
    } else {
      // possible events: timer
      if (_ms_updated) {
	_ms_updated = 0;
      } else if (_ms_init_done) {
	_ms_init_done = 0;
	//K_PRINTF(2, "mouse: PS/2 mouse detected\n");
      } else {
	// timeout, reset mouse
	//K_PRINTF(2, "mouse: reset\n");
	_ms_state = MS_SELF_TEST;
	_mouse_send (1, 0xFF);
      }

      // post next monitoring event
      msg_out.type     = SLEEP;
      msg_out.body.u32 = K_MOUSE_TIMOUT_MS;
      send(clock_pid, &msg_out);
    }
  }
}

static void _mouse_send (u8_t par, u8_t val)
{
  u16_t lbkp = k_lock();
  u8_t mfp_ad_save = B68K_MFP->ad;
  B68K_MFP->ad = B68K_MFP_REG_PS2P1_STS;
  B68K_MFP->dt = par ? 2 : 0;
  B68K_MFP->ad = B68K_MFP_REG_PS2P1_DATA;
  B68K_MFP->dt = (u16_t)val;
  B68K_MFP->ad = mfp_ad_save;
  k_unlock(lbkp);
}

static void _mouse_decode(u16_t ms_byte1, u16_t ms_byte2, u16_t ms_byte3)
{
  // first char
  //       d7     d6     d5     d4     d3     d2     d1     d0
  // B1  Y-ovfl X-ovfl Y-sign X-sign   1      mb     rb     lb
  // B2                      X move
  // B3                      Y move
  _ms_nfo.buttons = ms_byte1 & 0x7;

  s16_t ms_x = _ms_nfo.x;
  s16_t dt_x;
  s16_t ms_y = _ms_nfo.y;
  s16_t dt_y;
  
  if (ms_byte1 & 0x10) {
    // negative
    if (ms_byte1 & 0x40) {
      // overflow
      dt_x = -255;
    } else {
      dt_x = 0xff00 | ms_byte2;
    }

    ms_x += dt_x;

    if (ms_x < 0) ms_x = 0;
  } else {
    // positive
    if (ms_byte1 & 0x40) {
      // overflow
      dt_x = 255;
    } else {
      dt_x = ms_byte2;
    }
  
    ms_x += dt_x;

    ms_x = ms_x >= SSZX ? SSZX - 1 : ms_x;
  } 
  
  _ms_nfo.x = ms_x;

  // invert Y sign
  if (ms_byte1 & 0x20) {
    // negative
    if (ms_byte1 & 0x80) {
      // overflow
      dt_y = 255;
    } else {
      dt_y = 0xff00 | ms_byte3;
      dt_y = -dt_y;
    }

    ms_y += dt_y;

    ms_y = ms_y >= SSZY ? SSZY - 1 : ms_y;
  } else {
    // positive
    if (ms_byte1 & 0x80) {
      // overflow
      dt_y = -255;
    } else {
      dt_y = -ms_byte3;
    }
  
    ms_y += dt_y;

    if (ms_y < 0) ms_y = 0;
  } 
  
  _ms_nfo.y = ms_y;
}

static void _mouse_interrupt (void)
{
  static u8_t ms_byte1, ms_byte2;

  // PS/2 keyboard key code decoding
  u8_t rxc;
  
  {
    u16_t lbkp = k_lock();
    
    u8_t mfp_ad_save = B68K_MFP->ad;
    B68K_MFP->ad = B68K_MFP_REG_PS2P1_DATA;
    rxc = B68K_MFP->dt;
    B68K_MFP->ad = mfp_ad_save;

    k_unlock(lbkp);
  }

  //K_PRINTF(2, "m%c-%02X\n", '0' + _ms_state, rxc);
  
  switch (_ms_state) {
  case MS_SELF_TEST:
  case MS_ERROR:
    if (rxc == 0xAA) {
      _ms_state = MS_MOUSE_ID;
    } else {
      _ms_state = MS_ERROR;
    }
    break;
    
  case MS_MOUSE_ID:
    // mouse ID received
    // send "enable device"
    _mouse_send(1, 0xF3);
    _ms_state = MS_ACK_SAMPLE_SET;
    break;
    
  case MS_ACK_SAMPLE_SET:
    if (rxc == 0xFA) {
      _mouse_send(1, 0x14);   // 20 samples/s
      _ms_state = MS_ACK_SAMPLE_RATE;
    } else {
      _ms_state = MS_ERROR;
    }
    break;
    
  case MS_ACK_SAMPLE_RATE:
    if (rxc == 0xFA) {
      _mouse_send(0, 0xF4);   // Enable data reporting
      _ms_state = MS_ACK;
    } else {
      _ms_state = MS_ERROR;
    }
    break;
    
  case MS_ACK:
    if (rxc == 0xFA) {
      _ms_init_done = 1;
      proc_sig(_mouse_pid, SIGALRM);
      
      _ms_state = MS_BYTE1;
    } else {
      _ms_state = MS_ERROR;
    }
    break;
    
  case MS_BYTE1:
    ms_byte1 = rxc;
    _ms_state = MS_BYTE2;
    break;
    
  case MS_BYTE2:
    ms_byte2 = rxc;
    _ms_state = MS_BYTE3;
    break;
    
  case MS_BYTE3:
    _ms_state = MS_BYTE1;
    
    _mouse_decode((u16_t)ms_byte1, (u16_t)ms_byte2, (u16_t)rxc);
    _ms_updated = 1;
    break;
    
  default:
    break;
  }
}

