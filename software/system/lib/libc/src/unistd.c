//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc - unistd
//

#include <types.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <syscall.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "trap.h"

int open(const char *pathname, int flags)
{
  // message to vfs task
  volatile message_t msg = { .type               = OPEN,
			     .body.open.pathname = (char *)pathname,
			     .body.open.len      = strlen((char *)pathname),
			     .body.open.flags    = flags };
  u32_t rval;

  SENDRECEIVE(msg, 3, rval);

  if (rval != 3) {
    return -1;
  }

  if (msg.body.s32 < 0) {
    errno = -msg.body.s32;
    return -1;
  }
  
  return msg.body.s32;
}

int close(int fd)
{
  // message to vfs task
  volatile message_t msg = { .type = CLOSE,
			     .body.close.fd = fd };

  u32_t rval;

  SENDRECEIVE(msg, 3, rval);

  if (rval != 3) {
    return -1;
  }
  
  if (msg.body.s32) {
    errno = -msg.body.s32;
    return -1;
  }
  
  return 0;
}

ssize_t read(int fd, void *buf, size_t count)
{
  // message to vfs task
  volatile message_t msg = { .type = READ,
			     .body.read.fd = fd,
			     .body.read.buf = buf,
			     .body.read.count = count };

  u32_t rval;

  SENDRECEIVE(msg, 3, rval);

  if (rval != 3) {
    return -1;
  }
  
  if (msg.body.s32 < 0) {
    errno = -msg.body.s32;
    return -1;
  }
  
  return msg.body.s32;
}

ssize_t write(int fd, const void *buf, size_t count)
{
  // message to vfs task
  volatile message_t msg = { .type = WRITE,
			     .body.write.fd = fd,
			     .body.write.buf = (void *)buf,
			     .body.write.count = count };

  u32_t rval;

  SENDRECEIVE(msg, 3, rval);

  if (rval != 3) {
    return -1;
  }

  if (msg.body.s32 < 0) {
    errno = -msg.body.s32;
    return -1;
  }
  
  return msg.body.s32;
}
  
int fork(void)
{
  // message to system task
  volatile message_t msg = { .type = FORK };

  u32_t rval;

  SENDRECEIVE(msg, 0, rval);

  if (rval != 0) {
    return -1;
  }

  if (msg.body.s32 < 0) {
    errno = -msg.body.s32;
    return -1;
  }
  
  return msg.body.s32;
}

int execve(const char *pathname, char *const argv[], char *const envp[])
{
  // message to system task
  volatile message_t msg = {
    .type = EXEC,
    .body.exec.argv = (char **)argv,
    .body.exec.envp = (char **)envp };

  u32_t rval;

  SENDRECEIVE(msg, 0, rval);
  
  if (rval != 0) {
    return -1;
  }

  // if this system call returns, something went wrong
  errno = -msg.body.s32;
  return -1;
}

int chdir(const char *path)
{
  // message to vfs task
  volatile message_t msg = { .type = CHDIR,
			     .body.chdir.path = path,
			     // add 1 for terminating null byte
			     .body.chdir.len = strlen((char *)path) + 1 };

  u32_t rval;

  SENDRECEIVE(msg, 3, rval);

  if (rval != 3) {
    return -1;
  }
  
  if (msg.body.s32 < 0) {
    errno = -msg.body.s32;
    return -1;
  }
  
  return 0;
}

int mount(const char *source, const char *target,
	  const char *filesystemtype, unsigned long mountflags,
	  const void *data)
{
  // message to vfs task
  volatile message_t msg = {
    .type               = MOUNT,
    .body.mount.dev     = (char *)source,
    .body.mount.path_to = (char *)target,
    .body.mount.type    = (char *)filesystemtype
  };

  u32_t rval;

  SENDRECEIVE(msg, 3, rval);

  if (rval != 3) {
    // system error ...
    return -1;
  }

  if (msg.body.s32 < 0) {
    errno = -msg.body.s32;
    return -1;
  }
  
  return 0;
}

intptr_t __libc_brk;

int brk(void *addr)
{
  // message to system task
   message_t msg;

  msg.type = BRK;
  msg.body.u32 = (u32_t)addr;

  u32_t rval;

  SENDRECEIVE(msg, 0, rval);
  
  if (rval != 0) {
    return -1;
  }

  if (msg.body.u32) {
    errno = -msg.body.u32;
    return -1;
  }

  __libc_brk = (intptr_t)addr;

  return 0;
}

void *sbrk(intptr_t increment)
{
  intptr_t old_brk = __libc_brk;
  
  if (increment) {
    if (brk((void *)(old_brk + increment))) {
      // failed
      return (void *) -1;
    }
  }

  return (void *)old_brk;
}
