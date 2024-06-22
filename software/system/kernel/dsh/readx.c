int readx(char *str, unsigned int *val) {
  unsigned int n = 0;

  while(*str) {
    if (*str >= '0' && *str <= '9') {
      n = (n << 4) + (unsigned int)(*str - '0');
    } else if (*str >= 'a' && *str <= 'f') {
      n = (n << 4) + (unsigned int)(*str - 'a' + 10);
    } else if (*str >= 'A' && *str <= 'F') {
      n = (n << 4) + (unsigned int)(*str - 'A' + 10);
    } else {
      /* invalid char */
      *val = 0;
      return 0;
    }

    str++;
  }

  *val = n;
  return 1;
}
