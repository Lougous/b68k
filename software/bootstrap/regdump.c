
#include "types.h"
#include "b68k.h"

extern void outs(char *msg);
extern void outx(unsigned int x, int to_pad);

struct regs {
  // CPU registers
  // !!! coherency with boot_sector.s !!!
  // and with movem instruction
  u32_t d[8];
  u32_t a[8];
  u32_t pc;    // 64
  u16_t sr;    // 68

  // interrupt context (bus/address error)
  u16_t x_status;  // 70
  u32_t x_addr;    // 72
  u16_t x_instr;   // 76
};

u32_t g_tmp;
struct regs g_regs;

void reg_dump (void)
{
  u16_t i;
  
  for (i = 0; i < 8; i++) {
    outs(" d");outx(i,0);outs("=");outx(g_regs.d[i],8);outs("h  a");outx(i,0);outs("=");outx(g_regs.a[i],8);outs("\n");
  }
  outs(" pc=");outx(g_regs.pc,8);outs("h  sr=");outx(g_regs.sr,8);outs("\n");
  outs("stack frame extension:\n");
  outs(" status      ");outx(g_regs.x_status,8);outs("\n");
  outs(" address     ");outx(g_regs.x_addr,8);outs("\n");
  outs(" instruction ");outx(g_regs.x_instr,8);outs("\n");
  
  while(1);
}
