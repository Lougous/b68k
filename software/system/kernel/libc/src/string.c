
#include <string.h>
#include <types.h>

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

void *memcpy32(void *dest, const void *src, size_t n)
{
  u32_t *c_dest = (u32_t *)dest;
  u32_t *c_src = (u32_t *)src;
  
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

size_t strnlen(const char *s, size_t maxlen)
{
  const char *start = s;

  while (maxlen-- && *s++);

  return (size_t)(s - start - 1);
}
