
#include <types.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>

#include "b68k.h"
#include "mem.h"

/* TODO: use connexion to mio driver ! */

int com_putchar (int c)
{
  B68K_IO->ad = B68K_IO_REG_UART_STS;

  /* wait for TX ready */
  while (! (B68K_IO->dt & 0x80));    

  B68K_IO->ad = B68K_IO_REG_UART_DATA;
  B68K_IO->dt = (unsigned char) c;

  return c;
}

int com_getchar(void)
{
  while (1) {
    B68K_IO->ad = B68K_IO_REG_IRQSTS;

    if (B68K_IO->dt & 1) {
      // RX FIFO not empty
      B68K_IO->ad = B68K_IO_REG_UART_DATA;

      // need to mask out MSB (read void on the bus ...)
      return B68K_IO->dt & 0xFF;
    }
  }

  return 0;
}

extern void copy_to_user(pid_t pid, mem_va_t dst, mem_pa_t src, int len);

int serial_bootloader(pid_t pid)
{
  int cmd, last_cmd = 0;
  int sum;
  int i;
  unsigned int addr_u = 0x100000;
  char *address = (char *)addr_u;
  u8_t buf[64];
  
  printf("\n<=> default address is %Xh", addr_u);

  while (1) {
    cmd = com_getchar();

    switch (cmd) {
    case 'A':
      /* set address (3 bytes, MSB first) */
      addr_u = com_getchar() & 0xff;
      addr_u = (addr_u << 8) | (com_getchar() & 0xff);
      addr_u = (addr_u << 8) | (com_getchar() & 0xff);
      com_putchar('K');

      printf("\n<=> address is now %Xh", addr_u);
      address = (char *)(addr_u);
      break;
	
    case 'W':
      /* write packet */
      sum = 0;
      
      for (i = 0; i < 64; i++) {
	int c = com_getchar() & 0xff;
	sum += c;
	buf[i] = c;
      }

      copy_to_user(pid, (mem_va_t)address, (mem_pa_t)buf, 64);
      address += 64;

      com_putchar(sum);
      
      if (last_cmd != 'W') printf("\n<=> ");
      printf(".");
      break;
	
    case 'E':
      /* echo */
      i = com_getchar() & 0xff;
      com_putchar(i);
      printf("\n<=> echo: %u", i);
      break;

    case 'Q':
      /* quit */
      printf("\n");
      return 0;
      break;

    case 'B':
      /* boot/run */
      printf("\n");
      return 1;
      break;

    default:
      break;
    }

    last_cmd = cmd;
  }
}
  

int load (int argc, char *argv[]) {
  return serial_bootloader(0);
}
  
