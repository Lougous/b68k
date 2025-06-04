//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//

// to be replaced by fopen/fgetc implementation

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

char _buf[32];
int16_t _len;
uint8_t _rptr;

void _finit()
{
  _len = 0;
  _rptr = 0;
}

int16_t _fgetc(int fd)
{
  if (_len) {
    _len--;
    return _buf[_rptr++];
  } else {
    _len = read(fd, _buf, sizeof(_buf));

    if (_len <= 0) {
      return -1;
    }
    
    _len--;
    _rptr = 1;
    return _buf[0];
  }
}

