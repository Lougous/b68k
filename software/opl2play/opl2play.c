//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// audio player for b68k-av board OPL2 chip
// support: VGM and DRO files
//

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include <types.h>
#include <sys/opl2.h>

#define DEVICE  "/dev/opl2"

volatile opl2_ring_buffer_t rb;

static uint16_t read_UINT16LE(int fd)
{
  uint16_t le;
  read(fd, &le, 2);

  return (le >> 8) + (le << 8);
}

static uint32_t read_UINT32LE(int fd)
{
  uint32_t le;
  read(fd, &le, 4);

  return (le >> 24) + ((le >> 8) & 0xff00) + ((le & 0xff00) << 8) + (le << 24);
}

int do_DRO(char *fname, int fd)
{
  // read header
  int n = read(fd, (void *)rb.buf, 8);

  if (n != 8) return 0;
  if (strncmp((char *)rb.buf, "DBRAWOPL", 8)) return 0;
  
  // read version
  uint16_t iVersionMajor = read_UINT16LE(fd);
  uint16_t iVersionMinor = read_UINT16LE(fd);
  printf("%s: DRO version %d.%d\n", fname, iVersionMajor, iVersionMinor);

  if (iVersionMajor != 0) {
    printf("invalid version\n");
    return 0;
  }

  if (iVersionMajor == 0 && iVersionMinor == 1) {
    // Version 0.1
    uint32_t iLengthMS = read_UINT32LE(fd);
    uint32_t iLengthBytes = read_UINT32LE(fd);
    char iHardwareType;
    read(fd, &iHardwareType, 1);

    printf(" %u ms\n", iLengthMS);
    printf(" %u bytes\n", iLengthBytes);

    if (iHardwareType != 0) {
      printf("hardware type (%d) is not OPL2\n", iHardwareType);
      return 0;
    }

    // first 3 bytes may be header or data
    read(fd, (void *)rb.buf, 3);

    if (rb.buf[0] == 0 && rb.buf[1] == 0 && rb.buf[2] == 0 ) {
      // discard header
      rb.wptr = 0;
    } else {
      // actual data
      rb.wptr = 3;
      iLengthBytes -= 3;
    }

    while (iLengthBytes > 0) {
      uint8_t used = rb.wptr - rb.rptr;  // works when rptr > wptr as well !

#define CSIZE  64
      if (used < (256-CSIZE)) {
	// can add CSIZE byte in buffer, refill
	uint16_t len = iLengthBytes > CSIZE ? CSIZE : iLengthBytes;
	uint16_t lc = 256 - (uint16_t)rb.wptr;  // possible len for first chunk (up to end of buffer)
	
	if (lc >= len) {
	  // ok for 1 chunk
	  read(fd, (void *)(rb.buf + rb.wptr), len);
	} else {
	  // need 2 chunks
	  read(fd, (void *)(rb.buf + rb.wptr), lc);  // first chunk (up to end of buffer)
	  read(fd, (void *)rb.buf, len-lc);        // second chunk (from start of buffer)
	}

	rb.wptr += len;
	iLengthBytes -= len;
      }
    }
  } else {
    // TODO: Version 2.0
    printf("invalid version\n");
    return 0;
  }
   
  return 1;
}

char _buf[32];
s16_t _len;
u8_t _rptr;

void b_init()
{
  _len = 0;
  _rptr = 0;
}

s16_t b_getc(int fd)
{
  if (_len) {
    _len--;
    return _buf[_rptr++];
  } else {
    _len = read(fd, _buf, sizeof(_buf));

    if (_len <= 0) {
      return -1;
    }
    
    _len--;
    _rptr = 1;
    return _buf[0];
  }
}

u16_t _time_error;

void _pause(u16_t time)
{
  while (time >= 441) {
    uint8_t used = rb.wptr - rb.rptr;

    if (used < 250) {
      // >= 10 ms
      rb.buf[rb.wptr++] = 0;
      rb.buf[rb.wptr++] = 9;
      time -= 441;
    }	  

    //printf("  10\n");
  }
  
  time = time*10 + _time_error;
  
  while (time >= 441) {
    uint8_t used = rb.wptr - rb.rptr;
    
    if (used < 250) {
      // >= 1 ms
      rb.buf[rb.wptr++] = 0;
      rb.buf[rb.wptr++] = 0;
      time -= 441;
    }	  

    //printf("  1\n");
  }
  
  _time_error = time;
}
  
int do_VGM(char *fname, int fd)
{
  // read header
  int n = read(fd, (void *)rb.buf, 4);

  if (n != 4) return 0;
  if (strncmp((void *)rb.buf, "Vgm ", 4)) return 0;

  // EoF offset
  read(fd, (void *)rb.buf, 4);

  // Version
  u32_t version = read_UINT32LE(fd);
  printf("%s: version %Xh\n", fname, version);

  // rest of header < 0x161
  read(fd, (void *)rb.buf, 128-12);

  // rest of header >= 0x161
  if (version >= 0x161) read(fd, (void *)rb.buf, 128);
  
  //
  rb.wptr = 0;

  b_init();
  
  //
  _time_error = 0;
  
  while (1) {
    uint8_t used = rb.wptr - rb.rptr;  // works when rptr > wptr as well !

    if (used < 250) {
      s16_t cmd = b_getc(fd);

      if (cmd < 0) break;

      if (cmd == 0x5A) {
	// address
	u8_t reg = b_getc(fd);
	u8_t dt  = b_getc(fd);

	if (reg > 4) {
	  rb.buf[rb.wptr++] = reg;
	  rb.buf[rb.wptr++] = dt;
	} else {
	  printf(" ignore @%02Xh=%02Xh\n", reg, dt);
	}
      } else if (cmd == 0x61) {
	u16_t time_lsb = (u8_t)b_getc(fd);
	u16_t time_msb = (u8_t)b_getc(fd);
	u16_t time     = time_lsb + (time_msb << 8);
	//	time = (time << 8) + b_getc(fd);
	//time = time + (((u16_t)b_getc(fd)) << 8);
	// unit: 22.6757... us (44.1kHz)

	//printf("P %04Xh\n", time);
	
	_pause(time);
	
      } else if (cmd == 0x62) {
	_pause(0x2df);
      } else if (cmd == 0x63) {
	_pause(0x372);
      } else if (cmd == 0x66) {
	// end
	break;
      } else if ((cmd & 0xF0) == 0x70) {
	// 0x7n       : wait n+1 samples, n can range from 0 to 15.
	_time_error += 10*(1+(cmd & 0xf));
      } else {
	printf("%s: unsupported command %02Xh\n", fname, cmd);
	break;
      }
    }
  }

  return 1;
}


void opl2_soft_reset(void)
{
  u8_t channel_map[9] = { 0,1,2,8,9,10,16,17,18 };
  u8_t c;

  while (rb.wptr != rb.rptr);
  
  rb.buf[rb.wptr++] = 0x08;
  rb.buf[rb.wptr++] = 0x00;
  rb.buf[rb.wptr++] = 0xBD;
  rb.buf[rb.wptr++] = 0x00;

  for (c = 0; c < 9; c++) {
    rb.buf[rb.wptr++] = 0xA0+c;
    rb.buf[rb.wptr++] = 0x00;
    rb.buf[rb.wptr++] = 0xB0+c;
    rb.buf[rb.wptr++] = 0x00;
    rb.buf[rb.wptr++] = 0xC0+c;
    rb.buf[rb.wptr++] = 0x00;

    u8_t cm = channel_map[c];
    
    rb.buf[rb.wptr++] = 0x20+cm;
    rb.buf[rb.wptr++] = 0x00;
    rb.buf[rb.wptr++] = 0x20+cm+3;
    rb.buf[rb.wptr++] = 0x00;

    rb.buf[rb.wptr++] = 0x40+cm;
    rb.buf[rb.wptr++] = 0x00;
    rb.buf[rb.wptr++] = 0x40+cm+3;
    rb.buf[rb.wptr++] = 0x00;

    rb.buf[rb.wptr++] = 0x60+cm;
    rb.buf[rb.wptr++] = 0x00;
    rb.buf[rb.wptr++] = 0x60+cm+3;
    rb.buf[rb.wptr++] = 0x00;

    rb.buf[rb.wptr++] = 0x90+cm;
    rb.buf[rb.wptr++] = 0x00;
    rb.buf[rb.wptr++] = 0x90+cm+3;
    rb.buf[rb.wptr++] = 0x00;

    rb.buf[rb.wptr++] = 0xE0+cm;
    rb.buf[rb.wptr++] = 0x00;
    rb.buf[rb.wptr++] = 0xE0+cm+3;
    rb.buf[rb.wptr++] = 0x00;
  }

}

int main (int argc, char *argv[])
{
  int opl2_fd = open(DEVICE, O_RDONLY);

  if (opl2_fd < 0) {
    printf("%s: cannot open device\n", DEVICE);
    return -1;
  }

  rb.rptr = 0;
  rb.wptr = 0;

  // register the ring buffer 
  if (ioctl(opl2_fd, OPL2_IOCTL_SETBUF, (void *)&rb) < 0) {
    printf("%s: cannot register ring buffer\n", DEVICE);
    return -1;
  }

  while (argc > 1) {
    int played = 0;

    argc--;
    argv++;

    //    opl2_soft_reset();

    // DRO
    int fd = open(argv[0], O_RDONLY);

    if (fd >= 0) {
      played = do_DRO(argv[0], fd);
    } else {
      printf("%s: cannot open file\n", argv[0]);
      continue;
    }

    
    close(fd);

    // VGM
    if (!played) {
      fd = open(argv[0], O_RDONLY);

      if (fd >= 0) {
	played = do_VGM(argv[0], fd);
      }

      close(fd);
    }

    // TODO: other formats ...
   
    if (!played) printf("%s: unsupported format\n", argv[0]); 

  }

  return 0;
}


