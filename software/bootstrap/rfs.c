
#include "types.h"
#include "b68k.h"

extern void outs(char *msg);
extern void outx(unsigned int x, int to_pad);

////////////////////////////////////////////////////////////////////////////////
// send/receive data through serial on multi-IO board
////////////////////////////////////////////////////////////////////////////////

u32_t _size;
u32_t _lseek;

static void _send_char (unsigned char c)
{
  B68K_IO->ad = B68K_IO_REG_UART_STS;

  /* wait for TX ready */
  while (! (B68K_IO->dt & 0x80));    

  B68K_IO->ad = B68K_IO_REG_UART_DATA;
  B68K_IO->dt = c;

  return;
}

static void _send(u8_t *msg, u16_t len)
{
  while (len--) _send_char(*msg++);
}

static void _receive(u8_t *msg, u16_t len)
{
  while (len--) {
    // wait until a char is received
    B68K_IO->ad = B68K_IO_REG_IRQSTS;

    while ((B68K_IO->dt & B68K_IO_IRQ_SERIAL_MASK) == 0);  // until UART receive flag rises

    // read char from UART FIFO
    B68K_IO->ad = B68K_IO_REG_UART_DATA;
    *msg++ = B68K_IO->dt;
  }
}

/* RFS protocol message integrity checker */
static u8_t _check_msg(u8_t *msg)
{
  if (*msg++ != 0x55) return 0;
  u8_t len = *msg++;
  u8_t sum = 0;

  while (len--) {
    sum += *msg++;
  }

  if (sum != *msg++) return 0;
  if (*msg++ != 0xAA) return 0;

  return 1;
}
  
u8_t _buf[256];

int rfs_init(void)
{
  outs("rfs: probing connection ...\n");
  
  // send reset command
  _buf[0] = 0x55;
  _buf[1] = 1;  // len
  _buf[2] = 'I';
  _buf[3] = 'I';  // checksum
  _buf[4] = 0xAA;

  _send(_buf, 5);

  // get reset command return
  _receive(_buf, 5);
  
  if (_check_msg(_buf)) {
    outs("rfs: connected\n");
    return 0;
  } else {
    outs("rfs: failed to connect\n");
    return -1;
  }
}

static u8_t _sum_name(u8_t *name)
{
  u8_t sum = 0;

  while (*name) {
    sum += *name++;
  }

  return sum;
}

char *_strcpy(char *dest, const char *src) {
  char *ret = dest;
  
  while (*src) {
    *dest++ = *src++;
  }

  // copy terminating null byte
  *dest = 0;

  return ret;
}

#define _FLAGS 1  // RDONLY

int rfs_open(const char *pathname, u8_t len)
{
  outs("rfs: opening ");
  outs((char *)pathname);
  outs(" ...\n");
  
  // send open command
  _buf[0] = 0x55;
  _buf[1] = 2 + len;      // len
  _buf[2] = 'O';          // open command
  _buf[3] = _FLAGS;       // flags
  (void)_strcpy((char *)&_buf[4], pathname);
  _buf[4+len] = 'O' + _FLAGS + _sum_name((u8_t *)pathname);  // checksum
  _buf[5+len] = 0xAA;
  
  _send(_buf, 6+len);

  // get open command return
  _receive(_buf, 10);

  // answer
  // 0: 55h
  // 1: len=6 
  // 2: status 1=OK
  // 3: fd (if OK)
  // 4: file size (LSB)
  // 5: file size
  // 6: file size
  // 7: file size (MSB)
  // 8: checksum
  // 9: AAh
  if (_check_msg(_buf) && _buf[2] == 1) {
    int fd = _buf[3];
    _size = 
      ((u32_t)_buf[4]) +
      (((u32_t)_buf[5]) << 8) +
      (((u32_t)_buf[6]) << 16) +
      (((u32_t)_buf[7]) << 24);
    _lseek = 0;
    
    outs("rfs: OK, size ");
    outx(_size, 0);
    outs("h\n");
   
    return fd;
  }
  
  /* couldn't open file */
  outs("rfs: failed to open\n");

  return -1;  
}

static u8_t _sum(u8_t *buf, u16_t len)
{
  u8_t sum = 0;

  while (len--) {
    sum += *buf++;
  }

  return sum;
}

u32_t rfs_read(int fd, void *buf, u32_t count)
{
  u8_t *dst = (u8_t *)buf;
  u32_t done = 0;

  outs("rfs: reading ...\n");

  while (done < count) {
    u8_t len = (count - done) > 64 ? 64 : count - done;
    
    // send read command
    _buf[0] = 0x55;
    _buf[1] = 7;
    _buf[2] = 'R';
    _buf[3] = (u8_t)fd;
    _buf[4] = len;
    _buf[5] = (unsigned char)(_lseek & 0xff);
    _buf[6] = (unsigned char)((_lseek >> 8) & 0xff);
    _buf[7] = (unsigned char)((_lseek >> 16) & 0xff);
    _buf[8] = (unsigned char)((_lseek >> 24) & 0xff);
    _buf[9] = _sum(_buf + 2, 7);  // checksum
    _buf[10] = 0xAA;
  
    _send(_buf, 11);

    // get open command return
    _receive(_buf, 5+len);

    // answer
    // 0: 55h
    // 1: len
    // 2: actually used length in buffer, 0xFF if access failed
    // 3+: buf[count]
    // : checksum
    // : AAh

    char blen = _buf[2];
    
    if (_check_msg(_buf) && blen >= 0) {
      // valid frame
      if (blen == 0) {
	// end of file
	break;
      }
      
      u8_t *src = &_buf[3];

      _lseek += blen;
      
      while (blen--) {
	*dst++ = *src++;
	done++;
      }
    } else {
      outs("rfs: warning: bad frame\n");
      break;
    }
  }
  
  return done;
}



