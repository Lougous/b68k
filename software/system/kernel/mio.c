// multi-io board driver
#include <fcntl.h>
#include <stddef.h>
#include <types.h>
#include <syscall.h>

#include "mem.h"
#include "proc.h"
#include "msg.h"
#include "config.h"
#include "b68k.h"
#include "debug.h"
#include "lock.h"
#include "dev.h"

extern u32_t clock_now();  // TODO header

#define _K_MIO_SERIAL_TIMEOUT_MS 500
  
static size_t _read (dev_read_msg_body_t *body);
static size_t _write (dev_write_msg_body_t *body);
static u32_t _open (dev_open_msg_body_t *body);
static u16_t _spi_dev_probe ();
void __attribute__ ((interrupt)) mio_interrupt (void);

#define SD_CARD_TYPE_NONE    0
#define SD_CARD_TYPE_V1      1
#define SD_CARD_TYPE_V2      2
#define SD_CARD_TYPE_V2HC    6

struct mio_drive_t {
  u16_t card_type;
};

struct mio_serial_t {
  u8_t rbin[256];
  u8_t wptr;
  u8_t rptr;
};
  
struct mio_i2c_t {
  u16_t dummy;
};
  
struct mio_drive_t _drive_a;
struct mio_drive_t _drive_b;
struct mio_serial_t _serial;
struct mio_i2c_t _i2c;

// serial input ring buffer

//struct mio_drive_t *_current_drive;

static u8_t _mbr[512];

static void _disk_probe(struct mio_drive_t *pdrive, char *name)
{
  if (! _spi_dev_probe(pdrive)) return;

  dev_open_msg_body_t omsg = { .handle = pdrive };
  
  _open(&omsg);

  dev_read_msg_body_t rmsg = { .src_seek = 0, .dst = _mbr, .count = 512, .handle = pdrive };
  
  if (_read(&rmsg) != 512) return;
  
  dev_register_disk(name, proc_current(), (void *)pdrive, _mbr);

  // TODO: close
}

void mio_task (void)
{
  K_PRINTF(2, "MIO: IO memory %Xh\n", B68K_IO_ADDRESS);

  // SPI SD card disks
  _disk_probe(&_drive_a, "sda");
  _disk_probe(&_drive_b, "sdb");

  // serial port
  dev_register_char("serial", proc_current(), &_serial);
  dev_register_char("i2c", proc_current(), &_i2c);

  // setup RX buffer
  _serial.wptr = 0;
  _serial.rptr = 0;

  // register interrupts
  B68K_IRQ_VECTOR[B68K_MIO_IRQ] = (u32_t)mio_interrupt;

  // enable interrupt
  B68K_IO->ad = B68K_IO_REG_IRQCFG;
  B68K_IO->dt =
    B68K_IO_IRQ_SERIAL_MASK;

  static message_t msg_out;

  // not used but worth to set
  //  msg_out.src = proc_current();
  msg_out.type = 0;
 
  while (1) {
    static message_t msg;

    pid_t from = receive(ANY, &msg);

    if (proc_get_uid(from) == PROC_UID_KERNEL) {    
      switch (msg.type) {
      case DEV_READ:
	msg_out.body.s32 = _read(&msg.body.dev_read);
	send(from, &msg_out);
	break;
	
      case DEV_WRITE:
	msg_out.body.s32 = _write(&msg.body.dev_write);
	send(from, &msg_out);
	break;
	
      case DEV_OPEN:
	msg_out.body.u32 = _open(&msg.body.dev_open);
	send(from, &msg_out);
	break;

      case DEV_CLOSE:
	msg_out.body.u32 = 0;
	send(from, &msg_out);
	break;

      default:
	K_PRINTF(2, "mio: error: unknown command %Xh\n", msg.type);
	break;
      }
    }
    
  }
}

void __attribute__ ((interrupt)) mio_interrupt (void)
{
  // for serial input only, for now
  u16_t lbkp = k_lock();

  u16_t io_ad_save = B68K_IO->ad;

  while (1) {
    B68K_IO->ad = B68K_IO_REG_UART_DATA;

    // characters will be discarded when the buffer is full
    if ((_serial.wptr + 1) != _serial.rptr) {
      _serial.rbin[_serial.wptr++] = B68K_IO->dt;
    }

    B68K_IO->ad = B68K_IO_REG_IRQSTS;

    if ((B68K_IO->dt & B68K_IO_IRQ_SERIAL_MASK) == 0) {
      // UART FIFO is empty
      break;
    }
  }

  B68K_IO->ad = io_ad_save;

  k_unlock(lbkp);
}

#define SECTOR_SIZE      512

#define SD_R1                1
#define SD_R7                7

#define SD_NO_ERROR          0
#define SD_NO_RESPONSE       -1
#define SD_ILLEGAL_COMMAND   -2

#define SD_R1_RETRY          100
#define SD_TOKEN_RETRY       1000
#define SD_INIT_RETRY        1000

static void _spi_select(u16_t spi_dev)
{
  B68K_IO->ad = B68K_IO_REG_SPI_SEL;
  B68K_IO->dt = spi_dev;
}

static void _spi_deselect()
{
  B68K_IO->ad = B68K_IO_REG_SPI_SEL;
  B68K_IO->dt = B68K_IO_SPI_SEL_NONE;

  // padding between commands
  //  B68K_IO->CMDDT = CMD_CHIP_SELECT | 1;  // deassert chip select
  //  B68K_IO->CMDDT = CMD_CHIP_SELECT | 1;  // deassert chip select
  //  B68K_IO->CMDDT = CMD_CHIP_SELECT | 1;  // deassert chip select
  //  B68K_IO->CMDDT = CMD_CHIP_SELECT | 1;  // deassert chip select
}

static void _spi_write_mode ()
{
  B68K_IO->ad = B68K_IO_REG_SPI_CTRL;
  B68K_IO->dt = B68K_IO_SPI_CLRBF_MASK | B68K_IO_SPI_CLRBF_TXR ;
}

static void _spi_send_byte (u8_t c)
{
  B68K_IO->ad = B68K_IO_REG_SPI_CTRL;
    
  // wait for read command to end
  while (B68K_IO->dt & B68K_IO_SPI_BUSY_MASK);
    
  B68K_IO->ad = B68K_IO_REG_SPI_DATA;
  B68K_IO->dt    = c;
}

static void _spi_sync (void)
{
  B68K_IO->ad = B68K_IO_REG_SPI_CTRL;
    
  // wait for read command to end
  while (B68K_IO->dt & B68K_IO_SPI_BUSY_MASK);
}

static void _spi_read_mode ()
{
  B68K_IO->ad = B68K_IO_REG_SPI_CTRL;
  B68K_IO->dt = B68K_IO_SPI_CLRBF_MASK | B68K_IO_SPI_CLRBF_TXR | B68K_IO_SPI_CLRBF_RXR;
}

static u8_t _spi_read_byte ()
{
  B68K_IO->ad = B68K_IO_REG_SPI_DATA;
  B68K_IO->dt    = 0xff;
   
  B68K_IO->ad = B68K_IO_REG_SPI_CTRL;
    
  // wait for read command to end
  while (B68K_IO->dt & B68K_IO_SPI_BUSY_MASK);
    
  // get read byte
  B68K_IO->ad = B68K_IO_REG_SPI_DATA;
  return B68K_IO->dt & 0xff;
}


static u32_t _spi_sd_resp_r1 ()
{
  u16_t retry = SD_R1_RETRY;
  u16_t resp;

  _spi_sync();
  _spi_read_mode();

  while (retry) {
    resp = _spi_read_byte();
    
    // check valid bit
    if (!(resp & 0x80)) {
      // good !
      return resp;
    }

    retry--;
  }

  // timeout
  return 0xff;
}

static u32_t _spi_sd_resp_extend ()
{
  u32_t resp;
  u8_t i;
  
  resp = 0;
    
  for (i = 0; i < 4; i++) {
    // get read byte
    resp = (resp << 8) | (u32_t)_spi_read_byte();
  }

  return resp;
}

static u32_t crc7 (u32_t crc7, u32_t bits, u32_t count) {

  while (count--) {
    u32_t crc;
    //unsigned int b6 = (((_crc7_val >> 0) ^ (bits >> count)) & 1);
    u32_t b0 = (((crc7 >> 6) ^ (bits >> count)) & 1);
    
    crc =
      //     (b6 << 6) |   // di^0->6xs
      b0 |
      //      ((((_crc7_val >> 4) ^ b6) & 1) << 3) |  // di^0^4->3
      ((((crc7 >> 2) ^ b0) & 1) << 3) |  // di^0^2->3          
      //      ((_crc7_val >> 1) & 0b0110111);   // 1->0, 2->1, 3->2, 5->4, 6->5
      ((crc7 << 1) & 0b1110110);   // 0->1, 1->2, 3->4, 4->5, 5->6 

    crc7 = crc;
  }

  return crc7;
}

static u32_t _spi_sd_command (u16_t spi_dev,
			      u8_t cmd, u32_t arg,
			      u8_t rtype,
			      u8_t *resp_head,
			      u32_t *resp_ext)
{
  //0: 0x95,
  //8: 0x87
  //58: 0xFD,
  u32_t crc = crc7(0, 0x40 + cmd, 8);
  crc = crc7(crc, arg, 32) * 2 + 1;

  _spi_select(spi_dev);

  // TX only
  _spi_write_mode();

  _spi_send_byte(0xff);         // needed for old cards ...
  _spi_send_byte(0x40 + cmd);   // CMDxx
  _spi_send_byte(arg >> 24);
  _spi_send_byte(arg >> 16);
  _spi_send_byte(arg >> 8);
  _spi_send_byte(arg);
  _spi_send_byte(crc);          // CRC7

  *resp_head = (u8_t) _spi_sd_resp_r1();

  if (*resp_head == 0xff) {
    K_PRINTF(3, "SD: CMD%u got no response\n", cmd);
    _spi_deselect();
    return SD_NO_RESPONSE;
  }

  if (*resp_head & 0x4) {
    K_PRINTF(3, "SD: CMD%u is not supported (0x%x)\n", cmd, *resp_head);
    _spi_deselect();
    return SD_ILLEGAL_COMMAND;
  }
  
  if (rtype == SD_R1) {
    //K_PRINTF(3, "SD: CMD%d returned 0x%x\n", cmd, *resp_head);
    _spi_deselect();
   return SD_NO_ERROR;
  }

  // extended response
  *resp_ext = _spi_sd_resp_extend();
  _spi_deselect();

  //  K_PRINTF(3, "SD: CMD%d returned 0x%x-0x%x\n", cmd, *resp_head, *resp_ext);

  return SD_NO_ERROR;
}
  
// wait for read command to end
//while (B68K_IO->CTRL_STS & (BUSY_FLAG | READ_EMPTY_FLAG));

static u16_t _spi_sd_read_sector (struct mio_drive_t * pdrive,
				  u32_t sector, u8_t *dst)
{
  u16_t retry = SD_TOKEN_RETRY;
  u16_t resp;
  u32_t i;
  u32_t arg = pdrive->card_type & SD_CARD_TYPE_V2HC ? sector : sector * SECTOR_SIZE;
  u32_t crc = crc7(0, 0x40 + 17, 8);
  crc = crc7(crc, arg, 32) * 2 + 1;

  if (pdrive->card_type == SD_CARD_TYPE_NONE) {
    return -1;
  }

  // CMD17
  _spi_select(pdrive == &_drive_a ?
    B68K_IO_SPI_SEL_SDA : B68K_IO_SPI_SEL_SDB);

  _spi_write_mode();

  _spi_send_byte(0xff);         // needed for old cards ...
  _spi_send_byte(0x40 + 17);    // CMD17
  _spi_send_byte(arg >> 24);
  _spi_send_byte(arg >> 16);
  _spi_send_byte(arg >> 8);
  _spi_send_byte(arg);
  _spi_send_byte(crc);           // CRC7

  // R1
  resp = _spi_sd_resp_r1();

  if (resp & 0x7C) {
    // command not supported/error
    K_PRINTF(3, "SD: CMD17 not supported (0x%x)\n", resp);

    _spi_deselect();
    return -2;
  }

  //B68K_IO->ad = B68K_IO_REG_SPI_CTRL;
  //B68K_IO->dt = B68K_IO_SPI_CLRBF_MASK | B68K_IO_SPI_CLRBF_TXR | B68K_IO_SPI_CLRBF_RXR;

  // token
  while (retry) {
    
    resp = _spi_read_byte();
    
    // check valid bit
    if (!(resp & 0x01)) {
      // good !
      break;
    }

    retry--;
  }

  if (retry == 0) {
    // no token / timeout
    _spi_deselect();
    K_PRINTF(3, "SD: sector read ended abnormally\n");
    return -1;
  }

  // data + CRC
  //while (B68K_IO->CTRL_STS & (BUSY_FLAG));
  
  for (i = 0; i < SECTOR_SIZE; i++) {
    *dst++ = _spi_read_byte();
  }

  _spi_deselect();

  return 0;
}

static u16_t _spi_dev_probe (struct mio_drive_t * pdrive)
{
  u16_t i;
  u8_t resp;
  u32_t resp_ext;
  u16_t retry;
  u16_t status;
  u16_t spi_dev = pdrive == &_drive_a ?
    B68K_IO_SPI_SEL_SDA : B68K_IO_SPI_SEL_SDB;

  pdrive->card_type = SD_CARD_TYPE_NONE;
  
  // dummy clock cycles, 74+ with chip select deasserted
  _spi_deselect();

  B68K_IO->ad = B68K_IO_REG_SPI_CTRL;
  B68K_IO->dt    = B68K_IO_SPI_CLRBF_MASK | B68K_IO_SPI_CLRBF_TXR;

  B68K_IO->ad = B68K_IO_REG_SPI_DATA;
  B68K_IO->dt    = 0xff;

  for (i = 0; i < 9; i++) {
    B68K_IO->ad = B68K_IO_REG_SPI_DATA;
    B68K_IO->dt = 0xff;

    B68K_IO->ad = B68K_IO_REG_SPI_CTRL;
    while (B68K_IO->dt & B68K_IO_SPI_BUSY_MASK);
  }

  // insert delay in the bus
  _spi_deselect();
  _spi_deselect();

  // CMD0
  status = _spi_sd_command(spi_dev, 0, 0, SD_R1, &resp, &resp_ext);
  
  if (status != SD_NO_ERROR) {
    K_PRINTF(3, "SD: cannot reset device (CMD0)\n");
    return 0;
  }

  K_PRINTF(3, "SD: device properly reset (CMD0)\n");
  
  // CMD8
  status = _spi_sd_command(spi_dev, 8, 0x000001AA, SD_R7, &resp, &resp_ext);

  // todo check return pattern
  
  if (status != SD_NO_ERROR || resp & 0x7C) {
    // CMD8 not supported, SD card v1 or MMC
    pdrive->card_type = SD_CARD_TYPE_V1;
    K_PRINTF(2, "SD: SD card V1 detected\n");
  } else {
    // CMD8 supported
    pdrive->card_type = SD_CARD_TYPE_V2;
    K_PRINTF(2, "SD: SD card V2+ detected\n");
  }
  
  // CMD58
  status = _spi_sd_command(spi_dev, 58, 0x00000000, SD_R7, &resp, &resp_ext);
  
  if (resp & 0x7C) {
    // CMD58 not supported / error
    K_PRINTF(3, "SD: CMD58 not supported (%x)\n", resp);
  } else {
    // CMD58 supported, read remaining 4 bytes (R3)
    K_PRINTF(3, "SD: OCR is 0x%x\n", resp_ext);
  }

  // poll ACMD41
  retry = SD_INIT_RETRY;

  while (retry) {
    // CMD55
    status = _spi_sd_command(spi_dev, 55, 0x00000000, SD_R1, &resp, &resp_ext);
 
    if (resp & 0x7C) {
      // CMD55 not supported / error
      K_PRINTF(3, "SD: CMD55 not supported (%x)\n", resp);
      return 0;
    }

    if (retry == SD_INIT_RETRY && pdrive->card_type == SD_CARD_TYPE_V2) {
      status = _spi_sd_command(spi_dev,
			       41,
			       0x40000000,
			       SD_R1, &resp, &resp_ext);
    } else {
      status = _spi_sd_command(spi_dev,
			       41,
			       0x00000000,
			       SD_R1, &resp, &resp_ext);
    }

    if (resp & 0x7C) {
      // CMD41 not supported / error
      K_PRINTF(3, "SD: CMD41 not supported (%x)\n", resp);
      return 0;
    }

    if ((resp & 0x01) == 0) break;  // IDLE == 0

    retry--;
  }

  if (!retry) {
    K_PRINTF(3, "SD: cannot initialize memory device\n");
    return 0;
  }

  K_PRINTF(3, "SD: memory device successfully initialized\n");

  return 1;
}

static u32_t _open (dev_open_msg_body_t *body)
{
  // pathname not used here for such block device
  //struct mio_drive_t * pdrive = (struct mio_drive_t *)body->handle;
  
  return 0;
}

static size_t _read (dev_read_msg_body_t *body)
{
  if ((struct mio_serial_t *)body->handle == &_serial) {
    u32_t count = body->count;
    size_t done = 0;
    u8_t *dst = (u8_t *)body->dst;

    u32_t st = clock_now();
    
    while (count) {
      while (_serial.rptr != _serial.wptr) {
	*dst++ = _serial.rbin[_serial.rptr++];
	done++;
	count--;
      }

      // not enough words in buffer yet
      if (clock_now() - st > _K_MIO_SERIAL_TIMEOUT_MS) break;

      // let's be fair while waiting for incoming chars
      proc_yield();
    }

    return done;

  } else if ((struct mio_i2c_t *)body->handle == &_i2c) {

    return -1;

  } else {
    // SPI
    struct mio_drive_t * pdrive = (struct mio_drive_t *)body->handle;
  
    u32_t status;
  
    if (body->count != SECTOR_SIZE) return -1;

    status = _spi_sd_read_sector (pdrive, body->src_seek / SECTOR_SIZE, (u8_t *)body->dst);

    if (!status) {
      return SECTOR_SIZE;
    }
  
    return -1;
  }
}

static void _serial_putchar (unsigned char c)
{
  B68K_IO->ad = B68K_IO_REG_UART_STS;

  /* wait for TX ready */
  while (! (B68K_IO->dt & 0x80));    

  B68K_IO->ad = B68K_IO_REG_UART_DATA;
  B68K_IO->dt = c;

  return;
}

static size_t _write (dev_write_msg_body_t *body)
{
  if ((struct mio_serial_t *)body->handle == &_serial) {
    int count = body->count;
    char *src = body->src;

    while (count--) {
      _serial_putchar(*src++);
    }

    return body->count;
    
  } else if ((struct mio_i2c_t *)body->handle == &_i2c) {

    return -1;

  } else {
    // SPI
    /*
    struct mio_drive_t * pdrive = (struct mio_drive_t *)body->handle;
  
    u32_t status;
  
    if (body->count != SECTOR_SIZE) return -1;

    status = _spi_sd_write_sector (pdrive, body->src_seek / SECTOR_SIZE, (u8_t *)body->dst);

    if (!status) {
      return SECTOR_SIZE;
    }
    */
    
    return -1;
  }
}

