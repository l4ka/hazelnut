/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/exception.S
 * Description:   Kernel entry points and interrupt/exception stubs
 *                for ARM.
 *                
 * @LICENSE@
 *                
 * $Id: exception.S,v 1.36 2001/12/13 12:28:43 ud3 Exp $
 *                
 ********************************************************************/
#include <config.h>
#include <macros.h>
#include INC_ARCH(config.h)
#include INC_ARCH(syscalls.h)
#include INC_ARCH(cpu.h)
        


#define SWITCH_TO_SVC					  \
	str	lr, saved_lr				; \
	mrs	lr, spsr				; \
	str	lr, saved_spsr				; \
	mov	lr, $(IRQ_MASK|FIQ_MASK|KERNEL_MODE)	; \
	msr	cpsr, lr				; \
	nop						; \
	nop						; \
	nop						; \
        str	lr, [sp, #-8]!				; \
        ldr	lr, saved_spsr				; \
	msr	spsr, lr				; \
	ldr	lr, saved_lr	
	










/*	
 * Stack layout:
 *
 *  after SWITCH_TO_SVC !!!!
 *
 *             |       |
 *	       +-------+
 *             |  lr'  |  (pc after syscall)
 *             +-------+
 *   old sp -> | km_lr |
 *	       +-------+
 *             |  lr^  |  (user lr)
 *             +-------+
 *             |  sp^  |  (user sp)
 *             +-------+
 *       sp -> |  spsr |
 *             +-------+
 *             |       |
 *
	sub	lr, lr, $adjust		@ adjust lr
	str	lr, [sp, #4]		@ Stack adjusted lr
        mrs	lr, spsr                @ load SPSR into kernel_lr	     
        str     lr, [sp, #-12]!		@ Stack SPSR 
	stmib	sp, {sp,lr}^		@ Stack users sp and lr

 */


	
#define createexceptionframe(adjust)		\
	sub	lr, lr, $adjust	; 		\
	str	lr, [sp, $4]	; 		\
        mrs	lr, spsr        ; 		\
        str     lr, [sp, $-12]!	; 		\
	stmib	sp, {sp,lr}^	;		\
	nop







/*	
 * Stack layout:
 *
 *  after SWITCH_TO_SVC !!!!
 *
 *             |       |
 *	       +-------+
 *             |  lr^  |  (pc after syscall)
 *             +-------+
 *   old sp -> | km_lr |
 *	       +-------+
 *             |  xx   |
 *             +-------+
 *             |  sp^  |  (user sp)
 *             +-------+
 *       sp -> |  spsr |
 *             +-------+
 *             |       |
 *
	stmib	sp, {lr}^		@ Stack user lr as return address
        mrs	lr, spsr                @ load SPSR into kernel_lr	     
        str     lr, [sp, #-12]!		@ Stack SPSR
	stmib	sp, {sp}^		@ Stack user sp

 */


	
#define createsyscallframe			\
	stmib	sp, {lr}^	; 		\
	nop			;		\
        mrs	lr, spsr        ; 		\
        str     lr, [sp, #-12]!	; 		\
	stmib	sp, {sp}^	;		\
	nop






/*
**
** Exception vector (located on either 0x00000000 or 0xffff0000).
**
*/
        .section	.vect,"a"

	.globl	exception_vector
exception_vector:
	nop				/* RESET	*/
	b	undef_entry		/* UNDEF	*/
	b	swi_entry		/* SWI		*/
	b	inst_abort_entry	/* IABORT	*/
	b	data_abort_entry	/* DABORT	*/
	nop				/* reserved	*/
	b	irq_entry		/* IRQ		*/
	b	fiq_entry		/* FIQ		*/


/* locations to pass lr and spsr from one mode to the other
   these are globally shared !!! */
	.section	.excp,"xa"
saved_lr:	.word	0
saved_spsr:	.word	0

/*
**
** Exception entry points.
**
*/

/*
 * Exception undef ()
 *
 *    Exception is an undefined instruction.
 *
 */
undef_entry:
	SWITCH_TO_SVC
	mov	r0,#0xFE
	b	kern_kdebug_entry
	



/*
 * Exception swi ()
 *
 *    Exception is a software interrupt (typically a syscall in normal
 *    OSes).
 *
 */
swi_entry:
	SWITCH_TO_SVC
	mov	r0,#0xFD
	b	kern_kdebug_entry


	
/*
 * Exception inst_abort ()
 *
 *    Exception is a prefetch (instruction) abort.  This exception is also
 *    used for L4 syscalls.  If the exception address is in the range 0x00
 *    to 0x24 (in the exception vector page), this is interpreted as a
 *    syscall number.  Some platforms allow the exception vector to be
 *    relocated to the beginning of the last 64K of memory.  For these
 *    platforms, we use a negative (i.e. end of address space) value to
 *    indicate the syscall number.  If exception is not within the syscall
 *    range, generate a pager IPC (or panic if within the kernel).
 *
 */
inst_abort_entry:

	SWITCH_TO_SVC

#ifdef EXCEPTION_VECTOR_RELOCATED
	cmn	lr, #SYSCALL_LIMIT	@ Range Check !!! UNSIGNED !!!
	bls	prefetch_abort		@ no syscall
	sub	pc, pc, lr		@ Jump to relevant syscall
#else
	cmp	lr, #SYSCALL_LIMIT	@ Range Check !!! UNSIGNED !!!
	bhi	prefetch_abort		@ no syscall
	add	pc, pc, lr		@ Jump to relevant syscall
#endif
	nop				
	nop
	b	prefetch_abort		@ NULL pointer access ?
	b	ipc_entry		@ abort_lr = 0x08
	b	id_nearest_entry	@ abort_lr = 0x0C
	b	fpage_unmap_entry	@ abort_lr = 0x10
	b	thread_switch_entry	@ abort_lr = 0x14
	b	thread_schedule_entry	@ abort_lr = 0x18
	b	lthread_ex_regs_entry	@ abort_lr = 0x1C
	b	create_thread_entry	@ abort_lr = 0x20
	b	user_kdebug_entry	@ abort_lr = 0x24

prefetch_abort:
	createexceptionframe(4)

	stmdb   sp, {r0 - r12}^ 	@ Stack rest of user state
	nop
	sub	sp, sp, #(13*4)

	ands	lr, lr, #MODE_MASK      @ Mask all but relevant mode bits
	bne	kernel_prefetch_abort	@ Kernel abort?
	/* user prefetch abort */
	mrc     p15, 0, r0, c5, c0, 0	@ Load FSR into r0
	ldr	r1, [sp, #68]
	mov	r2, r1

	adr	lr, exception_return
#ifdef EXCEPTION_VECTOR_RELOCATED
	b	user_pagefault__FUiUiUi
#else
	ldr	pc, 1f			@ Jump to C code
#endif

#ifndef EXCEPTION_VECTOR_RELOCATED
1:	.word	user_pagefault__FUiUiUi
#endif

kernel_prefetch_abort:			@ Kernel generated IAbort
					@ Should not get IAborts in kernel
					@ Kernel Panic
	SWITCH_TO_SVC
	mov	r0,#0xFC
	b	kern_kdebug_entry

/*
 * Exception data_abort ()
 *
 *    Exception is a data abort.  If exception happened in user mode,
 *    generate pager IPC.  If exception happened in kernel mode, it is
 *    probably due to a non-mapped TCB (or else we panic).
 *
 *
 * Stack layout:
 *
 *   old sp->  |       |
 *             +-------+
 *             |  lr'  | +68
 *             +-------+
 *             | km_lr | +64
 *	       +-------+
 *             |  ulr  | +60
 *             +-------+
 *             |  usp  | +56
 *             +-------+
 *             |  spsr | +52
 *             +-------+
 *             |  r12  | +48
 *             +-------+
 *               :  : 	
 *             +-------+
 *       sp -> |  r0   | +0
 *             +-------+
 *             |       |
 *
 *
 *
 */

data_abort_entry:

	SWITCH_TO_SVC

	createexceptionframe(8)
	stmdb   sp, {r0 - r12}^ 	@ Stack rest of user state
	nop
	sub	sp, sp, #(13*4)

	ands	lr, lr, #MODE_MASK      @ Mask all but relevant mode bits
					@ NE -> kernel
	/* user data abort */
	mrc     p15, 0, r0, c5, c0, 0	@ Load FSR into r0
	mrc     p15, 0, r1, c6, c0, 0	@ Load FAR into r1
	ldr	r2, [sp, #68]

	adr	lr, exception_return	@ set return address
#ifdef EXCEPTION_VECTOR_RELOCATED
	bne	kernel_pagefault__FUiUiUi
	b	user_pagefault__FUiUiUi
#else
	ldrne	pc, 1f			@ kernel pagefault
	ldr	pc, 2f			@ user pagefault
#endif
	b	kern_kdebug_entry

1:	.word	kernel_pagefault__FUiUiUi
2:	.word	user_pagefault__FUiUiUi


/*
 * Generic return code for restoring the thread state after exceptions.
 *
 * Stack layout:
 *
 *	 sp->  |       |
 *             +-------+
 *             |  lr'  | +68
 *             +-------+
 *             | km_lr | +64
 *             +-------+
 *             |  ulr  | +60
 *             +-------+
 *             |  usp  | +56
 *             +-------+
 *             |  spsr | +52
 *             +-------+
 *             |  r12  | +48
 *             +-------+
 *               :  : 	
 *             +-------+
 *   old sp -> |  r0   | +0
 *             +-------+
 *             |       |
 *
 *
 */	
exception_return:
	ldmia	sp, {r0 - r12}^		@ Restore user state
	nop
	add	sp, sp, #(13*4)

	ldr	lr, [sp], #12		@ Unstack SPSR
	msr	spsr, lr		@ Load SPSR from kernel_lr
	ands	lr, lr, #MODE_MASK      @ Mask all but relevant mode bits
					@ NE -> kernel
	ldmeqdb	sp, {sp,lr}^		@ restore user sp and lr
	nop
	add	sp, sp, #8		@ Read return address
	ldr	lr, [sp, #-4]		@ Read return address
	moveqs	pc, lr			@ return and switch mode

	ldmdb	sp, {pc}^		@ switches mode - doesnt do anything
	nop

	bl	kern_kdebug_entry
/*
 * Exception irq ()
 *
 *    Exception is an interrupt.  Generate interrupt IPC.
 *
 */
irq_entry:	
	SWITCH_TO_SVC
	createexceptionframe(4)

	stmdb   sp, {r0 - r12}^ 	@ Stack rest of user state
	nop
	sub	sp, sp, #(13*4)

	mov	r0, lr
#ifdef EXCEPTION_VECTOR_RELOCATED
        bl      irq_handler__FUi
#else
        mov     lr, pc
        ldr     pc, 1f
#endif
	b	exception_return
1:	.word	irq_handler__FUi



/*
 * Exception fiq ()
 *
 *    Exception is a fast interrupt.  Generate interrupt IPC.
 *
 */
fiq_entry:
	SWITCH_TO_SVC
	mov	r0,#0xFD
	b	kern_kdebug_entry


	
/*
**
** Syscall entry points
**
*/








	
		
/*
 * Syscall ipc ()
 *
 *    Pre:	r0 = destination thread id
 *		r1 = Snd descriptor,
 *		r2 = Rcv descriptor,
 *		r3 = timeout
 *		r4 - r12 = 9 msg words to send
 *    Post:	r0 = Msg dope + cc,
 *		r1 = Source thread ID,
 *		r4 - r12 = 9 msg words received (or undefined).
 *
 */
ipc_entry:
	createsyscallframe

#if (L4_NUMBITS_TCBS > 10)
# error TCB is too large for get_current_tcb()!
#endif
	/* get current tcb */
	and	lr, sp, #(L4_TCB_MASK | 0x03)

	/* Stack non register arguments for C call */
#warning HACK
	stmia   lr, {r4 - r6}		@ Stack some ipc_args
	str	r3, [lr, #12]		@ Store timeout
	/* Call ipc() */
#ifdef EXCEPTION_VECTOR_RELOCATED
        bl      sys_ipc__FG13l4_threadid_tUiUi
#else
        mov     lr, pc
        ldr     pc, 1f
#endif
	and	lr, sp, #(L4_TCB_MASK | 0x03)
	/* Load registers with ipc return values */
	ldmia   lr, {r4 - r6}	@ Load return values pointer in r0
	/* Return */
        b       syscall_return
#ifndef EXCEPTION_VECTOR_RELOCATED
1:      .word   sys_ipc__FG13l4_threadid_tUiUi
#endif


	
/*
 * Syscall id_nearest ()
 *
 *    Pre:	r0 = Destination thread ID.
 *    Post:	r0 = Type,
 *		r1 = Nearest thread ID.
 *
 */
id_nearest_entry:
	createsyscallframe
	/* Call id_nearest() */
#ifdef EXCEPTION_VECTOR_RELOCATED
	bl	sys_id_nearest__FG13l4_threadid_t
#else
	mov	lr, pc
	ldr	pc, 1f
#endif
	/* Load registers with lthread_ex_regs return values */
	/* Return */
        b       syscall_return

	b	kern_kdebug_entry
#ifndef EXCEPTION_VECTOR_RELOCATED
1:	.word	sys_id_nearest__FG13l4_threadid_t
#endif



/*
 * Syscall fpage_unmap ()
 *
 *    Pre:	r0 = Fpage to unmap,
 *		r1 = Map mask.
 *
 */
fpage_unmap_entry:
	createsyscallframe
	/* Stack SPSR */
        mrs     lr, spsr        @ Load SPSR into abort_lr
        str     lr, [sp, #12]   @ Stack SPSR
	/* Call fpage_unmap() */
#ifdef EXCEPTION_VECTOR_RELOCATED
        bl      sys_fpage_unmap__FG7fpage_tUi
#else
        mov     lr, pc
        ldr     pc, 1f
#endif
	/* Return */
        b       syscall_return
#ifndef EXCEPTION_VECTOR_RELOCATED
1:      .word   sys_fpage_unmap__FG7fpage_tUi
#endif



/*
 * Syscall thread_switch ()
 *
 *    Pre:	r0 = Destination thread ID.
 *    Post:	switch to destination ID if ready or just schedule.
 *
 */
thread_switch_entry:
	createsyscallframe
	/* Call thread_switch() */
#ifdef EXCEPTION_VECTOR_RELOCATED
	bl	sys_thread_switch__FG13l4_threadid_t
#else
	mov	lr, pc
	ldr	pc, 1f
#endif
	b	syscall_return
#ifndef EXCEPTION_VECTOR_RELOCATED
1:	.word	sys_thread_switch__FG13l4_threadid_t
#endif


	
/*
 * Syscall thread_schedule ()
 *
 *    Pre:	r0 = Destination thread ID,
 *		r1 = Parameter word.
 *    Post:	r0 = Time high,
 *		r1 = Time low,
 *		r2 = Old parameter word,
 *		r3 = Partner.
 *
 */
thread_schedule_entry:
	createsyscallframe
	/* Stack SPSR */
        mrs     lr, spsr        @ Load SPSR into abort_lr
        str     lr, [sp, #12]   @ Stack SPSR
	/* Call thread_schedule() */
#ifdef EXCEPTION_VECTOR_RELOCATED
        bl	sys_schedule__FG16schedule_param_tG13l4_threadid_t
#else
	mov	lr, pc
        ldr     pc, 1f
#endif
	/* Return */
        b       syscall_return
#ifndef EXCEPTION_VECTOR_RELOCATED
1:      .word   sys_schedule__FG16schedule_param_tG13l4_threadid_t
#endif


	
/*
 * Syscall lthread_ex_regs ()
 *
 *    Pre:	r0 = thread_id,
 *		r1 = instruction pointer,
 *		r2 = stack pointer, 
 *		r3 = pager
 *    Post:	r0 = Old CPSR,
 *		r1 = Old program pointer,
 *		r2 = Old stack counter,
 *		r4 = Old pager
 *		r3 = Old preempter
 *
 */
lthread_ex_regs_entry:	
	createsyscallframe
	/* Stack Non register arguments for C call */
	str	r4, [sp, #-4]!	@ Stack r4

	/* Call lthread_ex_regs() */
#ifdef EXCEPTION_VECTOR_RELOCATED
	bl	sys_lthread_ex_regs__FUiUiUiG13l4_threadid_t
#else
	mov	lr, pc
	ldr	pc, 1f
#endif
	add	sp, sp, #4	@ Ajust stack to remove arguments
	/* Return */
	b	syscall_return
#ifndef EXCEPTION_VECTOR_RELOCATED
1:	.word	sys_lthread_ex_regs__FUiUiUiG13l4_threadid_t
#endif



/*
 * Syscall create_thread ()
 *
 *    Pre:	r0 = thread ID,
 *		r1 = template thread ID,
 *		r2 = pager thread ID,
 *		r3 = instruction pointer,
 *		r4 = stack pointer,
 *    Post:	nothing
 *
 */
create_thread_entry:
	createsyscallframe
	/* Stack Non register arguments for C call */
        stmfd	sp!, {r4}	@ Stack r4
	/* Call create_thread() */

#ifdef EXCEPTION_VECTOR_RELOCATED
        bl      sys_task_new__FG13l4_threadid_tUiT0UiUi
#else
        mov     lr, pc
        ldr     pc, 1f
#endif
        add     sp, sp, #4      @ Ajust stack to remove arguments
	/* Return */
        b       syscall_return
#ifndef EXCEPTION_VECTOR_RELOCATED
1:      .word   sys_task_new__FG13l4_threadid_tUiT0UiUi
#endif






user_kdebug_entry:
	createsyscallframe
	bl	kern_kdebug_entry
	b	syscall_return











/* Both "functions" return the value from the stack to the user,
   but obviously for different reasons. */
	.globl	switch_to_roottask__Fv
switch_to_roottask__Fv: 
	.globl	abort_ipc__Fv
abort_ipc__Fv:
	ldr	r0, [sp]		@ load the error code
	b	switch_to_user__Fv



1:	.word	KERNEL_VIRT
2:	.word	KERNEL_PHYS
	.globl	switch_to_sigma0__Fv
switch_to_sigma0__Fv: 
	ldr	r0, 1b
	ldr	r1, 2b
	sub	r1, r0, r1		@ d=v-p
	ldr	r0, [sp]		@ virt addr of kernel_info_page
	sub	r0, r0, r1		@ v-d
	b	switch_to_user__Fv


	

	.globl	switch_to_user__Fv
switch_to_user__Fv: 
	add	sp, sp, #4		@ throw away parameter
					@ fall thru to syscall_return
	
	
/*
 * Generic code for restoring the thread state after syscalls. Upon
 * entry registers should contain the syscall return values.
 *
 * Stack layout:
 *
 *   new sp -> |       |
 *	       +-------+
 *             |  lr^  |  (pc after syscall)
 *             +-------+
 *             | km_lr |  
 *	       +-------+
 *             |  xx   |  <- we could use the ulr as a return param
 *             +-------+
 *             |  sp^  |  (user sp)
 *             +-------+
 *   old sp -> | spsr  |
 *             +-------+
 *             |       |
 *
 */
	.globl syscall_return
syscall_return:
	ldr	lr, [sp]		@ Unstack SPSR
	msr	spsr, lr		@ Load SPSR from abort_lr
	ldmib	sp, {sp,lr}^		@ restore user sp and lr
	nop
	add	sp, sp, #20
	ldr	lr, [sp, #-4]		@ Read return address
	movs	pc, lr			@ return and switch mode

	
	
	
	
	


/**********************************************************************
	kdebug entry
 **********************************************************************/
	
		

3:	.word	kdebug_entry__FP17exception_frame_t

	.globl	kern_kdebug_entry
kern_kdebug_entry:
	sub	sp, sp, #12
	str	sp, [sp, #-0x10]
	str	lr, [sp, #-0x0C]
	mrs	lr, cpsr
	str	lr, [sp, #-0x08]
//	str	lr, [sp, #-0x04] /* Store cpsr in frame->spsr ??? */ 
	mrs	lr, spsr
	str	lr, [sp, #-0x04] /* Store spsr in frame->spsr */ 
	str	lr, [sp, # 0x04] /* Store spsr on stack */
	sub	sp, sp, #(15*4 + 16)
	stmia	sp, {r0-r14}^
	nop
	mov	r0, sp
	
	/* use kdebug's own stack */
	ldr	sp, _kdebug_stack_top
	stmdb	sp!, {r0}
	adr	lr, 1f
	ldr	pc, 3b
1:
	ldr	sp, _kdebug_stack_top
	ldr	sp, [sp, #-4]
	ldmia	sp, {r0-r14}^
	nop
	add	sp, sp, #(15*4 + 16)
	ldr	lr, [sp, #-0x08]
	msr	cpsr, lr
	ldr	lr, [sp, # 0x04] /* Restore spsr from stack */
	msr	spsr, lr
	ldr	lr, [sp, #-0x0C]
	ldr	sp, [sp, #-0x10]
	add	sp, sp, #12
	mov	pc, lr

	.globl	_kdebug_stack_bottom
_kdebug_stack_bottom:
	.space	1024

	.globl	_kdebug_stack_top
_kdebug_stack_top:
	.word	_kdebug_stack_top

