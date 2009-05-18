/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/thread.h
 * Description:   MIPS thread switch code and inline functions for
 *                manipulating user stack frame.
 *                
 * @LICENSE@
 *                
 * $Id: thread.h,v 1.7 2001/11/22 14:56:36 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __MIPS_THREAD_H__
#define __MIPS_THREAD_H__

#include INC_ARCH(config.h)
#include INC_ARCH(cpu.h)

/* defined in mips/memory.c */
extern dword_t * current_page_table;
extern dword_t * current_stack;

INLINE 
tcb_t * get_current_tcb()
{
    tcb_t * tmp;
    __asm__ __volatile__ (
	"move	%0, $sp		\n"
	"srl	%0, %0, %1	\n"
	"sll	%0, %0, %1	\n"
	: "=r"(tmp)
	: "i"(L4_NUMBITS_TCBS));
    return (tmp);
}


INLINE 
ptr_t get_tcb_stack_top(tcb_t *tcb)
{
    return ptr_t (((dword_t)tcb) + L4_TOTAL_TCB_SIZE - 4);
}

INLINE 
void tcb_stack_push(tcb_t *tcb, dword_t value)
{
    *(--(tcb->stack)) = value;
}

#define USER_STATUS	(0x10)

INLINE void create_user_stack_frame(tcb_t *tcb, dword_t usp, dword_t uip, void* entry, void* param)
{
    tcb->stack = get_tcb_stack_top(tcb);
    tcb_stack_push(tcb, usp);
    tcb_stack_push(tcb, USER_STATUS);
    tcb_stack_push(tcb, uip);
    tcb_stack_push(tcb, (dword_t)param);
    tcb_stack_push(tcb, (dword_t)entry);
}

INLINE tcb_t* tid_to_tcb(l4_threadid_t tid)
{
    return (tcb_t*)((tid.id.thread << L4_NUMBITS_TCBS) + TCB_AREA);
}

INLINE tcb_t *ptr_to_tcb(ptr_t ptr)
{
    return (tcb_t *) ((dword_t) ptr & L4_TCB_MASK);
}

INLINE void allocate_tcb(tcb_t *tcb)
{
    tcb->scratch = 0;
}

INLINE void switch_to_initial_thread(tcb_t * tcb)
{
    __asm__ __volatile__ (
	"move	$sp, %0		\n"
	"lw	$31, ($sp)	\n"
	"addiu	$sp, $sp, 4	\n"
	"jr	$31		\n"
	: 
	: "r"(tcb->stack)
	);
}

INLINE void switch_to_thread(tcb_t * tcb, tcb_t * current)
{
    /* modify stack in tss */
    current_stack = get_tcb_stack_top(tcb);
    __asm__ __volatile__ (
	"addiu	$sp, -16	\n"
	"sw	$25, 0($sp)	\n"
	"sw	$28, 4($sp)	\n"
	"sw	$30, 8($sp)	\n"
	"sw	$31, 12($sp)	\n"
	"addiu	$sp, -4		\n"
	"jal	1f		\n"
	" nop			\n"
	"b	3f		\n"
	" nop			\n"
	"1:sw	$31, 0($sp)	\n"
	"sw	$sp, 0(%0)	\n"
	"move	$sp, %1		\n"
	"/* switch pagetable */	\n"
	"sw	%2, 0(%3)	\n"
	"lw	$31, 0($sp)	\n"
	"addiu	$sp, 4		\n"
	"jr	$31		\n"
	" nop			\n"
	"3:			\n"
	"lw	$31, 12($sp)	\n"
	"lw	$30, 8($sp)	\n"
	"lw	$28, 4($sp)	\n"
	"lw	$25, 0($sp)	\n"
	"addiu	$sp, 16		\n"
	:
	: "r" (&current->stack), "r"(tcb->stack), "r"(tcb->page_dir), 
	"r"(&current_page_table)
	:
#if 0
	: "$1", "$2", "$3", "$4", "$5", "$6", "$7", "$8",
	"$9", "$10", "$11", "$12", "$13", "$14", "$15", "$16",
	"$17", "$18", "$19", "$20", "$21", "$22", "$23", "$24", 
#else
	"memory");
#endif
}

INLINE void switch_to_idle(tcb_t * current)
{
    __asm__ __volatile__ (
	"addiu	$sp, -16	\n"
	"sw	$25, 0($sp)	\n"
	"sw	$28, 4($sp)	\n"
	"sw	$30, 8($sp)	\n"
	"sw	$31, 12($sp)	\n"
	"addiu	$sp, -4		\n"
	"jal	1f		\n"
	" nop			\n"
	"b	3f		\n"
	" nop			\n"
	"1:sw	$31, 0($sp)	\n"
	"sw	$sp, 0(%0)	\n"
	"move	$sp, %1		\n"
	"lw	$31, 0($sp)	\n"
	"addiu	$sp, 4		\n"
	"jr	$31		\n"
	" nop			\n"
	"3:			\n"
	"lw	$31, 12($sp)	\n"
	"lw	$30, 8($sp)	\n"
	"lw	$28, 4($sp)	\n"
	"lw	$25, 0($sp)	\n"
	"addiu	$sp, 16		\n"
	:
	: "r" (&current->stack), "r"(get_idle_tcb()->stack)
	: "$1", "$2", "$3", "$4", "$5", "$6", "$7", "$8",
	"$9", "$10", "$11", "$12", "$13", "$14", "$15", "$16",
	"$17", "$18", "$19", "$20", "$21", "$22", "$23", "$24", "memory");
}


INLINE dword_t tcb_exist(tcb_t * tcb)
{
    return (tcb->myself.raw); 
}


#define UIP(tcb) (get_tcb_stack_top(tcb)[-2])
#define USP(tcb) (get_tcb_stack_top(tcb)[-5])
#define UFL(tcb) (get_tcb_stack_top(tcb)[-3])

INLINE dword_t get_user_ip(tcb_t* tcb) { return UIP(tcb); }
INLINE dword_t get_user_sp(tcb_t* tcb) { return USP(tcb); }
INLINE dword_t get_user_flags(tcb_t* tcb) { return UFL(tcb); }
INLINE dword_t set_user_ip(tcb_t* tcb, dword_t val) return old; { old = UIP(tcb); UIP(tcb) = val; }
INLINE dword_t set_user_sp(tcb_t* tcb, dword_t val) return old; { old = USP(tcb); USP(tcb) = val; }
INLINE dword_t set_user_flags(tcb_t* tcb, dword_t val) return old; { old = UFL(tcb); UFL(tcb) = val; }


#endif /* __MIPS_THREAD_H__ */
