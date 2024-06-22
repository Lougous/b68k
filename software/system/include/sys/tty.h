
#ifndef _tty_h_
#define _tty_h_

typedef struct {
  u8_t buf[16];  // 16x8 bits => 128 keys
} tty_ioctl_kbd_pos_t;

// IOCTL
#define TTY_IOCTL_KBD_POS    1
// one argument, tty_ioctl_kbd_pos_t *

#define TTY_IOCTL_SET_FLAGS  2
// one argument, u32_t ORing of flags below:
#define TTY_FLAG_ECHO        0x0001
#define TTY_FLAG_CURSOR      0x0002


#endif // _tty_h_
