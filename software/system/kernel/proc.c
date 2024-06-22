
#include <types.h>
#include <stddef.h>
#include <fcntl.h>
#include <dirent.h>
#include <syscall.h>

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "mem.h"
#include "proc.h"
#include "lock.h"
#include "msg.h"
#include "debug.h"

#include "b68k.h"

extern void k_restart (void);  // TODO header
extern void k_yield (void);  // TODO header

struct proc_desc_t {
  // CPU registers
  // !!! coherency with srt0.s !!!
  // and with movem instruction
  u32_t d[8];
  u32_t a[8];
  u32_t pc;    // 64
  u16_t sr;    // 68

  // interrupt context (bus/address error)
  u16_t x_status;  // 70
  u32_t x_addr;    // 72
  u16_t x_instr;   // 76

  // MFP registers
  u8_t mfp_ad;   // 78
  
  // process state
  proc_state_t state;
  s32_t exit_val;

  // IPC
  pid_t m_from;
  pid_t m_to;
  message_t *pa_msg; // physical address

  // signals
  u16_t sig_mask;
  
  // process attributes
  pid_t ppid;  // parent process ID
  uid_t uid;   // user ID
  char  name[K_PROC_NAME_MAX_LEN];

  // memory
  mem_pa_t mem_ad;
  u32_t    mem_sz;
};

struct proc_list_t {
  struct proc_desc_t d;

  // process ID
  pid_t pid;
  // linkage for chained list
  struct proc_list_t *next, *prev;
};
  
/* process data */
struct proc_list_t _proc_table[K_PROC_COUNT];

/* chained list of ready processes */
struct proc_list_t *_proc_ready;

/* chained list of unused processes */
struct proc_list_t *_proc_free;

/* pointer to process storage to save context at */
struct proc_list_t *_proc_ptr;

/* idle process */
u16_t idle_stack[256];
 
void idle_task (void)
{
  while(1);
}

u32_t proc_init (void)
{
  int p;

  for (p = 0; p < K_PROC_COUNT; p++) {
    memset(&_proc_table[p], 0, sizeof(struct proc_list_t));
    _proc_table[p].d.state = PROC_STATE_NONE;
    _proc_table[p].pid = p;
    _proc_table[p].next = &_proc_table[p + 1];
    _proc_table[p].prev = &_proc_table[p - 1];
  }

  _proc_table[K_PROC_COUNT - 1].next = &_proc_table[0];
  _proc_table[0].prev = &_proc_table[K_PROC_COUNT - 1];

  _proc_free = &_proc_table[0];
  _proc_ready = NULL;
  _proc_ptr = NULL;

  /* create idle task */
  proc_create_task (K_PROC_IDLE, "idle", idle_task, idle_stack, sizeof(idle_stack));

  return 0;
}

pid_t proc_get_pid(char *name)
{
  int p;

  for (p = 0; p < K_PROC_COUNT; p++) {
    if (_proc_table[p].d.state != PROC_STATE_NONE && strcmp(name, _proc_table[p].d.name) == 0) {
      return p;
    }
  }

  return PROC_PID_NONE;
}

pid_t proc_current (void)
{
  return _proc_ptr->pid;
}

proc_state_t proc_get_state (pid_t pid)
{
  return _proc_table[pid].d.state;  
}

static void _proc_add_to_ready_list (struct proc_list_t *proc)
{
  /* add process to ready list */
  if (_proc_ready) {
    proc->prev = _proc_ready;
    proc->next = _proc_ready->next;
    _proc_ready->next->prev = proc;
    _proc_ready->next = proc;
  } else {
    /* no ready process yet */
    _proc_ready = proc;
    proc->prev = proc;
    proc->next = proc;
  }
}

static void _proc_remove_from_ready_list (struct proc_list_t *proc)
{
  if (proc->prev == proc) {
    /* last one in ready list */
    K_PRINTF(3, "fatal error: READY process list empty !\n");
    k_panic();
  }

  proc->next->prev = proc->prev;
  proc->prev->next = proc->next;

  if (_proc_ready == proc) {
    _proc_ready = proc->next;
  }

  proc->prev = NULL;
  proc->next = NULL;
}

static void _proc_add_to_free_list (struct proc_list_t *proc)
{
  /* add process to free list */
  if (_proc_free) {
    proc->prev = _proc_free;
    proc->next = _proc_free->next;
    _proc_free->next->prev = proc;
    _proc_free->next = proc;
  } else {
    /* no free process yet */
    _proc_free = proc;
    proc->prev = proc;
    proc->next = proc;
  }
}

static void _proc_remove_from_free_list (struct proc_list_t *proc)
{
  if (proc->prev == proc) {
    /* last one in free list */
    _proc_free = NULL;
  } else {
    proc->next->prev = proc->prev;
    proc->prev->next = proc->next;
    _proc_free = proc->next;
  }
}

pid_t proc_create_task (pid_t p, char *name, void *pc, void *stk, u32_t stk_sz)
{
  u16_t lbkp = k_lock();

  struct proc_list_t *proc;
  
  if (p == PROC_PID_ANY) {

    if (_proc_free) {
      proc = _proc_free;

    } else {
      /* no more process available */
      p = PROC_PID_NONE;
      goto exit_lbl;
    }
  } else {
    proc = &_proc_table[p];
    
    if (proc->d.state != PROC_STATE_NONE) {
      p = PROC_PID_NONE;
      goto exit_lbl;
    }
  }

  /* that process has to be in free list, remove from it */
  _proc_remove_from_free_list(proc);
  
  /* add process to ready list */
  _proc_add_to_ready_list(proc);

  /* init process structure */
  memset(&proc->d, 0, sizeof(struct proc_desc_t));
  
  proc->d.pc       = (u32_t)pc;
  proc->d.a[7]     = (u32_t)stk + stk_sz;
  proc->d.sr       = 0x2000;  // supervisor
  proc->d.sig_mask = 0;
  proc->d.ppid     = 0;
  proc->d.uid      = PROC_UID_KERNEL;
  strncpy(proc->d.name, name, K_PROC_NAME_MAX_LEN);
  proc->d.state    = PROC_STATE_READY;
  proc->d.mem_ad   = 0;
  proc->d.mem_sz   = 0xffffff;

  //  K_PRINTF(3, "task %s created (%i)\n", name, proc->pid);

 exit_lbl:
  k_unlock(lbkp);

  return p;
}

/* first user space process to create. further user processes will be created through fork calls */
pid_t proc_create_init (const char *name, u32_t size)
{
  u16_t lbkp = k_lock();

  struct proc_list_t *proc = _proc_free;

  if (proc == NULL) {
    /* no more process available */
    k_unlock(lbkp);
    return PROC_PID_NONE;
  }

  /* remove process from free list */
  _proc_remove_from_free_list(proc);

  /* do not add process to ready list, it is created BLOCKED */
 
  /* init process structure */
  memset(&proc->d, 0, sizeof(struct proc_desc_t));

  proc->d.sr        = 0x0000;         // user mode
  proc->d.sig_mask  = 0;
  proc->d.ppid      = 0;
  proc->d.uid       = PROC_UID_ROOT;  // user is root
  strncpy(proc->d.name, name, K_PROC_NAME_MAX_LEN);
  proc->d.m_from    = PROC_PID_NONE;
  proc->d.m_to      = PROC_PID_NONE;
  proc->d.mem_ad    = mem_realloc(proc->pid, 0, 0, size, 0);
  proc->d.mem_sz    = size;
  proc->d.state     = PROC_STATE_BLOCKED;  // will wait for a signal
  
  k_unlock(lbkp);

  return proc->pid;
}

pid_t proc_fork (pid_t ppid)
{
  u16_t lbkp = k_lock();

  struct proc_list_t *proc = _proc_free;

  if (proc == NULL) {
    /* no more process available */
    k_unlock(lbkp);
    return PROC_PID_NONE;
  }

  /* remove process from free list */
  _proc_remove_from_free_list(proc);

  /* copy process structure */
  memcpy(&proc->d, &_proc_table[ppid].d, sizeof(struct proc_desc_t));

  /* change PPID */
  proc->d.ppid = ppid;

  /* setup memory space */
  mem_pa_t ad = mem_realloc(proc->pid, 0 /* 1st allocation */, 0, _proc_table[ppid].d.mem_sz, 0);

  if (ad) {
    proc->d.mem_ad = ad;
    proc->d.mem_sz = _proc_table[ppid].d.mem_sz;

    /* copy parent process memory */
    memcpy((void *)ad, (void *)_proc_table[ppid].d.mem_ad, _proc_table[ppid].d.mem_sz);

    /* new process is blocked until reception of syscall message answer */
    proc->d.state = PROC_STATE_BLOCKED;
  
    /* fix message address according to allocated address space */
    proc->d.pa_msg = _proc_table[ppid].d.pa_msg - _proc_table[ppid].d.mem_ad + ad;
    
    k_unlock(lbkp);
    
    return proc->pid;
  }

  k_unlock(lbkp);

  /* coudn't allocate memory for new process */
  return PROC_PID_NONE;
}

void proc_set_name (pid_t pid, char *name)
{
  u16_t lbkp = k_lock();
  strncpy(_proc_table[pid].d.name, name, K_PROC_NAME_MAX_LEN);
  k_unlock(lbkp);
}

void proc_set_uid (pid_t pid, uid_t new_uid)
{
  u16_t lbkp = k_lock();
  if (_proc_table[pid].d.uid == PROC_UID_KERNEL || _proc_table[pid].d.uid == PROC_UID_ROOT) {
    _proc_table[pid].d.uid = new_uid;
  }
  k_unlock(lbkp);
}
  
void proc_kill (pid_t pid)
{
  u16_t lbkp = k_lock();

  struct proc_list_t *pp = &_proc_table[pid];

  if (pp->d.uid != PROC_UID_KERNEL) {
    // free allocated memory
    mem_free(pp->d.mem_ad, pp->d.mem_sz);
  }

  if (pp->d.state == PROC_STATE_READY) {
    /* remove from ready list */
    _proc_remove_from_ready_list(pp);
  }
  
  /* free process */
  pp->d.state = PROC_STATE_NONE;

  /* add to free list */
  _proc_add_to_free_list(pp);
  
  k_unlock(lbkp);

  return;
}

//void proc_block()
//{
//  struct proc_list_t *pp = _proc_ptr;
//  _proc_remove_from_ready_list(pp);
//  pp->d.state = PROC_STATE_BLOCKED;
//}

void proc_exit(pid_t pid, s32_t eval)
{
  u16_t lbkp = k_lock();

  struct proc_list_t *pp = &_proc_table[pid];

  /* processes request exit via system call message and must be blocked here */

  /* => stopped */
  pp->d.exit_val = eval;

  if (pp->d.state == PROC_STATE_READY) {
    _proc_remove_from_ready_list((struct proc_list_t *)pp);
  }
  
  pp->d.state = PROC_STATE_STOPPED;

  /* warn parent process */
  proc_sig(pp->d.ppid, SIGCHILD);

  k_unlock(lbkp);
}

void copy_to_user(pid_t pid, mem_va_t dst, mem_pa_t src, int len) {
  // note: copy_to_user not used for any system task
  mem_pa_t pdst = dst + _proc_table[pid].d.mem_ad;

  memcpy((void *)pdst, (void *)src, len);
}

mem_pa_t va_to_pa(pid_t pid, mem_va_t dst, int len)
{
  struct proc_desc_t *pptr = &_proc_table[pid].d;
  
  if ((dst + len) > pptr->mem_sz) {
    return 0;
  }
  
  return dst + pptr->mem_ad;
}

#define ALIGN32(a) ((((u32_t)a) + 3) & ~((u32_t)3))

// TODO: change memory allocation BEFORE to start loading data:
// in case of insufficent memory the process shall return to prior state 
int proc_exec(pid_t pid, int argc, const char *argv[], char *pa_argbuf, u16_t argbuflen)
{
  static u8_t pa_buf[1024];
  message_t msg;

  int fd = sendreceive_vfs_open(&msg, argv[0], O_RDONLY);

  if (fd < 0) {
    K_PRINTF(2, "EXEC: %s: cannot open file\n", argv[0]);
    return -1;
  }
  
  /* read ELF header */
  if (sendreceive_vfs_read(&msg, fd, &pa_buf[0], 0x34) != 0x34) {
    goto exit_error;
  }

  /* check magic */
  if ((pa_buf[0] != 0x7f) ||
      (pa_buf[1] != 0x45) ||
      (pa_buf[2] != 0x4c) ||
      (pa_buf[3] != 0x46)) {
    K_PRINTF(3, "not an ELF file\n");
    goto exit_error;
  }

  /* check class */
  if (pa_buf[4] != 1) {
    K_PRINTF(3, "not an 32-bits ELF file\n");
    goto exit_error;
  }
  
  /* check endianness */
  if (pa_buf[5] != 2) {
    K_PRINTF(3, "not a big endian ELF file\n");
    goto exit_error;
  }

  /* check type */
  if (*((u16_t *)&pa_buf[16]) != 2) {
    K_PRINTF(3, "not an executable ELF file\n");
    goto exit_error;
  }

  /* check ISA */
  if (*((u16_t *)&pa_buf[18]) != 4) {
    K_PRINTF(3, "not a Motorola 68000 ISA ELF file\n");
    goto exit_error;
  }

  u32_t e_entry = *((u32_t *)&pa_buf[0x18]);
  u32_t e_phoff = *((u32_t *)&pa_buf[0x1C]);
  u16_t e_phnum = *((u16_t *)&pa_buf[0x2C]);
  u16_t ph;

  //  K_PRINTF(3, "%s: %u program entries:\n", argv[0], e_phnum);

  struct proc_desc_t *pp = &_proc_table[pid].d;

  for (ph = 0; ph < e_phnum; ph++) {

    /* read program header */
    sendreceive_vfs_lseek(&msg, fd, e_phoff + 0x20*ph, SEEK_SET);
    
    if (sendreceive_vfs_read(&msg, fd, &pa_buf[0], 0x20) != 0x20) goto exit_error;

    u32_t p_type   = *((u32_t *)&pa_buf[0]);
    u32_t p_offset = *((u32_t *)&pa_buf[4]);   // offset in file
    u32_t p_vaddr  = *((u32_t *)&pa_buf[8]);   // virtual address
    u32_t p_filesz = *((u32_t *)&pa_buf[16]);  // size in file
    u32_t p_memsz  = *((u32_t *)&pa_buf[20]);  // size in memory

    //    K_PRINTF(3, " #%u: type %x, @%Xh +%Xh (@%Xh +%Xh)\n", ph, p_type, p_vaddr, p_memsz, p_offset, p_filesz);

   if ((p_vaddr + p_memsz) > pp->mem_sz) {
      /* need to get more memory */
      mem_pa_t new_ad = mem_realloc(pid, pp->mem_ad, pp->mem_sz, p_vaddr + p_memsz, 0);
      
      if (new_ad == 0) {
	K_PRINTF(2, "%s: fail to allocate process memory\n", argv[0]);
	goto exit_error;
      }

      pp->mem_ad = new_ad;
      pp->mem_sz = ALIGN32(p_vaddr + p_memsz);
    }
      
   if (p_filesz == 0) {
      // discard empty segments
      continue;
    }
    
    if (p_type == 1) {
      /* loadable segment */
      sendreceive_vfs_lseek(&msg, fd, p_offset, SEEK_SET);

      while (p_filesz) {
	u32_t len = p_filesz > sizeof(pa_buf) ? sizeof(pa_buf) : p_filesz;

	//printf("ld @%xh - %u\n", p_vaddr, len);

	if (sendreceive_vfs_read(&msg, fd, &pa_buf[0], len) != len) goto exit_error;

	copy_to_user(pid, p_vaddr, (mem_pa_t)&pa_buf[0], len);

	p_vaddr  += len;
	p_filesz -= len;
      }
    }
  }

  /* setup arguments memory space */
  mem_va_t va_argv   = pp->mem_sz;                              // VA for argv table
  mem_va_t va_argbuf = pp->mem_sz + sizeof(char *)*(argc + 1);  // VA for argv strings
  u32_t    new_size  = va_argbuf + ALIGN32(argbuflen);

  mem_pa_t new_ad = mem_realloc(pid, pp->mem_ad, pp->mem_sz, new_size, 0);
  
  if (new_ad == 0) {
    K_PRINTF(3, "%s: fail to allocate process memory\n", argv[0]);
    goto exit_error;
  }

  pp->mem_ad = new_ad;
  pp->mem_sz = new_size;

  /* copy arguments */
  int arg;
  char **pa_argv = (char **)va_to_pa(pid, va_argv, sizeof(char *)*(argc + 1));

  for (arg = 0; arg < argc; arg++) {
    *pa_argv++ = (char *)va_argbuf;
    
    /* add one character for ending null char */
    int len = strlen(argv[arg]) + 1;
    
    copy_to_user(pid, va_argbuf, (mem_pa_t)argv[arg], len);
            
    va_argbuf += len + 1;
  }

  *pa_argv = NULL;

  /* setup  */
  strncpy(pp->name, argv[0], K_PROC_NAME_MAX_LEN-1);
  
  pp->a[0] = argc;
  pp->a[1] = va_argv;
  pp->pc = e_entry;
  /* invalid SP, will do address error if not properly initialized by process itself */
  pp->a[7] = 0xffffffff;
  
  // TODO: check e_entry valid ?

  //proc_stat(pid);
  //K_PRINTF(3, "%s: pc=%x\n", argv[0], pp->pc);

  // TODO: setup heap ?
  
  sendreceive_vfs_close(&msg, fd);

  return 0;

 exit_error:
  K_PRINTF(2, "EXEC: %s: bad file format\n", argv[0]);
  
  sendreceive_vfs_close(&msg, fd);

  return -1;
}

void proc_wait(pid_t pid)
{
  u16_t lbkp = k_lock();

  /* any process requesting for wait is in PROC_STATE_BLOCKED state due to WAIT
     message it sends to system task */
  _proc_table[pid].d.m_from = PROC_PID_NONE;
  _proc_table[pid].d.m_to   = PROC_PID_NONE;

  /* now only a signal will unblock this process */
  
  k_unlock(lbkp);
}

pid_t proc_schedule (pid_t next)
{
  struct proc_list_t *pp = NULL;

  if (next == PROC_PID_ANY) {
    /* use next ready process in the list */
    /* assumes always at least one ready process (idle process) */
    if (! _proc_ready) {
      K_PRINTF(3, "proc_schedule: fatal error: READY process list empty !\n");
      k_panic();
    }
    
    pp = _proc_ready->next;
  } else {
    if (_proc_table[next].d.state == PROC_STATE_READY) {
      pp = &_proc_table[next];
    } else {
      /* this process isn't ready */
    }
  }

  if (! pp) {
    /* coudn't schedule any process */
    K_PRINTF(2, "S:NONE\n");
    return PROC_PID_NONE;
  }

  /* run next process */
  if (pp->d.uid != PROC_UID_KERNEL) {
    /* user process, setup MMU */
    mem_setup_mmu(pp->d.mem_ad, pp->d.mem_sz);
  }

  _proc_ready = pp;
  _proc_ptr = pp;

  //K_PRINTF(3, "S:%i\n", pp->pid);

  return pp->pid;
}

void proc_yield (void)
{
  u16_t lbkp = k_lock();
  // enforce immediate TIMER interrupt
  k_yield();

  k_unlock(lbkp);
}

void proc_display_info(pid_t pid)
{
  if (pid < K_PROC_COUNT) {
    printf("process %u ('%s'), ", pid, _proc_table[pid].d.name);

    if (_proc_table[pid].d.state == PROC_STATE_BLOCKED) {
      printf("BLOCKED");
    } else if (_proc_table[pid].d.state == PROC_STATE_STOPPED) {
      printf("STOPPED");
    } else {
      if (pid == _proc_ptr->pid) {
	printf("RUNNING\n");
      } else {
	printf("READY\n");
      }
    }

    printf("  ppid: %u   uid: %u\n", _proc_table[pid].d.ppid, _proc_table[pid].d.uid);
    printf("registers:\n");
    printf(" d0=%08Xh    a0=%08Xh\n", _proc_table[pid].d.d[0], _proc_table[pid].d.a[0]);
    printf(" d1=%08Xh    a1=%08Xh\n", _proc_table[pid].d.d[1], _proc_table[pid].d.a[1]);
    printf(" d2=%08Xh    a2=%08Xh\n", _proc_table[pid].d.d[2], _proc_table[pid].d.a[2]);
    printf(" d3=%08Xh    a3=%08Xh\n", _proc_table[pid].d.d[3], _proc_table[pid].d.a[3]);
    printf(" d4=%08Xh    a4=%08Xh\n", _proc_table[pid].d.d[4], _proc_table[pid].d.a[4]);
    printf(" d5=%08Xh    a5=%08Xh\n", _proc_table[pid].d.d[5], _proc_table[pid].d.a[5]);
    printf(" d6=%08Xh    a6=%08Xh\n", _proc_table[pid].d.d[6], _proc_table[pid].d.a[6]);
    printf(" d7=%08Xh    sp=%08Xh\n", _proc_table[pid].d.d[7], _proc_table[pid].d.a[7]);
    printf(" pc=%08Xh    sr=%04Xh\n", _proc_table[pid].d.pc, _proc_table[pid].d.sr);
    printf(" MFP address: %Xh\n", _proc_table[pid].d.mfp_ad);
    printf("stack frame extension:\n");
    printf(" status      %04Xh\n", _proc_table[pid].d.x_status);
    printf(" address     %08Xh\n", _proc_table[pid].d.x_addr);
    printf(" instruction %04Xh\n", _proc_table[pid].d.x_instr);
    printf("virtual memory\n");
    printf(" PA: %Xh, size %u bytes\n", _proc_table[pid].d.mem_ad, _proc_table[pid].d.mem_sz);
    printf("signals\n");
    printf(" %s%s\n",
	   _proc_table[pid].d.sig_mask & SIGCHILD ? "SIGCHILD " : "",
	   _proc_table[pid].d.sig_mask & SIGALRM ? "SIGALRM " : "");
  }
}

void proc_stat(pid_t pid)
{
  int p;

  if (pid == PROC_PID_NONE) {
    /* display the list of prcesses */
    for (p = 0; p < K_PROC_COUNT; p++) {
      if (_proc_table[p].d.state != PROC_STATE_NONE) {
	printf("%u%c '%s', ", p, _proc_table[p].d.uid == PROC_UID_KERNEL ? '*' : ' ', _proc_table[p].d.name);

	if (_proc_table[p].d.state == PROC_STATE_BLOCKED) {
	  printf("BLOCKED ");

	  if (_proc_table[p].d.m_to != PROC_PID_NONE) {
	    printf(" (send to %i)\n", _proc_table[p].d.m_to);
	  } else {
	    if (_proc_table[p].d.m_from == ANY) {
	      printf(" (receive from ANY)\n");
	    } else if (_proc_table[p].d.m_from == PROC_PID_NONE) {
	      printf(" (receive from NONE)\n");
	    } else {
	      printf(" (receive from %i)\n", _proc_table[p].d.m_from);
	    }
	  }	    
	} else if (_proc_table[p].d.state == PROC_STATE_STOPPED) {
	  printf("STOPPED\n");
	} else {
	  if (p == _proc_ptr->pid) {
	    printf("RUNNING\n");
	  } else {
	    printf("READY\n");
	  }
	}
      }
    }

    /*        printf("ptr   : %i\n", _proc_ptr->pid);
        printf("free  : %i\n", _proc_free->pid);
        printf("ready : %i\n", _proc_ready->pid);
        
        for (p = 0; p < K_PROC_COUNT; p++) {
	  if (_proc_table[p].d.state == PROC_STATE_NONE) continue;
	  
          printf("  %i <- %i -> %i\n",
    	     _proc_table[p].prev ? _proc_table[p].prev->pid : -1,
    	     p,
    	     _proc_table[p].next ? _proc_table[p].next->pid : -1);
        }
    */
  } else {
    if (pid < K_PROC_COUNT && _proc_table[pid].d.state != PROC_STATE_NONE) {
      proc_display_info(pid);
    } else {
      printf("no such process.\n");
    }
  }
}

uid_t proc_get_uid(pid_t pid)
{
  if (pid < K_PROC_COUNT && _proc_table[pid].d.state != PROC_STATE_NONE) {
    return _proc_table[pid].d.uid;
  } else {
    return PROC_UID_NONE;
  }
}

uid_t proc_get_ppid(pid_t pid)
{
  if (pid < K_PROC_COUNT && _proc_table[pid].d.state != PROC_STATE_NONE) {
    return _proc_table[pid].d.ppid;
  } else {
    return PROC_UID_NONE;
  }
}

#define DEBUG_COM 0

pid_t sendreceive (pid_t tofrom, message_t *pa_msg, u16_t ops)
{
  u16_t lbkp = k_lock();

  if (DEBUG_COM) K_PRINTF(3, "(+) %i -> %i  (%u)\n", _proc_ptr->pid, tofrom, ops);

  volatile struct proc_list_t *pp_tofrom = &_proc_table[tofrom];
  volatile struct proc_list_t *pp_src    = _proc_ptr;

  pp_src->d.pa_msg = pa_msg;

  if (ops & O_SEND) {
    if (tofrom == ANY) {
      /* cannot send to ANY ! */
      k_unlock(lbkp);
      return ESENDANY;
    }

    pp_src->d.m_to = tofrom;

    if (pp_tofrom->d.state == PROC_STATE_BLOCKED) {
      if (pp_tofrom->d.m_from == pp_src->pid || pp_tofrom->d.m_from == PROC_PID_ANY) {
	// immediate match
	memcpy32(pp_tofrom->d.pa_msg, pa_msg, sizeof(message_t)/4);
	
	if (DEBUG_COM) K_PRINTF(3, "(s) copy to %Xh\n", (u32_t)pp_tofrom->d.pa_msg);

	// nothing left to send
	pp_src->d.m_to = PROC_PID_NONE;

	// unblock destination process
	pp_tofrom->d.m_from = pp_src->pid;
	pp_tofrom->d.d[0]   = pp_src->pid;  // d0 holds trap return value

	_proc_add_to_ready_list((struct proc_list_t *)pp_tofrom);
	pp_tofrom->d.state  = PROC_STATE_READY;    

	if (DEBUG_COM) K_PRINTF(3, "(s) %i -> %i\n", pp_src->pid, tofrom);
      } else if (pp_tofrom->d.m_to == pp_src->pid) {
	// dead lock
	K_PRINTF(3, "error: com dead lock\n");
	k_unlock(lbkp);
	return ELOCK;
      }
    }
  } else {
    // nothing to send
    pp_src->d.m_to = PROC_PID_NONE;
  }

  if (ops & O_RECV) {
    pp_src->d.m_from = tofrom;

    if (pp_src->d.m_to == PROC_PID_NONE ) {
      // nothing (left) to send, try to receive yet
      if (tofrom == ANY) {
	int p;

	// search for one process sending to me
	for (p = 0; p < K_PROC_COUNT; p++) {
	  if (_proc_table[p].d.state == PROC_STATE_BLOCKED && _proc_table[p].d.m_to == pp_src->pid) {
	    tofrom = p;
	    pp_tofrom = &_proc_table[p];
	    break;
	  }
	}
      }
    
      if (tofrom != ANY && pp_tofrom->d.state == PROC_STATE_BLOCKED) {
	if (pp_tofrom->d.m_to == pp_src->pid) {
	  // immediate match
	  memcpy32(pa_msg, pp_tofrom->d.pa_msg, sizeof(message_t)/4);

	  // nothing left to receive
	  pp_src->d.m_from = PROC_PID_NONE;

	  // unblock sender
	  pp_tofrom->d.d[0]   = pp_src->pid;  // d0 holds trap return value
	  pp_tofrom->d.m_to   = PROC_PID_NONE;

	  if (pp_tofrom->d.m_from == PROC_PID_NONE) {
	    _proc_add_to_ready_list((struct proc_list_t *)pp_tofrom);
	    pp_tofrom->d.state  = PROC_STATE_READY;
	  }
	  
	  if (DEBUG_COM) K_PRINTF(3, "(r) %i -> %i\n", tofrom, pp_src->pid);
      
	  k_unlock(lbkp);
	  return tofrom;
	}
      }
    }
  } else {
    // nothing to receive
    pp_src->d.m_from = PROC_PID_NONE;
  }

  if (pp_src->d.m_from == PROC_PID_NONE &&
      pp_src->d.m_to == PROC_PID_NONE) {
    // no communication left, all right !
    k_unlock(lbkp);
    return tofrom;
  }

  // some communication couldn't be done yet, block process unless some signal is pending
  if (DEBUG_COM) K_PRINTF(3, "(*) %i\n", pp_src->pid);

  if (pp_src->d.sig_mask == 0) {
    _proc_remove_from_ready_list((struct proc_list_t *)pp_src);
    pp_src->d.state = PROC_STATE_BLOCKED;
  } else {
    if (pp_src->d.sig_mask & SIGALRM) {
      pp_src->d.sig_mask &= ~SIGALRM;
    }

    // else SIGCHILD or others ?
    
    pp_src->d.m_from   = PROC_PID_NONE;
    pp_src->d.m_to     = PROC_PID_NONE;
    pp_src->d.d[0]     = EABORT;  // d0 holds trap return value
  }
  
  if (pp_src->d.uid > PROC_UID_KERNEL) {
    // has to be within a system call
    // process will be resumed later right after the trap instruction,
    // at the address saved in the stack then into process context

    // change process
    if (proc_schedule(PROC_PID_ANY) == K_PROC_IDLE) {
      // give a second chance
      proc_schedule(PROC_PID_ANY);
    }

    //    K_PRINTF(3, "+%i\n", proc_current());
    k_restart();

    // dead code
    k_panic();
  }

  // direct call from a system task

  // enforce rescheduling thanks to immediate interrupt
  k_yield();
  k_unlock(lbkp);
  
  pid_t rval = EABORT;
  
  if (ops & O_RECV) {
    if (pp_src->d.m_from != PROC_PID_NONE) {
      // recv not aborted
      rval = pp_src->d.m_from;
    }
  } else {
     if (pp_src->d.m_to != PROC_PID_NONE) {
      // send not aborted
      rval = pp_src->d.m_to;
    }
  }
  
  if (DEBUG_COM) K_PRINTF(3, "(-) %i (%i)\n", pp_src->pid, rval);
  //if (DEBUG_COM) proc_stat(pp_src->pid);
  
  return rval;
}

void proc_sig(pid_t pid, u16_t sig)
{
  u16_t lbkp = k_lock();

  struct proc_list_t *pp = &_proc_table[pid];

  if (pp->d.state == PROC_STATE_BLOCKED) {
    /* process will eventually resume with EABORT */
    pp->d.m_from   = PROC_PID_NONE;
    pp->d.m_to     = PROC_PID_NONE;
    //    pp->d.sig_mask &= ~sig;
    pp->d.state    = PROC_STATE_READY;
    pp->d.d[0]     = EABORT;  // d0 holds trap return value

    // TODO : get child PID for SIGCHILD

    /* add process to ready list */
    _proc_add_to_ready_list(pp);
  } else if (pp->d.state == PROC_STATE_READY) {
    pp->d.sig_mask |= sig;
  } else {
    K_PRINTF(3, "PROC: lost signal by stopped PID %i\n", pid);
  }
  
  k_unlock(lbkp);
}

u16_t proc_get_sig(pid_t pid)
{
  struct proc_list_t *pp = &_proc_table[pid];
  return pp->d.sig_mask;
}
