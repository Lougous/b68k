
#include <string.h>
#include <types.h>
#include <syscall.h>
#include <errno.h>
#include <sys/tty.h>

#include "irq.h"
#include "b68k.h"
#include "config.h"
#include "lock.h"
#include "mem.h"
#include "proc.h"
#include "msg.h"
#include "dev.h"
#include "debug.h"


#define RS_MODE

#if K_TTY_COUNT > 10
#error K_TTY_COUNT max value is 10
#endif

// character font configuration
#include "font/default.c"

// keyboard layout configuration
#include "keymap/fr.c"

// screen/character sizes
#define SSZX  640
#define SPCH  512  // screen pitch in bytes
#define SSZY  480
#define CSZX  8
#define CSZY  10

// GFX memory mapping
#define DL_ADDR    0x8000  // shall be < 64k !!!
#define FONT_ADDR  0x7400  // shall be < 32k !!!
#define FB_ADDR    0x20000 // page 0

#define ROLL_FB(ad) (((ad) & (FB_ADDR - 1)) + FB_ADDR);

// TTY attributes
struct tty_attr_t {
  // current mode (color ...), use MSB only
  u16_t char_mode;
  // input buffer
  u16_t i_buf[256];
  u8_t i_wp;   /* input buffer write pointer */
  u8_t i_rp;   /* input buffer read pointer */
  u8_t i_dp;   /* input buffer display pointer */
  volatile u8_t i_nline;
  // character table on screen
  u16_t screen[(SSZX/CSZX)*(SSZY/CSZY)];
  // cursor position
  u8_t px;
  u8_t py;
  u8_t ptoggle;
  // TTY flags - see sys/tty.h
  u16_t flags;
};

static struct tty_attr_t _tty_table[K_TTY_COUNT];
static struct tty_attr_t *_tty_ptr;
static u32_t _tty_fb_addr;

// binary firmware for the AV FPGA
#include "flex-img.c"

static u8_t _gfx_ready;

// prototypes
static void _keyboard_interrupt (void);

static void _gpu_redraw();
static void _gpu_draw(u8_t x, u8_t y, u8_t c, u8_t lut);
static void _gpu_set_fb_addr(void);
static void _gpu_start_dl(void);
static void _gpu_flush_dl(void);

static void _tty_putc(struct tty_attr_t *pt, u8_t c);
static int  _tty_putchar(int c);

static pid_t _tty_pid;

// GPU display list
static u16_t *_gpu_pdl;

// keyboard
static u8_t _kbd_extended;
static u8_t _kbd_break;
static u8_t _kbd_shifted;
static u8_t _kbd_ctrled;
static u8_t _kbd_alted;
static u8_t _kbd_keys_pos[16];


void tty_task ()
{
  u16_t lbkp = k_lock();
  
  //////////////////////////////////////////////////////////////////////////////
  // setup RAMDAC
  //////////////////////////////////////////////////////////////////////////////
  K_PRINTF(3, "AV: reset RAMDAC colors\n");
  B68K_AV_RAMDAC->rd_ad_write = 0;

  u16_t entry;

  // all black
  for (entry = 0; entry < 256; entry++) {
    B68K_AV_RAMDAC->rd_color = 0;
    B68K_AV_RAMDAC->rd_color = 0;
    B68K_AV_RAMDAC->rd_color = 0;
  }

  // color mask
  B68K_AV_RAMDAC->rd_mask = 0xff;
  
  //////////////////////////////////////////////////////////////////////////////
  // load AV FPGA firmware
  //////////////////////////////////////////////////////////////////////////////
  K_PRINTF(3, "AV: probing device at %06Xh\n", (u32_t)B68K_AV_AVMGR);
  _gfx_ready = 0;

  while (_gfx_ready == 0) {
    
  u32_t timeout = 100000;
  B68K_AV_AVMGR->cfgnsts = 0;  // assert nCONFIG (+RSTn)

  while (B68K_AV_AVMGR->cfgnsts & B68K_AV_AVMGR_STS_nSTATUS_MASK) {
    timeout--;

    if (timeout == 0) {
      K_PRINTF(3, "AV: failed to get nSTATUS low\n");
      //      goto av_init_failed;
    }
  }

  timeout = 1000000;
  B68K_AV_AVMGR->cfgnsts = B68K_AV_AVMGR_STS_nCONFIG_MASK;  // deassert nCONFIG (RSTn=0)

  while (!(B68K_AV_AVMGR->cfgnsts & B68K_AV_AVMGR_STS_nSTATUS_MASK)) {
    timeout--;

    if (timeout == 0) {
      K_PRINTF(3, "AV: failed to get nSTATUS high\n");
      //      goto av_init_failed;
    }
  }

  if (B68K_AV_AVMGR->cfgnsts & B68K_AV_AVMGR_STS_CONF_DONE_MASK) {
    K_PRINTF(3, "AV: failed to get CONF_DONE low at configuration startup\n");
    //    goto av_init_failed;
  }

  u16_t byte, bit;
  
  for (byte = 0; byte < sizeof(_________boards_b68k_av_flex_flex_rbf); byte++) {
    u8_t b = _________boards_b68k_av_flex_flex_rbf[byte];

    for (bit = 0; bit < 8; bit++) {
      // set data (data is in bit 0, other bits in register are don't care)
      // a single register write generates proper pulse on DCLK
      B68K_AV_AVMGR->dat = b;

      b = b >> 1;
    }
  }

  // add 10 extra clock cycles
  for (bit = 0; bit < 10; bit++) {
    B68K_AV_AVMGR->dat = 0;
  }

  // short delay
  //  B68K_AV_AVMGR->dat = 1;
  //  B68K_AV_AVMGR->dat = 1;

  // RSTn = 1
  B68K_AV_AVMGR->cfgnsts = B68K_AV_AVMGR_STS_nCONFIG_MASK | B68K_AV_AVMGR_STS_RSTn_MASK;
  
  // CONF_DONE
  if (! (B68K_AV_AVMGR->cfgnsts & B68K_AV_AVMGR_STS_CONF_DONE_MASK)) {
    K_PRINTF(3, "AV: failed to get CONF_DONE high\n");
    //    goto av_init_failed;
  } else {
    K_PRINTF(3, "AV: device ready\n");
    _gfx_ready = 1;
  }
  }
  
  // av_init_failed:
  k_unlock(lbkp);

  //////////////////////////////////////////////////////////////////////////////
  // load font
  //////////////////////////////////////////////////////////////////////////////
  int ch;

  if (_gfx_ready) {
    K_PRINTF(0, "TTY: loading font\n");
  
    const u32_t *src = (u32_t *)font_8x10;
    u32_t *dst = (u32_t *)(B68K_AV_VRAM_ADDRESS + FONT_ADDR);

    //    B68K_AV_FLEX->rbcfg0_vbk = 0;

    // TODO: use word or long transfer to speed up
    for (ch = 0; ch < 256; ch++) {
      // 64 pixels: 8 bytes
      *dst++ = *src++;
      *dst++ = *src++;
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // init TTY data
  //////////////////////////////////////////////////////////////////////////////
  K_PRINTF(0, "TTY: terminals setup\n");
  memset(_tty_table, 0, sizeof(_tty_table));
  
  u8_t tty;

  for (tty = 0; tty < K_TTY_COUNT; tty++) {
    _tty_table[tty].py = 1;
#if K_TTY_CBLK_MS == 0
    _tty_table[tty].ptoggle = 1;
#endif
    _tty_table[tty].flags = TTY_FLAG_ECHO | TTY_FLAG_CURSOR;
  }

  _tty_ptr = &_tty_table[0];
  _tty_pid = proc_current();

  _kbd_shifted  = 0;
  _kbd_ctrled   = 0;
  _kbd_alted    = 0;
  _kbd_break    = 0;
  _kbd_extended = 0;
  memset(_kbd_keys_pos, 0, sizeof(_kbd_keys_pos));

  char tty_name[6] = "ttyX\0\0";  // !!! table size needs to be 16-bits aligned !!!

  {
    u16_t lbkp = k_lock();

    for (tty = 0; tty < K_TTY_COUNT; tty++) {
      tty_name[3] = '0' + tty;

      // use pointer to TTY structure as handle 
      dev_register_char(tty_name, _tty_pid, &_tty_table[tty]);
    }

    k_unlock(lbkp);
  }

  //////////////////////////////////////////////////////////////////////////////
  // setup frame buffer
  //////////////////////////////////////////////////////////////////////////////
  K_PRINTF(0, "TTY: frame buffer setup\n");
  
  if (_gfx_ready) {
    /*    B68K_AV_FLEX->rbcfg0_vbk = 0;
    B68K_AV_FLEX->rbcfg1     = 0;
    B68K_AV_FLEX->rbcfg2     = 0;
    */
    
    _tty_fb_addr = FB_ADDR;

    // DL pointer init
    _gpu_pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + DL_ADDR);

    _gpu_set_fb_addr();

    /* clear screen, actually */
    _gpu_redraw();

  }
  
  //////////////////////////////////////////////////////////////////////////////
  // setup color palette
  //////////////////////////////////////////////////////////////////////////////
  // regular colors: use 2 (x4 with x/y shifters) colors: 128 and 129
  if (_gfx_ready) {
    B68K_AV_RAMDAC->rd_ad_write = 128;

    for (ch = 0; ch < 4; ch++) {
      // background
      B68K_AV_RAMDAC->rd_color = 16;
      B68K_AV_RAMDAC->rd_color = 16;
      B68K_AV_RAMDAC->rd_color = 16;
      // foreground
      B68K_AV_RAMDAC->rd_color = 63;
      B68K_AV_RAMDAC->rd_color = 63;
      B68K_AV_RAMDAC->rd_color = 63;
    }

    // reverse video
    for (ch = 0; ch < 4; ch++) {
      // background
      B68K_AV_RAMDAC->rd_color = 63;
      B68K_AV_RAMDAC->rd_color = 63;
      B68K_AV_RAMDAC->rd_color = 63;
      // foreground
      B68K_AV_RAMDAC->rd_color = 16;
      B68K_AV_RAMDAC->rd_color = 16;
      B68K_AV_RAMDAC->rd_color = 16;
    }  
  }
  
  //////////////////////////////////////////////////////////////////////////////
  // register hooks/interrupts
  //////////////////////////////////////////////////////////////////////////////
  irq_register_interrupt(IRQ_KEYBOARD, _keyboard_interrupt);
  irq_enable_interrupt(IRQ_KEYBOARD);
  
  //////////////////////////////////////////////////////////////////////////////
  // display banner
  //////////////////////////////////////////////////////////////////////////////
  K_PRINTF(0, "TTY: ready\n");

  {
    const struct _IO_FILE f = {
      .putchar = _tty_putchar,
      .getchar = 0
    };

    _gpu_start_dl();
    fprintf((FILE *)&f, " ___   __  ___  _\n");
    fprintf((FILE *)&f, "| |_) / / ( (_)| |/  system v%i.%i\n", K_VERSION_MAJOR, K_VERSION_MINOR);
    fprintf((FILE *)&f, "|_|_)(_)_)(_(_)|_|\\  build %s %s\n\n", __DATE__, __TIME__);
    _gpu_flush_dl();
  }
  
  //////////////////////////////////////////////////////////////////////////////
  // timer for cursor
  //////////////////////////////////////////////////////////////////////////////
  static message_t msg_out;
  pid_t clock_pid = proc_get_pid("clock");

  //  K_PRINTF(2, "TTY: clock PID %u\n", clock_pid);
  
#if K_TTY_CBLK_MS > 0
  // post 1st cursor event
  msg_out.type     = SLEEP;
  msg_out.body.u32 = K_TTY_CBLK_MS / 2;
  send(clock_pid, &msg_out);
#endif


  //////////////////////////////////////////////////////////////////////////////
  // main loop
  //////////////////////////////////////////////////////////////////////////////
  while (1) {
    static message_t msg;
    u8_t cursor_reset = 0;
    u8_t cursor_draw  = 0;

    pid_t from = receive(ANY, &msg);

    msg_out.type = 0;

    // this receive may be interrupted by signals
    if (from >= 0) {
      // proceed with messages from system tasks only (users need to use char devices => VFS)
      if (proc_get_uid(from) == PROC_UID_KERNEL) {
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
	    s32_t count = msg.body.dev_write.count;
	    u8_t *pc = msg.body.dev_write.src;
	    struct tty_attr_t *pt = (struct tty_attr_t *)msg.body.dev_write.handle;

	    // default to TTY0
	    if (! pt) pt = &_tty_table[0];
	    
	    _gpu_start_dl();
	  
	    while (count--) {
	      _tty_putc(pt, *pc++);
	    }

	    cursor_reset = 1;
  
	    msg_out.body.s32 = msg.body.dev_write.count;
	    send(from, &msg_out);
	  }
	  break;

	case DEV_READ:
	  {
	    struct tty_attr_t *pt = (struct tty_attr_t *)msg.body.dev_read.handle;
	    int len = 0;
	    u8_t *dst = msg.body.dev_read.dst;

	    if (pt->i_nline && msg.body.dev_read.count) {
	      /* something to send to receiver process ? */
	      while (pt->i_rp != pt->i_wp) {
		u8_t c = pt->i_buf[pt->i_rp++];
	      
		*dst++ = c;
		len++;

		// TODO: raw mode
		if (c == '\n') {
		  /* end of line */
		  pt->i_nline--;
		  break;
		} else if (len == msg.body.dev_read.count) {
		  /* read buffer full */
		  break;
		}
	      }
	    }

	    msg_out.body.s32 = len;
	    send(from, &msg_out);
	  }  
	  break;

	case DEV_IOCTL:
	  {
	    struct tty_attr_t *pt = (struct tty_attr_t *)msg.body.dev_read.handle;

	    if (msg.body.dev_ioctl.request == TTY_IOCTL_KBD_POS) {
	    
	      mem_pa_t pa = va_to_pa(msg.body.dev_ioctl.pid,
				     (mem_va_t)msg.body.dev_ioctl.va_ptr,
				     sizeof(tty_ioctl_kbd_pos_t));

	      if (pa) {
		tty_ioctl_kbd_pos_t *ptr = (tty_ioctl_kbd_pos_t *)pa;
		u8_t i = 0;

		while (i < sizeof(ptr->buf)) {
		  ptr->buf[i] = _kbd_keys_pos[i];
		  i++;
		}

		msg_out.body.s32 = 0;
	      } else {
		K_PRINTF(2, "tty: error: IOCTL with bad address from %i\n", from);
		msg_out.body.s32 = EFAULT;
	      }
	    } else if (msg.body.dev_ioctl.request == TTY_IOCTL_SET_FLAGS) {
	      pt->flags = (u16_t)((u32_t)msg.body.dev_ioctl.va_ptr);
	      msg_out.body.s32 = 0;
	    } else {
	      msg_out.body.s32 = EINVAL;
	      K_PRINTF(2, "tty: error: unknown IOCTL %Xh from %i\n", msg.body.dev_ioctl.request, from);
	    }
	  
	    send(from, &msg_out);
	  }
	  break;
	
	default:
	  K_PRINTF(2, "tty: error: unknown command %Xh from %i\n", msg.type, from);
	  break;
	}
      } else {
	K_PRINTF(2, "tty: error: message from user process %i (UID=%i)\n", from, proc_get_uid(from));
	break;
      }	
    } else {
      // possible events: keyboard inputs, timer for cursor
      _gpu_start_dl();

      // refresh with what is available yet in input buffer
      while (_tty_ptr->i_dp != _tty_ptr->i_wp) {
	/* update display */
	u8_t c = _tty_ptr->i_buf[_tty_ptr->i_dp];

	if (c & 0x80) {
	  // special characters
	  /*
	  if ((c & 0xF0) == 0xF0) {
	    // "F" keys
	    u8_t tty = (c & 0xF)-1;

	    if (tty < K_TTY_COUNT) {
	      _tty_ptr = &_tty_table[tty];
	    }
	  }
	  */
	  _tty_ptr->i_dp++;
	} else {
	  // normal characters
	  if (_tty_ptr->flags & TTY_FLAG_ECHO) {
	    _tty_putc(_tty_ptr, _tty_ptr->i_buf[_tty_ptr->i_dp++]);
	  } else {
	    _tty_ptr->i_dp++;
	  }
	}

	/* if not open yet for read, discard */
	if (c == '\n') {
	  _tty_ptr->i_nline++;
	  //K_PRINTF(2, "tty: nline=%i\n", _tty_ptr->i_nline);
	}

	cursor_reset = 1;
      }

      cursor_draw = 1;
      _tty_ptr->ptoggle = 1 - _tty_ptr->ptoggle;
    }

    if (cursor_reset) {
      // reset cursor anim
      _tty_ptr->ptoggle = 1;
      cursor_draw = 1;
    }
    
    if (cursor_draw) {
#if K_TTY_CBLK_MS > 0
      // post next cursor event
      msg_out.type     = SLEEP;
      msg_out.body.u32 = K_TTY_CBLK_MS / 2;
      send(clock_pid, &msg_out);
#endif

    }

    // hide/sow cursor
    if (cursor_draw && (_tty_ptr->flags & TTY_FLAG_CURSOR)) _gpu_draw(_tty_ptr->px, _tty_ptr->py, ' ', _tty_ptr->ptoggle);

    _gpu_flush_dl();

  }
}

static void _keyboard_interrupt (void)
{
  // PS/2 keyboard key code decoding
  u8_t code;

  {
    u16_t lbkp = k_lock();
   
    u8_t mfp_ad_save = B68K_MFP->ad;

    B68K_MFP->ad = B68K_MFP_REG_PS2P0_DATA;
    code = B68K_MFP->dt;
    
    B68K_MFP->ad = mfp_ad_save;

    k_unlock(lbkp);
  }
  
  u8_t key = 0;

  if (code == 0xE0) {
    _kbd_extended = 1;
  } else if (code == 0xF0) {
    _kbd_break = 1;
  } else {
    if (_kbd_break) {
      _kbd_keys_pos[(code & 0x7F) >> 3] &= ~(1 << (code & 0x7));
    } else {
      _kbd_keys_pos[(code & 0x7F) >> 3] |= (1 << (code & 0x7));
    }

    if (code == 0x12 || code == 0x59) {
      // left/right shift
      _kbd_shifted = 1 - _kbd_break;
    } else if (code == 0x14) {
      // left control
      _kbd_ctrled = 1 - _kbd_break;
    } else if (code == 0x11) {
      // ALT
      _kbd_alted = 1 - _kbd_break;
    } else if (! _kbd_break) {
      if (_kbd_ctrled) {
	if (_kbd_alted && (code == 0x71)) {
	  // CTRL-ALT-DEL
	  proc_sig(PROC_PID_SYSTEM_TASK, SIGALRM);
	}

	if (code == 0x4B) {
	  // CTRL-L
	}
      }
      
      if (_kbd_shifted) {
	key = _ps2_codes_shifted_tbl[code & 0x7F];
      } else {
	key = _ps2_codes_tbl[code & 0x7F];
      }
    }

    _kbd_break = 0;
    _kbd_extended = 0;
  }

  if (key) {
    if ((_tty_ptr->i_wp + 1) != _tty_ptr->i_rp) {
      _tty_ptr->i_buf[_tty_ptr->i_wp++] = key;

      // usually raised by the clock task, used here to wake up the TTY task
      proc_sig(_tty_pid, SIGALRM);
    }
  }  
}

static void _gpu_set_fb_addr(void)
{
  u32_t ad = _tty_fb_addr;

  // build DL
  *_gpu_pdl++ = ad & 0x7fff;     // address (14:0)
  *_gpu_pdl++ =
    0x8000               |  //
    ((ad >> 6) & 0x0600) |  // address (16:15)
    0x100                |  // STOP
    (4 << 4)             |  // 5 lines
    0x0008;                 // F: change frame buffer

}

static void _gpu_draw(u8_t x, u8_t y, u8_t c, u8_t lut)
{
  
  u16_t src = FONT_ADDR + (u16_t)c*10;
  u32_t dst = ROLL_FB(_tty_fb_addr + y*(CSZY/2*SPCH) + x*4);

  // build DL
  *_gpu_pdl++ = src; // address (14:0) <32k
  *_gpu_pdl++ =
    0x8000    |  //
    (0 << 9)  |  // address (18:15)
    (0 << 8)  |  // no stop
    (4 << 4)  |  // 5 lines
    0;           // no FB change
  *_gpu_pdl++ =
    0xc000    |  // block move
    (0 << 9)  |  // 1 word
    (0 << 8)  |  // read
    (3 << 4)  |  // bank 3
    0;           // LUT: don't care
  
  *_gpu_pdl++ = dst & 0x7fff;  // address (14:0) 
  *_gpu_pdl++ =
    0x8000                 |
    ((dst & 0x78000) >> 6) |  // address (18:15)
    (4 << 4)               |  // 5 lines
    0;                        // no FB change
  *_gpu_pdl++ =
    0xc000                 |  // block move
    (1 << 9)               |  // 2 words
    (1 << 8)               |  // write
    (3 << 4)               |  // bank 3
    ((lut & 1) | 8);          // LUT 8 or 9 (hi-res)

}

static void _gpu_clear_line(u8_t y)
{

  u16_t src = FONT_ADDR;  // char 0

  // load character
  *_gpu_pdl++ = src; // address (14:0) <32k
  /*
  *pdl++ =
    0x8000    |  //
    (0 << 9)  |  // address (18:15)
    (0 << 8)  |  // no stop
    (4 << 4)  |  // 5 lines
    0;           // no FB change
  */
  *_gpu_pdl++ =
    0xc000    |  // block move
    (0 << 9)  |  // 1 word
    (0 << 8)  |  // read
    (3 << 4)  |  // bank 3
    0;           // LUT: don't care

  
  u16_t x;
  u32_t dst = ROLL_FB(_tty_fb_addr + y*(CSZY/2*SPCH));

  for (x = 0; x < (SSZX/CSZX); x++) {
    
  //  --  15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
  //  --   0 |                               ADDR(14:0)                                 |
  //  --   1 |  0 |  - |    ADDR(18:15)    |  S |    HEIGHT(3:0)    |  F | LR | LP | WS |
  //  --   1 |  1 |  - |    WIDTH(3:0)     | RW |    -    | BK(3:0) |      LUT(3:0)     |
  
    *_gpu_pdl++ = dst & 0x7fff;      // address (14:0)
    *_gpu_pdl++ =
      0x8000                 |  //
      ((dst & 0x78000) >> 6) |  // address (18:15)
      (0 << 8)               |  // no stop
      (4 << 4)               |  // 5 lines
      0;                        // no FB change
    *_gpu_pdl++ =
      0xc000    |  // block move
      (1 << 9)  |  // 2 words
      (1 << 8)  |  // write
      (3 << 4)  |  // bank 3
      8;           // LUT 8 (hi-res)

    dst += 4;
  }

}

static void _gpu_start_dl(void)
{
  _gpu_pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + DL_ADDR);

  // check business
  while (B68K_AV_FLEX->cfgsts & B68K_AV_FLEX_CFGSTS_GPU_BUSY_MASK);

}

static void _gpu_flush_dl(void)
{
  if (_gpu_pdl == (u16_t *)(B68K_AV_VRAM_ADDRESS + DL_ADDR)) {
    // nothing to do
    return;
  }
  
  *_gpu_pdl++ =
    0x8000   |
    0x0100   |  // STOP
    (4 << 4);   // 5 lines
  
  // start DL
  B68K_AV_FLEX->gpu0 = (DL_ADDR/2) & 0xff;
  B68K_AV_FLEX->gpu1 = ((DL_ADDR/2) >> 8) & 0xff;

  // reset start of DL
  _gpu_pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + DL_ADDR);
}

static void _gpu_redraw(void)
{
  u8_t x, y;

  /* draw all characters in (current) screen */
  for (y = 0; y < (SSZY/CSZY); y++) {

    _gpu_start_dl();
    
    for (x = 0; x < (SSZX/CSZX); x++) {
      _gpu_draw(x, y, _tty_ptr->screen[x+y*(SSZX/CSZX)], 0 /* TODO */);
    }

    _gpu_flush_dl();
  }
}

static int _tty_putchar(int c)
{
  _tty_putc(_tty_ptr, (u8_t)c);

  return c;
}

static void _tty_putc(struct tty_attr_t *pt, u8_t c)
{
  u8_t x;
  u8_t live = pt == _tty_ptr ? 1 : 0;

  // TODO: manage escape chars
  
  if (c == '\n') {
    // new line

    // possibly remove cursor
    if (live) {
      _gpu_draw(pt->px, pt->py, ' ', 0);
    }
    
    pt->px = 0;

    if (pt->py < (SSZY/CSZY -1)) {
      pt->py = pt->py + 1;

      // clear next line
      for (x = 0; x < (SSZX/CSZX); x++) {
	pt->screen[x+pt->py*(SSZX/CSZX)] = 0;
      }

      if (live) _gpu_clear_line(pt->py);
    } else {
      // need to scroll
      u16_t ch;


      for (ch = 0; ch < (SSZX/CSZX)*(SSZY/CSZY -1); ch++) {
	pt->screen[ch] = pt->screen[ch+(SSZX/CSZX)];
      }
      
      // clear new/next line
      for (x = 0; x < (SSZX/CSZX); x++) {
	pt->screen[x+pt->py*(SSZX/CSZX)] = 0;
      }
      
      // clear one line ahead cause FB base address is not updated yet
      if (live) _gpu_clear_line(SSZY/CSZY);

      // new start address
      _tty_fb_addr = ROLL_FB(_tty_fb_addr + (CSZY/2)*SPCH);

      _gpu_set_fb_addr();
    }

    if (live) _gpu_flush_dl();

  } else {    
    pt->screen[pt->px+pt->py*(SSZX/CSZX)] = c;

    if (live) {
      _gpu_draw(pt->px, pt->py, c, 0);
    }

    if (pt->px < (SSZX/CSZX - 1)) {
      pt->px = pt->px + 1;
    }
    
    //    if (live) _draw(pt->px, pt->py, 0, _cursor);
  }

  return;
}

