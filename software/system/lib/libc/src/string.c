//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc - string
//

#include <string.h>
#include <stdlib.h>  // malloc

int strcmp(const char *s1, const char *s2) {
  while (*s1 == *s2++)
    if (*s1++ == 0)
      return (0);
  return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}

int
strncmp(const char *s1, const char *s2, register size_t n)
{
  register unsigned char u1, u2;

  while (n-- > 0)
    {
      u1 = (unsigned char) *s1++;
      u2 = (unsigned char) *s2++;
      if (u1 != u2)
	return u1 - u2;
      if (u1 == '\0')
	return 0;
    }
  return 0;
}

void *memcpy(void *dest, const void *src, size_t n)
{
  char *c_dest = (char *)dest;
  char *c_src = (char *)src;
  
  while (n--) *c_dest++ = *c_src++;

  return dest;
}

void *memset(void *s, int c, size_t n)
{
  char *c_s = (char *)s;
  
  while (n--) *c_s++ = (char)c;

  return s;  
}

char *strcpy(char *dest, const char *src) {
  char *ret = dest;
  
  while (*src) {
    *dest++ = *src++;
  }

  // copy terminating null byte
  *dest = 0;

  return ret;
}

// from man page !
char *strncpy(char *dest, const char *src, size_t n)
{
  size_t i;

  for (i = 0; i < n && src[i] != '\0'; i++)
    dest[i] = src[i];
  for ( ; i < n; i++)
    dest[i] = '\0';
  
  return dest;
}

size_t strlen(const char *s) {
  const char *start = s;

  while (*s++);

  return (size_t)(s - start - 1);
}

char *strchr(const char *s, int c)
{
  const char ch = c;

  for ( ; *s != ch; s++)
    if (*s == '\0')
      return 0;
  return (char *)s;
}

char *strdup(const char *s)
{
  char *s1;
  int i;
  
  i = strlen(s) + 1;
  s1 = malloc(i);

  if (s1) {
    strncpy(s1, s, i);
  }

  return s1;
}
