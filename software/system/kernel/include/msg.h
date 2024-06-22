
#ifndef _msg_h_
#define _msg_h_

extern pid_t sendreceive (pid_t to, message_t *pa_msg, u16_t ops);

#define send(to, pa_msg)  sendreceive((to), (pa_msg), O_SEND)
#define receive(to, pa_msg)  sendreceive((to), (pa_msg), O_RECV)

/* macros */
extern int sendreceive_vfs_open (message_t *msg, const char *pathname, int flags);
extern int sendreceive_vfs_close(message_t *msg, int fd);
extern ssize_t sendreceive_vfs_read(message_t *msg, int fd, void *buf, size_t count);
extern int sendreceive_vfs_getdents (message_t *msg, unsigned int fd, struct dirent *dirp, unsigned int count);
extern off_t sendreceive_vfs_lseek (message_t *msg, int fildes, off_t offset, int whence);

#endif /* _msg_h_ */
