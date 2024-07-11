//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/binaries - sh
//

#include <stdio.h>
#include <stddef.h>
#include <types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFLEN    128
#define MAX_ARGS  8

char line[BUFLEN];
char *c_args[MAX_ARGS];

void cut_words (char *cline)
{
  int argc = 0;
  int in = 0;
  in = 0;

  c_args[0] = 0;

  while (*cline) {
    if (in == 0 && *cline != ' ') {
      // start of word
      in = 1;
      c_args[argc++] = cline;

    } else if (in == 1 && *cline == ' ') {
      // end of word
      *cline = 0;
      in = 0;

      // wont'be able to cut more
      if (argc == (MAX_ARGS - 1)) {
	break;
      }
    }

    cline++;
  }

  c_args[argc] = 0;   
}

int main (int argc, char *argv[])
{

  while (1) {
    // TODO: use PS1 environment variable
    printf("$ ");

    // get a line
    {
      char *pc = line;
      short len = 0;

      while (1) {
	int n = read(0 /* stdin */, pc, 1);

	if (n < 0) break;  // IO error

	if (n) {
	  if (*pc == '\n') {
	    *pc = 0;
	    break;
	  } else if (*pc == 0) {
	    break;
	  }
	  
	  pc++;
	  len++;  // TODO: buffer overflow !
	}
      }
    }

    // break down command & arguments
    cut_words(line);

    if (c_args[0]) {
      // internal commands
      if (strcmp(c_args[0], "exit") == 0) break;

      if (strcmp(c_args[0], "cd") == 0) {
	char *path = c_args[1] ? c_args[1] : "/";  // TODO use HOME 
	
	if (chdir(path) < 0) {
	  printf("sh: cd: %s: No such directory\n", path);
	}

	continue;
      }

      // external commands
      int fd = open(c_args[0], O_RDONLY);

      if (fd < 0) {
	printf("Command '%s' not found\n", c_args[0]);
	continue;
      }

      // file exists, create new process
      close(fd);
      
      int pid = fork();
    
      if (pid < 0) {
	printf("cannot fork\n");
      } else if (pid == 0) {
	// child
	execv(c_args[0], c_args);

	// only when exec fails
	printf("cannot exec\n");
	exit(0);
      } else {
	// parent
	wait(NULL);

	// TODO: process exit condition to look at
	kill(pid, 1);
      }
    }
  }

  return 0;
}
