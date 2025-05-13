//
// B68K Computer - Copyright (c) 2025 Lougous
// https://github.com/Lougous/b68k
//
// remote file system messages format
//

#ifndef _rfs_h_
#define _rfs_h_

typedef u32_t rnode_t;

////////////////////////////////////////////////////////////////////////////////
// configuration
////////////////////////////////////////////////////////////////////////////////
#define RFS_MAX_NAME_LEN   16
#define RFS_MAX_READ       64

////////////////////////////////////////////////////////////////////////////////
// supporting functions/macros
////////////////////////////////////////////////////////////////////////////////
//#define U32LE_TO_U8(at, u32) [at] = (u32), [at+1] = ((u32) >> 8), [at+2] = ((u32) >> 16), [at+3] = ((u32) >> 24)

static inline u32_t rfs_read_u32le(u8_t *pc)
{
  return (u32_t)pc[0] + ((u32_t)pc[1] << 8) + ((u32_t)pc[2] << 16) + ((u32_t)pc[3] << 24);
}

static inline u8_t * rfs_u32le_at(u8_t *pc, u32_t w32)
{
  *pc++ = (u8_t)w32;
  *pc++ = (u8_t)(w32 >> 8);
  *pc++ = (u8_t)(w32 >> 16);
  *pc++ = (u8_t)(w32 >> 24);

  return pc;
}

// 2 bytes header, 2 bytes footer
#define RFS_PAYLOAD_LEN(full_len) ((full_len) - 4)

////////////////////////////////////////////////////////////////////////////////
// reset command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid; u8_t csum; u8_t tail;
typedef u8_t rfs_creset_t[5] __attribute__ ((aligned (4)));

// u8_t head; u8_t len; u8_t cid; u8_t root[4]; u8_t csum; u8_t tail;
typedef u8_t rfs_areset_t[9] __attribute__ ((aligned (4)));

////////////////////////////////////////////////////////////////////////////////
// open command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid; u8_t rnode[4]; u8_t flags; u8_t pid; u8_t csum; u8_t tail;
typedef u8_t rfs_copen_t[11] __attribute__ ((aligned (4)));

// u8_t head; u8_t len; u8_t cid; u8_t status; u8_t size[4]; u8_t csum; u8_t tail;
typedef u8_t rfs_aopen_t[10] __attribute__ ((aligned (4)));

////////////////////////////////////////////////////////////////////////////////
// close command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid; u8_t rnode[4]; u8_t csum; u8_t tail;
typedef u8_t rfs_cclose_t[9];

// u8_t head; u8_t len; u8_t cid; u8_t status; u8_t csum; u8_t tail;
typedef u8_t rfs_aclose_t[6];

////////////////////////////////////////////////////////////////////////////////
// read command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid; u8_t len; u8_t rnode[4]; u8_t lseek[4]; u8_t csum; u8_t tail;
typedef u8_t rfs_cread_t[14] __attribute__ ((aligned (4)));

// u8_t head; u8_t len; u8_t cid; u8_t status; u8_t buf[]; u8_t csum; u8_t tail;
typedef u8_t rfs_aread_ko_t[6] __attribute__ ((aligned (4)));

#define RFS_AREAD_OK  0
#define RFS_AREAD_KO  255

typedef u8_t rfs_aread_t[] __attribute__ ((aligned (4)));

////////////////////////////////////////////////////////////////////////////////
// getdents command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid='D'; u8_t rnode[4]; u8_t dpos, u8_t csum; u8_t tail;
typedef u8_t rfs_cgetdents_t[10] __attribute__ ((aligned (4)));

/*#define RFS_CGETDENTS(rnode, dpos) { [0] = 0x55, [1] = sizeof(rfs_cgetdents_t) - 4, [2] = 'D', \
      U32LE_TO_U8(3, (rnode)),						\
      [7] = (dpos), [8] = 'D' + SUM32((rnode)),				\
      [9] = 0xAA }
*/

// u8_t head; u8_t len; u8_t cid='d'; u8_t type; u8_t buf[RFS_MAX_NAME_LEN]; u8_t csum; u8_t tail;
typedef u8_t rfs_agetdents_t[6+RFS_MAX_NAME_LEN] __attribute__ ((aligned (4)));

// if buf is empty => error

// types
#define RFS_TYPE_UNK  0
#define RFS_TYPE_REG  1
#define RFS_TYPE_DIR  2


//#define RFS_AGETDENTS_TYPE(agetdents) ((u8_t *)(agetdents))[3]
//#define RFS_AGETDENTS_BUF(agetdents) ((u8_t *)&(((u8_t *)(agetdents))[4]))

////////////////////////////////////////////////////////////////////////////////
// mkdir command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid='M'; u8_t rnode[4]; u8_t flags; u8_t pid; u8_t name[RFS_MAX_NAME_LEN], u8_t csum; u8_t tail;
typedef u8_t rfs_cmkdir_t[11+RFS_MAX_NAME_LEN] __attribute__ ((aligned (4)));

/*
#define RFS_CMKDIR(rnode, flags, pid, name) {				\
  [0] = 0x55, [1] = sizeof(rfs_cmkdir_t) - 4, [2] = 'M',		\
    U32LE_TO_U8(3, (rnode)),						\
    [7] = (flags), [8] = (pid),						\
    [9+RFS_MAX_NAME_LEN] = 'M' + (flags) + (pid) + SUM32((rnode)) + SUMSTR((u8_t *)name), \
    [10+RFS_MAX_NAME_LEN] = 0xAA }

#define RFS_CMKDIR_NAME(cmkdir) ((char *)&(((u8_t *)(cmkdir))[9]))
*/

// u8_t head; u8_t len; u8_t cid; u8_t errno; u8_t csum; u8_t tail;
typedef u8_t rfs_amkdir_t[6] __attribute__ ((aligned (4)));

/*
#define RFS_AMKDIR(errno) { [0] = 0x55, [1] = sizeof(rfs_amkdir_t) - 4, [2] = 'M', \
      [3] = (errno), [4] = 'M' + (status), [5] = 0xAA }

#define RFS_AMKDIR_ERRNO(amkdir) (amkdir)[3]
*/

////////////////////////////////////////////////////////////////////////////////
// lookup command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid; u8_t rnode[4]; u8_t pid; u8_t name[RFS_MAX_NAME_LEN], u8_t csum; u8_t tail;
typedef u8_t rfs_clookup_t[10+RFS_MAX_NAME_LEN] __attribute__ ((aligned (4)));

// u8_t head; u8_t len; u8_t cid; u8_t rnode[4]; u8_t type; u8_t csum; u8_t tail;
typedef u8_t rfs_alookup_t[10] __attribute__ ((aligned (4)));

#endif // _rfs_h_
