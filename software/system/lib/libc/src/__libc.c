//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc - interns (globals, process init hook)
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

struct _IO_FILE __stdin_struct;
struct _IO_FILE __stdout_struct;
struct _IO_FILE __stderr_struct;

extern struct _IO_FILE * stdin;
extern struct _IO_FILE * stdout;
extern struct _IO_FILE * stderr;

// malloc management (stdlib)
extern void __mem_init ();
extern void __mem_add_chunk(void *chkp, int len);

char **environ;
int errno;

void __libc_init (void)
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

  // errno
  errno = 0;
  
  // environ
  environ = NULL;
  
  // rand
  srand(12345678);
}

// memory at envp contains pointer table then data strings all together in a
// envlen wide memory space. see EXEC in kernel/system.c
void __libc_env_init (char *envp[], int envlen)
{
  // environment
  short envc = 1;  // at least terminating NULL
  
  if (envp) {
    // copy to allocated memory so that environment variable could be freed
    char **ep = (char **)envp;

    // count variables
    while (*ep++) {
      envc++;
    }
  }
  
  environ = (char **)malloc(envc*sizeof(char *));
  
  if (environ) {
    char **emp = environ;

    if (envp) {
      char **ep = (char **)envp;
    
      while (*ep) {
	*emp = strdup(*ep);
	ep++;
	if (! *emp++) break;
      }
    }
    
    // end the list
    *emp = NULL;
  }

  if (envp) {
    // free space for further allocations with malloc
    __mem_add_chunk(envp, envlen);
  }
}
