
#include <stddef.h>
#include <fcntl.h>
#include <types.h>
#include <syscall.h>

#include <stdio.h>

#include "dirent.h"
#include "msg.h"

message_t _msg;

int cat (int argc, char *argv[])
{
  if (argc < 2) return -1;

  int fd = sendreceive_vfs_open (&_msg, argv[1], O_RDONLY);
  int line = 0;

  if (fd >= 0) {
    while (1) {
      char c;
      int n = sendreceive_vfs_read(&_msg, fd, &c, 1);

      if (!n) {
	sendreceive_vfs_close(&_msg, fd);
	break;
      }

      putchar(c);

      if (c == '\n') line++;

      if (line == 24) {
	printf("<press return to continue>");
	getchar();
	line = 0;
      }
    }
  } else {
    printf("%s: cannot open file\n", argv[1]);
  }

  return 0;
}


