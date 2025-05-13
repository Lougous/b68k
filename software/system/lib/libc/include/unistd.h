//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc
//

#ifndef _unistd_h_
#define _unistd_h_

typedef int ssize_t;

int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count); 

void _exit(int status);
int fork(void);
int execv(const char *pathname, char *const argv[]);

int chdir(const char *path);

#endif /* _unistd_h_ */
