//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>

//#include "wm.h"
//#include "font.h"

// console/keyboard
#include <types.h>
#include <sys/tty.h>

// GPU
#include "../kernel/b68k.h"

// VGM
#include "vgm.h"

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 200

#define TILE_SIZE  8

// playfield is 10x20 tiles
#define PF_WIDTH   10
#define PF_HEIGHT  20

#define NEXT_X (20+PF_WIDTH/2+5)
#define NEXT_Y (6)

char _playfield[PF_WIDTH][PF_HEIGHT];

const u8_t _tile_sprite[32] = {
  0x11, 0x11, 0x11, 0x21,
  0x11, 0x11, 0x11, 0x21,
  0x11, 0x11, 0x11, 0x21,
  0x11, 0x11, 0x11, 0x21,
  0x11, 0x11, 0x11, 0x21,
  0x11, 0x11, 0x11, 0x21,
  0x11, 0x11, 0x11, 0x21,
  0x22, 0x22, 0x22, 0x22
};

// 7 bricks, 4 positions, 4x4 bounding box
const char _brick_defs[7][4][4][4] = {
  // cube
  {
    // 0°
    {
      { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 }
    },
    // 90°
    {
      { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 }
    },
    // 180°
    {
      { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 }
    },
    // 270°
    {
      { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 }
    }
  },

  // line
  {
    // 0°
    {
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 }
    },
    // 90°
    {
      { 0, 0, 0, 0 },
      { 1, 1, 1, 1 },
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }
    },
    // 180°
    {
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 }
    },
    // 270°
    {
      { 0, 0, 0, 0 },
      { 1, 1, 1, 1 },
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }
    }    
  },

  // L
  {
    // 0°
    {
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 }
    },
    // 90°
    {
      { 0, 0, 0, 0 },
      { 0, 0, 1, 0 },
      { 1, 1, 1, 0 },
      { 0, 0, 0, 0 }
    },
    // 180°
    {
      { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 1, 0 },
      { 0, 0, 1, 0 }
    },
    // 270°
    {
      { 0, 0, 0, 0 },
      { 0, 1, 1, 1 },
      { 0, 1, 0, 0 },
      { 0, 0, 0, 0 }
    }        
  },

  // _|
  {
    // 0°
    {
      { 0, 0, 1, 0 },
      { 0, 0, 1, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 }
    },
    // 90°
    {
      { 0, 0, 0, 0 },
      { 1, 1, 1, 0 },
      { 0, 0, 1, 0 },
      { 0, 0, 0, 0 }
    },
    // 180°
    {
      { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 0, 0 }
    },
    // 270°
    {
      { 0, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 1, 1 },
      { 0, 0, 0, 0 }
    }        
  },

  // T
  {
    // 0°
    {
      { 0, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 1, 1, 1, 0 },
      { 0, 0, 0, 0 }
    },
    // 90°
    {
      { 0, 0, 0, 0 },
      { 0, 0, 1, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 1, 0 }
    },
    // 180°
    {
      { 0, 0, 0, 0 },
      { 0, 1, 1, 1 },
      { 0, 0, 1, 0 },
      { 0, 0, 0, 0 }
    },
    // 270°
    {
      { 0, 1, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 0, 0 },
      { 0, 0, 0, 0 }
    }        
  },

  // _-
  {
    // 0°
    {
      { 0, 0, 0, 0 },
      { 0, 1, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 1, 0 }
    },
    // 90°
    {
      { 0, 0, 0, 0 },
      { 0, 0, 1, 1 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 }
    },
    // 180°
    {
      { 0, 1, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 1, 0 },
      { 0, 0, 0, 0 }
    },
    // 270°
    {
      { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 1, 1, 0, 0 },
      { 0, 0, 0, 0 }
    }        
  },

  // -_
  {
    // 0°
    {
      { 0, 0, 0, 0 },
      { 0, 0, 1, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 0, 0 }
    },
    // 90°
    {
      { 0, 0, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 1, 1 },
      { 0, 0, 0, 0 }
    },
    // 180°
    {
      { 0, 0, 1, 0 },
      { 0, 1, 1, 0 },
      { 0, 1, 0, 0 },
      { 0, 0, 0, 0 }
    },
    // 270°
    {
      { 0, 0, 0, 0 },
      { 1, 1, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 }
    }        
  }
};

static int _test_position(s16_t x, s16_t y, u8_t rot, u8_t type) {
  u16_t zx, zy;

  for (zy = 0; zy < 4; zy++) {
    for (zx = 0; zx < 4; zx++) {
      if (_brick_defs[type][rot][zy][zx]) {
	if ((x+zx) < 0) return 0;
	if ((x+zx) > (PF_WIDTH-1)) return 0;
	if ((y+zy) < 0) return 0;
	if ((y+zy) > (PF_HEIGHT-1)) return 0;
	
	if (_playfield[x+zx][y+zy]) return 0;
      }
    }
  }

  return 1;
}

static void _update_playfield(s16_t x, s16_t y, u8_t rot, u8_t type) {
  u16_t zx, zy;

  for (zy = 0; zy < 4; zy++) {
    for (zx = 0; zx < 4; zx++) {
      if (_brick_defs[type][rot][zy][zx]) {
	_playfield[x+zx][y+zy] = type + 1;

	if ((x+zx) < 0) printf("oups\n");
	if ((x+zx) > (PF_WIDTH-1)) printf("oups\n");
	if ((y+zy) < 0) printf("oups\n");
	if ((y+zy) > (PF_HEIGHT-1)) printf("oups\n");
      }
    }
  }
}

static int _line_removal(void) {
  s16_t px, py;
  u16_t last = 0;
  u16_t score = 0;

  for (py = 0; py < PF_HEIGHT; py++) {
    u16_t full = 1;

    for (px = 0; px < PF_WIDTH; px++) {
      if (_playfield[px][py] == 0) {
	full = 0;
	break;
      }
    }

    if (full) {
      score++;
      last = py;
    }
  }

  if (score) {
    // remove lines in playfield
    for (py = last; py >= 0; py--) {
      for (px = 0; px < PF_WIDTH; px++) {
	_playfield[px][py] = py-score >= 0 ? _playfield[px][py-score] : 0;
      }
    }
  }
  
  return score;
}

// keyboard status
volatile tty_ioctl_kbd_pos_t kbuf_prev;
volatile tty_ioctl_kbd_pos_t kbuf;

#define DL_ADDR_SP  0x9FC0
#define DL_ADDR_NX  0x9F00
#define DL_ADDR_PF  0xA000
#define SP_ADDR     0x7000  // < 32-k
#define FB_ADDR     0x20000  // in range 0x20000-0x3FFFF

void gpu_set_fb_addr(u32_t ad)
{
  u16_t *_gpu_pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + DL_ADDR_SP);

  // check business
  while (B68K_AV_FLEX->cfgsts & B68K_AV_FLEX_CFGSTS_GPU_BUSY_MASK);

  // build DL
  *_gpu_pdl++ = ad & 0x7fff;     // address (14:0)
  *_gpu_pdl++ =
    0x8000               |  //
    ((ad >> 6) & 0x0600) |  // address (16:15)
    0x100                |  // STOP
    (4 << 4)             |  // 5 lines
    0x0008;                 // F: change frame buffer

  *_gpu_pdl++ =
    0x8000   |
    0x0100   |  // STOP
    (7 << 4);   // 8 lines
  
  // start DL
  B68K_AV_FLEX->gpu0 = (DL_ADDR_SP/2) & 0xff;
  B68K_AV_FLEX->gpu1 = ((DL_ADDR_SP/2) >> 8) & 0xff;
}

static void _draw_brick(s16_t x, s16_t y, u8_t rot, u8_t type) {
  s16_t zx, zy;

  for (zy = 0; zy < 4; zy++) {
    for (zx = 0; zx < 4; zx++) {
      if (_brick_defs[type][rot][zy][zx]) {
	u16_t *pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + DL_ADDR_PF);

	pdl[(x+zx + (y+zy)*PF_WIDTH)*4+3] = 0xC700 + type + 1;  // write width=4
      }
    }
  }
}

static void _erase_brick(s16_t x, s16_t y, u8_t rot, u8_t type) {
  s16_t zx, zy;

  for (zy = 0; zy < 4; zy++) {
    for (zx = 0; zx < 4; zx++) {
      if (_brick_defs[type][rot][zy][zx]) {
	u16_t *pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + DL_ADDR_PF);

	pdl[(x+zx + (y+zy)*PF_WIDTH)*4+3] = 0xC700 + 0;  // write width=4
      }
    }
  }
}

static void _draw_playfield(void)
{
  s16_t x, y;

  for (y = 0; y < PF_HEIGHT; y++) {
    for (x = 0; x < PF_WIDTH; x++) {
      u16_t *pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + DL_ADDR_PF);
      pdl += (x + (y)*PF_WIDTH)*4+3;
      *pdl = 0xC700 + _playfield[x][y];  // write width=4
    }
  }
}

static void _draw_next(u8_t type) {
  s16_t zx, zy;

  for (zy = 0; zy < 4; zy++) {
    for (zx = 0; zx < 4; zx++) {
      if (_brick_defs[type][0][zy][zx]) {
	u16_t *pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + DL_ADDR_NX);

	pdl[(zx + (zy)*4)*4+3] = 0xC700 + type + 1;  // write width=4
      }
    }
  }
}

static void _erase_next(u8_t type) {
  s16_t zx, zy;

  for (zy = 0; zy < 4; zy++) {
    for (zx = 0; zx < 4; zx++) {
      if (_brick_defs[type][0][zy][zx]) {
	u16_t *pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + DL_ADDR_NX);

	pdl[(zx + (zy)*4)*4+3] = 0xC700 + 0;  // write width=4
      }
    }
  }
}

u8_t _get_type()
{
  u8_t r = 7;

  while (r == 7) r = rand() & 7;

  return r;
}

int main(int argc, char *argv[])
{
  u8_t in_loop = 2;
  int score = 0;
  u8_t mode = 0;
  s16_t x = 0, y = 0;
  u8_t rot = 0;
  u8_t type = _get_type();
  u8_t next_type = _get_type();
  u16_t down_cycle = 0;
  u8_t fall = 0;
  u8_t vgm_enable = 0;
  char *music_file = argc > 1 ? argv[1] : "TETRIS.VGM";

  // audio init
  if (vgm_init() < 0 || vgm_load_file(music_file) < 0) {
    puts("warning: couldn't start audio");
  } else {
    vgm_enable = 1;
  }

  puts("press 'r' to (re)start, ESC to quit");
  
  // setup TTY - 0 = stdin
  // disable echo & cursor
  if (ioctl(0, TTY_IOCTL_SET_FLAGS, 0) < 0) {
    printf("IOCTL error\n");
    return -1;
  }

  memset((void *)kbuf.buf, 0, sizeof(kbuf.buf));
  memset((void *)kbuf_prev.buf, 0, sizeof(kbuf_prev.buf));

  //  font_init(args.fb, SCREEN_WIDTH, SCREEN_HEIGHT);
  memset(_playfield, 0, sizeof(_playfield));

  // write @128k
  gpu_set_fb_addr(FB_ADDR);

  // colors
  // color LUT
  // LUT #0, color 0
  B68K_AV_RAMDAC->rd_ad_write = 0;

  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 0;

  // LUT #0, color 1
  B68K_AV_RAMDAC->rd_color = 15;
  B68K_AV_RAMDAC->rd_color = 15;
  B68K_AV_RAMDAC->rd_color = 15;
  B68K_AV_RAMDAC->rd_color = 7;
  B68K_AV_RAMDAC->rd_color = 7;
  B68K_AV_RAMDAC->rd_color = 7;

  // LUT #1, color 1
  // cube, blue
  B68K_AV_RAMDAC->rd_ad_write = 17;

  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 63;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 31;
  
  // LUT #2, color 1
  // line: red
  B68K_AV_RAMDAC->rd_ad_write = 33;

  B68K_AV_RAMDAC->rd_color = 63;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 31;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 0;
  
  // LUT #3, color 1
  // L: magenta
  B68K_AV_RAMDAC->rd_ad_write = 49;

  B68K_AV_RAMDAC->rd_color = 63;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 63;
  B68K_AV_RAMDAC->rd_color = 31;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 31;
  
  // LUT #4, color 1
  // _|: yellow
  B68K_AV_RAMDAC->rd_ad_write = 65;

  B68K_AV_RAMDAC->rd_color = 63;
  B68K_AV_RAMDAC->rd_color = 63;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 31;
  B68K_AV_RAMDAC->rd_color = 31;
  B68K_AV_RAMDAC->rd_color = 0;
  
  // LUT #5, color 1
  // T: brown
  B68K_AV_RAMDAC->rd_ad_write = 81;

  B68K_AV_RAMDAC->rd_color = 63;
  B68K_AV_RAMDAC->rd_color = 31;
  B68K_AV_RAMDAC->rd_color = 31;
  B68K_AV_RAMDAC->rd_color = 31;
  B68K_AV_RAMDAC->rd_color = 15;
  B68K_AV_RAMDAC->rd_color = 15;
  
  // LUT #6, color 1
  // _-: green
  B68K_AV_RAMDAC->rd_ad_write = 97;

  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 63;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 31;
  B68K_AV_RAMDAC->rd_color = 0;
  
  // LUT #7, color 1
  // -_: cyn
  B68K_AV_RAMDAC->rd_ad_write = 113;

  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 63;
  B68K_AV_RAMDAC->rd_color = 63;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 31;
  B68K_AV_RAMDAC->rd_color = 31;
  
  // load sprite for one tile
  {
    // wait til load done
    while (B68K_AV_FLEX->cfgsts & B68K_AV_FLEX_CFGSTS_GPU_BUSY_MASK);

    u8_t *pdst = (u8_t *)(B68K_AV_VRAM_ADDRESS + SP_ADDR);
    u8_t *psrc = (u8_t *)_tile_sprite;
    u16_t sz = 32;  // 8x8x4-bits

    while (sz--) *pdst++ = *psrc++;

    // DL 
    u16_t *pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + DL_ADDR_SP);

    *pdl++ = SP_ADDR;
    *pdl++ =
      0x8000    |
      (0 << 9)  |  // src address <32k
      (7 << 4)  |  // 8 lines
      0;           // 
    *pdl++ =
      0xc000    |
      (1 << 9)  |  // 2 words
      (0 << 8)  |  // read
      (0 << 4)  |  // bank 0
      0;           // LUT: don't care
    *pdl++ = 0x8000 | 0x100; // STOP

    // start DL
    B68K_AV_FLEX->gpu0 = (DL_ADDR_SP/2) & 0xff;
    B68K_AV_FLEX->gpu1 = ((DL_ADDR_SP/2) >> 8) & 0xff;

    // wait til load done
    while (B68K_AV_FLEX->cfgsts & B68K_AV_FLEX_CFGSTS_GPU_BUSY_MASK);
  }    
  
  // setup display list for playfield
  {
    s16_t tx, ty;
    u16_t *pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + DL_ADDR_PF);

    for (ty = 4; ty < (PF_HEIGHT+4); ty++) {
      for (tx = 0; tx < PF_WIDTH; tx++) {
	u16_t dst = (20-PF_WIDTH/2+tx)*8 + (ty & 7)*8*512;

	*pdl++ = 0;  // stub to align (4x 16-bits word per tile
	*pdl++ = dst & 0x7fff;  // ADDR(14:0)
	*pdl++ =
	  0x8000 |
	  0x0800 |  // @ 128k+ ADDR(17)
	  ((ty >> 3) << 9) | // ADDR(18:15)
	  (7 << 4) |  // 8 lines
	  0;          // 
	*pdl++ =
	  0xc000      |  // block move
	  (3 << 9)    |  // 4 words
	  (1 << 8)    |  // write
	  (0 << 4)    |  // bank 0
	  ((0 & 15));    // LUT 0
      }
    }
    
    *pdl++ = 0x8000 | 0x100; // STOP
  }

  // wait til GPU busy
  while (B68K_AV_FLEX->cfgsts & B68K_AV_FLEX_CFGSTS_GPU_BUSY_MASK);

  // draw playfiled
  B68K_AV_FLEX->gpu0 = (DL_ADDR_PF/2) & 0xff;
  B68K_AV_FLEX->gpu1 = ((DL_ADDR_PF/2) >> 8) & 0xff;

  // setup display list for next
  {
    s16_t tx, ty;
    u16_t *pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + DL_ADDR_NX);

    for (ty = 0; ty < 4; ty++) {
      for (tx = 0; tx < 4; tx++) {
	s16_t oy = ty+NEXT_Y;
	u16_t dst = (tx+NEXT_X)*8 + (oy)*8*512;

	*pdl++ = 0;  // stub to align (4x 16-bits word per tile
	*pdl++ = dst & 0x7fff;  // ADDR(14:0)
	*pdl++ =
	  0x8000 |
	  0x0800 |  // @ 128k+ ADDR(17)
	  ((oy >> 3) << 9) | // ADDR(18:15)
	  (7 << 4) |  // 8 lines
	  0;          // 
	*pdl++ =
	  0xc000      |  // block move
	  (3 << 9)    |  // 4 words
	  (1 << 8)    |  // write
	  (0 << 4)    |  // bank 0
	  ((0 & 15));    // LUT 0
      }
    }
    
    *pdl++ = 0x8000 | 0x100; // STOP
  }

  // draw next
  // wait til draw done
  while (B68K_AV_FLEX->cfgsts & B68K_AV_FLEX_CFGSTS_GPU_BUSY_MASK);
  
  B68K_AV_FLEX->gpu0 = (DL_ADDR_NX/2) & 0xff;
  B68K_AV_FLEX->gpu1 = ((DL_ADDR_NX/2) >> 8) & 0xff;

  // setup frame buffer

  // main loop
  while (in_loop) {
    u8_t update = in_loop == 2 ? 1 : 0;
    s16_t nx, ny;
    u8_t nrot;
    
    in_loop = 1;
    
    nx = x;
    ny = y;
    nrot = rot;

    if (fall) down_cycle = 0;

    if ( down_cycle == 0 ) {
      if (mode) {
	
	if (_test_position(x, y+1, rot, type)) {
	  ny = ny+1;
	  update = 1;
	}
      }
    }

    memcpy((void *)kbuf_prev.buf, (void *)kbuf.buf, sizeof(kbuf_prev.buf));

    if (ioctl(0, TTY_IOCTL_KBD_POS, (void *)&kbuf) < 0) {
      printf("IOCTL error\n");
      break;
    }
    
    if ((kbuf.buf[13] & 0x08) && !(kbuf_prev.buf[13] & 0x08)) {
      // left key
      if (_test_position(x-1, down_cycle ? y : y+1, rot, type)) {
	nx = x-1;
	ny = down_cycle ? y : y+1;
	update = 1;
      }
    } else if ((kbuf.buf[14] & 0x10) && !(kbuf_prev.buf[14] & 0x10)) {
      // right key
      if (_test_position(nx+1, down_cycle ? ny : ny+1, nrot, type)) {
	nx = x+1;
	ny = down_cycle ? y : y+1;
	update = 1;
      }
    }

    if ((kbuf.buf[14] & 0x20) && !(kbuf_prev.buf[14] & 0x20)) {
      // up key
      // rotate
      if (_test_position(nx, down_cycle ? ny : ny+1, (nrot+1) & 3, type)) {
	ny = down_cycle ? y : y+1;
	nrot = (rot+1) & 3;
	update = 1;
      }
    }

    if ((kbuf.buf[14] & 0x04) && !(kbuf_prev.buf[14] & 0x04)) {
      // down key
      // fall
      fall = 1;
    }

    if ((kbuf.buf[14] & 0x40) && !(kbuf_prev.buf[14] & 0x40)) {
      // ESC
      in_loop = 0;
    }
    
    if ((kbuf.buf[5] & 0x20) && !(kbuf_prev.buf[5] & 0x20)) {
      // 'r'
      // reset
      memset(_playfield, 0, sizeof(_playfield));
      _draw_playfield();
      
      x = 3;
      y = 0;
      nx = x;
      ny = y;
      rot = 0;
      type = _get_type();
      next_type = _get_type();
      down_cycle = 30;
	
      // draw new brick at start location/rotation
      _draw_brick(x, y, rot, type);
	
      // draw new next
      _draw_next(next_type);
      
      mode = 1;
    }

    if (mode) {
      if (update) {
	// erase brick at previous location/rotation 
	_erase_brick(x, y, rot, type);

	// draw brick at new location/rotation 
	_draw_brick(nx, ny, nrot, type);
      }

      if (down_cycle == 0 && !update) {
	// can't fall more
	_update_playfield(x, y, rot, type);
	
	// test full line removal
	int score_add = _line_removal();
	if (score_add) _draw_playfield();
	score += score_add;
	
	// erase old next
	_erase_next(next_type);

	// choose next brick
	x = 3;
	y = 0;
	rot = 0;
	type = next_type;
	next_type = _get_type();
	
	fall = 0;

	// draw new next
	_draw_next(next_type);

	// draw new brick
	_draw_brick(x, y, rot, type);
      } else {
	// continue
	x = nx;
	y = ny;
	rot = nrot;
      }

      if (down_cycle == 0) {
	down_cycle = 100; // TODO manage speed
      } else {
	down_cycle--;
      }
    
    } else {
      // game over / not started
      //      font_set_bg_color(0);
      //      font_set_fg_color(0xffffffff);
      //      font_printf(SCREEN_WIDTH/2 - 27, SCREEN_HEIGHT/2 - 3, "GAME OVER");
    }
    
    //    font_set_bg_color(0);
    //    font_printf(0, 0, "SCORE: %d   ", score);

    // draw next
    B68K_AV_FLEX->gpu0 = (DL_ADDR_NX/2) & 0xff;
    B68K_AV_FLEX->gpu1 = ((DL_ADDR_NX/2) >> 8) & 0xff;
    
    // wait til draw done
    while (B68K_AV_FLEX->cfgsts & B68K_AV_FLEX_CFGSTS_GPU_BUSY_MASK);
  
    // draw playfiled
    B68K_AV_FLEX->gpu0 = (DL_ADDR_PF/2) & 0xff;
    B68K_AV_FLEX->gpu1 = ((DL_ADDR_PF/2) >> 8) & 0xff;

    // VGM periodic
    if (vgm_enable) {
      vgm_update();
    }

    // wait til draw done
    while (B68K_AV_FLEX->cfgsts & B68K_AV_FLEX_CFGSTS_GPU_BUSY_MASK);
  
  }

  // resore TTY
  ioctl(0, TTY_IOCTL_SET_FLAGS, (void *)(TTY_FLAG_ECHO | TTY_FLAG_CURSOR));

  /*
  B68K_AV_FLEX->rbcfg1     = 0;
  B68K_AV_FLEX->rbcfg2     = 0;
  B68K_AV_FLEX->rbcfg0_vbk = 
    (0 & 1)          |  // @ bit 16
    (0 << 1)         |  // hi res
    (0 << 2)         |  // buffer 0
    (0 << 3)         |  // color select (not use in low res)
    (0 << 4);           // bank 0
    ;
  */
  
  return 0;
}
