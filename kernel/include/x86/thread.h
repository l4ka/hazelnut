/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001, 2002,  Karlsruhe University
 *                
 * File path:     x86/thread.h
 * Description:   Thread switch code and definitions and inlines for
 *                manipulating user stack frame.
 *                
 * @LICENSE@
 *                
 * $Id: thread.h,v 1.52 2002/05/07 19:12:33 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __X86_THREAD_H__
#define __X86_THREAD_H__

#include <universe.h>
#include <tcb_layout.h>

#include INC_ARCH(config.h)
#include INC_ARCH(memory.h)
#include INC_ARCH(cpu.h)


INLINE ptr_t get_tcb_stack_top(tcb_t *tcb)
{
    return ptr_t (((dword_t)tcb) + L4_TOTAL_TCB_SIZE - 4);
}


/*
 * Stack positions during sys_ipc.
 */
#define IPCSTK_USP	(-2)
#define IPCSTK_UFLAGS	(-3)
#define IPCSTK_UIP	(-5)
#define IPCSTK_RCVDESC	(-6)
#define IPCSTK_SNDDESC	(-7)
#define IPCSTK_DESTID	(-8)
#define IPCSTK_RETADDR	(-9)	/* Return address for `call sys_ipc' */


#define USP(tcb) (get_tcb_stack_top(tcb)[-2])
#define UFL(tcb) (get_tcb_stack_top(tcb)[-3])
#define UIP(tcb) (get_tcb_stack_top(tcb)[-5])

INLINE dword_t get_user_ip(tcb_t* tcb) { return UIP(tcb); }
INLINE dword_t get_user_sp(tcb_t* tcb) { return USP(tcb); }
INLINE dword_t get_user_flags(tcb_t* tcb) { return UFL(tcb); }

INLINE dword_t set_user_ip(tcb_t* tcb, dword_t val)
{ dword_t old = UIP(tcb); UIP(tcb) = val; return old; }
INLINE dword_t set_user_sp(tcb_t* tcb, dword_t val)
{ dword_t old = USP(tcb); USP(tcb) = val; return old; }
INLINE dword_t set_user_flags(tcb_t* tcb, dword_t val)
{ dword_t old = UFL(tcb); UFL(tcb) = val; return old; }



INLINE void tcb_stack_push(tcb_t* const tcb, const dword_t value)
{
    *(--(tcb->stack)) = value;
}

INLINE void create_user_stack_frame(tcb_t *tcb, dword_t usp, dword_t uip, void (*entry)(), void* param)
{
    tcb->stack = get_tcb_stack_top(tcb);
    tcb_stack_push(tcb, X86_UDS);		// stack segment
    tcb_stack_push(tcb, usp);			// user stack
    tcb_stack_push(tcb, X86_USER_FLAGS);	// flags
    tcb_stack_push(tcb, X86_UCS);		// user code segment
    tcb_stack_push(tcb, uip);			// user instruction pointer
    tcb_stack_push(tcb, (dword_t) param);	// parameter
    tcb_stack_push(tcb, (dword_t) entry);	// entry point in kernel
}


/*
 * Function is_ipc_sysenter (tcb)
 *
 *    Checks the kernel stack of TCB to see if thread is doing a sysenter/
 *    sysexit IPC.  Assumes that thread is currently performing an IPC.
 *
 */
INLINE int is_ipc_sysenter (tcb_t * tcb)
{
    extern dword_t return_from_sys_ipc;
#if defined(CONFIG_ENABLE_SMALL_AS)
    extern dword_t return_from_sys_ipc_small;
#   define SYSENTER_IPC_CHECK \
    (*stkptr == (dword_t) &return_from_sys_ipc_small || \
     *stkptr == (dword_t) &return_from_sys_ipc)
#else
#   define SYSENTER_IPC_CHECK \
    (*stkptr == (dword_t) &return_from_sys_ipc)
#endif

    dword_t *stkptr = &get_tcb_stack_top(tcb)[IPCSTK_RETADDR];

    return ((dword_t *) tcb->stack  < stkptr) && SYSENTER_IPC_CHECK;
}


INLINE tcb_t * get_current_tcb()
{
    tcb_t * tmp;
    asm("/* get_current_tcb */	\n\t"
	"movl	%%esp, %0	\n\t"
	"andl	%1, %0		\n\t"
	"/* get_current_tcb */	\n"
	: "=r"(tmp)
	: "i"(L4_TCB_MASK)
	);
    return (tmp);
}

INLINE tcb_t* const tid_to_tcb(const l4_threadid_t tid)
{
    return (tcb_t*)((tid.id.thread << L4_NUMBITS_TCBS) | TCB_AREA);
}

INLINE tcb_t *ptr_to_tcb(ptr_t ptr)
{
    return (tcb_t *) ((dword_t) ptr & L4_TCB_MASK);
}

INLINE void allocate_tcb(tcb_t *tcb)
{
    asm("orl	$0, (%0)\n"
	:
	: "r"(tcb)
	);
}


INLINE void switch_to_initial_thread(tcb_t * tcb)
{
    TRACE_THREAD_SWITCH (get_idle_tcb (), tcb);

    asm("movl %0, %%esp\n"
	"ret\n"
	:
	: "r"(tcb->stack));
	
}


#if defined(CONFIG_ENABLE_SMALL_AS)

#if !defined(OFS_TCB_STACK) || !defined(OFS_TCB_SPACE)
/* Building TCB layout. */
#else

INLINE void switch_to_thread(tcb_t* const tcb, tcb_t* const current)
{
    extern x86_tss_t __tss;

    TRACE_THREAD_SWITCH (current, tcb);

    if (EXPECT_FALSE (current->resources))
	save_resources (current, tcb);

    /* Modify stack in TSS. */
    __tss.esp0 = (dword_t) get_tcb_stack_top (tcb);

    dword_t dummy;
    __asm__ __volatile__ (
	/*
	 * Note: We do not use any memory inputs since this prohibits
	 * us from using args 0-3 or EBP to store temporary results,
	 * and more importantly WILL lead to wrong result if the input
	 * parameter is specified relative to ESP.
	 *
	 * Clobbering EBP is ignored by gcc.  We therefore have to
	 * save it manually.
	 */
	"/* switch_to_thread */			\n"
	"	pushl	%%ebp			\n"
	"	pushl	$9f			\n"
	"	movl	(%6), %%ecx		\n"
	"	movl	%%esp, %c8(%5)		\n"
	"	movl	%c8(%4), %%esp		\n"
	"	movl	__is_small, %%edx	\n"
	"	movl	%c9(%4), %%ebp		\n"
	"	cmpb	$0xff, %%cl		\n"
	"	je	2f			\n"

	"	/* switch to small space */	\n"
	"	cmpl	%c9(%5), %%ebp		\n"
	"	je	1f			\n"
	"	movl	4(%6), %%ecx		\n"
	"	movl	8(%6), %%eax		\n"
	"	movl	$gdt, %%ebp		\n"
	"	movl	%%ecx, 32(%%ebp)	\n"
	"	movl	%%eax, 36(%%ebp)	\n"
	"	orl	$0x800, %%eax		\n"
	"	movl	%%ecx, 24(%%ebp)	\n"
	"	movl	%%eax, 28(%%ebp)	\n"
	"	movl	%7, %%ecx		\n"
	"	movl	%%ecx, %%es		\n"
#if !defined(CONFIG_TRACEBUFFER)
	"	movl	%%ecx, %%fs		\n"
#endif        
	"	movl	%%ecx, %%gs		\n"
	"	testl	%%edx, %%edx		\n"
	"	jne	1f			\n"
	"	movl	$1, __is_small		\n"
	"1:	popl	%%eax			\n"
	"	jmp	*%%eax			\n"

	"2:	/* switch to large space */	\n"
	"	testl	%%edx, %%edx		\n"
	"	je	3f			\n"
	"	movl	$0, __is_small		\n"
	"	movl	$gdt, %%ebp		\n"
	"	movl	$0x0000ffff, 24(%%ebp)	\n"
	"	movl	$0x00cbfb00, 28(%%ebp)	\n"
	"	movl	$0x0000ffff, 32(%%ebp)	\n"
	"	movl	$0x00cbf300, 36(%%ebp)	\n"
	"	movl	%7, %%edx		\n"
	"	movl	%%edx, %%es		\n"
#if !defined(CONFIG_TRACEBUFFER)
	"	movl	%%edx, %%fs		\n"
#endif        
	"	movl	%%edx, %%gs		\n"

	"3:	movl	%c9(%4), %%ebp		\n"
	"	movl	%c9(%5), %%ecx		\n"
	"	popl	%%edx			\n"
	"	cmpl	%%ebp, %%ecx		\n"
	"	je	4f			\n"
	"	movl	%%ebp, %%cr3		\n"
	"4:	jmp	*%%edx			\n"

	"9:	popl	%%ebp			\n"

	: /* trash everything */
	"=a" (dummy),				// 0
	"=b" (dummy),				// 1
	"=S" (dummy),				// 2
	"=D" (dummy)				// 3

	:
	"0"(tcb),				// 4
	"1"(current),				// 5
	"2"((dword_t) tcb->space->pagedir () + SPACE_ID_IDX*4),	// 6
	"i"(X86_UDS),				// 7
	"i"(OFS_TCB_STACK),			// 8
	"i"(OFS_TCB_PAGEDIR_CACHE)		// 9

	: "ecx", "edx", "memory");

    /*
     * Check if we need to restore any resources.  Note that we have
     * to check against CURRENT rather than TCB (i.e., we want to
     * restore our own resources, not the resources of the thread we
     * switched to).
     */
    if (EXPECT_FALSE (current->resources))
	load_resources (current);
}
#endif /* Not building TCB layout */

#else /* !CONFIG_ENABLE_SMALL_AS */

INLINE void switch_to_thread(tcb_t* const tcb, tcb_t* const current)
{
    extern x86_tss_t __tss;

    TRACE_THREAD_SWITCH (current, tcb);

    if ( EXPECT_FALSE(current->resources) )
	save_resources(current, tcb);

    /* modify stack in tss */
    __tss.esp0 = (dword_t)get_tcb_stack_top(tcb);

    dword_t dummy;
    __asm__ __volatile__ (
	"/* switch_to_thread */	\n\t"
	"pushl	%%ebp		\n\t"

	"pushl	$2f		\n\t"	/* store return address	*/

	"movl	%%esp, %6	\n\t"	/* switch stacks	*/
	"movl	%7, %%esp	\n\t"

	"popl	%%ecx		\n\n"

	"cmpl	%4, %1		\n\t"	/* same address space?	*/
	"je	1f		\n\t"
	"movl	%4, %%cr3	\n\t"	/* no -> reload pagedir	*/
	"1:			\n\t"

	"jmp	*%%ecx		\n\t"

	"2:			\n\t"
	"popl	%%ebp		\n\t"
	"/* switch_to_thread */	\n\t"
	: /* trash everything */
	"=a" (dummy),				// 0
	"=b" (dummy),				// 1
	"=S" (dummy),				// 2
	"=D" (dummy)				// 3
	:
	"0" (tcb->pagedir_cache),		// 4
	"1" (current->pagedir_cache),		// 5
	"m" (current->stack),			// 6 
	"m" (tcb->stack)			// 7

	: "edx", "memory", "ecx"
	);

    if ( EXPECT_FALSE(current->resources) )
	load_resources(current);
}

#endif /* !CONFIG_ENABLE_SMALL_AS */


INLINE void switch_to_idle(tcb_t * current)
{
    dword_t dummy;

    TRACE_THREAD_SWITCH (current, get_idle_tcb ());

    if ( EXPECT_FALSE(current->resources) )
	save_resources(current, get_idle_tcb());

    /* store current pagedir in idle tcb for efficient switching */
    get_idle_tcb()->pagedir_cache = current->pagedir_cache;

    __asm__ __volatile__(
	"/* switch_to_idle */	\n\t"
	"pushl	%%ebp		\n\t"
	"pushl	$1f		\n\t"
	"movl	%%esp, %2	\n\t"
	"movl	%3, %%esp	\n\t"
	"ret			\n\t"
	"1:			\n\t"
	"popl	%%ebp		\n\t"
	"/* switch_to_idle */ \n\t"
	: "=a" (dummy), "=d"(dummy)
	: "m"(current->stack), "m"(get_idle_tcb()->stack)
	: "ebx", "ecx", "esi", "edi", "memory"
	);
}




INLINE dword_t tcb_exist(tcb_t * tcb)
{
#if 1
    return (tcb->myself.raw);
#else
    extern dword_t kernel_ptdir[];
    return (kernel_ptdir[(dword_t) tcb >> 22] & PAGE_VALID ? ((ptr_t) (kernel_ptdir[(dword_t)tcb >> 22] & ~PAGE_MASK))[((dword_t)tcb >> 12) & 0x3FF] & PAGE_VALID : 0);
#endif
}


#endif /* __X86_THREAD_H__ */

