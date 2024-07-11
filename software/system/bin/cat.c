//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/binaries - cat
//

#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define IO_BUFSIZE 1024

static char _buf[IO_BUFSIZE];

int main (int argc, char *argv[])
{

  while (argc > 1) {
    int fd = open(argv[1], O_RDONLY);

    if (fd >= 0) {
      while (1) {
	ssize_t n = read(fd, _buf, sizeof(_buf));

	if (n > 0) {
	  write(1 /* stdout */, _buf, n);
	} else {
	  // EOF or error
	  close(fd);
	  break;
	}
      }
    } else {
      printf("cat: %s: No such file or directory\n", argv[1]);
    }

    argc--;
    argv++;
  }

  return 0;
}


