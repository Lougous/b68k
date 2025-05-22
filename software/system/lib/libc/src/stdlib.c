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

void __mem_init()
{
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

  // align (round down) length
  len = len & ~3;
  
  // smallest aligned chunk size is 4 bytes
  if (len >= (sizeof(mem_chunk_head_t)+4)) {
    mem_chunk_head_t *new_list = (mem_chunk_head_t *)chkp;
    new_list->len = len-sizeof(mem_chunk_head_t);
    new_list->next = _ck_list;

    _ck_list = new_list;
  }
}
  
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

# if 0
// debug tool
void mem_stat()
{
  mem_chunk_head_t *pch = _ck_list;

  while (pch) {
    printf(" @%06Xh len=%u, %s\n", pch, pch->len & ~_CK_ALLOCATED, pch->len & _CK_ALLOCATED ? "allocated" : "free");
    
    pch = pch->next;
  }  
}
#endif

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

  // allocation failed
  errno = ENOMEM;
  return NULL;
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
	
	pch->len &= ~_CK_ALLOCATED;
	
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
