
#ifndef _dirent_h_
#define _dirent_h_

#define DT_UNKNOWN  0
#define DT_REG      1
#define DT_DIR      2
#define DT_CHR      3
#define DT_BLK      4

struct dirent {
  u16_t d_size;
  u8_t  d_type;
  char  d_name[];
};

#endif /* _dirent_h_ */
