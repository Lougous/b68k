//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc - stdio
//

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

////////////////////////////////////////////////////////////////////////////////
// file functions
////////////////////////////////////////////////////////////////////////////////

struct _IO_FILE * stdin;
struct _IO_FILE * stdout;
struct _IO_FILE * stderr;

#define F_ZERO_PADDED  1

static int _f_flags;
static int _f_field_width;
static int _f_lower_upper;

#if !(O_RDWR == O_RDONLY | O_WRONLY)
#error broken assumption
#endif

FILE *fopen(const char *pathname, const char *mode)
{
  FILE *pfd = malloc(sizeof(struct _IO_FILE));

  if (!pfd || !mode) {
    return 0;
  }

  const char *pm = mode;
  short flags = 0;

  while (*pm) {
    switch (*pm) {
    case 'r':
      flags |= O_RDONLY;
      break;
    case 'w':
      flags |= O_WRONLY;
      break;
    case 'b':
      // void
      break;
    default:
      // error
      flags = -1;
      break;
    }

    pm++;
  }

  if (flags <= 0) goto fopen_error;
 
  pfd->fd    = open(pathname, (int)flags);
  pfd->flags = (unsigned short)flags;
  pfd->blen  = 0;

  if (pfd->fd < 0) goto fopen_error;

  return pfd;
  
 fopen_error:
  free(pfd);
  return 0;
}

int fclose(FILE *stream)
{
  if (stream) {
    // TODO: flush
    return close(stream->fd);
  }

  return -1;
}

static int __putchar(FILE *stream, int c)
{
  unsigned char ct = (unsigned char)c;
  
  return write(stream->fd, &ct, 1);
}
  
static void __putchars(FILE *stream, const char *s, int len)
{
  write(stream->fd, s, len);
}
  
static void __print_s(FILE *stream, const char *s)
{
  int len = strlen(s);
  
  write(stream->fd, s, len);
}

static void __print_x(FILE *stream, unsigned int x, int to_pad)
{
  if (x & ~15 || to_pad > 0) __print_x(stream, x >> 4, to_pad-1);
  
  x = x & 15;

  if (x >= 10) {
    __putchar(stream, _f_lower_upper + x);
  } else {
    __putchar(stream, ((int)'0') + x);
  }
}

static unsigned short __div10w(unsigned short n, unsigned int *rem)
{
  unsigned int nl = (unsigned int) n;
  unsigned int qr;

  asm ("move.l %1, %0\n\t"
       "divu #10, %0"
       : "=r" (qr)
       : "r" (nl));

  *rem = (qr >> 16);  // remainder in upper word

  return (unsigned short)qr;  // quotient in lower word
}

unsigned int __div10(unsigned int n, unsigned int *rem)
{
  if (n < 65536U) {
    return (unsigned int)__div10w((unsigned short)n, rem);
  } else {
    unsigned int r;
    // n = 0x A B C D E F G H
    //       \_______/           step 1
    //           \_______/       step 2
    //               \_______/   step 3

    // step 1
    unsigned short q1 = __div10w(n >> 16, &r);
    r = (r << 8) | ((n & 0xffff) >> 8);
    // step 2
    unsigned short q2 = __div10w(r, &r);
    r = (r << 8) | (n & 0xff);
    // step 2
    unsigned short q3 = __div10w(r, &r);

    *rem = r;
    return (((unsigned int)q1) << 16) + (q2 << 8) + q3;
  }
}

static void __print_u(FILE *stream, unsigned int d)
{
  if (d >= 10) {
    unsigned int rem;
    unsigned int q;
    q = __div10(d, &rem);
    __print_u(stream, q);
    __putchar(stream, ((int)'0') + rem);
  } else {
    __putchar(stream, ((int)'0') + d);
  }
}

int fputc(int c, FILE *stream)
{
  return __putchar(stream, c);
}

int putchar(int c)
{
  return __putchar(stdout, c);
}

int puts(const char *s)
{
  __print_s(stdout, s);
  __putchar(stdout, (int)'\n');

  return 0;
}


static int __buf_getc(FILE *stream)
{
  if (stream->blen == 0) {
    // refill
    int inbytes = read(stream->fd, stream->buf, sizeof(stream->buf));

    if (inbytes <= 0) {
      // error or EOF
      return -1;
    } else {
      stream->blen = (unsigned short)inbytes;
      stream->bat = 0;
    }
  }

  stream->blen--;
  
  return (int)stream->buf[stream->bat++];
}

int fgetc(FILE *stream)
{
  unsigned char ct;

  if (stream->flags == O_RDONLY) {
    return __buf_getc(stream);
  }

  // direct, or read/write => no cache
  if (read(stream->fd, &ct, 1) == 1) return (int)ct;

  return -1;
}

char *fgets(char *s, int size, FILE *stream)
{
  char *pc = s;

  if (size <= 0) return NULL;

  // keep last entry for ending '\0'
  size--;
  
  // TODO: read should be blocking at some point, stop if read returns 0
  while (size) {
    short c = fgetc(stream);

    if (c > 0) {
      *pc++ = c;
      size--;
      
      if (c == '\n') {
	return s;
      }
    } else if (c == 0) {
      // EOF
      break;
    } else {
      // error
      return NULL;
    }
  }

  // size-1 characters read
  *pc = 0;
  
  return s;
}

int getchar(void)
{
  return fgetc(stdin);
}

int vfprintf(FILE *stream, const char *format, va_list aps)
{
  va_list ap;
  int d;
  unsigned int x;
  char c, *s;

  va_copy(ap, aps);
  //va_start(ap, format);

  #define M_FORMAT  1
  #define M_FLAG    2

  int mode = M_FORMAT;
  _f_flags = 0;
  _f_field_width = 0;
  _f_lower_upper = 'a';

  const char *c_start = format;
  int c_len = 0;

  while (*format) {
    switch (mode) {
    case M_FORMAT:
      if (*format == '%') {
	/* end current chunk */
	if (c_len) __putchars(stream, c_start, c_len);

	/* start new chunk */
	c_start = format;
	c_len = 0;

	/* will decode flags now */
	mode = M_FLAG;
	_f_field_width = 0;
	_f_flags = 0;
	_f_lower_upper = 'a';
      } else {
	/* continue current chunk */
	c_len++;
      }

      format++;
      break;

    case M_FLAG:
      switch (*format) {
      case 's':              /* string */
	s = va_arg(ap, char *);
	if (s) __print_s(stream, s);
	else __print_s(stream, "(null)");
	mode = M_FORMAT;
	break;
      case 'd':              /* int */
      case 'i':              /* int */
	d = va_arg(ap, int);
	if (d < 0) {
	  __putchar(stream, (int)'-');
	  __print_u(stream, (unsigned int)-d);
	} else {
	  __print_u(stream, (unsigned int)d);
	}
	mode = M_FORMAT;
	
	break;
      case 'u':              /* int */
	x = va_arg(ap, int);
	__print_u(stream, x);
	mode = M_FORMAT;
	break;
      case 'x':              /* int */
      case 'p':              /* void * */
	x = va_arg(ap, unsigned int);
	_f_lower_upper = (int)'a'-10;
	__print_x(stream, x, _f_field_width - 1);
	mode = M_FORMAT;
	break;
      case 'X':              /* int */
	x = va_arg(ap, unsigned int);
	_f_lower_upper = (int)'A'-10;
	__print_x(stream, x, _f_field_width - 1);
	mode = M_FORMAT;
	break;
      case 'c':              /* char */
	/* need a cast here since va_arg only
	   takes fully promoted types */
	c = (char) va_arg(ap, int);
	__putchar(stream, (int)c);
	mode = M_FORMAT;
	break;
      case '%':
	__putchar(stream, (int)'%');
	mode = M_FORMAT;
	break;
      case '0':
	_f_flags |= F_ZERO_PADDED;
	break;
      default:
	if (*format >= '2' && *format <= '8') {
	  /* field width */
	  _f_field_width = *format - '0';
	} else {
	  mode = M_FORMAT;
	}
	break;
      }

      format++;
      c_start = format;
    }

  }

  /* last chunk */
  if (c_len) __putchars(stream, c_start, c_len);
  
  va_end(ap);

  return 0;
}

int printf(const char *format, ...)
{
  va_list ap;
  int r;

  va_start(ap, format);
  r = vfprintf(stdout, format, ap);
  va_end(ap);

  return r;
}

int fprintf(FILE *stream, const char *format, ...)
{
  va_list ap;
  int r;

  va_start(ap, format);
  r = vfprintf(stream, format, ap);
  va_end(ap);

  return r;
}

int fileno(FILE *stream)
{
  if (stream) return stream->fd;

  // TODO: set errno to EBADF
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
// string formatting functions
////////////////////////////////////////////////////////////////////////////////

static char * __sputs(char *str, char *s)
{
  while (*s) {
    *str++ = *s++;
  }

  return str;
}

static char * __sprint_u(char *str, unsigned int d)
{
  if (d >= 10) {
    unsigned int rem;
    unsigned int q;
    
    q = __div10(d, &rem);
    str = __sprint_u(str, q);
    *str++ = '0' + rem;
  } else {
    *str++ = '0' + d;
  }

  return str;
}

static char * __sprint_x(char *str, unsigned int x, int to_pad)
{
  if (x & ~15 || to_pad > 0) str = __sprint_x(str, x >> 4, to_pad-1);
  
  x = x & 15;

  if (x >= 10) {
    *str++ = _f_lower_upper + x;
  } else {
    *str++ = '0' + x;
  }

  return str;
}

int vsprintf(char *str, const char *format, va_list aps)
{
  va_list ap;
  int d;
  unsigned int x;
  char c, *s;

  va_copy(ap, aps);
  //va_start(ap, format);

  #define M_FORMAT  1
  #define M_FLAG    2

  int mode = M_FORMAT;
  _f_flags = 0;
  _f_field_width = 0;
  _f_lower_upper = 'a';

  while (*format) {
    switch (mode) {
    case M_FORMAT:
      if (*format == '%') {
	/* will decode flags now */
	mode = M_FLAG;
	_f_field_width = 0;
	_f_flags = 0;
	_f_lower_upper = 'a';
      } else {
	/* continue current chunk */
	*str++ = *format;
      }

      format++;
      break;

    case M_FLAG:
      switch (*format) {
      case 's':              /* string */
	s = va_arg(ap, char *);
	if (s) {
	  str = __sputs(str, s);
	}
	else {
	  str = __sputs(str, "(null)");
	}
	mode = M_FORMAT;
	break;
      case 'd':              /* int */
      case 'i':              /* int */
	d = va_arg(ap, int);
	if (d < 0) {
	  *str++ = '-';
	  str = __sprint_u(str, (unsigned int)-d);
	} else {
	  str = __sprint_u(str, (unsigned int)d);
	}
	mode = M_FORMAT;
	
	break;
      case 'u':              /* int */
	x = va_arg(ap, int);
	str = __sprint_u(str, x);
	mode = M_FORMAT;
	break;
      case 'x':              /* int */
      case 'p':              /* void * */
	x = va_arg(ap, unsigned int);
	_f_lower_upper = (int)'a'-10;
	str = __sprint_x(str, x, _f_field_width - 1);
	mode = M_FORMAT;
	break;
      case 'X':              /* int */
	x = va_arg(ap, unsigned int);
	_f_lower_upper = (int)'A'-10;
	str = __sprint_x(str, x, _f_field_width - 1);
	mode = M_FORMAT;
	break;
      case 'c':              /* char */
	/* need a cast here since va_arg only
	   takes fully promoted types */
	c = (char) va_arg(ap, int);
	*str++ = c;
	mode = M_FORMAT;
	break;
      case '%':
	*str++ = '%';
	mode = M_FORMAT;
	break;
      case '0':
	_f_flags |= F_ZERO_PADDED;
	break;
      default:
	if (*format >= '2' && *format <= '8') {
	  /* field width */
	  _f_field_width = *format - '0';
	} else {
	  mode = M_FORMAT;
	}
	break;
      }

      format++;
    }

  }

  // terminating null byte
  *str = '\0';

  va_end(ap);

  return 0;
}

int sprintf(char *str, const char *format, ...)
{
  va_list ap;
  int r;

  va_start(ap, format);
  r = vsprintf(str, format, ap);
  va_end(ap);

  return r;
}

