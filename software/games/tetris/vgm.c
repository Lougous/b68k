
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//

#include <fcntl.h>
#include <sys/ioctl.h>

#include <types.h>
#include <sys/opl2.h>

#define DEVICE    "/dev/opl2"
#define BUF_SIZE  16384

extern int16_t _fgetc(int fd);
extern void _finit();

static volatile opl2_ring_buffer_t _rb;
static int _fd;

static uint32_t _read_UINT32LE(int fd)
{
  uint32_t le;
  read(fd, &le, 4);

  return (le >> 24) + ((le >> 8) & 0xff00) + ((le & 0xff00) << 8) + (le << 24);
}

// DRO buffer
static u8_t _dbuf[BUF_SIZE];
uint16_t _dlen;
uint16_t _dptr;


uint16_t _time_error;

static void _pause(u16_t time)
{
  while (time >= 441) {
    if (_dlen < sizeof(_dbuf)) {
      // >= 10 ms
      _dbuf[_dlen++] = 0;
      _dbuf[_dlen++] = 9;
      time -= 441;
    }	  

    //printf("  10\n");
  }
  
  time = time*10 + _time_error;
  
  while (time >= 441) {
    if (_dlen < sizeof(_dbuf)) {
      // >= 1 ms
      _dbuf[_dlen++] = 0;
      _dbuf[_dlen++] = 0;
      time -= 441;
    }	  

    //printf("  1\n");
  }
  
  _time_error = time;
}
  

int16_t vgm_init(void)
{
  _fd = 0;

  int opl2_fd = open(DEVICE, O_RDONLY);

  if (opl2_fd < 0) {
    printf("%s: cannot open device\n", DEVICE);
    return -1;
  }

  _rb.rptr = 0;
  _rb.wptr = 0;

  // register the ring buffer 
  if (ioctl(opl2_fd, OPL2_IOCTL_SETBUF, (void *)&_rb) < 0) {
    printf("%s: cannot register ring buffer\n", DEVICE);
    return -1;
  }

  return 0;
}



int16_t vgm_load_file(char *fname)
{
  printf("loading %s ...\n", fname);
  
  _fd = open(fname, O_RDONLY);

  if (_fd < 0) {
    return -1;
  }

  // read header
  int n = read(_fd, (void *)_rb.buf, 4);

  if (n != 4) return 0;
  if (strncmp((void *)_rb.buf, "Vgm ", 4)) {
    close(_fd);
    _fd = 0;
    return -1;
  }
  
  // EoF offset
  read(_fd, (void *)_rb.buf, 4);

  // Version
  uint32_t version = _read_UINT32LE(_fd);
  //  printf("%s: version %Xh\n", fname, version);

  // rest of header < 0x161
  read(_fd, (void *)_rb.buf, 128-12);

  // rest of header >= 0x161
  if (version >= 0x161) read(_fd, (void *)_rb.buf, 128);
  
  //
  _rb.wptr = 0;

  _finit();

  // DRO buffer
  _dlen = 0;
  _dptr = 0;
  
  //
  _time_error = 0;

  while (_dlen < sizeof(_dbuf)) {
    int16_t cmd = _fgetc(_fd);

    if (cmd < 0) return -1;

    if (cmd == 0x5A) {
      // address
      uint8_t reg = _fgetc(_fd);
      uint8_t dt  = _fgetc(_fd);

      if (reg > 4) {
	_dbuf[_dlen++] = reg;
	_dbuf[_dlen++] = dt;
      } else {
	//printf(" ignore @%02Xh=%02Xh\n", reg, dt);
      }
    } else if (cmd == 0x61) {
      uint16_t time_lsb = (uint8_t)_fgetc(_fd);
      uint16_t time_msb = (uint8_t)_fgetc(_fd);
      uint16_t time     = time_lsb + (time_msb << 8);
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
      printf("VGM: unsupported command %02Xh\n", cmd);
      return -1;
    }
  }

  printf("DRO data: %u bytes\n", _dlen);
  
  return 0;
}


int16_t vgm_update(void)
{
  if (_fd < 0) {
    return -1;
  }
  
  uint8_t used = _rb.wptr - _rb.rptr;  // works when rptr > wptr as well !

#define CSIZE  32
  
  while (used < (256-CSIZE)) {
    // can add CSIZE byte in buffer, refill
    if ((_dlen - _dptr) < CSIZE) {
      // 1st chunk
      memcpy((void *)(_rb.buf + _rb.wptr), (void *)(_dbuf + _dptr), _dlen - _dptr);

      // 2nd chunk
      uint16_t to_load = CSIZE - (_dlen - _dptr);
      memcpy((void *)(_rb.buf + _rb.wptr + _dlen - _dptr), (void *)(_dbuf), to_load);
      _dptr = to_load;
    } else {
      memcpy((void *)(_rb.buf + _rb.wptr), (void *)(_dbuf + _dptr), CSIZE);
      _dptr += CSIZE;
    }

    _rb.wptr += CSIZE;
   
    used = _rb.wptr - _rb.rptr; 
  }
  
  return 0;  
}
