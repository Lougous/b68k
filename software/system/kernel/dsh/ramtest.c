
#include <stdio.h>

extern int readx(char *str, unsigned int *val);

int ramtest (int argc, char *argv[]) {
  unsigned int start, end;
  volatile unsigned int *a;

#define TEST_ERROR(msg) \
  { printf("error during %s test\n  address: %x\n", msg, (unsigned int)a); }

  if (argc != 3 ||
      readx(argv[1], &start) == 0 ||
      readx(argv[2], &end) == 0) {
    printf("usage: ramtest start_ad end_ad\n");
    return 0;
  }

  /* sliding bit test */

  /* 0x55555555 pattern */
  for (a = (unsigned int *)start; (unsigned int) a < end; a ++) {
    *a = 0x55555555;
  }

  for (a = (unsigned int *)start; (unsigned int) a < end; a ++) {
    if (*a != 0x55555555) TEST_ERROR("0x55555555 pattern");
  }

  /* 0xaaaaaaaa pattern */
  for (a = (unsigned int *)start; (unsigned int) a < end; a ++) {
    *a = 0xaaaaaaaa;
  }

  for (a = (unsigned int *)start; (unsigned int) a < end; a ++) {
    if (*a != 0xaaaaaaaa) TEST_ERROR("0xaaaaaaaa pattern");
  }

  return 0;
}
