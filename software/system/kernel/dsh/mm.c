#include <stdio.h>
#include <string.h>

extern int readx(char *str, unsigned int *val);

static int _usage(void)
{
  printf("usage: mm [-h|-l] address value\n");
  printf("  options:\n");
  printf("    -h   write half words\n");
  printf("    -l   write words\n");
  return 0;
}

int mm (int argc, char *argv[]) {
  unsigned int addr, value;
  int wsize = 1;
  int has_address = 0;
  int has_value = 0;

  while (argc > 1) {
    if (argv[1] && argv[1][0] == '-') {
      if (strcmp(argv[1] + 1, "h") == 0) {
	wsize = 2;
      } else if (strcmp(argv[1] + 1, "l") == 0) {
	wsize = 4;
      } else {
	// unknown switch
	return _usage();
      }
    } else if (has_address == 0) {
      if (readx(argv[1], &addr) == 0) {
	// error while reading address
	return _usage();
      }

      has_address = 1;
    } else if (has_value == 0) {
      if (readx(argv[1], &value) == 0) {
	// error while reading value
	return _usage();
      }

      has_value = 1;
    } else {
      // too much arguments
      return _usage();
    }

    // next argument
    argc--;
    argv++;
  }

  if (has_address == 0) return _usage();
  if (has_value == 0) return _usage();
  
  if (wsize == 1) {
    *((unsigned char *)addr) = value;
  } else if (wsize == 2) {
    *((unsigned short *)addr) = value;
  } else {
    *((unsigned int *)addr) = value;
  }
  
  return 0;
}
