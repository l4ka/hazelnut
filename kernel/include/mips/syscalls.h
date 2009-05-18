/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/syscalls.h
 * Description:   System call return wrappers for MIPS.
 *                
 * @LICENSE@
 *                
 * $Id: syscalls.h,v 1.3 2001/11/22 14:56:36 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __MIPS_SYSCALLS_H__
#define __MIPS_SYSCALLS_H__

#include <ipc.h>

void sys_lthread_ex_regs(l4_threadid_t tid, dword_t uip, dword_t usp, l4_threadid_t pager) __attribute__((noreturn));// __attribute__((regparm(3)));

#if 1
#define return_lthread_ex_regs(_uip, _usp, _pager, _tid, _flags)
#define return_create_thread()					
#define return_thread_switch()					
#define return_ipc(_error)					
#define return_ipc_args(_from, _error, _myself)			
#define return_id_nearest(_tid, _type)
#define return_task_new(_newtask)
#define return_thread_schedule(_oldparam)
#else
#define return_lthread_ex_regs(_uip, _usp, _pager, _tid, _flags)\
{								\
    asm("/* load values into regs */\n"				\
	"movl %0, %%esp\n"					\
	"iret\n"						\
	:							\
	: "r"(&pager + 1), "a"(_flags), "c"(_usp), "d"(_uip), "S"(_pager));\
    while(1);							\
}

#define return_create_thread()					\
{								\
    asm("movl %0, %%esp\n"					\
	"iret\n"						\
	:							\
	: "r"(&usp + 1));					\
    while(1);							\
}

#define return_thread_switch()					\
{								\
    asm("movl %0, %%esp\n"					\
        "iret\n"						\
	:							\
	: "r"(&tid + 1));					\
     while(1);							\
}

/* does not load argument into registers */
#define return_ipc(_error)					\
{								\
    asm("movl %0, %%esp\n"					\
	"ret\n"							\
	:							\
	: "r"(&dest - 1), "a"(_error));				\
    while(1);							\
}

/* loads the arguments into registers */
#define return_ipc_args(_from, _error, _myself)			\
{								\
    asm("movl %0, %%esp\n"					\
	"movl 8(%%edx), %%ecx\n"				\
	"movl 4(%%edx), %%ebx\n"				\
	"movl 0(%%edx), %%edx\n"				\
	"ret\n"							\
	:							\
	: "r"(&dest - 1), "S"(_from),				\
	  "a"(current->msg_desc | _error), "d"(_myself));	\
    while(1);							\
}
#endif


INLINE void do_pagefault_ipc(tcb_t * current, dword_t fault, dword_t ip)
{
    current->ipc_buffer[0] = fault & ~3;
    current->ipc_buffer[1] = ip;
    current->ipc_buffer[2] = 0;
    current->ipc_buffer[3] = 0;
#if 0
    __asm__ __volatile__ ("pushl	%%ebp\n"
			  "pushl	%0\n"		/* rcv fpage	*/
			  "pushl	$0\n"		/* reg send	*/
			  "pushl	%1\n"		/* dest		*/
			  "call	sys_ipc__FG13l4_threadid_tUiUi\n"
			  "addl	$12, %%esp\n"
			  "popl		%%ebp\n"
			  :
			  : "i"(IPC_PAGEFAULT_DOPE), "r"(current->pager)
			  : "eax", "ebx", "ecx", "edx", "esi", "edi", "memory"
	);
#endif
} 

#endif

