
#include <types.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "mem.h"
#include "proc.h"
#include "lock.h"
#include "config.h"
#include "b68k.h"
#include "debug.h"

// allocation unit
#define _K_MEM_BLOCK_SIZE       4096
#define _K_MEM_BLOCK_SIZE_BITS  12
// reserved block for system
// shall match with MEMORY section of linker script
// user process virtual address starts just after these block
#define _K_MEM_RSVD_BLOCKS      48    // 64kiB code + 128kiB data

/* blocks status */
pid_t _blk_sts[K_MEM_BLOCK_COUNT - _K_MEM_RSVD_BLOCKS];

u16_t _last_base;
u16_t _last_msk;

void mem_init (void)
{
  K_PRINTF(0, "memory size: %u KiB\n", K_MEM_BLOCK_COUNT*_K_MEM_BLOCK_SIZE/1024);  

  memset(_blk_sts, 0, sizeof(_blk_sts));

  _last_base = 0;
  _last_msk = 0;

  K_PRINTF(2, "%u blocks available\n", K_MEM_BLOCK_COUNT - _K_MEM_RSVD_BLOCKS);  
}

static u16_t _mem_blocks (u32_t size)
{
  u16_t nblk = 1;

  if (size) {
    size = (size - 1) >>_K_MEM_BLOCK_SIZE_BITS;

    while (size) {
      size >>= 1;
      nblk <<= 1;
    }
  }

  return nblk;
}

// allocate memory blocks for process pid
mem_pa_t mem_realloc (pid_t pid, mem_pa_t addr, u32_t old_sz, u32_t new_sz, u16_t options)
{
  u16_t new_cnt = _mem_blocks(new_sz);
  u16_t blk, sblk;
  u16_t ok = 0;

  for (blk = 0; blk < K_MEM_BLOCK_COUNT; blk += new_cnt) {
    if (blk >= _K_MEM_RSVD_BLOCKS) {
      ok = 1;
      
      for (sblk = blk; sblk < blk + new_cnt; sblk++) {
	if (_blk_sts[sblk] != pid && _blk_sts[sblk] > 0) {
	  ok = 0;
	  break;
	}
      }

      if (ok) break;
    }
  }

  if (ok) {
    mem_pa_t new_addr = blk << _K_MEM_BLOCK_SIZE_BITS;
    
    if (! addr) {
      // first allocation
      for (sblk = blk; sblk < blk + new_cnt; sblk++) {
	_blk_sts[sblk] = pid;
      }
    } else if (addr != new_addr) {
      // reallocation and need to move data
      memcpy((void *)new_addr,
	     (void *)addr,
	     old_sz);
    }

    K_PRINTF(3, "PID %i: base address %Xh, size %ukiB\n", pid, new_addr, (new_cnt << _K_MEM_BLOCK_SIZE_BITS) >> 10);
    
    return new_addr;
  }

  // not able to find memory segment
  return 0;
}

void mem_free(mem_pa_t addr, u32_t sz)
{
  u16_t cnt = _mem_blocks(sz);
  u16_t blk, sblk;

  K_PRINTF(3, "mem: free %u bytes (%u blocks) at %Xh\n", sz, cnt, addr);
    
  /* first block */
  sblk = addr >> _K_MEM_BLOCK_SIZE_BITS;

  for (blk = 0; blk < cnt; blk++) {
    _blk_sts[sblk+blk] = 0;
  }
}

void mem_setup_mmu (mem_pa_t addr, u32_t sz) {

  // B68K_MMU registers: in bits 15..8 only !

  //K_PRINTF(2, "+ MMU %X %X\n", addr, sz);
  
  u32_t base = addr >> (_K_MEM_BLOCK_SIZE_BITS - 8);
  
  B68K_MMU->base_lo = base & 0xff00;
  B68K_MMU->base_hi = base >> 8;

  u16_t mask = 0;
  sz = (sz - 1) >> _K_MEM_BLOCK_SIZE_BITS;

  while (sz) {
    sz >>= 1;
    mask = (mask << 1) + 1;
  }
  
  B68K_MMU->mask_lo = mask << 8;
  B68K_MMU->mask_hi = mask & 0xff00;

  //  if (base != _last_base || mask != _last_msk) {
  //    K_PRINTF(3, "MMU: %Xh, %Xh\n", base, mask);
  //    _last_base = base;
  //    _last_msk = mask;
  //  }

}
