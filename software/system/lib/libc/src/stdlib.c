//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc - stdlib
//

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <types.h>
#include <syscall.h>

#include "trap.h"

void exit(int status)
{
  // TODO:
  //  - execute functions registered with atexit
  //  - stdio(3) streams are flushed and closed

  // use message to system task
  volatile message_t msg = { .type = EXIT,
			     .body.s32 = status };

  u32_t rval;

  // this system call should never returns !
  SENDRECEIVE(msg, 0, rval);

  (void)rval;  // prevent warning
  
  while(1);
}

long _c_holdrand;

void srand(unsigned int seed) {
  _c_holdrand = (long)seed;
}

int rand(void)
{
  return(((_c_holdrand = _c_holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}

int atoi(const char *nptr)
{
  int val = 0;
  char sign = 0;

  if (*nptr == '-') {
    sign = 1;
    nptr++;
  }


  while (*nptr) {
    if (*nptr > '0' && *nptr <= '9') {
      val = val*10 + (int)(*nptr - '0');
      nptr++;
    } else {
      return 0;
    }
  }

  return sign ? -val : val;
}

int abs(int j)
{
  if (j < 0) return -j;

  return j;
}

////////////////////////////////////////////////////////////////////////////////
// memory allocation
////////////////////////////////////////////////////////////////////////////////

typedef struct {
  int len;     // data chunk len (without header) + flags in LSB
  void *next;  // pointer to next data chunk header
  // data chunk is right after
} mem_chunk_head_t;

#define _CK_ALLOCATED  1

mem_chunk_head_t *_ck_list;

// linker script symbols at start/end of heap
extern char __s_heap;
extern char __e_heap;

void __mem_init()
{
  _ck_list = (mem_chunk_head_t *)&__s_heap;
  _ck_list->len = ((int)&__e_heap - (int)&__s_heap) - sizeof(mem_chunk_head_t);
  _ck_list->next = 0;
}  

#define ALIGN32(u32) (((u32)+3) & ~3)

void *malloc(size_t size)
{
  if (! size) return 0;
  
  mem_chunk_head_t *pch = _ck_list;

  // free chunk will need to be split in 2 chunks (1 allocated, 1 free)
  size = ALIGN32(size);
  size_t size_s = size + sizeof(mem_chunk_head_t);

  size_t best_diff = 0;
  mem_chunk_head_t *best_pch  = 0;

  // try to find the best match, ie the closest in size free chunk
  while (pch) {
    int len = pch->len;

    //printf("@%Xh: %i\n", (unsigned int)pch, pch->len);
    
    if (! (len & _CK_ALLOCATED)) {
      // free chunk
      if (len == size) {
	// fit within exactly
	best_pch  = pch;
	best_diff = 0;
	break;
      } else if (len >= size_s) {
	if (best_pch) {
	  // better ?
	  if ((len - size_s) < best_diff) {
	    best_pch  = pch;
	    best_diff = len - size_s;
	  }
	} else {
	  // first match
	  best_pch  = pch;
	  best_diff = len - size_s;
	}
      }
    }

    // try next chunk
    pch = pch->next;
  }
   
  //printf(" >%Xh\n", (unsigned int)best_pch);

  if (best_pch) {
    // candidate found
    mem_chunk_head_t *new_ch;
    
    if (! best_diff) {
      // perfect fit
      new_ch = best_pch;

      new_ch->len |= _CK_ALLOCATED;
    } else {
      // split
      // allocated chunk get second part
      new_ch = (void *)best_pch + (best_pch->len + sizeof(mem_chunk_head_t) - size_s);
      new_ch->len  = size | _CK_ALLOCATED;
      new_ch->next = best_pch->next;

      // free chunk keep first part, shrunk
      best_pch->len -= size_s;
      best_pch->next = new_ch;
    }

    return ((void *)new_ch) + sizeof(mem_chunk_head_t);
  }
  
  return 0;
}

void free(void *ptr)
{
  if (ptr) {
    mem_chunk_head_t *ptr_h = ptr - sizeof(mem_chunk_head_t);
    mem_chunk_head_t *pch = _ck_list;
    mem_chunk_head_t *prev = 0;

    while (pch) {
      if (pch == ptr_h) {
	mem_chunk_head_t *pnext = pch->next;
	
	if (pnext && (!(pnext->len & _CK_ALLOCATED))) {
	  // next chunk not allocated, merge
	  pch->len += pnext->len + sizeof(mem_chunk_head_t);
	  pch->next = pnext->next;
	}

	if (prev && (!(prev->len & _CK_ALLOCATED))) {
	  // previous chunk not allocated, merge
	  prev->len += pch->len + sizeof(mem_chunk_head_t);
	  prev->next = pch->next;
	}

	pch->len &= ~_CK_ALLOCATED;
	
	break;
      }

      // try next chunk
      prev = pch;
      pch = pch->next;
    }

    // pch shall be not null here
    //if (!pch) printf("free: bad address %Xh\n", (unsigned int)ptr);
  }

  return;
}
