/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/ep7211/startup.S
 * Description:   EP7211 startup code.
 *                
 * @LICENSE@
 *                
 * $Id: startup.S,v 1.5 2001/11/22 13:18:49 skoglund Exp $
 *                
 ********************************************************************/
#include <macros.h>
#include INC_ARCH(cpu.h)

	.globl	_start

	.section .init

_start:
	/* Switch to abort mode */
	mov	r0, #(IRQ_MASK|FIQ_MASK|KERNEL_MODE)
	msr	cpsr, r0
	
	/* reset spsr */
	mov	r0, #0xd3
	msr	spsr, r0

	/* Initialize Stack Pointer */
	ldr	sp, 1f

	/* Build the initial Page Table */
	bl	setup_kernel_pgtable__Fv
	
	/* Enable the MMU */
#if 1
	ldr	r1, 3f			/* jump there after mmu on	*/
#else
	ldr	r1, 8f
#endif	
	ldr	r0, 6f			/* set domain register		*/
	MCR	p15, 0, r0, c3, c0
	LDR	r0, 4f			/* install page table		*/
	MCR	p15, 0, r0, c2, c0

	MCR     p15, 0, r0, c7, c7, 0	/* flush ID cache		*/

	LDR	r0, 5f			/* switch on mmu		*/
	MCR	p15, 0, r0, c1, c0

	mov	pc, r1			/* jump to init()		*/


	nop
	nop
	nop

		
#if 1
flash:	
	/* LED Flasher		*/
	ldr	r0, 7f
	mov	r1, #((1 << 6) | (0x7 << 2) | 0)
	str	r1, [r0]
0:	b	0b
#endif
	

1:	.word	_init_stack_top
3:	.word	init__Fv
4:	.word	kernel_ptdir_p
#if defined(EXCEPTION_VECTOR_RELOCATED)
5:	.word	(CTL_MMU_ENABLED | CTL_IVECTOR | CTL_SYS_PROT | CTL_ICACHE | CTL_DCACHE | CTL_WRITE_BUF)
#else
5:	.word	(CTL_MMU_ENABLED | CTL_SYS_PROT | CTL_ICACHE | CTL_DCACHE | CTL_WRITE_BUF)
#endif
	
	
6:	.word	0x00000005
#if 0
7:	.word	0x800022c0
#else
7:	.word	0xFFF022c0
#endif
8:	.word	flash



	
_init_stack_bottom:
	.space	512
_init_stack_top:	
