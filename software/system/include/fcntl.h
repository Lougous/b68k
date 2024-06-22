
#ifndef _fcntl_h_
#define _fcntl_h_

#define O_RDONLY        0x00000001
#define O_WRONLY        0x00000002
#define O_RDWR          0x00000003
#define O_DIRECTORY     0x00000004
#define O_DIRECT        0x00000100

#define SEEK_SET  0
#define SEEK_CUR  1
#define SEEK_END  2

int open(const char *pathname, int flags);

#endif /* _fcntl_h_ */
