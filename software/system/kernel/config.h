
#ifndef _config_h_
#define _config_h_

/* kernel version */
#define K_VERSION_MAJOR  0
#define K_VERSION_MINOR  18

/* kernel debug / verbose level */
#define K_DEBUG_LEVEL  2

/* hardware configuration */
#define K_HAS_AV_BOARD

/* kernel space stack size */
#define K_STACK_BYTES  0x2000

/* serial bootstrap init process size */
#define K_BSTP_BYTES  0x10000

/* max number of TTYs */
#define K_TTY_COUNT        4

/* cursor blinking speed, 0 = no blink */
#define K_TTY_CBLK_MS      1000

/* max number of processes */
#define K_PROC_COUNT_BITS  5
#define K_PROC_COUNT       32
#define K_PROC_IDLE        (K_PROC_COUNT - 1)

#if (K_PROC_COUNT > (1 << K_PROC_COUNT_BITS))
#error encoding not possible
#endif

/* max length for processes name */
#define K_PROC_NAME_MAX_LEN    16

/* process arguments area maximum occupency */
#define K_PROC_ARGS_SIZE  1024

/* process max number of arguments */
#define K_PROC_ARGS_COUNT 8

/* timer interrupt period */
#define K_TIMER_PERIOD_MS   1

/* memory size, number of 4k blocks */
#define K_MEM_BLOCK_COUNT     512  // 2MiB

/* device manager */
#define K_DEV_COUNT   12

/* file system */
#define K_MOUNT_COUNT        8
#define K_VNODE_COUNT        16
#define K_PROC_FD_COUNT      16
#define K_MAX_FILENAME_LEN   64
#define K_MAX_DIRNAME_LEN    128
#define K_MAX_DIRENT_LEN     128   /* max length for dirent syscalls */
#define K_MAX_READWRITE_LEN  512   /* max length for read/write syscalls */

#endif /* _config_h_ */
