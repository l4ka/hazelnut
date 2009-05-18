/*********************************************************************
 *                
 * Copyright (C) 1999-2002,  Karlsruhe University
 *                
 * File path:     x86/syscalls.h
 * Description:   System call return paths for x86.
 *                
 * @LICENSE@
 *                
 * $Id: syscalls.h,v 1.38 2002/05/14 12:51:53 ud3 Exp $
 *                
 ********************************************************************/
#ifndef __X86_SYSCALLS_H__
#define __X86_SYSCALLS_H__

#include <ipc.h>

#if defined(CONFIG_ENABLE_SMALL_AS)
# define SET_UDS(_reg)			\
	"mov %1, %%"#_reg"	\n"	\
	"mov %%"#_reg", %%ds	\n"
#else
# define SET_UDS(_reg)
#endif

#define return_lthread_ex_regs(_uip, _usp, _pager, _tid, _flags)	\
{									\
    asm("/* return lthread_ex_regs */	\n"				\
	"	movl	%0, %%esp	\n"				\
	SET_UDS(ebx)							\
	"	iret			\n"				\
	:								\
	: "r"(&pager + 1), "i"(X86_UDS),				\
	  "a"(_flags), "c"(_usp), "d"(_uip), "S"(_pager)		\
	: "ebx");							\
    while(1);								\
}

#define return_create_thread()			\
{						\
    asm("/* return create_thread */	\n"	\
	"	movl	%0, %%esp	\n"	\
	SET_UDS(eax)				\
	"	iret			\n"	\
	:					\
	: "r"(&usp + 1), "i"(X86_UDS)		\
	: "eax");				\
    while(1);					\
}

#define return_thread_switch()			\
{						\
    asm("/* return thread_switch */	\n"	\
	"	movl	%0, %%esp	\n"	\
	SET_UDS(eax)				\
        "	iret			\n"	\
	:					\
	: "r"(&tid + 1), "i"(X86_UDS)		\
	: "eax");				\
     while(1);					\
}

/* does not load argument into registers */
#define return_ipc(_error)			\
{						\
    asm("/* return_ipc */	\n"		\
	"	movl %0, %%esp	\n"		\
	"	ret		\n"		\
	:					\
	: "r"(&dest - 1), "a"(_error));		\
    while(1);					\
}

/* loads the arguments into registers */
#define return_ipc_args(_from, _error, _myself)			\
{								\
    const l4_threadid_t* p = &dest - 1;				\
    asm("leal	%0, %%esp	\n"				\
	"ret			\n"				\
	: /* no output */					\
	: "m"(*p), "S"(_from),					\
	  "a"(_myself->msg_desc | _error),			\
	  "d"(_myself->ipc_buffer[0]),				\
	  "b"(_myself->ipc_buffer[1]),				\
	  "D"(_myself->ipc_buffer[2]));				\
    while(1);							\
}

#define return_task_new(_newtask)			\
{							\
    asm("/* return_task_new */		\n"		\
	"	movl	%0, %%esp	\n"		\
	SET_UDS(eax)					\
	"	iret			\n"		\
	:						\
	: "r"(&usp + 1), "i"(X86_UDS), "S"(_newtask)	\
	: "eax");					\
    while(1);						\
}

#if !defined(CONFIG_SMP)
#define return_id_nearest(_nearestid, _type)				\
{									\
    asm("/* return id_nearest */	\n"				\
	"	movl	%0, %%esp	\n"				\
	SET_UDS(ebx)							\
	"	iret			\n"				\
	:								\
	: "r"(&tid + 1), "i"(X86_UDS), "a"(_type), "S"(_nearestid)	\
	: "ebx");							\
    while(1);								\
}
#else
#define return_id_nearest(_nearestid, _type)				\
{									\
    asm("/* return id_nearest */	\n"				\
	"	movl	%0, %%esp	\n"				\
	SET_UDS(ebx)							\
	"	iret			\n"				\
	:								\
	: "r"(&tid + 1), "i"(X86_UDS), "a"(_type), "S"(_nearestid),	\
	  "c"(get_cpu_id())						\
	: "ebx");							\
    while(1);								\
}
#endif

#define return_thread_schedule(_oldparam)		\
{							\
    asm("/* return thread_schedule */	\n"		\
	"	movl	%0, %%esp	\n"		\
	SET_UDS(ebx)					\
	"	iret			\n"		\
	:						\
	: "r"(&tid + 1), "i"(X86_UDS), "a"(_oldparam)	\
	: "ebx");					\
    while(1);						\
}


INLINE dword_t do_pagefault_ipc(tcb_t * current, dword_t fault, dword_t ip)
{
    current->ipc_buffer[0] = fault;
    current->ipc_buffer[1] = ip;
    current->ipc_buffer[2] = 0;

    ptr_t tmp = current->unwind_ipc_sp;
    dword_t result;

    __asm__ __volatile__ (
	"pushl	%%ebp		\n\t"
	"pushl	$1f		\n\t"
	"mov  	%%esp,%3	\n\t"
	"pushl	%1		\n\t"	/* rcv desc	*/
	"pushl	$0		\n\t"	/* reg desc	*/
	"pushl	%2		\n\t"	/* dest		*/
#if (__GNUC__ >= 3)
	"call	_Z7sys_ipc13l4_threadid_tjj	\n\t"
#else
	"call	sys_ipc__FG13l4_threadid_tUiUi	\n\t"
#endif
	"addl 	$16, %%esp	\n"
	"1:			\n\t"
	"popl 	%%ebp		\n\t"
	: "=a"(result)
	: "i"(IPC_PAGEFAULT_DOPE), "m"(current->pager), 
	"m" (current->unwind_ipc_sp)
	: "ebx", "ecx", "edx", "esi", "edi", "memory"
	);
    current->unwind_ipc_sp = tmp;
    return result;
} 


INLINE msgdope_t do_ipc(l4_threadid_t dest, dword_t snd, dword_t rcv)
{
    msgdope_t result;
    dword_t dummy;
    __asm__ __volatile__ ("/* do_ipc */					\n"
			  "	pushl	%%ebp				\n"
			  "	pushl	%0	/* rcv desc	*/	\n"
			  "	pushl	%1	/* snd desc	*/	\n"
			  "	pushl	%2	/* dest		*/	\n"
#if (__GNUC__ >= 3)
			  "	call	_Z7sys_ipc13l4_threadid_tjj	\n"
#else
			  "	call	sys_ipc__FG13l4_threadid_tUiUi	\n"
#endif
			  "	addl	$12, %%esp			\n"
			  "	popl	%%ebp				\n"
			  : "=a" (result), "=r" (dummy), "=r" (dummy)
			  : "0"(rcv),
			    "1"(snd),
			    "2"(dest)
			  : "ecx", "esi", "edi", "memory"
	);
    return result;
} 

#endif

