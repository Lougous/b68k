//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

// console/keyboard
#include <types.h>
#include <sys/tty.h>

// GPU
#include "../kernel/b68k.h"

// 
#include "sprites.h"

#define MS_DEVICE  "/dev/mouse"

#define TILE_SIZE  8

// unscaled resolution 320x200
#define TILE_NB_X  (320/TILE_SIZE)
#define TILE_NB_Y  (240/TILE_SIZE)

char _play_field[TILE_NB_Y][TILE_NB_X];

#define PF_MINE_MASK      0x01
#define PF_FLAGGED_MASK   0x02
#define PF_REVEALED_MASK  0x04
#define PF_NEIGHBOR_MASK  0xf0

#define VISUAL_COVERED  0
#define VISUAL_EMPTY    1
#define VISUAL_BOMB     2
#define VISUAL_BLEWED   3
#define VISUAL_FLAG     4
#define VISUAL_COUNT    8  // to 15

// GFX memory mapping
#define GFX_SP_COVERED     0x7000  // < 32-k
#define GFX_SP_EMPTY       0x7020  // < 32-k
#define GFX_SP_BOMB        0x7040  // < 32-k
#define GFX_SP_BLEWED      0x7060  // < 32-k
#define GFX_SP_FLAG        0x7080  // < 32-k
#define GFX_SP_COUNT1      0x7100  // < 32-k
#define GFX_SP_COUNT2      0x7120  // < 32-k
#define GFX_SP_COUNT3      0x7140  // < 32-k
#define GFX_SP_COUNT4      0x7160  // < 32-k
#define GFX_SP_COUNT5      0x7180  // < 32-k
#define GFX_SP_COUNT6      0x71A0  // < 32-k
#define GFX_SP_COUNT7      0x71C0  // < 32-k
#define GFX_SP_COUNT8      0x71E0  // < 32-k
#define GFX_SP_CURSOR      0x7200  // < 32-k

#define GFX_DL             0x9FC0

static void load_sprite(u16_t dst, u8_t *src, u16_t sz)
{
  u8_t *pdst = (u8_t *)(B68K_AV_VRAM_ADDRESS + dst);

  while (sz--) {
    *pdst++ = *src++;
  }
}

void gpu_set_fb_addr(u32_t ad)
{
  u16_t *_gpu_pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + GFX_DL);

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
  B68K_AV_FLEX->gpu0 = (GFX_DL/2) & 0xff;
  B68K_AV_FLEX->gpu1 = ((GFX_DL/2) >> 8) & 0xff;
}

static void draw_sprite (u16_t px, u16_t py, u16_t sp_ad) {

  // wait til GPU ready
  while (B68K_AV_FLEX->cfgsts & B68K_AV_FLEX_CFGSTS_GPU_BUSY_MASK);
  while (B68K_AV_FLEX->cfgsts & B68K_AV_FLEX_CFGSTS_GPU_BUSY_MASK);

  u16_t *pdl = (u16_t *)(B68K_AV_VRAM_ADDRESS + GFX_DL);
  
  // load sprite to scratch pad
  *pdl++ = sp_ad;
  *pdl++ =
    0x8000    |
    (0 << 9)  |  // src address <32k
    (7 << 4)  |  // 8 lines
    0;           // 
  *pdl++ =
    0xc000    |  // block move
    (1 << 9)  |  // 2 words
    (0 << 8)  |  // read
    (0 << 4)  |  // bank 0
    0;           // LUT: don't care

  // draw sprite
  u16_t dst = px + (py & 63)*512;

  *pdl++ = dst & 0x7fff;  // ADDR(14:0)
  *pdl++ =
    0x8000           |
    0x0800           |  // @ 128k+ ADDR(17)
    ((py >> 6) << 9) |  // ADDR(18:15)
    (7 << 4)         |  // 8 lines
    0;                  // 
  *pdl++ =
    0xC000       |  // block move
    (3 << 9)     |  // 4 words
    (1 << 8)     |  // write
    (1 << 7)     |  // color key
    (0 << 4)     |  // bank 0
    ((0 & 15));     // LUT 0

  // end of DL
  *pdl++ = 0x8000 | 0x100; // STOP

  // start DL
  B68K_AV_FLEX->gpu0 = (GFX_DL/2) & 0xff;
  B68K_AV_FLEX->gpu1 = ((GFX_DL/2) >> 8) & 0xff;
}


static void reset (int mine_nb) {
  int x, y, m;  

  // draw play field
  for (y = 0; y < TILE_NB_Y*TILE_SIZE; y += TILE_SIZE) {
    for (x = 0; x < TILE_NB_X*TILE_SIZE; x += TILE_SIZE) {
      draw_sprite(x, y, GFX_SP_COVERED);
    }
  }

  // generate mines positions
  for (y = 0; y < TILE_NB_Y; y++) {
    for (x = 0; x < TILE_NB_X; x++) {
      _play_field[y][x] = 0;  // no mine
    }
  }
  
  for (m = 0; m < mine_nb; m++) {
    char retry = 1;

    while (retry) {
      x = TILE_NB_X;
      while (x >= TILE_NB_X) x = rand() & 127;
      
      y = TILE_NB_Y;
      while (y >= TILE_NB_Y) y = rand() & 127;

      if (_play_field[y][x] == 0) {
	_play_field[y][x] = PF_MINE_MASK;  // mine
	retry = 0;
      }
    }
  }
}

static void touch_tile(int px, int py) {
  uint8_t nb_neighbours = 0;

  // discover cleared tile
  // discard revealed or flagged tiles
  if (_play_field[py][px] & (PF_FLAGGED_MASK | PF_REVEALED_MASK)) return;
  
  _play_field[py][px] = PF_REVEALED_MASK;
    
  // left
  if (px > 0 && (_play_field[py][px-1] & 1) == 1) {
    nb_neighbours++;
  }

  // up left 
  if (px > 0 && py > 0 && (_play_field[py-1][px-1] & 1) == 1) {
    nb_neighbours++;
  }

  // bottom left
  if (px > 0 && py < (TILE_NB_Y - 1) && (_play_field[py+1][px-1] & 1) == 1) {
    nb_neighbours++;
  }

  // up
  if (py > 0 && (_play_field[py-1][px] & 1) == 1) {
    nb_neighbours++;
  }

  // bottom
  if (py < (TILE_NB_Y - 1) && (_play_field[py+1][px] & 1) == 1) {
    nb_neighbours++;
  }

  // up right
  if (px < (TILE_NB_X - 1) && py > 0 && (_play_field[py-1][px+1] & 1) == 1) {
    nb_neighbours++;
  }

  // right
  if (px < (TILE_NB_X - 1) && (_play_field[py][px+1] & 1) == 1) {
    nb_neighbours++;
  }

  // bottom right
  if (px < (TILE_NB_X - 1) && py < (TILE_NB_Y - 1) && (_play_field[py+1][px+1] & 1) == 1) {
    nb_neighbours++;
  }

  _play_field[py][px] |= nb_neighbours << 4;
  
  switch (nb_neighbours) {
  case 0:
    // no neighbour, explore
    if (px > 0) touch_tile(px-1, py);
    if (px > 0 && py > 0) touch_tile(px-1, py-1);
    if (px > 0 && py < (TILE_NB_Y - 1)) touch_tile(px-1, py+1);
    if (py > 0) touch_tile(px, py-1);
    if (py < (TILE_NB_Y - 1)) touch_tile(px, py+1);
    if (px < (TILE_NB_X - 1) && py > 0) touch_tile(px+1, py-1);
    if (px < (TILE_NB_X - 1)) touch_tile(px+1, py);
    if (px < (TILE_NB_X - 1) && py < (TILE_NB_Y - 1)) touch_tile(px+1, py+1);

    draw_sprite(px*TILE_SIZE, py*TILE_SIZE, GFX_SP_EMPTY);


    break;

  case 1:
    draw_sprite(px*TILE_SIZE, py*TILE_SIZE, GFX_SP_COUNT1);
    break;
      
  case 2:
    draw_sprite(px*TILE_SIZE, py*TILE_SIZE, GFX_SP_COUNT2);
    break;
      
  case 3:
    draw_sprite(px*TILE_SIZE, py*TILE_SIZE, GFX_SP_COUNT3);
    break;
      
  case 4:
    draw_sprite(px*TILE_SIZE, py*TILE_SIZE, GFX_SP_COUNT4);
    break;
      
  case 5:
    draw_sprite(px*TILE_SIZE, py*TILE_SIZE, GFX_SP_COUNT5);
    break;
      
  case 6:
    draw_sprite(px*TILE_SIZE, py*TILE_SIZE, GFX_SP_COUNT6);
    break;
      
  case 7:
    draw_sprite(px*TILE_SIZE, py*TILE_SIZE, GFX_SP_COUNT7);
    break;
      
   case 8:
    draw_sprite(px*TILE_SIZE, py*TILE_SIZE, GFX_SP_COUNT8);
    break;
           
  default:
    break;
  }
    
  return;
}

static void redraw(u16_t tx, u16_t ty)
{
  if ((_play_field[ty][tx] & (PF_FLAGGED_MASK | PF_REVEALED_MASK)) == 0) {
    draw_sprite(tx*TILE_SIZE, ty*TILE_SIZE, GFX_SP_COVERED);
  } else if ((_play_field[ty][tx] & (PF_FLAGGED_MASK | PF_REVEALED_MASK)) == PF_FLAGGED_MASK) {
    draw_sprite(tx*TILE_SIZE, ty*TILE_SIZE, GFX_SP_FLAG);
  } else if (_play_field[ty][tx] & PF_MINE_MASK) {
    draw_sprite(tx*TILE_SIZE, ty*TILE_SIZE, GFX_SP_BOMB);
  } else {
    u8_t nb = _play_field[ty][tx] >> 4;
      
    if (nb) {
      draw_sprite(tx*TILE_SIZE, ty*TILE_SIZE, GFX_SP_COUNT1 + 0x20*(nb-1));
    }
    else {
      draw_sprite(tx*TILE_SIZE, ty*TILE_SIZE, GFX_SP_EMPTY);
    }
  }
}


// keyboard status
volatile tty_ioctl_kbd_pos_t kbuf_prev;
volatile tty_ioctl_kbd_pos_t kbuf;

// mouse status
uint16_t ms_nfo[3];

int main (int argc, char *argv[]) {

  int in_loop = 1;
  int mode = 1;
  int mine_nb = TILE_NB_X * TILE_NB_Y / 8;
  int flagged = 0;

  // setup keyboard
  // disable echo & cursor
  if (ioctl(0, TTY_IOCTL_SET_FLAGS, 0) < 0) {
    printf("IOCTL error\n");
    return -1;
  }

  memset((void *)kbuf.buf, 0, sizeof(kbuf.buf));
  memset((void *)kbuf_prev.buf, 0, sizeof(kbuf_prev.buf));

  // open mouse
  int mfd = open(MS_DEVICE, O_RDONLY);
  
  if (mfd < 0) {
    printf("%s: cannot open mouse device\n", MS_DEVICE);
    return -1;
  }

  // load sprites
  load_sprite(GFX_SP_COVERED, (u8_t *)sprite_covered, 32);
  load_sprite(GFX_SP_EMPTY, (u8_t *)sprite_empty, 32);
  load_sprite(GFX_SP_BOMB, (u8_t *)sprite_bomb, 32);
  load_sprite(GFX_SP_BLEWED, (u8_t *)sprite_covered, 32);
  load_sprite(GFX_SP_FLAG, (u8_t *)sprite_flag, 32);
  load_sprite(GFX_SP_COUNT1, (u8_t *)sprite_count1, 32);
  load_sprite(GFX_SP_COUNT2, (u8_t *)sprite_count2, 32);
  load_sprite(GFX_SP_COUNT3, (u8_t *)sprite_count3, 32);
  load_sprite(GFX_SP_COUNT4, (u8_t *)sprite_count4, 32);
  load_sprite(GFX_SP_COUNT5, (u8_t *)sprite_count5, 32);
  load_sprite(GFX_SP_COUNT6, (u8_t *)sprite_count6, 32);
  load_sprite(GFX_SP_COUNT7, (u8_t *)sprite_count7, 32);
  load_sprite(GFX_SP_COUNT8, (u8_t *)sprite_count8, 32);
  load_sprite(GFX_SP_CURSOR, (u8_t *)sprite_cursor, 32);

  // setup color table
  B68K_AV_RAMDAC->rd_ad_write = 0;

  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 0;
  
  B68K_AV_RAMDAC->rd_color = 56;  // LG
  B68K_AV_RAMDAC->rd_color = 56;
  B68K_AV_RAMDAC->rd_color = 56;
  
  B68K_AV_RAMDAC->rd_color = 32;  // MG
  B68K_AV_RAMDAC->rd_color = 32;
  B68K_AV_RAMDAC->rd_color = 32;
  
  B68K_AV_RAMDAC->rd_color = 8;   // DG
  B68K_AV_RAMDAC->rd_color = 8;
  B68K_AV_RAMDAC->rd_color = 8;
  
  B68K_AV_RAMDAC->rd_color = 0;   // BL
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 63;
  
  B68K_AV_RAMDAC->rd_color = 0;   // BK
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 0;
  
  B68K_AV_RAMDAC->rd_color = 63;   // RD
  B68K_AV_RAMDAC->rd_color = 0;
  B68K_AV_RAMDAC->rd_color = 0;
  
  B68K_AV_RAMDAC->rd_color = 24;   // NG
  B68K_AV_RAMDAC->rd_color = 24;
  B68K_AV_RAMDAC->rd_color = 24;
  
  B68K_AV_RAMDAC->rd_color = 63;   // WT
  B68K_AV_RAMDAC->rd_color = 63;
  B68K_AV_RAMDAC->rd_color = 63;
  
  B68K_AV_RAMDAC->rd_color = 0;   // GR
  B68K_AV_RAMDAC->rd_color = 63;
  B68K_AV_RAMDAC->rd_color = 0;
  
  // reset game
  reset(mine_nb);

  u16_t mx_prev = 0, my_prev = 0, mb_prev = 0;

  gpu_set_fb_addr(0);

  // main loop
  while (in_loop) {

    // inspect keyboard
    memcpy((void *)kbuf_prev.buf, (void *)kbuf.buf, sizeof(kbuf_prev.buf));

    if (ioctl(0, TTY_IOCTL_KBD_POS, (void *)&kbuf) < 0) {
      printf("IOCTL error\n");
      break;
    }
    
    if ((kbuf.buf[14] & 0x40) && !(kbuf_prev.buf[14] & 0x40)) {
      // ESC
      in_loop = 0;
    }
    
    if ((kbuf.buf[5] & 0x20) && !(kbuf_prev.buf[5] & 0x20)) {
      // 'r'
      // reset
      reset(mine_nb);
      mode = 1;
    }

    // get mouse status
    read(mfd, ms_nfo, sizeof(ms_nfo));
    
    u16_t mx, my;
    u16_t tx, ty;
	
    mx = ms_nfo[0] / 2;
    tx = mx / TILE_SIZE;
    my = ms_nfo[1] / 2;
    ty = my / TILE_SIZE;

    if (tx < TILE_NB_X && ty < TILE_NB_Y) {
      if (mode == 1) {
	if ((mb_prev & 1 ) && ! (ms_nfo[2] & 1)) {
	  // left button
	  if ((_play_field[ty][tx] & (PF_FLAGGED_MASK | PF_REVEALED_MASK)) == 0) {
	    // unrevealed yet
	    if (_play_field[ty][tx] & PF_MINE_MASK) {
	      // boom !
	      for (ty = 0; ty < TILE_NB_Y; ty++) {
		for (tx = 0; tx < TILE_NB_X; tx++) {
		  if (_play_field[ty][tx] & PF_MINE_MASK) {
		    // TODO: show errors with flags
		    _play_field[ty][tx] |= PF_REVEALED_MASK;
		    draw_sprite(tx*TILE_SIZE, ty*TILE_SIZE, GFX_SP_BOMB);
		  }
		}
	      }
		
	      mode = 0;
	    } else {
	      touch_tile(tx, ty);
	    }
	  }
	} else if ((mb_prev & 2) && ! (ms_nfo[2] & 2)) {
	  // right button
	  if ((_play_field[ty][tx] & (PF_FLAGGED_MASK | PF_REVEALED_MASK)) == 0) {
	    // flag tile
	    if (flagged < mine_nb) {
	      _play_field[ty][tx] |= PF_FLAGGED_MASK;
	      flagged++;
	      draw_sprite(tx*TILE_SIZE, ty*TILE_SIZE, GFX_SP_FLAG);
	    }
	  } else if ((_play_field[ty][tx] & (PF_FLAGGED_MASK | PF_REVEALED_MASK)) == PF_FLAGGED_MASK) {
	    // unflag tile
	    _play_field[ty][tx] &= ~(PF_FLAGGED_MASK | PF_REVEALED_MASK);
	    flagged--;
	    draw_sprite(tx*TILE_SIZE, ty*TILE_SIZE, GFX_SP_COVERED);
	  }
	}
      }
    }

    mb_prev = ms_nfo[2];
    
    //font_printf(0, TILE_NB_Y*TILE_SIZE, "MINES: %d  ", mine_nb - flagged);

    // display cursor
    u16_t tx_prev = mx_prev / TILE_SIZE;
    u16_t ty_prev = my_prev / TILE_SIZE;
    

    redraw(tx_prev, ty_prev);
    if (tx_prev < TILE_NB_X) redraw(tx_prev+1, ty_prev);
    if (ty_prev < TILE_NB_Y) redraw(tx_prev, ty_prev+1);
    if (ty_prev < TILE_NB_Y && tx_prev < TILE_NB_X) redraw(tx_prev+1, ty_prev+1);

    draw_sprite(mx, my, GFX_SP_CURSOR);

    mx_prev = mx;
    my_prev = my;

  }

  // resore TTY
  ioctl(0, TTY_IOCTL_SET_FLAGS, (void *)(TTY_FLAG_ECHO | TTY_FLAG_CURSOR));

  return 0;
}
