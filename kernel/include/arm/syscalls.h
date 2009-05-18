/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/syscalls.h
 * Description:   System call return wrappers for ARM.
 *                
 * @LICENSE@
 *                
 * $Id: syscalls.h,v 1.17 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __ARM_SYSCALLS_H__
#define __ARM_SYSCALLS_H__


#ifndef ASSEMBLY

#define return_lthread_ex_regs(_uip, _usp, _pager, _tid, _flags)	\
{									\
    __asm__ __volatile__ (						\
	"/* return_lthread_ex_regs */			\n"		\
	"	mov	r0, %0				\n"		\
	"	mov	r1, %1				\n"		\
	"	mov	r2, %2				\n"		\
	"	mov	r3, %3				\n"		\
	"	mov	r4, %4				\n"		\
	:								\
	: "r" (_tid), "r"(_uip), "r"(_usp), "r"(_pager), "r"(_flags)	\
	: "r0", "r1", "r2", "r3", "r4");				\
    return;								\
}

#define return_create_thread() \
	return

#define return_thread_switch() \
	return

/* this is used in version X0 only */
#define return_task_new(_newtask)				\
{								\
    __asm__ __volatile__ (					\
	"/* return_task_new(_newtask) */		\n"	\
	"	mov	r0, %0				\n"	\
	:							\
	: "r"(_newtask)						\
	: "r0"							\
	);							\
    return;							\
}




/* does not load argument into registers */
#define return_ipc(_error)					\
{								\
    __asm__ __volatile__ (					\
	"/* return_ipc(_error) */			\n"	\
	"	mov	r0, %0		@ error	"#_error"\n"	\
	:							\
	: "r"(_error)						\
	: "r0"							\
	);							\
    return;							\
}

/* loads the arguments into registers */
#define return_ipc_args(_from, _error, _myself)			\
{								\
    __asm__ __volatile__ (					\
	"/* return_ipc_args(_from, _error) */		\n"	\
	"	mov	r1, %0		@ from in r1	\n"	\
	"	mov	r0, %1		@ error "#_error"\n"	\
	:							\
	: "r"(_from), "r"(current->msg_desc | _error)		\
	: "r0", "r1"						\
	);							\
    return;							\
}

#define return_id_nearest(_nearestid, _type)				\
{									\
    __asm__ __volatile__ (						\
	"/* return_id_nearest(_nearestid, _type) */	\n"		\
	"	mov	r0, %0				\n"		\
	"	mov	r1, %1				\n"		\
	:								\
	: "r" (_nearestid), "r" (_type)					\
	: "r0", "r1"							\
	);								\
    return;								\
}

#define return_thread_schedule(_oldparam)				\
{									\
    __asm__ __volatile__ (						\
	"/* return_schedule(_oldparam)		*/	\n"		\
	"	mov	r0, %0				\n"		\
	:								\
	: "r" (_oldparam)						\
	: "r0"								\
	);								\
    return;								\
}

INLINE void do_pagefault_ipc(tcb_t * current, dword_t fault, dword_t ip)
{
    current->ipc_buffer[0] = fault;
    current->ipc_buffer[1] = ip;
    current->ipc_buffer[2] = 0;
    l4_threadid_t pager = current->pager;
    __asm__ ("/* do_pagefault_ipc(start) */	\n"
	     "	mov	r2, %0		\n"		/* rcv fpage		*/
	     "	mov	r1, #0		\n"		/* register only send	*/
	     "	ldr	r0, %1		\n"		/* dest			*/
	     "	bl	sys_ipc__FG13l4_threadid_tUiUi\n"
	     "\t/* do_pagefault_ipc(end) */	\n"
	     :
	     : "i"(IPC_PAGEFAULT_DOPE), "m"(pager)
	     : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r14", "memory"
	     );
} 

#endif /* ASSEMBLY */




/* 0x20 -> enter kernel debugger */
#define SYSCALL_LIMIT 0x20 + 4 + 3


#endif /* __ARM_SYSCALLS_H__ */

