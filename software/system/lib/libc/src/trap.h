//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc - trap macros/headers
//

#ifndef _trap_h_
#define _trap_h_

// 80h - TRAP #0 vector
extern pid_t trap_sendreceive(message_t *msg, u32_t pid_to);

#define SENDRECEIVE(msg, to, ret) ret = trap_sendreceive((message_t *)&(msg), (to))

// 84h - TRAP #1 vector
extern u32_t trap_mticks(void);

#endif /* _trap_h_ */
