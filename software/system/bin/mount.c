//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/binaries - mount
//

#include <stdio.h>
#include <stddef.h>
#include <sys/mount.h>

int main (int argc, char *argv[])
{
  if (argc != 4) {
    puts("mount: bad usage");
    puts("Usage:");
    puts(" mount <source> <directory> <type>");
    return -1;
  }

  int ret = mount(argv[1], argv[2], argv[3], 0, NULL);
  
  if (ret) {
    printf("mount failed: code %i\n", ret);
    return -1;
  }

  return 0;
}
