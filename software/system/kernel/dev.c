
#include <types.h>

#include <string.h>
#include <stdio.h>

#include "config.h"
#include "debug.h"
#include "mem.h"
#include "proc.h"
#include "dev.h"

dev_t _dev_list[K_DEV_COUNT];

void dev_init (void)
{
  u32_t dev;
  
  for (dev = 0; dev < K_DEV_COUNT; dev++) {
    strcpy(_dev_list[dev].name, "none");
    _dev_list[dev].drv = 0;
    _dev_list[dev].handle = 0;
    _dev_list[dev].attr.attr_type = DEV_ATTR_NONE;
  }
}

dev_t *dev_get (const char *name)
{
  u32_t dev;

  for (dev = 0; dev < K_DEV_COUNT; dev++) {
    dev_t *pdev = &_dev_list[dev];

    if (strcmp(name, pdev->name) == 0) {
      return pdev;
    }
  }

  return 0;
}

char *dev_name (u32_t id)
{
  if ((id < K_DEV_COUNT) &&
      (_dev_list[id].attr.attr_type != DEV_ATTR_NONE)) {
    return &_dev_list[id].name[0];
  }

  return 0;  
}


static dev_t *_dev_register (const char *name, pid_t drv, void *handle)
{
  u32_t dev;

  // get empty entry
  for (dev = 0; dev < K_DEV_COUNT; dev++) {
    if (_dev_list[dev].drv == 0) {
      strcpy(_dev_list[dev].name, name);
      _dev_list[dev].drv = drv;
      _dev_list[dev].handle = handle;

      K_PRINTF(3, "%s: register device\n", name);
      break;
    }
  }

  if (dev == K_DEV_COUNT) {
    K_PRINTF(3, "error: cannot register device (out of memory)\n");
    k_panic();
    return 0;
  }

  return &_dev_list[dev];
}

dev_t *dev_register_char(const char *name, pid_t drv, void *handle)
{
  dev_t *pdev;

  if (! (pdev = _dev_register(name, drv, handle))) {
    return 0;
  }

  pdev->handle = handle;

  pdev->attr.chardev.attr_type = DEV_ATTR_CHAR;

  return pdev;
}
  
u32_t dev_register_disk(const char *name, pid_t drv, void *handle, u8_t *mbr)
{
  dev_t *pdev;
  u32_t pn;

  if (! (pdev = _dev_register(name, drv, handle))) {
    return 0;
  }

  pdev->handle = handle;
  
  pdev->attr.disk.attr_type   = DEV_ATTR_DISK;
  pdev->attr.disk.sector_size = 512;
  pdev->attr.disk.sector_cnt  = 0;  // TODO: need an ioctl

  K_PRINTF(2, "%s: disk\n", name);

  // executable marker
  if (mbr[510] != 0x55 || mbr[511] != 0xAA) return 1;
  
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

      pdev = _dev_register(xname, drv, handle);
      
      if (pdev) {
	pdev->attr.partition.attr_type    = DEV_ATTR_DISK_PARTITION;
	pdev->attr.partition.type         = type;
	pdev->attr.partition.sector_cnt   = nb_sec;
	pdev->attr.partition.sector_start = first_sec;

	K_PRINTF(1, "%s: type %Xh, start %x, %u sectors\n",
		 xname, type, first_sec, nb_sec);
      }
    }
  }
    
  return 0;
}
