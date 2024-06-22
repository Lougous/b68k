#include <stdio.h>
#include <string.h>

extern int readx(char *str, unsigned int *val);

static int _usage(void)
{
  printf("usage: md [-h|-l] base_ad [len]\n");
  printf("  options:\n");
  printf("    -h   read half words\n");
  printf("    -l   read words\n");
  return 0;
}

int md (int argc, char *argv[]) {
  static unsigned char cbuf[16]; 
  unsigned int base, len = 64, i, c;
  int wsize = 1;
  int has_address = 0;
  int has_len = 0;

  /* arguments */
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
      if (readx(argv[1], &base) == 0) {
	// error while reading address
	return _usage();
      }

      has_address = 1;
    } else if (has_len == 0) {
      if (readx(argv[1], &len) == 0) {
	// error while reading length
	return _usage();
      }

      has_len = 1;
    } else {
      // too much arguments
      return _usage();
    }

    // next argument
    argc--;
    argv++;
  }

  if (has_address == 0) return _usage();
  
  if (wsize == 1) {
    volatile unsigned char *a;
    a = (unsigned char *)base;

    /* align */
    int start = (unsigned int)a & 15;
    int lstart = start;

    for (i = start; i < (len+start); i++, a++) {
      cbuf[i & 15] = *a;

      if (((i & 15) == 15) || (i == (len+start-1))) {
	/* end of line */
	printf("%08x   ", (unsigned int)a & 0xfffffff0);

	for (c = 0; c < 16; c++) {
	  if ((c < lstart) || (c > (i & 15))) {
	    printf("   ");
	  } else {
	    printf("%02x ", cbuf[c]);
	  }
	}

	printf("   ");
      
	for (c = 0; c < 16; c++) {
	  if ((c < lstart) || (c > (i & 15))) {
	    printf(" ");
	  } else {
	    if (cbuf[c] >= 32) {
	      printf("%c", cbuf[c]);
	    } else {
	      printf(".");
	    }
	  }
	}

	putchar('\n');
	lstart = 0;
      }
    }
  } else if (wsize == 2) {
    volatile unsigned short *a = (unsigned short *)base;

    /* align */
    int start = (unsigned int)a & 15;
    int lstart = start;

    for (i = start; i < (len+start); i += 2) {

      if (((i & 15) == 14) || (i == (len+start-2))) {
	/* end of line */
	printf("%08x   ", (unsigned int)a & 0xfffffff0);

	for (c = 0; c < 16; c += 2) {
	  if ((c < lstart) || (c > (i & 14))) {
	    printf("     ");
	  } else {
	    printf("%04x ", *a);
	    a++;
	  }

	}

	putchar('\n');
	lstart = 0;
      }
    }
  } else {
    // 32-bits
    volatile unsigned int *a;
    a = (unsigned int *)base;

    /* align */
    int start = (unsigned int)a & 15;
    int lstart = start;

    for (i = start; i < (len+start); i += 4) {

      if (((i & 15) == 12) || (i == (len+start-4))) {
	/* end of line */
	printf("%08x   ", (unsigned int)a & 0xfffffff0);

	for (c = 0; c < 16; c += 4) {
	  if ((c < lstart) || (c > (i & 12))) {
	    printf("         ");
	  } else {
	    printf("%08x ", *a);
	    a++;
	  }

	}

	putchar('\n');
	lstart = 0;
      }
    }
  }
  
  return 0;
}

