
#include <stdarg.h>

#include "stdio.h"

struct _IO_FILE * stdin;
struct _IO_FILE * stdout;
struct _IO_FILE * stderr;

#define F_ZERO_PADDED  1

static int _f_flags;
static int _f_field_width;
static int _f_lower_upper;


static int __putchar(FILE *stream, int c)
{
  return stream->putchar(c);
}
  
static void __print_s(FILE *stream, const char *s)
{
  while (*s) __putchar(stream, *s++);
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

static unsigned int __umod10(unsigned int d)
{
  return d - (((d / 10)*5) << 1);
}

static void __print_u(FILE *stream, unsigned int d)
{
  if (d >= 10) {
    unsigned int rem;
    unsigned int q;
    q = d / 10;
    rem = __umod10(d);
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

int fgetc(FILE *stream)
{
  return stream->getchar();
}

int getchar(void)
{
  return fgetc(stdin);
}

static int _my_fprintf(FILE *stream, const char *format, va_list aps)
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
	mode = M_FLAG;
	_f_field_width = 0;
	_f_flags = 0;
	_f_lower_upper = 'a';
      } else {
	__putchar(stream, (int)*format);
      }
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
    }

    format++;
  }

  va_end(ap);

  return 0;
}

int printf(const char *format, ...)
{
  va_list ap;
  int r;

  va_start(ap, format);
  r = _my_fprintf(stdout, format, ap);
  va_end(ap);

  return r;
}

int fprintf(FILE *stream, const char *format, ...)
{
  va_list ap;
  int r;

  va_start(ap, format);
  r = _my_fprintf(stream, format, ap);
  va_end(ap);

  return r;
}

