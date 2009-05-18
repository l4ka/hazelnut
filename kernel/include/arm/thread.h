/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/thread.h
 * Description:   ARM thread switching code, and inline functions for
 *                manupilating a user stack frame.
 *                
 * @LICENSE@
 *                
 * $Id: thread.h,v 1.25 2002/05/28 20:11:27 ud3 Exp $
 *                
 ********************************************************************/
#ifndef __ARM__THREAD_H__
#define __ARM__THREAD_H__

#include INC_ARCH(config.h)
#include INC_ARCH(space.h)
#include INC_ARCH(cpu.h)
#include <kdebug.h>



INLINE tcb_t* get_current_tcb(void)
{
    tcb_t* tcb;
    /*
     * ASSUMPTION: sp is alway word aligned.  Therefore, we don't need
     * to zero out the lowest two bits.  This works only for tcb sizes
     * of up to 1K!
     */
#if (L4_NUMBITS_TCBS > 10)
# error TCB is too large for get_current_tcb()!
#endif

    __asm__ __volatile__(
        "\n\t/* get_current_tcb(start) */	\n"
	"	and	%0, sp, %1		\n\t"
        "/* get_current_tcb(end) */		\n"
        :"=r" (tcb)
        :"i" (L4_TCB_MASK | 0x3));

   return tcb;
}


INLINE ptr_t get_tcb_stack_top(tcb_t *tcb)
{
    return ptr_t (((dword_t)tcb) + L4_TOTAL_TCB_SIZE - 16);
}

#define UIP(tcb) (get_tcb_stack_top(tcb)[-1])
#define USP(tcb) (get_tcb_stack_top(tcb)[-4])
#define UFL(tcb) (get_tcb_stack_top(tcb)[-5])

INLINE dword_t get_user_ip(tcb_t* tcb) { return UIP(tcb); }
INLINE dword_t get_user_sp(tcb_t* tcb) { return USP(tcb); }
INLINE dword_t get_user_flags(tcb_t* tcb) { return UFL(tcb); }
INLINE dword_t set_user_ip(tcb_t* tcb, dword_t val) { dword_t old = UIP(tcb); UIP(tcb) = val; return old; }
INLINE dword_t set_user_sp(tcb_t* tcb, dword_t val) { dword_t old = USP(tcb); USP(tcb) = val; return old; }
INLINE dword_t set_user_flags(tcb_t* tcb, dword_t val) { dword_t old = UFL(tcb); UFL(tcb) = val; return old; }

INLINE void tcb_stack_push(tcb_t *tcb, dword_t value)
{
    *(--(tcb->stack)) = value;
}

INLINE void create_user_stack_frame(tcb_t *tcb, dword_t usp, dword_t uip, void (*entry)(), void* param)
{
    tcb->stack = get_tcb_stack_top(tcb);
    tcb_stack_push(tcb, uip);           // user instruction pointer
    tcb_stack_push(tcb, 0);		// kernel lr - dummy
    tcb_stack_push(tcb, 0);		// user lr - dummy
    tcb_stack_push(tcb, usp);           // user stack
#if defined(CONFIG_USERMODE_NOIRQ)
    tcb_stack_push(tcb, (IRQ_MASK|FIQ_MASK|USER_MODE));     // flags
#else
    tcb_stack_push(tcb, (/*IRQ_MASK|FIQ_MASK|*/USER_MODE));     // flags
#endif
    tcb_stack_push(tcb, (dword_t) param);  // parameter
    tcb_stack_push(tcb, (dword_t) entry);  // entry point in kernel
}

INLINE tcb_t* tid_to_tcb(l4_threadid_t tid)
{
    return (tcb_t*)((tid.id.thread << L4_NUMBITS_TCBS) + TCB_AREA);
}

INLINE tcb_t *ptr_to_tcb(ptr_t ptr)
{
    return (tcb_t *) ((dword_t) ptr & L4_TCB_MASK);
}

INLINE void allocate_tcb(tcb_t* tcb)
{
    __asm__ __volatile__ (
	"/* allocate_tcb */		\n"
	"	str	r0, %0		\n"
	"\t/* allocate_tcb end */		\n"
	:
	:"m"(tcb->scratch));
}

INLINE void switch_to_initial_thread(tcb_t* tcb)
{
    asm("  mov  %%sp, %0\n"
	"  ldmia %%sp!, {%%pc}\n"
	:
	: "r"(tcb->stack));
	
}

INLINE void switch_to_thread(tcb_t* tcb, tcb_t* current)
{
    if (!same_address_space(tcb, current))
    {
	register dword_t foo = (dword_t) current; /* value unused */
	__asm__ __volatile__
	("\n\t/* switch_to_thread_and_as(start) */			\n"
	 "	stmdb	sp!, {%3, %2, %1, lr}				\n"
	 "	adr	lr, 1f			@ save return address	\n"
	 "	str	lr, [sp, #-4]!		@ ...on the old stack	\n"
	 "	mcr	p15, 0, %3, c2, c0	@ install new page table\n"
	 "	@ clean cache loop					\n"
	 "	ldr	%3, 0f						\n"
	 "2:	ldr	%0, [%3, #0]					\n"
	 "	adds	%3, %3, #16					\n"
	 "	bne	2b						\n"
	 "	mcr     p15, 0, %3, c7, c7, 0	@ flush I/D Cache	\n"
	 "	mcr     p15, 0, %3, c7, c10, 4	@ drain write buffer	\n"
	 "	mcr     p15, 0, %3, c8, c7, 0	@ flush I/D TLB		\n"
	 "	@ since the kernel is mapped in ALL address spaces	\n"
	 "	@ we should never get a code pagefault here		\n"
	 "	mov     %3, sp			@ save old sp		\n"
	 "	mov	sp, %2			@ switch to new sp	\n"
	 "	@ hereby we might experience a data abort on the old TLB\n"
	 "	@ which is served via the stack in the destination TCB	\n"
	 "	str	%3, [%1]		@ save old sp in old tcb\n"
	 "	ldr	pc, [sp], #4					\n"
	 "0:	.word	0xFFFFC000					\n"
	 "1:	ldmia	sp!, {%3, %2, %1, lr}				\n\t"
	 "/* switch_to_thread_and_as(end) */				\n"
	 : "+r" (foo)
	 : "r"(&current->stack), "r"(tcb->stack),
	   "r" (tcb->space->pagedir_phys ())
	 : "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "memory" );
    }
    else
    {
    __asm__ __volatile__
	("\n\t/* switch_to_thread(start) */	\n"
	 "	stmdb	sp!, {lr}		\n"
	 "	adr	lr, 1f			\n"
	 "	str	lr, [sp, #-4]!		\n"
	 "	str	sp, [%0]		\n"
	 "	mov	sp, %1			\n"
	 "	ldr	pc, [sp], #4		\n"
	 "1:	ldmia	sp!, {lr}		\n\t"
	 "/* switch_to_thread(end) */		\n"
	 :
	 : "r"(&current->stack), "r"(tcb->stack)
	 : //"r0", "r1",
	 "r2", "r3", 
	 "r4", "r5", "r6", "r7", 
	 "r8", "r9", "r10", "r11",
	 "r12", "r13", "r14", "memory"
	    );
    }
}

INLINE void switch_to_idle(tcb_t * current)
{
    __asm__ __volatile__
	("\n\t/* switch_to_idle(start) */	\n"
	 "	stmdb	sp!, {lr}		\n"
	 "	adr	lr, 1f			\n"
	 "	str	lr, [sp, #-4]!		\n"
	 "	str	sp, [%0]		\n"
	 "	mov	sp, %1			\n"
	 "	ldr	pc, [sp], #4		\n"
	 "1:	ldmia	sp!, {lr}		\n\t"
	 "/* switch_to_idle(end) */		\n"
	 :
	 : "r"(&current->stack), "r"(get_idle_tcb()->stack)
	 : //"r0", "r1",
	 "r2", "r3", 
	 "r4", "r5", "r6", "r7", 
	 "r8", "r9", "r10", "r11",
	 "r12", "r13", "r14", "memory"
	    );
}

INLINE dword_t tcb_exist(tcb_t * tcb)
{
#if 1
    __asm__ __volatile__ ("/* tcb_exist */");
#endif
    return (tcb->myself.raw); 
}

#endif /* __ARM__THREAD_H__ */
