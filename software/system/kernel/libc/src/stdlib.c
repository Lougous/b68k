//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/kernel - kernel libc - stdlib
//

#include <stdlib.h>

int atoi(const char *nptr)
{
  int val = 0;
  char sign = 0;

  if (*nptr == '-') {
    sign = 1;
    nptr++;
  }


  while (*nptr) {
    if (*nptr >= '0' && *nptr <= '9') {
      val = val*10 + (int)(*nptr - '0');
      nptr++;
    } else {
      return 0;
    }
  }

  return sign ? -val : val;
}
