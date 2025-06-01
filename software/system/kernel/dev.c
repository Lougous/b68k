//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/kernel - device manager
//

#include <types.h>

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/sysmacros.h>

#include "config.h"
#include "debug.h"
#include "mem.h"
#include "proc.h"
#include "dev.h"

struct dev _dev_list[K_DEV_COUNT];

void dev_init (void)
{
  u32_t dev;
  
  for (dev = 0; dev < K_DEV_COUNT; dev++) {
    strcpy(_dev_list[dev].name, "none");
    _dev_list[dev].dev_id = 0;
    _dev_list[dev].attr.attr_type = DEV_ATTR_NONE;
  }
}

struct dev *dev_get (const char *name)
{
  u16_t dev;

  for (dev = 0; dev < K_DEV_COUNT; dev++) {
    struct dev *pdev = &_dev_list[dev];

    if (strcmp(name, pdev->name) == 0) {
      return pdev;
    }
  }

  return 0;
}

struct dev *dev_get_by_id(dev_t dev)
{
  u16_t i;

  for (i = 0; i < K_DEV_COUNT; i++) {
    struct dev *pdev = &_dev_list[i];

    if (pdev->dev_id == dev) {
      return pdev;
    }
  }

  return 0;
}
  

char *dev_name (u16_t id)
{
  if ((id < K_DEV_COUNT) &&
      (_dev_list[id].attr.attr_type != DEV_ATTR_NONE)) {
    return &_dev_list[id].name[0];
  }

  return 0;  
}


static struct dev *_dev_register (const char *name, u16_t major, u16_t minor)
{
  u32_t dev;
  dev_t dev_id = makedev(major, minor);

  {
    u16_t lbkp = k_lock();
  
    // get empty entry
    for (dev = 0; dev < K_DEV_COUNT; dev++) {
      if (_dev_list[dev].dev_id == 0) {
	strcpy(_dev_list[dev].name, name);
	_dev_list[dev].dev_id = dev_id;

	break;
      }
    }
    k_unlock(lbkp);
  }

  if (dev == K_DEV_COUNT) {
    K_PRINTF(0, "dev: error: cannot register device (out of memory)\n");
    k_panic();
    return 0;
  }

  K_PRINTF(1, "dev: %s: register device %Xh\n", name, dev_id);
  
  return &_dev_list[dev];
}

int dev_register_char(const char *name, pid_t maj, u16_t min)
{
  struct dev *pdev;

  if (! (pdev = _dev_register(name, maj, min))) {
    return -1;
  }

  pdev->attr.chardev.attr_type = DEV_ATTR_CHAR;

  return 0;
}
  
int dev_register_disk(const char *name, pid_t maj, u16_t min, u8_t *mbr)
{
  struct dev *pdev;
  u32_t pn;

  if (! (pdev = _dev_register(name, maj, min++))) {
    return -1;
  }

  pdev->attr.disk.attr_type   = DEV_ATTR_DISK;
  pdev->attr.disk.sector_size = 512;
  pdev->attr.disk.sector_cnt  = 0;  // TODO: need an ioctl

  //  K_PRINTF(2, "dev: %s: disk\n", name);

  // executable marker
  if (mbr[510] != 0x55 || mbr[511] != 0xAA) return -1;
  
  // check partitions (up to four primary)
  for (pn = 0; pn < 4; pn++) {
    //u8_t active = mbr[pn*16+0x1be];
    //u8_t beg_head = mbr[p*16+ 0x1be + 1];
    //u16_t beg_cylsec = *((u16_t *)&mbr[p*16 + 0x1be + 2]);   // little endian
    u8_t type = mbr[pn*16 + 0x1be + 4];
    //u8_t end_head = mbr[p*16+ 0x1be + 5];
    //u16_t end_cylsec = *((u16_t *)&mbr[p*16 + 0x1be + 6]);   // little endian
    u32_t first_sec =   // little endian
      (u32_t)mbr[pn*16 + 0x1be + 8] +
      ((u32_t)mbr[pn*16 + 0x1be + 9] << 8) +
      ((u32_t)mbr[pn*16 + 0x1be + 10] << 16) +
      ((u32_t)mbr[pn*16 + 0x1be + 11] << 24);
    u32_t nb_sec =   // little endian
      (u32_t)mbr[pn*16 + 0x1be + 12] +
      ((u32_t)mbr[pn*16 + 0x1be + 13] << 8) +
      ((u32_t)mbr[pn*16 + 0x1be + 14] << 16) +
      ((u32_t)mbr[pn*16 + 0x1be + 15] << 24);

    if (type) {
      char xname[8];

      strcpy(xname, name);
      xname[strlen(name)] = '0' + pn;
      xname[strlen(name)+1] = 0;

      pdev = _dev_register(xname, maj, min++);
      
      if (pdev) {
	pdev->attr.partition.attr_type    = DEV_ATTR_DISK_PARTITION;
	pdev->attr.partition.type         = type;
	pdev->attr.partition.sector_cnt   = nb_sec;
	pdev->attr.partition.sector_start = first_sec;

	K_PRINTF(1, "dev: %s: type %Xh, start %x, %u sectors\n",
		 xname, type, first_sec, nb_sec);
      }
    }
  }
    
  return 0;
}
