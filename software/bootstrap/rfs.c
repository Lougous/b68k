
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

static void _send_char (u8_t c)
{
  B68K_IO->ad = B68K_IO_REG_UART_STS;

  /* wait for TX ready */
  while (! (B68K_IO->dt & 0x80));    

  B68K_IO->ad = B68K_IO_REG_UART_DATA;
  B68K_IO->dt = c;

  return;
}

static void _send_msg(u8_t *msg, u16_t len)
{
  _send_char(0x55);
  _send_char(len);

  u8_t csum = 0;
  
  while (len--) {
    csum += *msg;
    _send_char(*msg++);
  }

  _send_char(csum);
  _send_char(0xAA);
}

static u8_t _receive_char (void)
{
  // wait until a char is received
  B68K_IO->ad = B68K_IO_REG_IRQSTS;

  while ((B68K_IO->dt & B68K_IO_IRQ_SERIAL_MASK) == 0);  // until UART receive flag rises

  // read char from UART FIFO
  B68K_IO->ad = B68K_IO_REG_UART_DATA;

  return B68K_IO->dt;
}

#define MAX_PAYLOAD 128

#define ANY_LEN  255

#define EBADHEAD    -1
#define ETOOLONG    -2
#define EBADLEN     -3
#define EBADCSUM    -4
#define EBADTAIL    -5

const char * const _error_msg[] = {
  [-EBADHEAD] = "bad header",
  [-ETOOLONG] = "length too long",
  [-EBADLEN]  = "bad length",
  [-EBADCSUM] = "bad checksum",
  [-EBADTAIL] = "bad tailer"
};

ssize_t _trace_error (ssize_t code)
{
  outs("rfs: receive error: ");
  outs((char *)_error_msg[-code]);
  outs("\n");

  return code;
}

static ssize_t _receive_msg(u8_t *msg, u8_t expected_len)
{
  enum fsm { F_SOF, F_LEN, F_PAYLOAD, F_CKS, F_EOF };

  enum fsm framer_fsm = F_SOF;

  u8_t len = 0;
  u8_t cnt;
  u8_t csum = 0;

  while (1) {

    switch (framer_fsm) {
    case F_SOF:
      if (_receive_char() != 0x55) return _trace_error(EBADHEAD);
      
      framer_fsm = F_LEN;
      break;

    case F_LEN:
      len = _receive_char();

      if (len > MAX_PAYLOAD) return _trace_error(ETOOLONG);

      if (expected_len != ANY_LEN && len != expected_len) return _trace_error(EBADLEN);
	
      framer_fsm = F_PAYLOAD;
      break;

    case F_PAYLOAD:
      cnt = len;
      
      while (cnt) {
	*msg = _receive_char();
	csum += *msg++;
	cnt--;
      }
      
      framer_fsm = F_CKS;
      break;

    case F_CKS:

      if (_receive_char() != csum) return _trace_error(EBADCSUM);
      
      framer_fsm = F_EOF;
      break;
	  
    case F_EOF:
      if (_receive_char() != 0xAA) return _trace_error(EBADTAIL);

      return len;

    default:
      break;
    }
  }
  
}

int rfs_init(void)
{
  u8_t buf[MAX_PAYLOAD];
  
  outs("rfs: probing connection ...\n");
  
  // send reset command
  buf[0] = 'I';
  _send_msg(buf, 1);

  // get reset command return
  if (_receive_msg(buf, RFS_PAYLOAD_LEN(sizeof(rfs_areset_t))) < 0) return -1;
 
  _root = rfs_read_u32le(buf + 1);
  outs("rfs: connected, root rnode ");
  outx(_root, 0);
  outs("h\n");
  
  return 0;
}

u8_t *_str_at(u8_t *pc, const char *src) {
  while (*src) {
    *pc++ = *src++;
  }

  // copy terminating null byte
  *pc++ = 0;

  // note: return value AFTER the copy
  return pc;
}

#define _FLAGS 1  // RDONLY
#define _PID   0

u32_t _size;
u32_t _lseek;

int rfs_open(const char *pathname)
{
  u8_t buf[MAX_PAYLOAD];
  
  // lookup
  outs("rfs: lookup ");
  outs((char *)pathname);
  outs("\n");

  u8_t *pc = buf;

  *pc++ = 'L';
  pc = rfs_u32le_at(pc, _root);
  *pc++ = _PID;
  pc = _str_at(pc, pathname);

  while ((pc - buf) < sizeof(rfs_clookup_t)) {
    *pc++ = 0;
  }

  _send_msg(buf, RFS_PAYLOAD_LEN(sizeof(rfs_clookup_t)));
  
  // lookup answer
  if (_receive_msg(buf, RFS_PAYLOAD_LEN(sizeof(rfs_alookup_t))) < 0) return -1;

  _rnode = rfs_read_u32le(buf+1);
  u8_t type = buf[5];

  if (type != RFS_TYPE_REG) {
    outs("rfs: error: not found or not regular file\n");
    return -1;
  }  
    
  outs("rfs: rnode ");
  outx(_rnode, 0);
  outs("h\n");

  // open
  outs("rfs: open file\n");

  pc = buf;

  *pc++ = 'O';
  pc = rfs_u32le_at(pc, _rnode);
  *pc++ = _FLAGS;
  *pc++ = _PID;
  
  _send_msg(buf, RFS_PAYLOAD_LEN(sizeof(rfs_copen_t)));
  
  // open answer
  if (_receive_msg(buf, RFS_PAYLOAD_LEN(sizeof(rfs_aopen_t))) < 0) return -1;

  if (buf[1] != 1) {
    outs("rfs: error: bad open status\n");
    return -1;
  }

  _size = rfs_read_u32le(buf+2);
  _lseek = 0;
  
  outs("rfs: OK, size ");
  outx(_size, 0);
  outs("h\n");
  
  return 1;  
}

u32_t rfs_read(void *to, u32_t count)
{
  u8_t buf[MAX_PAYLOAD];
  
  u8_t *dst = (u8_t *)to;
  u32_t done = 0;

  u32_t done_4k = 0;

  outs("rfs: reading file ");

  while (done < count) {
    u8_t len = (count - done) > RFS_MAX_READ ? RFS_MAX_READ : count - done;
    
    // send read command
    u8_t cread[RFS_PAYLOAD_LEN(sizeof(rfs_cread_t))];
    u8_t *pc = cread;

    *pc++ = 'R';
    *pc++ = len;
    pc = rfs_u32le_at(pc, _rnode);
    pc = rfs_u32le_at(pc, _lseek);

    _send_msg(cread, sizeof(cread));
  
    // get read command return
    ssize_t pl_len = _receive_msg(buf, ANY_LEN);

    if (pl_len <= 0) break;

    if (buf[1] != RFS_AREAD_OK) {
      outs("\nrfs: error: read KO\n");
      break;
    }

    u8_t blen = pl_len - 2;  // - CID and status byte
    
    u8_t *src = buf + 2;

    _lseek += blen;
      
    while (blen--) {
      *dst++ = *src++;
      done++;
    }

    while ((done >> 12) > done_4k) {
      outs(".");
      done_4k++;
    }
  }

  outs("\nrfs: read bytes: ");
  outx(done, 0);
  outs("h\n");
  
  return done;
}



