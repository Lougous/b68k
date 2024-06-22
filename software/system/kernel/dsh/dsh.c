
#define BUF_LEN   32
#define PROMPT    "> "
#define MAX_ARGC  4

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <types.h>
#include <fcntl.h>
#include <dirent.h>

#include <mem.h>
#include <proc.h>
#include <irq.h>
#include <syscall.h>
#include <msg.h>

#include "lock.h"

/* build-in commands */
extern int ramtest (int argc, char *argv[]);
extern int md (int argc, char *argv[]);
extern int mm (int argc, char *argv[]);
extern int ls (int argc, char *argv[]);
extern int cat (int argc, char *argv[]);
extern int load (int argc, char *argv[]);
extern int k_reset (int argc, char *argv[]);
int help(int argc, char *argv[]);

#include "b68k.h"

extern int readx(char *str, unsigned int *val);

int dsh_proc_stat (int argc, char *argv[])
{
  if (argc == 1) {
    proc_stat(PROC_PID_NONE);
  } else {
    argc--;
    argv++;
    
    while (argc) {
      int pid = atoi(argv[0]);

      proc_stat(pid);
      
      argc--;
      argv++;
    }
  }

  return 0;
}

int mfpw (int argc, char *argv[])
{
  unsigned int addr, value;

  if (readx(argv[1], &addr) == 0 ||
      readx(argv[2], &value) == 0) {
    printf("bad arguments\n");
    return -1;
  }

  printf("%Xh: %Xh\n", addr, value);

  {
    u16_t lbkp = k_lock();
    
    B68K_MFP->ad = addr;
    B68K_MFP->dt = value;

    k_unlock(lbkp);
  }

  return 0;
}
  
int mfpr (int argc, char *argv[])
{
  unsigned int addr;
  unsigned int value;

  argc--; argv++;

  while (argc) {
    if (readx(argv[0], &addr) == 0) {
      printf("skip address: '%s'\n", argv[0]);
    } else {
      {
	u16_t lbkp = k_lock();
    
	B68K_MFP->ad = addr;
	value = B68K_MFP->dt;
	
	k_unlock(lbkp);
      }

      printf("%Xh: %Xh\n", addr, value);
    }

    argc--; argv++;    
  }

  return 0;
}

int sig (int argc, char *argv[])
{
  argc--; argv++;

  while (argc) {
    int pid = atoi(argv[0]);
    
    proc_sig(pid, SIGALRM);
    
    argc--; argv++;    
  }

  return 0;
}
  
typedef struct {
  const char *name;
  int (* ptr)(int argc, char *argv[]);
} cmd_entry_t;

const cmd_entry_t cmd_list[] = {
  { "help", help },
  { "ramtest", ramtest },
  { "md", md },
  { "mm", mm },
  { "ls", ls },
  { "cat", cat },
  { "load", load },
  { "ps", dsh_proc_stat },
  { "reset", k_reset },
  { "mfpw", mfpw },
  { "mfpr", mfpr },
  { "sig", sig },
  { 0, 0 }
};

int help(int argc, char *argv[]) {
  int p = 0;

  printf("available commands:\n  ");

  while (cmd_list[p].name) {
    printf("%s ", cmd_list[p].name);
    p++;
  }
  
  putchar('\n');

  return 0;
}

char buf[BUF_LEN];

void dsh_do (char *cline)
{
  int argc;
  static char *argv[MAX_ARGC];
  int in;
 
  /* decode command: cuts words */
  argc = 0;
  in = 0;

  while (*cline) {
    if (in == 0 && *cline != ' ') {
      // start of word
      in = 1;
      argv[argc++] = cline;

      // wont'be able to cut more
      if (argc == MAX_ARGC) break;
    } else if (in == 1 && *cline == ' ') {
      // end of word
      *cline = 0;
      in = 0;
    }

    cline++;
  }
    
  /* check 1st word (command name) with build-in commands */
  if (argc) {
    int p = 0;
    
    while (cmd_list[p].name) {
      if (strcmp(cmd_list[p].name, argv[0]) == 0) {
	cmd_list[p].ptr(argc, argv);
	break;
      }
      
      p++;
    }
    
    if (! cmd_list[p].name) {
      /* not found, try to find executable */
      pid_t pid = proc_create_init(argv[0], 0);

      int ret = proc_exec(pid, argc, (const char **)argv, 0, BUF_LEN);

      if (ret == 0) {
	printf("add process '%s' (PID %d)\n", argv[0], pid);

/* back to dsh */
      } else {
	proc_kill(pid);
	printf("%s: command not found\n",  argv[0]);
      }
    }
    
  }
}


pid_t _dsh_pid;
int _c_in;

static void _uart_interrupt (void)
{
  // received char through MFP UART
  // TODO: use a buffer to sustain higher com speed ?
  {
    u16_t lbkp = k_lock();
    
    B68K_MFP->ad = B68K_MFP_REG_UART_DATA;
    _c_in = B68K_MFP->dt & 0xff;
    
    k_unlock(lbkp);
  }
  // usually raised by the clock task, used here to wake up the TTY task
  proc_sig(_dsh_pid, SIGALRM);
}

static int _dsh_getchar (void)
{
  // block until waken up by interrupt routine above
  static message_t msg;
  receive(ANY, &msg);

  // only interrupt should wake up here
  return _c_in;
}

void dsh (void)  {
  int pos = 0;

  printf("starting debug monitor\n");
  _dsh_pid = proc_current();

  // register interrupt handler for serial input
  irq_register_interrupt(IRQ_DBGSERIAL, _uart_interrupt);
  irq_enable_interrupt(IRQ_DBGSERIAL);
  
  printf(PROMPT);

  buf[0] = 0;
  
  while(1) {
    
      int c = _dsh_getchar();

      switch (c) {
      case '\n':
      case '\r':
	buf[pos] = 0;
	putchar('\n');

	dsh_do(&buf[0]);
		    
	buf[0] = 0;
	pos = 0;
	printf(PROMPT);
	break;

      case '\b':
	if (pos) {
	  pos--;
	  putchar('\b');
	}
	break;

      default:
	putchar(c);
	buf[pos++] = c;
	break;
      }
  }

  return;
}

