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
int execve(const char *pathname, char *const argv[], char *const envp[]);

int chdir(const char *path);

extern char **environ;

#endif /* _unistd_h_ */
