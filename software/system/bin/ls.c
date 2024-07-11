//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/binaries - ls
//

#include <stddef.h>
#include <types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <limits.h>

extern int getdents (unsigned int fd, struct dirent *dirp, unsigned int count);
  
static int _ls (char *path, int multi)
{
  int fd = open(path, O_DIRECTORY);
  char buf[20];

  if (fd >= 0) {
    if (multi) {
      printf("%s:\n", path);
    }
    
    while (1) {
      int len = getdents(fd, (struct dirent *)&buf[0], sizeof(buf));

      if (len < 0) {
	printf("ls: access error\n");
	close(fd);
	return -1;
      }

      if (len == 0) {
	close(fd);
	return 0;
      }
      
      struct dirent *de = (struct dirent *)&buf[0];
      
      while (len) {
	printf("%s\n", de->d_name);
	len -= de->d_size;
	de = (void *)de + de->d_size;
      }
    }
  
  } else {
    printf("ls: cannot access '%s': no such file or directory\n", path);
    return -1;
  }
}
  
int main (int argc, char *argv[])
{
  int multi = argc > 2;

  if (argc < 2) {
    static char buf[PATH_MAX];

    if (getcwd(buf, sizeof(buf))) {
      _ls(buf, 0);
      return 0;
    } else {
      printf("ls: failed to get current directory\n");
    }    
  } else {
    argc--;
    argv++;
    
    while (argc) {
      _ls(argv[0], multi);
      argc--;
      argv++;
    }
  }

  return 0;
}

