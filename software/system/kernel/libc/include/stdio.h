
#ifndef _STDIO_H_
#define _STDIO_H_

struct _IO_FILE {
  int (*putchar)(int c);
  int (*getchar)();
};

typedef struct _IO_FILE FILE;

extern struct _IO_FILE * stdin;
extern struct _IO_FILE * stdout;
extern struct _IO_FILE * stderr;

#define EOF ((int)-1)

FILE *fopen(const char *pathname, const char *mode);
int fputc(int c, FILE *stream);
int putchar(int c);
int puts(const char *s);
int fgetc(FILE *stream);
int getchar(void);
int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);

#endif /* _STDIO_H_ */
