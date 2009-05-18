/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/notify.h
 * Description:   Code for invoking a notify function in the context
 *                of another thread.
 *                
 * @LICENSE@
 *                
 * $Id: notify.h,v 1.4 2001/11/22 14:56:36 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __MIPS_NOTIFY_H__
#define __MIPS_NOTFIY_H__

#define notify_thread(tcb, func)			\
{							\
    tcb_stack_push(tcb, (dword_t)notify_##func);	\
}

#define notify_thread_1(tcb, func, param0)		\
{							\
    tcb_stack_push(tcb, param0);			\
    tcb_stack_push(tcb, 0); /* return addr */		\
    tcb_stack_push(tcb, (dword_t)notify_##func);	\
}

#define notify_thread_2(tcb, func, param0, param1)	\
{							\
    tcb_stack_push(tcb, param1);			\
    tcb_stack_push(tcb, param0);			\
    tcb_stack_push(tcb, 0); /* return addr */		\
    tcb_stack_push(tcb, (dword_t)notify_##func);	\
}

#define comma_nightmare(params...) params

#define notify_procedure(func, params...)	\
void notify_##func( comma_nightmare(params##,) dword_t __final)

/* restore stackpointer */
#define notify_return	return

#endif /* __MIPS_NOTIFY_H__ */
