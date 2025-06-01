//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/kernel - kernel header - device management
//

#ifndef _dev_h_
#define _dev_h_

#define DEV_ATTR_NONE            0
#define DEV_ATTR_DISK            1
#define DEV_ATTR_DISK_PARTITION  2
#define DEV_ATTR_CHAR            3

typedef u8_t dev_attr_type_t;

typedef struct {
  dev_attr_type_t attr_type;
  u32_t sector_size;
  u32_t sector_cnt;
} dev_attr_disk_t;

typedef struct {
  dev_attr_type_t attr_type;
  u8_t type;
  u32_t sector_cnt;
  u32_t sector_start;
} dev_attr_disk_partition_t;

typedef struct {
  dev_attr_type_t attr_type;
} dev_attr_char_t;

typedef union {
  dev_attr_type_t           attr_type;
  dev_attr_disk_t           disk;
  dev_attr_disk_partition_t partition;
  dev_attr_char_t           chardev;
} dev_attr_t;

struct dev {
  char name[8];
  dev_t dev_id;
  dev_attr_t attr;
};


extern void dev_init (void);
extern int dev_register_char (const char *name, pid_t maj, u16_t min);
extern int dev_register_disk (const char *name, pid_t maj, u16_t min, u8_t *mbr);
extern struct dev *dev_get(const char *name);
extern struct dev *dev_get_by_id(dev_t dev);
extern char *dev_name (u16_t id);

#endif /* _dev_h_ */
