#include "b68k.h"

	.text

	.global irq_default
	.global irq_group0

irq_default:
	move.b #B68K_MFP_REG_POST,B68K_MFP_AD
	move.b #0xFE,B68K_MFP_DT

	/* insert dummy words into stack to have the same stack frame as group 0 */
	clr.w	-(%sp)  /* instruction */
	clr.l	-(%sp)  /* access address */
	clr.w	-(%sp)  /* status */

	.global g_tmp
	.global g_regs
irq_group0:
	/* POST(0xFF) */
	move.b #B68K_MFP_REG_POST,B68K_MFP_AD
	move.b #0xFF,B68K_MFP_DT

	/* save registers */
	/* save a6 */
	move.l	%a6,(g_tmp)

	/* current process storage address */
	move.l	g_regs,%a6

	/* save process registers */
	movem.l %d0-%d7/%a0-%a7,(%a6)
	move.l	(g_tmp),56(%a6) /* save a6 */

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

	jmp reg_dump	
