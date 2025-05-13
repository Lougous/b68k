//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc - interns (globals, process init hook)
//

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

struct _IO_FILE __stdin_struct;
struct _IO_FILE __stdout_struct;
struct _IO_FILE __stderr_struct;

extern struct _IO_FILE * stdin;
extern struct _IO_FILE * stdout;
extern struct _IO_FILE * stderr;

extern void __mem_init ();

char **environ;
int errno;

void __libc_init (char *const envp[])
{
  // stdin
  __stdin_struct.fd    = 0;
  __stdin_struct.flags = O_RDONLY | O_DIRECT;
  stdin  = &__stdin_struct;

  // stdout
  __stdout_struct.fd    = 1;
  __stdout_struct.flags = O_WRONLY | O_DIRECT;
  stdout = &__stdout_struct;

  // stderr
  __stderr_struct.fd    = 2;
  __stderr_struct.flags = O_WRONLY | O_DIRECT;
  stderr = &__stderr_struct;

  // malloc
  __mem_init();

  // environment
  environ = (char **)envp;

  // errno
  errno = 0;
  
  // rand
  srand(12345678);
}
