
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>


int main (int argc, char *argv[])
{

  if (argc == 1) {
    // display all variables
    char **envp = environ;

    while (*envp) {
      puts(*envp);
      envp++;
    }
  } else {
    argc--;
    argv++;

    while (argc) {
      char *env = getenv(argv[0]);
      if (env) {
	puts(env);
      }

      argc--;
      argv++;
    }
  }
      

#if 0
  printf("add PATH and PS1\n");
  setenv("PATH", "/", 0);
  setenv("PS1", "path# ", 0);

  printf(" PATH=%s\n", getenv("PATH"));
  printf(" PS1=%s\n", getenv("PS1"));
  printf(" PS2=%s\n", getenv("PS2"));
	 
  printf("change PS1 - no overwrite\n");
  setenv("PS1", "not replaced", 0);
  printf(" PS1=%s\n", getenv("PS1"));
  printf("change PS1 - overwrite\n");
  setenv("PS1", "replaced", 1);
  printf(" PS1=%s\n", getenv("PS1"));

  printf("add PWD\n");
  setenv("PWD", "/audio", 0);

  printf(" PATH=%s\n", getenv("PATH"));
  printf(" PS1=%s\n", getenv("PS1"));
  printf(" PWD=%s\n", getenv("PWD"));

  printf("free PS1\n");
  unsetenv("PS1");
  printf(" PATH=%s\n", getenv("PATH"));
  printf(" PS1=%s\n", getenv("PS1"));
  printf(" PWD=%s\n", getenv("PWD"));
  
  printf("free PWD\n");
  unsetenv("PWD");
  printf(" PATH=%s\n", getenv("PATH"));
  printf(" PS1=%s\n", getenv("PS1"));
  printf(" PWD=%s\n", getenv("PWD"));
  
  printf("free PATH\n");
  unsetenv("PATH");
  printf(" PATH=%s\n", getenv("PATH"));
  printf(" PS1=%s\n", getenv("PS1"));
  printf(" PWD=%s\n", getenv("PWD"));

  printf("errno=%i\n", errno);
#endif
  
  return 0;
}
