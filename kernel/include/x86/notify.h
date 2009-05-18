/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86/notify.h
 * Description:   Wrappers for invoking a function within the context
 *                of another thread.
 *                
 * @LICENSE@
 *                
 * $Id: notify.h,v 1.4 2001/11/22 14:56:37 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __X86_NOTIFY_H__
#define __X86_NOTFIY_H__

#define notify_thread(tcb, func)			\
{							\
    tcb_stack_push(tcb, 0); /* dummy */			\
    tcb_stack_push(tcb, 0); /* return addr */		\
    tcb_stack_push(tcb, (dword_t)notify_##func);	\
}

#define notify_thread_1(tcb, func, param0)		\
{							\
    tcb_stack_push(tcb, param0);			\
    tcb_stack_push(tcb, 0); /* dummy */			\
    tcb_stack_push(tcb, 0); /* return addr */		\
    tcb_stack_push(tcb, (dword_t)notify_##func);	\
}

#define notify_thread_2(tcb, func, param0, param1)	\
{							\
    tcb_stack_push(tcb, param1);			\
    tcb_stack_push(tcb, param0);			\
    tcb_stack_push(tcb, 0); /* dummy */			\
    tcb_stack_push(tcb, 0); /* return addr */		\
    tcb_stack_push(tcb, (dword_t)notify_##func);	\
}

#define notify_procedure(func, params...)	\
void notify_##func(dword_t __dummy ,##params, dword_t __final)

/* restore stackpointer */
#define notify_return				\
	asm("movl %0, %%esp\n"			\
	    "ret\n"				\
	    :					\
	    : "r"(&__final));


#endif /* __X86_NOTIFY_H__ */

