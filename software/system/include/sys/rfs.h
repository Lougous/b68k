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
#define RFS_MAX_NAME_LEN  16

////////////////////////////////////////////////////////////////////////////////
// supporting functions/macros
////////////////////////////////////////////////////////////////////////////////
#define SUM32(w32) (((char *)&(w32))[0] + ((char *)&(w32))[1] + ((char *)&(w32))[2] + ((char *)&(w32))[3])

#define U32LE_TO_U8(at, u32) [at] = (u32), [at+1] = ((u32) >> 8), [at+2] = ((u32) >> 16), [at+3] = ((u32) >> 24)

#define U8_TO_U32LE(at, u8array) ((u32_t)(u8array)[at] + ((u32_t)(u8array)[at+1] << 8) + ((u32_t)(u8array)[at+2] << 16) + ((u32_t)(u8array)[at+3] << 24))

static u8_t SUMSTR(u8_t *name)
{
  u8_t sum = 0;

  while (*name) {
    sum += *name++;
  }

  return sum;
}

////////////////////////////////////////////////////////////////////////////////
// reset command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid; u8_t csum; u8_t tail;
typedef u8_t rfs_creset_t[5];

#define RFS_CRESET() { [0] = 0x55, [1] = sizeof(rfs_creset_t) - 4, [2] = 'I', [3] = 'I', [4] = 0xAA }
    
// u8_t head; u8_t len; u8_t cid; u8_t root[4]; u8_t csum; u8_t tail;
typedef u8_t rfs_areset_t[9];

#define RFS_ARESET(root) { [0] = 0x55, [1] = sizeof(rfs_creset_t) - 4, [2] = 'I', \
      U32LE_TO_U8(3, root), [7] = 'I' + SUM32((root), [8] = 0xAA }

#define RFS_ARESET_ROOT(areset) U8_TO_U32LE(3, (areset))

////////////////////////////////////////////////////////////////////////////////
// open command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid; u8_t rnode[4]; u8_t flags; u8_t pid; u8_t csum; u8_t tail;
typedef u8_t rfs_copen_t[11];

#define RFS_COPEN(rnode, flags, pid) { [0] = 0x55, [1] = sizeof(rfs_copen_t) - 4, [2] = 'O', \
      U32LE_TO_U8(3, (rnode)),						\
      [7] = (flags), [8] = (pid),					\
      [9] = 'O' + (flags) + (pid) + SUM32((rnode)),			\
      [10] = 0xAA }

// u8_t head; u8_t len; u8_t cid; u8_t status; u8_t size[4]; u8_t csum; u8_t tail;
typedef u8_t rfs_aopen_t[10];

#define RFS_AOPEN(status, size) { [0] = 0x55, [1] = sizeof(rfs_aopen_t) - 4, [2] = 'O', \
      [3] = (status), U32LE_TO_U8(4, (size)),				\
      [8] = 'O' + (status) + SUM32((size)),				\
      [9] = 0xAA }

#define RFS_AOPEN_STATUS(aopen) (aopen)[3]
#define RFS_AOPEN_SIZE(aopen) U8_TO_U32LE(4, (aopen))

////////////////////////////////////////////////////////////////////////////////
// close command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid; u8_t rnode[4]; u8_t csum; u8_t tail;
typedef u8_t rfs_cclose_t[9];

#define RFS_CCLOSE(rnode) { [0] = 0x55, [1] = sizeof(rfs_cclose_t) - 4, [2] = 'C', \
      U32LE_TO_U8(3, (rnode)), [7] = 'C' + SUM32((rnode)), [8] = 0xAA }

// u8_t head; u8_t len; u8_t cid; u8_t status; u8_t csum; u8_t tail;
typedef u8_t rfs_aclose_t[6];

#define RFS_ACLOSE(status) { [0] = 0x55, [1] = sizeof(rfs_aclose_t) - 4, [2] = 'C', \
    [3] = (status), [4] = 'C' + (status), [5] = 0xAA }

#define RFS_ACLOSE_STATUS(aclose) (aclose)[3]

////////////////////////////////////////////////////////////////////////////////
// read command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid; u8_t len; u8_t rnode[4]; u8_t lseek[4]; u8_t csum; u8_t tail;
typedef u8_t rfs_cread_t[14];

#define RFS_CREAD(len, rnode, lseek) { [0] = 0x55, [1] = sizeof(rfs_copen_t) - 4, [2] = 'R', \
      [3] = (len), U32LE_TO_U8(4, (rnode)), U32LE_TO_U8(8, (lseek)),	\
      [12] = 'R' + (len) + SUM32((rnode)) + SUM32((lseek)),		\
      [13] = 0xAA }

// u8_t head; u8_t len; u8_t cid; u8_t blen; u8_t buf[blen]; u8_t csum; u8_t tail;
#define RFS_AREAD_LEN_NO_DATA  6

#define RFS_AREAD_BLEN(buf) ((u8_t *)(buf))[3]
#define RFS_AREAD_BUF(buf) &(((u8_t *)(buf))[4])

////////////////////////////////////////////////////////////////////////////////
// getdents command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid; u8_t rnode[4]; u8_t dpos, u8_t csum; u8_t tail;
typedef u8_t rfs_cgetdents_t[10];

#define RFS_CGETDENTS(rnode, dpos) { [0] = 0x55, [1] = sizeof(rfs_copen_t) - 4, [2] = 'D', \
      U32LE_TO_U8(3, (rnode)),						\
      [7] = (dpos), [8] = 'D' + SUM32((rnode)),				\
      [9] = 0xAA }

// u8_t head; u8_t len; u8_t cid; u8_t type; u8_t buf[RFS_AGETDENTS_MAX_NAME_LEN]; u8_t csum; u8_t tail;
typedef u8_t rfs_agetdents_t[6+RFS_MAX_NAME_LEN];

#define RFS_AGETDENTS_TYPE_REG  1
#define RFS_AGETDENTS_TYPE_DIR  2

#define RFS_AGETDENTS_TYPE(agetdents) ((u8_t *)(agetdents))[3]
#define RFS_AGETDENTS_BUF(agetdents) ((u8_t *)&(((u8_t *)(agetdents))[4]))

////////////////////////////////////////////////////////////////////////////////
// mkdir command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid; u8_t rnode[4]; u8_t flags; u8_t pid; u8_t name[RFS_MAX_NAME_LEN], u8_t csum; u8_t tail;
typedef u8_t rfs_cmkdir_t[11+RFS_MAX_NAME_LEN];

#define RFS_CMKDIR(rnode, flags, pid, name) {				\
  [0] = 0x55, [1] = sizeof(rfs_cmkdir_t) - 4, [2] = 'M',		\
    U32LE_TO_U8(3, (rnode)),						\
    [7] = (flags), [8] = (pid),						\
    [9+RFS_MAX_NAME_LEN] = 'M' + (flags) + (pid) + SUM32((rnode)) + SUMSTR((u8_t *)name), \
    [10+RFS_MAX_NAME_LEN] = 0xAA }

#define RFS_CMKDIR_NAME(cmkdir) ((char *)&(((u8_t *)(cmkdir))[9]))

// u8_t head; u8_t len; u8_t cid; u8_t errno; u8_t csum; u8_t tail;
typedef u8_t rfs_amkdir_t[6];

#define RFS_AMKDIR(errno) { [0] = 0x55, [1] = sizeof(rfs_amkdir_t) - 4, [2] = 'M', \
      [3] = (errno), [4] = 'M' + (status), [5] = 0xAA }

#define RFS_AMKDIR_ERRNO(amkdir) (amkdir)[3]

////////////////////////////////////////////////////////////////////////////////
// lookup command
////////////////////////////////////////////////////////////////////////////////
// u8_t head; u8_t len; u8_t cid; u8_t rnode[4]; u8_t name[RFS_MAX_NAME_LEN], u8_t csum; u8_t tail;
typedef u8_t rfs_clookup_t[10+RFS_MAX_NAME_LEN];

#define RFS_CLOOKUP(rnode, pid, name) {					\
  [0] = 0x55, [1] = sizeof(rfs_clookup_t) - 4, [2] = 'L',		\
    U32LE_TO_U8(3, (rnode)), [7] = (pid),				\
    [8+RFS_MAX_NAME_LEN] = 'L' + SUM32((rnode)) + SUMSTR((u8_t *)name), \
    [9+RFS_MAX_NAME_LEN] = 0xAA }

#define RFS_CLOOKUP_NAME(clookup) ((char *)&(((u8_t *)(clookup))[7]))

// u8_t head; u8_t len; u8_t cid; u8_t rnode[4]; u8_t type; u8_t csum; u8_t tail;
typedef u8_t rfs_alookup_t[8];

#define RFS_ALOOKUP(rnode, type) { [0] = 0x55, [1] = sizeof(rfs_alookup_t) - 4, [2] = 'L', \
      U32LE_TO_U8(3, (rnode)), [7] = (type),				\
      [8] = 'L' + (status) + SUM32((size)), [9] = 0xAA }

#define RFS_ALOOKUP_TYPE(alookup) (alookup)[7]
#define RFS_ALOOKUP_RNODE(alookup) U8_TO_U32LE(3, (alookup))

#endif // _rfs_h_
