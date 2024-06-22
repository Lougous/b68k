
#include <stdio.h>
#include <types.h>
#include <stddef.h>
#include <string.h>
#include <syscall.h>

#include "config.h"
#include "mem.h"
#include "proc.h"
#include "irq.h"
#include "dev.h"
#include "msg.h"
#include "debug.h"
#include "lock.h"

#include "b68k.h"

// prototypes
extern void _POST_code(u8_t c);
extern void dsh(void);
extern int serial_bootloader(pid_t pid);
extern void k_restart (void);
void k_reset (void);

// IO handling with debug serial interface and/or tty task
struct _IO_FILE __stdin_struct;
struct _IO_FILE __stdout_struct;
struct _IO_FILE __stdout_msg_struct;

static pid_t _tty_pid;

static int _serial_putchar (int c)
{
  B68K_MFP->ad = B68K_MFP_REG_UART_STS;

  /* wait for TX ready */
  while (! (B68K_MFP->dt & 0x80));    

  B68K_MFP->ad = B68K_MFP_REG_UART_DATA;
  B68K_MFP->dt = (unsigned char) c;

  return c;
}

static int _serial_getchar(void)
{
  while (1) {
    B68K_MFP->ad = B68K_MFP_REG_UART_STS;

    if (! (B68K_MFP->dt & 0x02)) {
      // RX FIFO not empty
      B68K_MFP->ad = B68K_MFP_REG_UART_DATA;

      // need to mask out MSB (read void on the bus ...)
      return B68K_MFP->dt & 0xFF;
    }
  }

  return 0;
}

static u8_t _tp_buf[80];
static u16_t _tp_len;

static int _tty_putchar(int c)
{
  _serial_putchar(c);

  _tp_buf[_tp_len++] = (u8_t)c;

  if ((u8_t)c == '\n' || _tp_len == sizeof(_tp_buf)) {
    // need to flush
    static message_t msg_out;

    msg_out.type                  = DEV_WRITE;
    msg_out.body.dev_write.handle = NULL;  // to TTY0
    msg_out.body.dev_write.count  = _tp_len;
    msg_out.body.dev_write.src    = _tp_buf;

    // need for send+receive to avoid loss of message
    sendreceive(_tty_pid, &msg_out, O_SEND | O_RECV);

    // reset buffer
    _tp_len = 0;
  }
  
  return 0;
}

// system tasks entries and stacks
u16_t dsh_stack[512];

#define _SYSTEM_STACK_SIZE  2048
u16_t system_stack[_SYSTEM_STACK_SIZE/2];

extern void clock_task (void);
#define _CLOCK_STACK_SIZE  1024
u16_t clock_stack[_CLOCK_STACK_SIZE/2];

extern void mouse_task (void);
u16_t mouse_stack[256];

extern void tty_task (void);
#define _TTY_STACK_SIZE  1024
u16_t tty_stack[_TTY_STACK_SIZE/2];

extern void av_pcm_task (void);
u16_t av_pcm_stack[256];

extern void av_opl2_task (void);
u16_t av_opl2_stack[256];

extern void mio_task (void);
#define _MIO_STACK_SIZE  2048
u16_t mio_stack[_MIO_STACK_SIZE/2];

extern void vfs_task (void);
#define _VFS_STACK_SIZE  1024
u16_t vfs_stack[_VFS_STACK_SIZE/2];

char _k_argv_buf[K_PROC_ARGS_SIZE];



// root file system boot list (ordered)
const char * const _root_boot_list[] = {
  // device, file system (0=auto, for disk partitions only)
  "sda0",   0,
  "sda1",   0,
  "sda2",   0,
  "sda3",   0,
  "sdb0",   0,
  "sdb1",   0,
  "sdb2",   0,
  "sdb3",   0,
  "serial", "rfs",
  0
};


void system_task (void)
{
  //  pid_t me = proc_current();
  
  _POST_code(0xB);
  K_PRINTF(2, "starting system task\n");

  /* create tasks */
  _POST_code(0xC);

  //B68K_MFP->ad = B68K_MFP_REG_IRQSTS;
  u8_t btsp_mode = 0;  // B68K_MFP->dt & 0x80;

  // start with tty task, so that message can be displayed ASAP on screen
  // tty uses clock services
  proc_create_task(2, "clock", clock_task, clock_stack, _CLOCK_STACK_SIZE);

#ifdef K_HAS_AV_BOARD
  _tty_pid = proc_create_task(4, "tty", tty_task, tty_stack, _TTY_STACK_SIZE);
  while (proc_get_state(_tty_pid) != PROC_STATE_BLOCKED) proc_yield();
  _tp_len = 0;
  __stdout_msg_struct.putchar = _tty_putchar;
  
#endif

  /* be careful with process IDs (no duplication) */
  /* start device drivers before services (tty, vfs, ...) */
  pid_t mio_pid = proc_create_task(5, "mio", mio_task, mio_stack, _MIO_STACK_SIZE);
  proc_create_task(1, "debug", dsh, dsh_stack, sizeof(dsh_stack));
  pid_t vfs_pid = proc_create_task(3, "vfs", vfs_task, vfs_stack, _VFS_STACK_SIZE);

  proc_create_task(PROC_PID_ANY, "mouse", mouse_task, mouse_stack, sizeof(mouse_stack));
  
  /* ensure drivers are properly initialized before to go on */
  _POST_code(0xD);
  while (proc_get_state(mio_pid) != PROC_STATE_BLOCKED) proc_yield();

#ifdef K_HAS_AV_BOARD
  /* those drivers need the AV FPGA properly configured (tty driver) */
  proc_create_task(PROC_PID_ANY, "pcm", av_pcm_task, av_pcm_stack, sizeof(av_pcm_stack));
  proc_create_task(PROC_PID_ANY, "opl2", av_opl2_task, av_opl2_stack, sizeof(av_opl2_stack));
#endif
  
  while (proc_get_state(vfs_pid) != PROC_STATE_BLOCKED) proc_yield();
  _POST_code(0xE);

  /* create/load init process */
  pid_t init_pid;
  static message_t msg;

  /* check boot mode jumper position */
  if (btsp_mode) {
    K_PRINTF(0, "serial bootstrap enable\n");
    K_PRINTF(0, "fatal: serial bootstrap not implemented yet.\n");
    while(1);

  } else {
    /* mount points */
    u8_t root_mounted = 0;

    K_PRINTF(2, "mounting root file sytem\n");

    char **p_boot_list = (char **)_root_boot_list;

    msg.type = MOUNT;
    msg.body.mount.path_to = "/";

    while (*p_boot_list) {
      char *dev = *p_boot_list++;
      msg.type = MOUNT;
      msg.body.mount.dev = dev;
      msg.body.mount.path_to = "/";
      msg.body.mount.type = *p_boot_list++;

      sendreceive(vfs_pid, &msg, O_SEND | O_RECV);

      if (msg.body.u32 == 0) {
	K_PRINTF(0, "root: %s\n", dev);
	root_mounted = 1;
	break;
      } else {
	K_PRINTF(1, "%s: cannot mount (%i)\n", dev, msg.body.u32);
      }
    }
    
    if (!root_mounted) {
      K_PRINTF(0, "root mounting failed, system startup aborted\n");
      goto abort;
    }

    // create /dev directory to able mounting  devfs
    msg.type = MKDIR;
    msg.body.mkdir.path = "/dev";
    msg.body.mkdir.len  = 4;
    msg.body.mkdir.flags = 0;

    sendreceive(vfs_pid, &msg, O_SEND | O_RECV);

    if (msg.body.u32 != 0) {
      K_PRINTF(0, "failed to create /dev, system startup aborted\n");
      goto abort;
    }
    
    // devfs => /dev
    K_PRINTF(1, "mounting devfs\n");
    
    msg.type = MOUNT;
    msg.body.mount.dev = "devfs";
    msg.body.mount.path_to = "/dev";
    msg.body.mount.type = "devfs";

    sendreceive(vfs_pid, &msg, O_SEND | O_RECV);

    if (msg.body.u32 != 0) {
      K_PRINTF(0, "devfs mounting failed, system startup aborted\n");
      goto abort;
    }

    // load & start init
    init_pid = proc_create_init("init", 0);
    
    const char *argv[] = { "/init" };
  
    if (proc_exec(init_pid, 1, argv, (char *)argv[0], strlen(argv[0])) < 0) {
      K_PRINTF(0, "unable to load '%s'\n", &argv[0][0]);
    } else {
      // wake up init process
      proc_sig(init_pid, SIGALRM);
    }
  }

 abort:
  _POST_code(0xF);

  /* main loop */
  while(1) {
    static message_t msg_out;
    msg_out.type = 0;

    pid_t from = receive(ANY, &msg);
    pid_t child;

    // this receive may be interrupted by signals
    if (from >= 0) {
      // proceed with message

      switch (msg.type) {
      case EXIT:
	K_PRINTF(3, "system: EXIT pid %i, exit(%i)\n", from, msg.body.s32);

	/* stop process */
	proc_exit(from, msg.body.s32);
	
	break;

      case KILL:
	{
	  pid_t to_kill = msg.body.kill.pid;
	  
	  K_PRINTF(3, "system: %i: KILL %i\n", from, to_kill);

	  /* TODO: manage sig argument */
	
	  if (
	      /* kernel tasks are mighty */
	      (proc_get_uid(from) == PROC_UID_KERNEL) ||
	      /* root can kill any process but system tasks */
	      ((proc_get_uid(from) == PROC_UID_ROOT) &&
	       (proc_get_uid(to_kill) != PROC_UID_KERNEL)) ||
	      /* user can kill its child only */
	      (from == proc_get_ppid(to_kill))
	      ) {

	    /* close all open files */
	    msg_out.type = KILL;
	    msg_out.body.u32 = to_kill;
	    sendreceive(vfs_pid, &msg_out, O_SEND | O_RECV);
	    msg_out.type = 0;
	  
	    /* terminate process */
	    proc_kill(to_kill);
	  
	    msg_out.body.s32 = 0;	    
	  } else {
	    K_PRINTF(3, "system: KILL rejected\n");
	    msg_out.body.s32 = -1;
	  }

	  send(from, &msg_out);
	}
	break;
	
      case FORK:
	//K_PRINTF(3, "system: FORK pid %i\n", from);

	child = proc_fork (from);

	if (child != PROC_PID_NONE) {
	  /* duplicate all open files */
	  msg_out.type = VFS_FORK;
	  msg_out.body.vfs_fork.parent = from;
	  msg_out.body.vfs_fork.child = child;
	  sendreceive(vfs_pid, &msg_out, O_SEND | O_RECV);

	  /* setup message back to parent (with child PID) */
	  //K_PRINTF(3, "system: FORK child is %i\n", child);
	  msg_out.body.u32 = child;
	  send(from, &msg_out);
	  
	  /* setup message back to child (with PID=0) */
	  msg_out.body.u32 = 0;
	  send(child, &msg_out);
	} else {
	  /* fork failed */
	  msg_out.body.u32 = -1;
	  send(from, &msg_out);
	}

	break;

      case EXEC:
	{
	  K_PRINTF(3, "system: EXEC pid %i\n", from);

	  char **va_argv = msg.body.exec.argv;

	  //K_PRINTF(3, "system: argv VA: %Xh\n", (mem_va_t)va_argv);

	  // copy argument to kernel space, because process memory will be wiped out by proc_exec
	  const char *k_argv[K_PROC_ARGS_COUNT];
	  u16_t argc = 0;
	  u16_t len = 0;

	  while (1) {

	    const char **pa_argv = (const char **)va_to_pa(from, (mem_va_t)va_argv, sizeof(const char *));
	  
	    if (!pa_argv) {
	      /* exec failed */
	      K_PRINTF(3, "system: bad VA(1): %Xh\n", (mem_va_t)va_argv);
	      argc = 0;
	      break;
	    }

	    K_PRINTF(3, "system: argv PA: %Xh\n", (mem_pa_t)pa_argv);

	    va_argv++;

	    const char *va_arg = *pa_argv;

	    if (va_arg == NULL) {
	      /* last argument */
	      k_argv[argc] = NULL;
	      break;
	    }
	    
	    const char *pa_arg = (const char *)va_to_pa(from, (mem_va_t)va_arg, sizeof(const char *));
	    
	    if (!pa_arg) {
	      /* exec failed */
	      K_PRINTF(3, "system: bad VA(2): %Xh\n", (mem_va_t)va_arg);
	      argc = 0;
	      break;
	    }

	    u16_t alen = strlen(pa_arg);

	    if (len + alen < K_PROC_ARGS_SIZE) {
	      K_PRINTF(3, "system: EXEC arg %i = '%s'\n", argc, pa_arg);
	  
	      strcpy(&_k_argv_buf[len], pa_arg);
	      k_argv[argc] = &_k_argv_buf[len];
	      len += alen + 1;
	      argc++;
	    } else {
	      // buffer overflow
	      // TODO: discard additional arguments or discard whole exec ?
	      K_PRINTF(3, "system: arg buf ov\n");
	      k_argv[argc] = NULL;
	      break;
	    }
	  }
	    
	  if (argc == 0 || proc_exec(from, argc, k_argv, _k_argv_buf, len) < 0) {
	    /* exec failed */
	    msg_out.body.u32 = -1;
	    send(from, &msg_out);
	    break;
	  }

	  /* process image successfully replaced */
	  // possible optim: force process to READY state
	  msg_out.body.u32 = 0;
	  send(from, &msg_out);
	}

	break;

      case WAIT:
	K_PRINTF(3, "system: WAIT pid %i\n", from);
	proc_wait(from);
	break;
	
      default:
	K_PRINTF(3, "system: illegal message %Xh from process %i\n", msg.type, from);
	break;
      }
    } else {
      // signals
      // soft reset ! (sent by tty)
      k_reset();
    }
  }
}

// system scheduling interrupt
// the actual interrupt entry (and return) is in srt0.s, so no
// __attribute__ ((interrupt)) shall be used here
void k_sys_irq_handler ()
{
  u16_t sys_sts;

  {
    // TODO : critical section maybe not necessary, if higher level interrupts
    // manage carefully B68K_MFP->ad save & restore
    u16_t lbkp = k_lock();

    // 
    u8_t mfp_ad_save = B68K_MFP->ad;

    // read IRQ flags
    B68K_MFP->ad = B68K_MFP_REG_SYSSTS;
    sys_sts = B68K_MFP->dt;

    // reset flags
    B68K_MFP->dt = sys_sts;

    // restore MFP register
    B68K_MFP->ad = mfp_ad_save;

    k_unlock(lbkp);
  }

  // B68K_MFP_SYS_IRQ_SELF_MASK
  // no user interrupt handler for this; the self interrupt is used by processes
  // that yield. usally done when blocked during communication (see sendreceive)

  // B68K_MFP_SYS_IRQ_TMR_MASK
  
  // system interrupt involves a reschedule.
   if (proc_schedule(PROC_PID_ANY) == K_PROC_IDLE) {
     // second chance
     proc_schedule(PROC_PID_ANY);

     // note : cannot loop here until another process get scheduled: need to
     // enable other interrupts to possibly unblock some other processes 
  }
  
  // return to srt0.s
}

extern void k_restart (void);

// early OS initialization in system mode, while process scheduling is disabled.
// create system task, and start scheduling with it.
void k_sys_init () {
  _POST_code(4);

  // default console is serial
  // LIBC init
  stdout = &__stdout_struct;
  stdout->putchar = _serial_putchar;
  stdin = &__stdin_struct;
  stdin->getchar = _serial_getchar;

  // default console is serial
  __stdout_msg_struct.putchar = _serial_putchar;
  __stdout_msg_struct.getchar = _serial_getchar;

  // banner
  K_PRINTF(0, "\n\nsystem v%u.%u - build %s %s\n", K_VERSION_MAJOR, K_VERSION_MINOR, __DATE__, __TIME__);

  if (sizeof(message_t) != ((sizeof(message_t) / 4) * 4)) {
    K_PRINTF(0, "fatal error: message size (%u bytes) must be a multiple of 4\n", sizeof(message_t));
    while(1);
  }

  // initialize system components
  _POST_code(5);
  mem_init();

  _POST_code(6);
  
  // other interrupts (MFP, MIO...)
  irq_init();

  // enable system interrupt
  B68K_MFP->ad = B68K_MFP_REG_SYSCFG;
  B68K_MFP->dt = B68K_MFP_SYS_IRQ_TMR_MASK;

  _POST_code(7);
  proc_init();
  
  _POST_code(8);
  dev_init();

  // create system task
  _POST_code(9);
  pid_t pid = proc_create_task(PROC_PID_SYSTEM_TASK, "system", system_task, system_stack, _SYSTEM_STACK_SIZE);

  // promote and start system task
  
  proc_schedule(pid);
  _POST_code(10);

  k_restart();

  // shall not get there, as supervisor mode will only enter through interrupts
  // or traps now
  _POST_code(0xEE);
  k_panic();

  while(1);
}

void k_reset (void) {
  k_lock();
  K_PRINTF(0, "system reset...\n");
  B68K_MFP->ad = B68K_MFP_REG_SYSCFG;
  B68K_MFP->dt = B68K_MFP_SYS_RESET_MASK;

  while (1);
}
	
void _POST_code(u8_t c)
{
  u16_t lbkp = k_lock();

  B68K_MFP->ad = B68K_MFP_REG_POST;
  B68K_MFP->dt = c;

  k_unlock(lbkp);
}

