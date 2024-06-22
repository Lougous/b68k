
#ifndef _proc_h_
#define _proc_h_

typedef s16_t pid_t;
typedef u32_t uid_t;
typedef u32_t proc_state_t;

extern u32_t proc_init (void);
extern pid_t proc_create_init (const char *name, u32_t size);
extern pid_t proc_create_task (pid_t pid, char *name, void *pc, void *stk, u32_t stk_sz);
extern void proc_kill (pid_t pid);
extern pid_t proc_fork (pid_t ppid);
extern proc_state_t proc_get_state (pid_t pid);
extern int proc_exec(pid_t pid, int argc, const char *argv[], char *pa_argbuf, u16_t argbuflen);
extern pid_t proc_schedule (pid_t pid);
extern pid_t proc_current (void);
extern uid_t proc_get_uid(pid_t pid);
extern uid_t proc_get_ppid(pid_t pid);
extern pid_t proc_get_pid(char *name);
extern void proc_sig(pid_t pid, u16_t signal);
extern void proc_wait(pid_t pid);
extern void proc_exit(pid_t pid, s32_t eval);
extern void proc_stat(pid_t pid);
extern void proc_yield();
extern u16_t proc_get_sig(pid_t pid);
  
extern mem_pa_t va_to_pa(pid_t pid, mem_va_t dst, int len);

#define PROC_PID_SYSTEM_TASK   ((pid_t) 0)
#define PROC_PID_NONE          ((pid_t) -1)
#define PROC_PID_ANY           ((pid_t) -2)

#define PROC_UID_KERNEL ((uid_t) 0)
#define PROC_UID_ROOT   ((uid_t) 1)
#define PROC_UID_NONE   ((uid_t) -1)

#define PROC_STATE_ACTIVE_MASK  1

#define PROC_STATE_NONE      0
#define PROC_STATE_BLOCKED   (PROC_STATE_ACTIVE_MASK)
#define PROC_STATE_READY     (PROC_STATE_ACTIVE_MASK | 2)
#define PROC_STATE_STOPPED   4

#define SIGALRM          0x0001
#define SIGCHILD         0x0002

#endif /* _proc_h_ */
