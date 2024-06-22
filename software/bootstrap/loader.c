
#include "types.h"
#include "b68k.h"

extern int rfs_init(void);
extern int rfs_open(const char *pathname, u8_t len);
extern u32_t rfs_read(int fd, void *buf, u32_t count);
extern void outs(char *msg);
extern void outx(unsigned int x, int to_pad);

extern u32_t _size;

void loader(void)
{
  B68K_MFP->ad = B68K_MFP_REG_POST;
  B68K_MFP->dt = 0xB0;

  //  _puts("hello!\n");

  // enable interrupt so that interrupt flag will be set when some character arrives
  // (interrupt is masked in 68k SR register, so polling on this bit is used)
  B68K_IO->ad = B68K_IO_REG_IRQCFG;
  B68K_IO->dt = B68K_IO_IRQ_SERIAL_MASK;

  int fd;
  
  if (rfs_init() != 0) return;
  if ((fd = rfs_open("system.bin", 10)) < 0) return;
  if (rfs_read(fd, (u8_t *)0, _size) != _size) {
    outs("failed to load file\n");
    return;
  }
  
  outs("load complete, jump at \n");
  typedef void (*pfct_t)(void);
  pfct_t start_address = (pfct_t)*((u32_t *)0x4); 
  outx((u32_t)start_address, 6);
  outs("h\n");

  // set B68K_MFP_SYS_RAM_BOOT flag
  B68K_IO->ad = B68K_MFP_REG_SYSCFG;
  B68K_IO->dt = B68K_MFP_SYS_RAM_BOOT;
  
  start_address();
  
  return;
}
