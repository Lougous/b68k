
#ifndef _STRING_H_
#define _STRING_H_

#include <stddef.h>

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);

void *memcpy32(void *dest, const void *src, size_t n);

#endif /* _STRING_H_ */
