/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/notify.h
 * Description:   Wrappers for invoking notification functions within
 *                the context of other threads.
 *                
 * @LICENSE@
 *                
 * $Id: notify.h,v 1.3 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __ARM__NOTIFY_H__
#define __ARM__NOTFIY_H__

#define notify_thread(tcb, func)			\
{							\
    tcb_stack_push(tcb, (dword_t)notify_##func);	\
}

#define notify_thread_1(tcb, func, param0)		\
{							\
    tcb_stack_push(tcb, param0);			\
    tcb_stack_push(tcb, (dword_t)notify_##func);	\
}

#define notify_thread_2(tcb, func, param0, param1)	\
{							\
    tcb_stack_push(tcb, param1);			\
    tcb_stack_push(tcb, param0);			\
    tcb_stack_push(tcb, (dword_t)notify_##func);	\
}

#define comma_nightmare(params...) params

#define notify_procedure(func, params...)	\
void notify_##func( comma_nightmare(params##,) dword_t __final)

/* restore stackpointer */
#define notify_return				\
	asm("movl %0, %%esp\n"			\
	    "ret\n"				\
	    :					\
	    : "r"(&__final));


#endif /* __ARM__NOTIFY_H__ */
