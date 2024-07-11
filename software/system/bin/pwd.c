//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/binaries - pwd
//

#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <limits.h>

int main (int argc, char *argv[])
{
  static char buf[PATH_MAX];

  if (getcwd(buf, sizeof(buf))) {
    printf("%s\n", buf);

    return 0;
  }

  printf("pwd: failed to get directory\n");
  
  return -1;
}
