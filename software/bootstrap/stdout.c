
#include "types.h"
#include "b68k.h"

////////////////////////////////////////////////////////////////////////////////
// display info on MFP serial
////////////////////////////////////////////////////////////////////////////////
static int _serial_putchar (int c)
{
  B68K_MFP->ad = B68K_MFP_REG_UART_STS;

  /* wait for TX ready */
  while (! (B68K_MFP->dt & 0x80));    

  B68K_MFP->ad = B68K_MFP_REG_UART_DATA;
  B68K_MFP->dt = (unsigned char) c;

  return c;
}

void outs(char *msg)
{
  while (*msg) _serial_putchar(*msg++);
}

void outx(unsigned int x, int to_pad)
{
  if (x & ~15 || to_pad > 0) outx(x >> 4, to_pad-1);
  
  x = x & 15;

  if (x >= 10) {
    _serial_putchar('A' - 10 + x);
  } else {
    _serial_putchar(((int)'0') + x);
  }
}

