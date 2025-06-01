//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//

#ifndef _sys_macros_h_
#define _sys_macros_h_

// static dev_t makedev(u16_t maj, u16_t min)
#define makedev(maj, min)  (((maj) << 8) + (min))

// static u16_t major(dev_t dev)
#define major(dev)  ((dev) >> 8)

// static u16_t minor(dev_t dev)
#define minor(dev)  ((dev) & 0xFF)

#endif // _sys_macros_h_
