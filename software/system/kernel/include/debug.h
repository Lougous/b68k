
#ifndef _debug_h_
#define _debug_h_

#define K_ASSERT(cond, msg)

#include <stdio.h>

#include "lock.h"

extern struct _IO_FILE __stdout_msg_struct;

#define K_PRINTF(level, ...)  \
  { if (level && level <= K_DEBUG_LEVEL) {				\
    u16_t lbkp = k_lock(); fprintf(stdout, __VA_ARGS__); k_unlock(lbkp);  \
  }  else if (! level) {  \
    fprintf(&__stdout_msg_struct, __VA_ARGS__);  \
    } }

extern void k_panic (void);

//#define SIMULATION_MODE

#endif /* _debug_h_ */
