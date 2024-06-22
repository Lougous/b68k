
#ifndef _errno_h_
#define _errno_h_

#define	EPERM		1	/* Operation not permitted */
#define	ENOENT          2	/* No such file or directory */
#define	EACCESS		5
#define	EBADF		9	/* Bad file number */
#define ENOMEM          12      /* Out of memory */
#define	EFAULT          14	/* Bad address */
#define ENOTBLK		15	/* Block device required */
#define	EBUSY           16	/* Mount device busy */
#define	EEXIST          17	/* File exists */
#define	ENODEV          19	/* No such device */
#define ENOTDIR		20	/* Not a directory */
#define EISDIR		21	/* Is a directory */
#define EINVAL		22      /* Invalid argument */
#define ENFILE		23      /* File table overflow */
#define EROFS           30	/* Read-only file system */
#define ERANGE          34      /* Result too large */

#endif // _errno_h_
