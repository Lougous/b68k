//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/binaries - init (first user process started by the kernel)
//

#include <fcntl.h>
#include <stddef.h>
#include <types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

volatile int loop;
char buf[80];

#define DEVICE  "/dev/tty0"

int main (int argc, char *argv[])
{
  /* FILE mapper for libc stdin, stdout & stderr
     overwrite associations from __libc_init, through actual integer file
     descriptors should be the same : 0, 1 and 2 */
  stdin->fd = open(DEVICE, O_RDONLY);
  stdout->fd = open(DEVICE, O_WRONLY);
  stderr->fd = open(DEVICE, O_WRONLY);

  if (stdin->fd < 0) return -1;
  if (stdout->fd < 0) return -1;
  if (stderr->fd < 0) return -1;

  // setup default environment
  setenv("PATH", "/bin", 0);

  while (1) {
    printf("\nstarting shell ...\n");

    pid_t sh_pid = fork();

    if (sh_pid < 0) {
      printf("init: failed to fork\n");
    } else if (sh_pid == 0) {
      // child
      // start shell user process
      char *args[] = { "/bin/sh", (char*)0 };

      execve(args[0], args, environ);

      // only when exec fails
      printf("init: child: failed to exec\n");
    } else {
      // parent
      // wait for the end of child process
      wait(NULL);
      // kiil ended process
      kill(sh_pid, 1);
    }
  }  
    
  return 0;
}
