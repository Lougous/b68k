#include "config.h"
	.text

	/* interrupt vector table */
/*	.long k_stktop	/* 00h - reset SSP */
	.long 0		/* 00h - reset SSP */
	.long start	/* 04h - reset PC */
	.long group0	/* 08h - bus error */
	.long group0	/* 0Ch - address error */
	.long default	/* 10h - Illegal instruction */
	.long default	/* 14h - Divide by zero */
	.long default	/* 18h - CHK instruction */
	.long default	/* 1Ch - TRAPV instruction */
	.long default	/* 20h - Privilege violation */
	.long default	/* 24h - Trace */
	.long default	/* 28h - Line 1010 emulator */
	.long default	/* 2Ch - Line 1111 emulator */
	.long default	/* 30h - (Unassigned -- reserved) */
	.long default	/* 34h - (Unassigned -- reserved) */
	.long default	/* 38h - (Unassigned -- reserved) */
	.long default	/* 3Ch - Uninitialized interrupt vector */
	.long default	/* 40h - (Unassigned -- reserved) */
	.long default	/* 44h - (Unassigned -- reserved) */
	.long default	/* 48h - (Unassigned -- reserved) */
	.long default	/* 4Ch - (Unassigned -- reserved) */
	.long default	/* 50h - (Unassigned -- reserved) */
	.long default	/* 54h - (Unassigned -- reserved) */
	.long default	/* 58h - (Unassigned -- reserved) */
	.long default	/* 5Ch - (Unassigned -- reserved) */
	.long default	/* 60h - Spurious interrupt */
	.long sys_irq	/* 64h - Level 1 interrupt autovector */
	.long no_int2	/* 68h - Level 2 interrupt autovector */
	.long no_int3	/* 6Ch - Level 3 interrupt autovector */
	.long no_int4	/* 70h - Level 4 interrupt autovector */
	.long no_int5	/* 74h - Level 5 interrupt autovector */
	.long no_int6	/* 78h - Level 6 interrupt autovector */
	.long no_int7	/* 7Ch - Level 7 interrupt autovector */
	.long scall	/* 80h - TRAP #0 vector */
	.long mticks	/* 84h - TRAP #1 vector */
	.long default	/* 88h - TRAP #2 vector */
	.long default	/* 8Ch - TRAP #3 vector */
	.long default	/* 90h - TRAP #4 vector */
	.long default	/* 94h - TRAP #5 vector */
	.long default	/* 98h - TRAP #6 vector */
	.long default	/* 9Ch - TRAP #7 vector */
	.long default	/* A0h - TRAP #8 vector */
	.long default	/* A4h - TRAP #9 vector */
	.long default	/* A8h - TRAP #10 vector */
	.long default	/* ACh - TRAP #11 vector */
	.long default	/* B0h - TRAP #12 vector */
	.long default	/* B4h - TRAP #13 vector */
	.long default	/* B8h - TRAP #14 vector */
	.long default	/* BCh - TRAP #15 vector */
	.long default	/* C0h - (Unassigned -- reserved) */
	.long default	/* C4h - (Unassigned -- reserved) */
	.long default	/* C8h - (Unassigned -- reserved) */
	.long default	/* CCh - (Unassigned -- reserved) */
	.long default	/* D0h - (Unassigned -- reserved) */
	.long default	/* D4h - (Unassigned -- reserved) */
	.long default	/* D8h - (Unassigned -- reserved) */
	.long default	/* DCh - (Unassigned -- reserved) */
	.long default	/* E0h - (Unassigned -- reserved) */
	.long default	/* E4h - (Unassigned -- reserved) */
	.long default	/* E8h - (Unassigned -- reserved) */
	.long default	/* ECh - (Unassigned -- reserved) */
	.long default	/* F0h - (Unassigned -- reserved) */
	.long default	/* F4h - (Unassigned -- reserved) */
	.long default	/* F8h - (Unassigned -- reserved) */
	.long default	/* FCh - (Unassigned -- reserved) */

	.global main
	.global start
	.global k_reboot
	.local save
	.global k_restart
	.global k_sys_irq_handler
	.global k_scall
k_reboot:
start:
	/* disable interrupts */
	ori.w	#0x0700, %sr

	/* Clear bss */	
        lea.l   __s_bss,%a0
        move.l  #__e_bss,%d0
1:
        cmp.l   %d0,%a0
        beq.s   2f
        clr.b   (%a0)+
        bra.s   1b
2:
        /* Move data to ram */
/*
        lea.l   __e_text,%a0
        lea.l   __s_data,%a1
        move.l  #__e_data,%d0
1:
        cmp.l   %a1,%d0
        beq.s   2f
        move.b  (%a0)+,(%a1)+
        bra.s   1b
2:      
*/
	/* set stack pointer */
	lea.l  k_stktop,%sp

	/* read B68K_MFP_REG_SYSCFG register */
	move.b #12,0xfff00001
	move.b 0xfff00000,%d0

	/* if bit B68K_MFP_SYS_RAM_BOOT is set, then boot is done after bootstrap load: do not copy from flash to RAM */
	btst	#5,%d0
	beq	boot_from_ram
	
	/* boot loader */
	jsr bootloader
boot_from_ram:
	
	/* system init */
	jsr k_sys_init

	/* should never return */
	bra k_panic
	
	/******************************************************/
	/* bootloader : NEEDS TO BE IN FIRST 512 BYTES COPIED
	/*   by CPLD at reset
	/******************************************************/
bootloader:
	/* POST(0x01)
	/* TODO: use #define */
	move.b #7,0xfff00001
	move.b #1,0xfff00000

	/* MFP register: flash data */
 	move.b #0,0xfff00001

	/* TODO: use #define */
 	move.l #512,%d0
bl_loop:
	movea.l %d0,%a0
        addq.l #1,%d0
 	move.b 0xfff00000,%d1
        move.b %d1,%a0@
	cmpi.l #65536,%d0
	bnes bl_loop

	/* POST(0x02)
	/* TODO: use #define */
	move.b #7,0xfff00001
	move.b #2,0xfff00000

	rts

	/******************************************************/
	/* default interruption entry
	/******************************************************/
no_int2:
	ori	#0x0700,%sr
	move.b #7,0xfff00001
	move.b #130,0xfff00000
	bra halt

no_int3:
	ori	#0x0700,%sr
	move.b #7,0xfff00001
	move.b #131,0xfff00000
	bra halt

no_int4:
	ori	#0x0700,%sr
	move.b #7,0xfff00001
	move.b #132,0xfff00000
	bra halt

no_int5:
	ori	#0x0700,%sr
	move.b #7,0xfff00001
	move.b #133,0xfff00000
	bra halt

no_int6:
	ori	#0x0700,%sr
	move.b #7,0xfff00001
	move.b #134,0xfff00000
	bra halt

no_int7:
	ori	#0x0700,%sr
	move.b #7,0xfff00001
	move.b #135,0xfff00000
	bra halt

	/******************************************************/
	/* interruption entry for system (OS) interrupt
	/******************************************************/
sys_irq:
	/* disable all interrupts */
	ori.w	#0x0700, %sr

	/* save process context */
	bsr	save

	/* enable interrupts with higher priority only */
	andi.w	#0xfeff, %sr  /* sr = 001b */
	
	/* system interrupt handler */
	jsr	k_sys_irq_handler
	
	/* setup a process context and (re)start it */
	bra	k_restart

	/******************************************************/
	/* trap entry
	/******************************************************/
scall:
	/* mask interrupts ? */
	ori	#0x0700,%sr
	
	bsr	save			// d0, d1 and a0 not modified

	/* !!! each argument use a 32-bits space in stack (even 16-bits arguments) */
	/* !!! maybe gcc option */
	move.l	%a0,-(%sp)		// m_ptr
	move.l	%d1,-(%sp)		// src_dest
	move.l	%d0,-(%sp)		// SEND/RECEIVE/BOTH
	move.l	_proc_ptr,%a6		// needed to store system call return value
					// warning: k_scall may change _proc_ptr
	jsr	k_scall			// sys_call(func,src_dest,m_ptr)
	move.l  %d0,(%a6)
	add.l	#12,%sp

	bra	k_restart

	.global k_time_ms    // clock.c
mticks:
	move.l	k_time_ms,%d0
	
	/* return from exception */
	rte
	
	/******************************************************/
	/* process context save
	/******************************************************/
save:
	/* save a6 */
	move.l	%a6,-(%sp)

	/* current process storage address */
	move.l	_proc_ptr,%a6

	/* save process registers */
	movem.l %d0-%d7/%a0-%a5,(%a6)
	move.l	(%sp)+,56(%a6) /* save a6 */

	/* save MFP address register */
	move.b 0xfff00001,78(%a6)	

	/* would be supervisor stack before pushing irq frame (6 bytes)
	and this function return address (4 bytes) */
	lea	10(%sp),%a1

	/* btst works with 8-bits word in memory addressing,
	and MSB stored first for big endian */
	btst	#5,4(%sp)
	bne	0f  /* branch if supervisor */
	move.l	%usp,%a1  /* overwrite with user SP */
0:
	/* save SP */
	move.l	%a1,60(%a6)

	/* move return address from stack to a1 */
	move.l (%sp)+,%a1

	/* save SR */
	move.w (%sp)+,68(%a6)

	/* save PC */
	move.l (%sp)+,64(%a6)

	/* return */
	jmp	(%a1)

	/******************************************************/
	/* restore process execution
	/******************************************************/
k_restart:
	/* current process storage address */
	move.l	_proc_ptr,%a6

	/* restore USP or SSP */
	move.l	60(%a6),%a0
	/* btst works with 8-bits word in memory addressing,
	and MSB stored first for big endian */
	btst	#5, 68(%a6)
	bne	_s_mode
	/* user mode process */
	move.l	%a0,%usp
	ori	#0x0700,%sr
	lea.l	k_stktop,%sp
	bra	_rte_stack
_s_mode:
	move.l	%a0,%sp
_rte_stack:
	/* restore PC in stack */
	move.l	64(%a6),-(%sp)
	
	/* restore SR in stack */
	move.w	68(%a6),-(%sp)

	/* restore MFP address register */
	move.b	78(%a6),0xfff00001  /* TODO: use #define */

	/* restore user registers */
	movem.l (%a6),%d0-%d7/%a0-%a6

	/* return from interrupt */
	rte


	/******************************************************/
	/* fatal exception (or not ?)
	/******************************************************/
	.global k_panic
k_panic:
default:
	/* insert dummy words into stack to have the same stack frame as group 0 */
	clr.w	-(%sp)  /* instruction */
	clr.l	-(%sp)  /* access address */
	clr.w	-(%sp)  /* status */
	
group0:
	/* mask IRQ */
	ori #0x0700,%sr

	/* POST code */
	move.b #7,0xfff00001  /* TODO: use #define */
	move.b #0xFF,0xfff00000  /* TODO: use #define */

	/* save a6 */
	move.l	%a6,(k_stkbot)

	/* current process storage address */
	move.l	_proc_ptr,%a6

	/* save process registers */
	movem.l %d0-%d7/%a0-%a5,(%a6)
	move.l	(k_stkbot),56(%a6) /* save a6 */

	/* would be supervisor stack before pushing irq frame */
	lea	14(%sp),%a1

	/* btst works with 8-bits word in memory addressing,
	and MSB stored first for big endian */
	btst	#5,8(%sp)
	bne	0f  /* branch if supervisor */
	move.l	%usp,%a1  /* overwrite with user SP */
0:
	/* save SP */
	move.l	%a1,60(%a6)

	/* save groupe 0 stack frame status */
	move.w (%sp)+,70(%a6)

	/* save groupe 0 stack frame address */
	move.l (%sp)+,72(%a6)

	/* save groupe 0 stack frame instruction */
	move.w (%sp)+,76(%a6)

	/* save SR */
	move.w (%sp)+,68(%a6)

	/* save PC */
	move.l (%sp)+,64(%a6)

	/* use 'safe' stack to switch process */
	lea.l  k_stktop,%sp

	jsr proc_current
	/* TODO : add exit value argument */
	move.l	%d0,-(%sp)
	jsr proc_exit
	jsr proc_display_info  /* to remove eventually */

	/* reschedule */
	lea	4(%sp),%sp
	move.l	#0xFFFFFFFF,%d0  /* PROC_PID_NONE */
	move.l	%d0,-(%sp)
	jsr	proc_schedule
	bra	k_restart
	
	/* enable interrupts and wait for RTC to reschedule */
	/*andi #0xF8FF,%sr	*/
	
halt:
        bra halt

	/******************************************************/
	/* misc.
	/******************************************************/
	.global k_get_sp
k_get_sp:
	move.l	%sp,%d0
	rts
	
	/******************************************************/
	/* enable/disable interrupts
	/******************************************************/
	.global k_unlock
k_unlock:
	move	%sr,%d0
	andi	#0xF8FF,%d0   // clear mask
	or.l	4(%sp),%d0    // restore mask
	move	%d0,%sr
	rts

	.global k_lock
k_lock:
	move	%sr,%d0
	andi	#0x0700,%d0
	ori	#0x0700,%sr
	rts

	
	/******************************************************/
	/* yield
	/******************************************************/
	.global k_yield
k_yield:
	/* TODO: use #define */
	move.b	#12,0xfff00001       // B68K_MFP->ad = B68K_MFP_REG_SYSCFG
	ori.b	#0x40,0xfff00000
	nop
	rts
	
	/******************************************************/
	/* delay loops
	/******************************************************/
	.global k_loop
k_loop:
	move.w %sp@(6),%d0
	/* if argument is 0 */
	cmp.w #0,%d0
	beq 2f
1:
	subqw #1,%d0
	bne 1b
2:
	rts

	.global k_loop0
k_loop0:	
	rts
	
	/******************************************************/
	/* kernel space stack
	/******************************************************/
	.bss
k_stkbot:
	.space K_STACK_BYTES
k_stktop:


	
