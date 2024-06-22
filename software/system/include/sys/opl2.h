
#ifndef _opl2_h_
#define _opl2_h_

typedef struct {
  u8_t wptr;
  u8_t rptr;
  u8_t buf[256];
} opl2_ring_buffer_t;

// IOCTL
#define OPL2_IOCTL_SETBUF  1
// one argument, opl2_ring_buffer_t *

#endif // _opl2_h_
