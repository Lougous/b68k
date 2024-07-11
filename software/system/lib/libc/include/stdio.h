//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc
//

#ifndef _STDIO_H_
#define _STDIO_H_

struct _IO_FILE {
  int fd;  // file descriptor
  unsigned short flags;
  unsigned short blen;
  unsigned short bat;
  unsigned char buf[16];
};

typedef struct _IO_FILE FILE;

extern struct _IO_FILE * stdin;
extern struct _IO_FILE * stdout;
extern struct _IO_FILE * stderr;

#define EOF ((int)-1)

FILE *fopen(const char *pathname, const char *mode);
int fclose(FILE *stream);
int fputc(int c, FILE *stream);
int putchar(int c);
int puts(const char *s);
int fgetc(FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int getchar(void);
int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int fileno(FILE *stream);
int sprintf(char *str, const char *format, ...);

#ifdef _VA_LIST_
int vsprintf(char *str, const char *format, va_list ap);
int vfprintf(FILE *stream, const char *format, va_list ap);
#endif

#endif /* _STDIO_H_ */
