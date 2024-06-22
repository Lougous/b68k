
#include "config.h"
	.text

	/* interrupt vector table */
/*	.long k_stktop	/* 00h - reset SSP */
	.long 0		/* 00h - reset SSP */
	.long start	/* 04h - reset PC */
	.long default	/* 08h - bus error */
	.long default	/* 0Ch - address error */
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
	.long default	/* 64h - Level 1 interrupt autovector */
	.long default	/* 68h - Level 2 interrupt autovector */
	.long default	/* 6Ch - Level 3 interrupt autovector */
	.long default	/* 70h - Level 4 interrupt autovector */
	.long default	/* 74h - Level 5 interrupt autovector */
	.long default	/* 78h - Level 6 interrupt autovector */
	.long default	/* 7Ch - Level 7 interrupt autovector */
	.long default	/* 80h - TRAP #0 vector */
	.long default	/* 84h - TRAP #1 vector */
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

	.global start

start:
	/* disable interrupts */
	ori.w	#0x0700, %sr

	/* Clear bss */	
/*
        lea.l   __s_bss,%a0
        move.l  #__e_bss,%d0
1:
        cmp.l   %d0,%a0
        beq.s   2f
        clr.b   (%a0)+
        bra.s   1b
2:
*/
	/* set stack pointer */
	lea.l  #0x30000,%sp

	/* boot loader - step 2 */
	jsr bootloader

	/* call bootstrap */
	lea.l  #0x20000,%a0
	jsr %a0

	/* should never return */
	bra default
	
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
 	move.l #0x20000,%d0
bl_loop:
	movea.l %d0,%a0
        addq.l #1,%d0
 	move.b 0xfff00000,%d1
        move.b %d1,%a0@
	cmpi.l #0x21000,%d0  /* max 4kiB */
	bnes bl_loop

	/* POST(0x02)
	/* TODO: use #define */
	move.b #7,0xfff00001
	move.b #2,0xfff00000

	rts

	/******************************************************/
	/* default interruption entry
	/******************************************************/
default:
	/* POST code */
	move.b #7,0xfff00001  /* TODO: use #define */
	move.b #0xFF,0xfff00000  /* TODO: use #define */

dead:
	bra dead
	
