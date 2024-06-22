
#ifndef _syscall_h_
#define _syscall_h_

#define ANY  PROC_PID_ANY

////////////////////////////////////////////////////////////////////////////////
// VFS messages
////////////////////////////////////////////////////////////////////////////////
#define MOUNT     0x0101
#define OPEN      0x0102
#define GETDENTS  0x0103
#define CLOSE     0x0104
#define READ      0x0105
#define WRITE     0x0106
#define LSEEK     0x0107
#define VFS_FORK  0x0108  // reserved to kernel
#define CHDIR     0x0109
#define IOCTL     0x0110
#define GETCWD    0x0111
#define MKDIR     0x0112

typedef struct {
  char *dev;
  char *path_to;
  char *type;
} mount_msg_body_t;

typedef struct {
  char *pathname;
  u16_t len;
  u16_t flags;
} open_msg_body_t;

typedef struct {
  int  fd;
} close_msg_body_t;

typedef struct {
  unsigned int fd;
  struct dirent *dirp;
  unsigned int count;
} getdents_msg_body_t;

typedef struct {
  int fd;
  void *buf;
  size_t count;
} read_msg_body_t;

typedef struct {
  int fd;
  void *buf;
  size_t count;
} write_msg_body_t;

typedef struct {
  int fd;
  off_t offset;
  int whence;
} lseek_msg_body_t;

typedef struct {
  pid_t parent;
  pid_t child;
} vfs_fork_msg_body_t;

typedef struct {
  const char *path;
  u16_t len;  // includes terminating null byte
} chdir_msg_body_t;

typedef struct {
  int fd;
  int request;
  void *ptr;
} ioctl_msg_body_t;

typedef struct {
  const char *buf;
  u16_t size;
} getcwd_msg_body_t;

typedef struct {
  const char *path;
  u16_t len;
  u16_t flags;
} mkdir_msg_body_t;

////////////////////////////////////////////////////////////////////////////////
// devices messages
////////////////////////////////////////////////////////////////////////////////
#define DEV_READ      0x0202
#define DEV_WRITE     0x0203
#define DEV_OPEN      0x0204
#define DEV_CLOSE     0x0205
#define DEV_IOCTL     0x0206

typedef struct {
  void *handle;
  off_t src_seek;
  void *dst;
  u32_t count;
} dev_read_msg_body_t;

typedef struct {
  void *handle;
  void *src;
  off_t dst_seek;
  u32_t count;
} dev_write_msg_body_t;

typedef struct {
  void *handle;
  int  flags;
} dev_open_msg_body_t;

typedef struct {
  void *handle;
} dev_close_msg_body_t;

typedef struct {
  void *handle;
  pid_t pid;
  int request;
  void *va_ptr;
} dev_ioctl_msg_body_t;

////////////////////////////////////////////////////////////////////////////////
// process management messages
////////////////////////////////////////////////////////////////////////////////
#define EXIT      0x0301
#define FORK      0x0302
#define EXEC      0x0303
#define WAIT      0x0304
#define KILL      0x0305

typedef struct {
  char **argv;  // NULL terminated list
} exec_msg_body_t;

typedef struct {
  pid_t pid;
  int   sig;
} kill_msg_body_t;

// exit: uses s32 body
// fork: uses no body (no argument)
// wait: uses no body (no argument), yet (TODO)

////////////////////////////////////////////////////////////////////////////////
// clock messages
////////////////////////////////////////////////////////////////////////////////
#define SLEEP     0x0401

// sleep: uses u32 body

////////////////////////////////////////////////////////////////////////////////
typedef struct {
  // header
  //  pid_t src;
  u16_t type;

  // body
  union body {
    // general purpose
    u32_t u32;
    s32_t s32;
    
    // VFS
    mount_msg_body_t mount;
    getdents_msg_body_t getdents;
    open_msg_body_t open;
    close_msg_body_t close;
    read_msg_body_t read;
    write_msg_body_t write;
    lseek_msg_body_t lseek;
    vfs_fork_msg_body_t vfs_fork;
    chdir_msg_body_t chdir;
    ioctl_msg_body_t ioctl;
    getcwd_msg_body_t getcwd;
    mkdir_msg_body_t mkdir;

    // devices
    dev_read_msg_body_t dev_read;
    dev_write_msg_body_t dev_write;
    dev_open_msg_body_t dev_open;
    dev_close_msg_body_t dev_close;
    dev_ioctl_msg_body_t dev_ioctl;

    // processes
    exec_msg_body_t exec;
    kill_msg_body_t kill;
  } body __attribute__ ((aligned (4)));
} message_t __attribute__ ((aligned (4)));

#define ELOCK    ((pid_t) -1)
#define EABORT   ((pid_t) -2)
#define ESENDANY ((pid_t) -3)

#define O_SEND   1
#define O_RECV   2

#endif /* _syscall_h_ */
