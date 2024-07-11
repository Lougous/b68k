//
// B68K Computer - Copyright (c) 2024 Lougous
// https://github.com/Lougous/b68k
//
// System/libc - trap services
//

	// trap_sendreceive(message_t *msg, u32_t pid_to);
	.global trap_sendreceive
trap_sendreceive:
	// trap ABI:
	//  a0 = message address
	//  d0 = SEND+RECEIVE
	//  d1 = destination PID
	move.l 4(%sp),%a0	// message address
	move.w #0x03,%d0	// SEND+RECEIVE
	move.l 8(%sp),%d1	// destination
	trap #0
	rts

	.global trap_mticks
trap_mticks:
	// trap ABI:
	//  d0 = ms time (return)
	trap #1
	rts
	
