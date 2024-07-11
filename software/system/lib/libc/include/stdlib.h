//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc
//

#ifndef _stdlib_h_
#define _stdlib_h_

#include <stddef.h>  // size_t

void exit(int status);

// numerics
void srand(unsigned int seed);
int rand(void);
int atoi(const char *nptr);
int abs(int j);

// memory
void *malloc(size_t size);
void free(void *ptr);

#endif /* _stdlib_h_ */
