/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/exception.S
 * Description:   Exception entries for MIPS.
 *                
 * @LICENSE@
 *                
 * $Id: exception.S,v 1.2 2001/11/22 13:27:53 skoglund Exp $
 *                
 ********************************************************************/
#include <mips/config.h>
#include <mips/cpu.h>
#include <mips/memory.h>

	.set	noat
	
#define ENTRY(name)		  \
	.globl name		; \
	.align 2		; \
	.type name, @function	; \
	.ent name, 0		; \
name:

#define END(name)		  \
	.globl name##__end	; \
	.end	name		; \
	.size	name,.-name	; \
name##__end:


.MACRO open_frame
	mfc0	k1, CP0_STATUS
	andi	k0, k1, ST0_KUP
	beq	k0, zero, 9f		// If we came from kernel
		la	k0, current_stack
		lw	k0, 0(k0)
	
		addiu	k0, k0, -4
		sw	sp, 0(k0)
		move	sp, k0
9:	
	addiu	sp, sp, -8
	mfc0	k0, CP0_EPC		// Return address
	sw	k1, 4(sp)		// push mode	
	sw	k0, 0(sp)		// Save return address
.ENDM


.MACRO close_frame
	.set	push
	.set	reorder

	lw	k1, 4(sp)		// Status
	lw	k0, 0(sp)		// Return address	
     
	mtc0	k1, CP0_STATUS

	andi	k1, k1, ST0_KUP
	bne	k1, zero, 9f		// If we did not come from kernel
		addiu	sp, sp, 8
		eret
		jr	k0
9:

	lw	sp, 8(sp)		// User stack pointer
	eret				// Restore from exception
	jr	k0			// Return to user
	nop
	.set pop
.ENDM
	
	
ENTRY(tlb_refill)
	.set	noreorder
	.set	noat

	la	k0, current_page_table
	lw	k0, 0(k0)

	/* get pagedir entry */
	mfc0	k1, CP0_BADVADDR
	srl	k1, k1, PAGEDIR_BITS
	sll	k1, k1, 2
	add	k0, k0, k1

	lw	k0, 0(k0)	/* k0 is pdir-entry */
	beq	k0, zero, refill_fault

	/* now the pagetable */
	mfc0	k1, CP0_BADVADDR
	srl	k1, k1, (PAGE_BITS - 2)
	andi	k1, k1, ((~PAGE_MASK) << 2)
	add	k1, k1, k0
	lw	k0, 0(k1)	/* ptab entry */
	beq	k0, zero, refill_fault
	srl	k1, k1, 3
	sll	k1, k1, 3
#if 0
	move	a0, k0
	move	a1, k1
	j	9f
#endif
		
#if 1
	lw	k0, 0(k1)
	lw	k1, 4(k1)
	srl	k0, k0, 2
	srl	k1, k1, 2
	
	mtc0	k0, CP0_ENTRYLO0
	mtc0	k1, CP0_ENTRYLO1
	b	1f
	 tlbwr					# write random tlb entry
1:	
	nop
	eret					# return from trap
#else
	lw	a0, 0(k1)
	lw	a1, 4(k1)
	move	a2, k1
9:	
	la	k0, tlb_refill__FUiUi
	jr	k0
	nop
#endif
	

refill_fault:
	la	k0, handle_pagefault
	jr	k0
	nop
END(tlb_refill)

ENTRY(handle_pagefault)
	.set	reorder
	open_frame

        addiu	sp, sp, -15*4	
		
	sw	a0, 0(sp)
	sw	a1, 4(sp)
	sw	a2, 8(sp)

	sw	t0, 12(sp)
	sw	t1, 16(sp)
	sw	t2, 20(sp)
	sw	t3, 24(sp)
	sw	t4, 28(sp)
	sw	t5, 32(sp)
	sw	t6, 36(sp)
	sw	t7, 40(sp)
	sw	v1, 44(sp)
	sw	ra, 48(sp)
	sw	v0, 52(sp)
	sw	gp, 56(sp)

	mfc0	a0, CP0_BADVADDR
	mfc0	a1, CP0_CAUSE
	mfc0	a2, CP0_EPC
	mfc0	a3, CP0_STATUS

	jal	pagefault__FUiUiUi


	lw	a0, 0(sp)
	lw	a1, 4(sp)
	lw	a2, 8(sp)

	lw	t0, 12(sp)
	lw	t1, 16(sp)
	lw	t2, 20(sp)
	lw	t3, 24(sp)
	lw	t4, 28(sp)
	lw	t5, 32(sp)
	lw	t6, 36(sp)
	lw	t7, 40(sp)
	lw	v1, 44(sp)
	lw	ra, 48(sp)
	lw	v0, 52(sp)
	lw	gp, 56(sp)
	
	addiu	sp, sp, 15*4

	close_frame
END(handle_pagefault)

ENTRY(exception_handler)
	mfc0	k0, CP0_CAUSE
	and	k1, k0, 0x3c
	la	k0, except_table
	add	k1, k0, k1		
	lw	k1, 0(k1)
	beqz	k1, 1f
		jr	k1			// Jump

1:	
	la	k0, general_exception
	jr	k0
	nop
END(exception_handler)

ENTRY(exception_cache_error)
	la	k0, mips_cache_error__Fv
	jr	k0
1:
	j	1b
END(exception_cache_error)

ENTRY(switch_to_sigma0__Fv)
	addiu	sp, 4
	/* fall through */
END(switch_to_sigma0__Fv)

// this is the general entry point for all user functions
ENTRY(switch_to_user__Fv)
	lw	k0, 0(sp)	// uip
	lw	k1, 4(sp)	// status
	mtc0	k1, CP0_STATUS
	lw	sp, 8(sp)	// usp
	eret
	 jr	k0
	
END(switch_to_user__Fv)

general_exception:
	mfc0	a1, CP0_EPC
	mfc0	a2, CP0_STATUS
	la	k0, exception__FUiUiUi
	jalr	k0
	nop
	eret

	
except_table:
	.word		general_exception
	.word		0
	.word		tlb_refill
	.word		tlb_refill
	.word		tlb_refill
	.word		tlb_refill
	.word		general_exception
	.word		general_exception
	.word		general_exception
	.word		general_exception






