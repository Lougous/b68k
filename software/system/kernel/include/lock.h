//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/kernel - kernel header - critical section
//

#ifndef _lock_h_
#define _lock_h_

extern u16_t k_lock (void);
extern u16_t k_unlock (u16_t bkp);

#endif /* _lock_h_ */
