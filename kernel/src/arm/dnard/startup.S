/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/dnard/startup.S
 * Description:   Dnard startup code.
 *                
 * @LICENSE@
 *                
 * $Id: startup.S,v 1.11 2001/11/22 13:18:49 skoglund Exp $
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

	mcr     p15, 0, r0, c7, c7	/* flush ID cache		*/
	mcr	p15, 0, r0, c8, c7	/* flush I,D TLBs on v4		*/

	/* Initialize Stack Pointer */
	ldr	sp, 1f

	/* Build the initial Page Table */
	bl	setup_kernel_pgtable__Fv
	
	/* Enable the MMU */
	ldr	r1, 3f			/* jump there after mmu on	*/
	
	ldr	r0, 6f			/* set domain register		*/
	mcr	p15, 0, r0, c3, c0
	ldr	r0, 4f			/* install page table		*/
	mcr	p15, 0, r0, c2, c0

	mcr	p15, 0, r0, c8, c7	/* flush I,D TLBs on v4		*/
	mcr     p15, 0, r0, c7, c7, 0	/* flush ID cache		*/

	ldr	r0, 5f			/* switch on mmu		*/
	mcr	p15, 0, r0, c1, c0

	mov	pc, r1			/* jump to init()		*/


1:	.word	_init_stack_top
3:	.word	init__Fv
4:	.word	kernel_ptdir_p
5:	.word	(CTL_MMU_ENABLED | CTL_ICACHE | CTL_DCACHE | CTL_WRITE_BUF | CTL_SYS_PROT)
6:	.word	0x00000005
7:	.word	0x400003f8

_init_stack_bottom:
	.space	512
_init_stack_top:	
