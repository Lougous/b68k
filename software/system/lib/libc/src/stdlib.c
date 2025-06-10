//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc - stdlib
//

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <types.h>
#include <syscall.h>
#include <errno.h>
#include <string.h>

#include "trap.h"
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

// source: https://en.wikipedia.org/wiki/Xorshift
struct xorshift32_state {
    uint32_t a;
};

struct xorshift32_state _libc_xorshift32_state;

static uint32_t _xorshift32(struct xorshift32_state *state)
{
	/* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
	uint32_t x = state->a;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return state->a = x;
}

void srand(unsigned int seed) {
  _libc_xorshift32_state.a = seed;
}

int rand(void)
{
  return _xorshift32(&_libc_xorshift32_state);
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
    if (*nptr >= '0' && *nptr <= '9') {
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
#define BRK_INCREMENT_MIN  4096

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

#define ALIGN32(u32) (((u32)+3) & ~3)

# if 0    // debug tool

// return the amount of free memory
int __mem_free()
{
  mem_chunk_head_t *pch = _ck_list;
  int free = 0;

  while (pch) {
    if (! (pch->len & _CK_ALLOCATED)) free += pch->len;
    
    pch = pch->next;
  }

  return free;
}

// display list of memory chunks
void mem_stat()
{
  mem_chunk_head_t *pch = _ck_list;

  while (pch) {
    printf(" @%06Xh len=%u, %s\n", pch, pch->len & ~_CK_ALLOCATED, pch->len & _CK_ALLOCATED ? "allocated" : "free");

    if (pch == pch->next) {
      printf(" infinite loop !\n");
      while(1);
    }
    
    pch = pch->next;
  }  
}

#endif

void __mem_init()
{
  // only one chunk to start
  _ck_list = (mem_chunk_head_t *)&__s_heap;
  _ck_list->len = ((int)&__e_heap - (int)&__s_heap) - sizeof(mem_chunk_head_t);
  _ck_list->next = 0;
}  

void __mem_add_chunk(void *chkp, int len)
{
  // align start address to 32-bits
  intptr_t startp = (intptr_t)chkp;

  while (startp & 3) {
    startp++;
    len--;
  }

  // align (round down) length to 32-bits
  len = len & ~3;
  
  // smallest aligned chunk size is 4 bytes
  if (len >= (sizeof(mem_chunk_head_t)+4)) {
    mem_chunk_head_t *new_list = (mem_chunk_head_t *)chkp;
    new_list->len = len-sizeof(mem_chunk_head_t);
    new_list->next = _ck_list;

    _ck_list = new_list;
  }
}
  

static mem_chunk_head_t *_find_chunk (size_t size)
{
  mem_chunk_head_t *best_pch = 0;
  size_t best_diff = 0;
  mem_chunk_head_t *pch = _ck_list;

  // try to find the best match, ie the closest in size free chunk
  while (pch) {
    int len = pch->len;

    //printf("@%Xh: %i\n", (unsigned int)pch, pch->len);
    
    if (! (len & _CK_ALLOCATED)) {
      // free chunk
      if (len == size) {
	// fit within exactly
	return pch;
      } else if (len > size) {
	if (best_pch) {
	  // better ?
	  if ((len - size) < best_diff) {
	    best_pch  = pch;
	    best_diff = len - size;
	  }
	} else {
	  // first match
	  best_pch  = pch;
	  best_diff = len - size;
	}
      }
    }

    // try next chunk
    pch = pch->next;
  }

  return best_pch;
}

void *malloc(size_t size)
{
  if (! size) return (void *) 0;
  
  size = ALIGN32(size);

  mem_chunk_head_t *best_pch = _find_chunk(size);
  size_t chunk_size = size + sizeof(mem_chunk_head_t);

  if (best_pch == NULL) {
    // need to expend process date section
    size_t new_size = chunk_size > BRK_INCREMENT_MIN ? chunk_size : BRK_INCREMENT_MIN;
    mem_chunk_head_t *new_ch = sbrk(new_size);

    if (new_ch != (void *) -1) {
      // success
      new_ch->len  = new_size - sizeof(mem_chunk_head_t);
      new_ch->next = NULL;

      // add free chunk at the end of the list
      mem_chunk_head_t *pch = _ck_list;

      while (pch->next) {
	pch = pch->next;
      }

      pch->next = new_ch;

      // select new chunk
      best_pch = new_ch;
    }
  }

  if (best_pch) {
    // candidate found
    mem_chunk_head_t *new_ch;
    
    if (best_pch->len == size) {
      // perfect fit
      new_ch = best_pch;

      new_ch->len |= _CK_ALLOCATED;
    } else {
      // free chunk will need to be split in 2 chunks (1 allocated, 1 free)
      // allocated chunk get second part
      new_ch = (void *)best_pch + (best_pch->len - size);
      new_ch->len  = size | _CK_ALLOCATED;
      new_ch->next = best_pch->next;

      // free chunk keep first part, shrunk
      best_pch->len -= chunk_size;
      best_pch->next = new_ch;
    }

    return ((void *)new_ch) + sizeof(mem_chunk_head_t);
  }

  // allocation failed
  errno = ENOMEM;
  return (void *) 0;
}

void free(void *ptr)
{
  if (ptr) {
    mem_chunk_head_t *ptr_h = ptr - sizeof(mem_chunk_head_t);
    mem_chunk_head_t *pch = _ck_list;
    mem_chunk_head_t *prev = 0;

    // walk the chunk list to find the one to free
    while (pch) {
      if (pch == ptr_h) {
	mem_chunk_head_t *pnext = pch->next;
	
	pch->len &= ~_CK_ALLOCATED;
	
	if (pnext && (!(pnext->len & _CK_ALLOCATED)) && ((intptr_t)pnext == (pch->len + sizeof(mem_chunk_head_t) + (intptr_t)pch))) {
	  // next chunk not allocated, merge
	  pch->len += pnext->len + sizeof(mem_chunk_head_t);
	  pch->next = pnext->next;
	}

	if (prev && (!(prev->len & _CK_ALLOCATED)) && ((intptr_t)pch == (prev->len + sizeof(mem_chunk_head_t) + (intptr_t)prev))) {
	  // previous chunk not allocated, merge
	  prev->len += pch->len + sizeof(mem_chunk_head_t);
	  prev->next = pch->next;
	}

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

////////////////////////////////////////////////////////////////////////////////
// environment variables
////////////////////////////////////////////////////////////////////////////////

int setenv(const char *name, const char *value, int overwrite)
{
  // check name
  if (! name) {
    errno = EINVAL;
    return -1;
  }
  
  if (! *name) {
    errno = EINVAL;
    return -1;
  }
  
  const char *pc = name;
  
  while (*pc) {
    if (*pc++ == '=') {
      errno = EINVAL;
      return -1;
    }
  }

  int n_len = (int)(pc - name);

  // already exists ?
  char **env_list = environ;
  char **env_entry = 0;
  short table_size = 0;

  if (environ) {
    while (*env_list) {
      if (strncmp(*env_list, name, n_len) == 0) {
	// already defined
	if (overwrite) {
	  env_entry = env_list;
	} else {
	  // do not change current value
	  return 0;
	}
      }

      env_list++;
      table_size++;
    }
  }

  int v_len = value ? strlen(value) : 0;

  if (! env_entry) {
    // does not exist yet
    // TODO: use realloc when available !
    // table size + one more + null pointer
    char **new_table = (char **)malloc((table_size + 2)*sizeof(char *));

    if (!new_table) {
      errno = ENOMEM;
      return -1;
    }

    if (table_size) {
      memcpy(new_table, environ, table_size*sizeof(char *));
    }

    // +1 byte for = and +1 byte for terminating null char 
    new_table[table_size] = (char *)malloc(n_len+1+v_len+1);

    if (!new_table[table_size]) {
      free(new_table);
      errno = ENOMEM;
      return -1;
    }

    new_table[table_size+1] = NULL;

    env_entry = &new_table[table_size];

    if (environ) {
      free(environ);
    }
    
    environ = new_table;
  } else {
    // already exists
    char *new_env = (char *)malloc(n_len+1+v_len+1);

    if (!new_env) {
      errno = ENOMEM;
      return -1;
    }
    
    free(*env_entry);
    *env_entry = new_env;
  }

  memcpy(*env_entry, name, n_len);
  (*env_entry)[n_len]='=';

  if (v_len) {
    memcpy((*env_entry) + n_len + 1, value, v_len);
  }

  (*env_entry)[n_len+1+v_len] = 0;

  return 0;
}
  
int unsetenv(const char *name)
{
  // check name
  if (! name) {
    errno = EINVAL;
    return -1;
  }
  
  if (! *name) {
    errno = EINVAL;
    return -1;
  }
  
  const char *pc = name;
  
  while (*pc) {
    if (*pc++ == '=') {
      errno = EINVAL;
      return -1;
    }
  }

  // walk environment list
  char **env_list = environ;
  int n_len = strlen(name);

  while (*env_list) {
    if (strncmp(*env_list, name, n_len) == 0) {
      // found
      break;
    }

    env_list++;
  }

  if (!(*env_list)) {
    // not found: success
    return 0;
  }

  // free matching varible, if any
  if (*env_list) {
    free(*env_list);
  }

  // remove from list
  // note: no memory is freed despite removing one entry in the table
  while (*env_list) {
    *env_list = env_list[1];
    env_list++;
  }

  // success
  return 0;
}

char *getenv(const char *name)
{
  char **env_list = environ;

  int n_len = strlen(name);

  while (*env_list) {
    if (strncmp(*env_list, name, n_len) == 0) {
      // defined
      return (*env_list) + n_len + 1;
    }

    env_list++;
  }

  // not found
  return NULL;
}
