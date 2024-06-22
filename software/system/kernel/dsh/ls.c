
#include <types.h>
#include <stddef.h>
#include <fcntl.h>
#include <dirent.h>
#include <syscall.h>

#include <string.h>
#include <stdio.h>

#include "msg.h"

static message_t _msg;

int ls (int argc, char *argv[])
{
  char buf[20];

  if (argc < 1) return -1;

  char *path = argc > 1 ? argv[1] : "/";

  int fd = sendreceive_vfs_open (&_msg, path, O_DIRECTORY);

  if (fd < 0) {
    printf("ls: cannot access to '%s': no such directory\n", path);
    return -1;
  }

  printf("content of %s:\n", path);
    
  while (1) {
      
    int len = sendreceive_vfs_getdents (&_msg, fd, (struct dirent *)&buf[0], 20);

    if (len < 0) {
      printf("ls: access error\n");
      sendreceive_vfs_close(&_msg, fd);
      return -1;
    }

    if (len == 0) {
      sendreceive_vfs_close(&_msg, fd);
      return 0;
    }
      
    struct dirent *de = (struct dirent *)&buf[0];
      
    while (len > 0) {
      printf("%s\n", de->d_name);
      len = len - de->d_size;
      de = (void *)de + de->d_size;
    }
  }

  return 0;
}

