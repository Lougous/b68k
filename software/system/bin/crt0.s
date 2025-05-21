/*
** B68K Computer - Copyright (c) 2024 Lougous
** https://github.com/Lougous/b68k
**
** System/binaries - program entry
*/
#include "limits.h"

	.text
	.global	start
	
start:
	/* Clear bss */
	/* done 32-bits at a time : __s_bss and __e_bss must be aligned ! */
 	lea.l	__s_bss,%a3
	move.l	#__e_bss,%d0
1:
	cmp.l	%d0,%a3
	beq.s	2f
	clr.l	(%a3)+
	bra.s	1b
2:
	/* Move data to ram */
/*
	lea.l	__e_text,%a2
	lea.l	__s_data,%a3
	move.l	#__e_data,%d0
1:
	cmp.l	%a3,%d0
	beq.s	2f
	move.b	(%a2)+,(%a3)+
	bra.s	1b
2:	
	*/

	/* setup stack */
	lea.l  stktop,%sp

	/* save registers setup by exec, will be arguments for main and __libc_init */
	move.l	%a1,-(%sp)	/* argv (main) */
	move.l	%a0,-(%sp)	/* argc (main) */
	move.l	%a2,-(%sp)	/* envp (__libc_init) */

	/* libc internal init
	   among other things, this routine associates the streams stdin,
	   stdout and stderr with integer file descriptors 0, 1 and 2,
	   respectively */
	jsr	__libc_init
	add.l	#4,%sp

	/* main */
	jsr	main		/* Call the C-program */

	move.l	%d0,-(%sp)
	jsr	exit
halt:
	bra halt

	.section .stack, "a"
stkbot:
	/* reserved data space for stack - init step */
	.space 64
stkinit:
	.space ARG_MAX
stktop:

	.section .heap, "a"
	/* reserved data space for heap */
	.space 128

