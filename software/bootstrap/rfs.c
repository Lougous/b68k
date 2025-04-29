
#include "types.h"
#include "b68k.h"

#include <sys/rfs.h>

extern void outs(char *msg);
extern void outx(unsigned int x, int to_pad);

////////////////////////////////////////////////////////////////////////////////
// send/receive data through serial on multi-IO board
////////////////////////////////////////////////////////////////////////////////

// FS data
rnode_t _root;

// file data
rnode_t _rnode;
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


int rfs_init(void)
{
  outs("rfs: probing connection ...\n");
  
  // send reset command
  rfs_creset_t creset = RFS_CRESET();
  _send((u8_t *)&creset, sizeof(creset));

  // get reset command return
  rfs_areset_t areset;
  _receive((u8_t *)&areset, sizeof(areset));
  
  if (_check_msg((u8_t *)&areset)) {
    _root = RFS_ARESET_ROOT(areset);
    outs("rfs: connected, root rnode ");
    outx(_root, 0);
    outs("h\n");
    return 0;
  } else {
    outs("rfs: failed to connect\n");
    return -1;
  }
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
#define _PID   0

u32_t _size;
u32_t _lseek;

int rfs_open(const char *pathname)
{
  // lookup
  outs("rfs: lookup ");
  outs((char *)pathname);
  outs("\n");

  rfs_clookup_t clookup = RFS_CLOOKUP(_root, _PID, pathname);
  _strcpy(RFS_CLOOKUP_NAME(clookup), pathname);
  _send((u8_t *)&clookup, sizeof(clookup));

  // lookup answer
  rfs_alookup_t alookup;
  _receive((u8_t *)&alookup, sizeof(alookup));

  if (!_check_msg((u8_t *)&alookup)) {
    outs("rfs: error: invalid message\n");
    return -1;
  }  

  _rnode = RFS_ALOOKUP_RNODE(alookup);

  if (!_rnode) {
    outs("rfs: error: file not found\n");
    return -1;
  }  
    
  outs("rfs: rnode ");
  outx(_rnode, 0);
  outs("h\n");

  // open
  outs("rfs: open file\n");
  
  rfs_copen_t copen = RFS_COPEN(_rnode, _FLAGS, _PID);
  _send((u8_t *)&copen, sizeof(copen));

  // open answer
  rfs_aopen_t aopen;
  _receive((u8_t *)&aopen, sizeof(aopen));

  if (!_check_msg((u8_t *)&aopen)) {
    outs("rfs: error: invalid message\n");
    return -1;
  }  

  if (RFS_AOPEN_STATUS(aopen) != 1) {
    outs("rfs: error: bad status\n");
    return -1;
  }

  _size = RFS_AOPEN_SIZE(aopen);
  _lseek = 0;
  
  outs("rfs: OK, size ");
  outx(_size, 0);
  outs("h\n");
  
  return 1;  
}

u32_t rfs_read(void *buf, u32_t count)
{
  u8_t *dst = (u8_t *)buf;
  u32_t done = 0;

  outs("rfs: reading file\n");

  while (done < count) {
    u8_t len = (count - done) > 64 ? 64 : count - done;
    
    // send read command
    rfs_cread_t cread = RFS_CREAD(len, _rnode, _lseek);
    _send((u8_t *)&cread, sizeof(cread));
    
    // get read command return
    u8_t _buf[RFS_AREAD_LEN_NO_DATA+64];
    _receive(_buf, RFS_AREAD_LEN_NO_DATA+len);

    u8_t blen = RFS_AREAD_BLEN(buf);
    
    if (_check_msg(buf) && blen >= 0) {
      // valid frame
      if (blen == 0) {
	// end of file
	break;
      }
      
      u8_t *src = RFS_AREAD_BUF(buf);

      _lseek += blen;
      
      while (blen--) {
	*dst++ = *src++;
	done++;
      }
    } else {
      outs("rfs: warning: bad frame, aborting\n");
      break;
    }
  }
  
  return done;
}



