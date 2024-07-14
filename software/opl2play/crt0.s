/*
** B68K Computer - Copyright (c) 2024 Lougous
** https://github.com/Lougous/b68k
*/

	.text
	.global	start
	
start:
 	lea.l	__s_bss,%a2	/* Clear bss */
	move.l	#__e_bss,%d0
1:
	cmp.l	%d0,%a2
	beq.s	2f
	clr.b	(%a2)+
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

	/* save registers setup by exec, will be arguments for main */
	move.l	%a1,-(%sp)	/* argv */
	move.l	%a0,-(%sp)	/* argc */

	/* libc internal init
	   among other things, this routine associates the streams stdin,
	   stdout and stderr with integer file descriptors 0, 1 and 2,
	   respectively */
	jsr	__libc_init

	/* main */
	jsr	main		/* Call the C-program */
	/*lea	12(%sp),%sp	/* Clean up the stack */

	move.l	%d0,-(%sp)
	jsr	exit
halt:
	bra halt

	.section .stack
stkbot:
	.space 512
stktop:

	.section .heap
	.space 512 

