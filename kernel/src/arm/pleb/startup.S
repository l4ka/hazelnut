/*********************************************************************
 *                
 * Copyright (C) 2002,  Karlsruhe University
 *                
 * File path:     arm/pleb/startup.S
 * Description:   Startup code for the PLEB
 *                
 * @LICENSE@
 *                
 * $Id: startup.S,v 1.1 2002/01/24 20:20:06 ud3 Exp $
 *                
 ********************************************************************/
#include <macros.h>
#include INC_PLATFORM(platform.h)
#include INC_ARCH(cpu.h)

	.globl	_start
	.section .init,"xa"
_start:
	/* Switch to abort mode */
	mov	r0, #(IRQ_MASK|FIQ_MASK|KERNEL_MODE)
	msr	cpsr, r0

	/* reset spsr */
	mov	r0, #0xd3
	msr	spsr, r0

	/* Initialize stack pointer */
	ldr	sp, 5f

	/* Build the initial page table */
	bl	setup_kernel_pgtable__Fv

	/* Enable the MMU */
	ldr	r0, 1f
	mcr     p15, 0, r0, c2, c0	@ Set Translation Table Base
	mov	r0, #0x00000005
	mcr	p15, 0, r0, c3, c0	@ Access checking for Domain 1
	ldr	r0, 2f
	mcr	p15, 0, r0, c1, c0	@ Enable the MMU
	nop
	nop
	nop

	/* Call init() */
        ldr     pc, 4f

1:	.word	kernel_ptdir_p
#ifdef EXCEPTION_VECTOR_RELOCATED
2:	.word	(CTL_MMU_ENABLED | CTL_SYS_PROT | CTL_IVECTOR)
#else
2:	.word	(CTL_MMU_ENABLED | CTL_SYS_PROT)
#endif
3:	.word	kernel_ptdir
4:      .word   init__Fv
5:	.word	_stack_top

	/* Initializtation stack */
_stack_bottom:
	.space 4096
_stack_top:
