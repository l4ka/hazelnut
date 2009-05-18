/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     thread.h
 * Description:   Definitions of thread IDs and functions operating on
 *                threads.
 *                
 * @LICENSE@
 *                
 * $Id: thread.h,v 1.58 2001/12/04 16:55:46 uhlig Exp $
 *                
 ********************************************************************/
#ifndef __THREAD_H__
#define __THREAD_H__


#define L4_X0_VERSION_BITS	    10
#define L4_X0_THREADID_BITS	    6
#define L4_X0_TASKID_BITS	    8

typedef union {
    struct {
	unsigned	version		: L4_X0_VERSION_BITS;
	unsigned	thread		: (L4_X0_THREADID_BITS + L4_X0_TASKID_BITS);
	unsigned	chief		: L4_X0_TASKID_BITS;
    } id;

    struct {
	unsigned	version		: L4_X0_VERSION_BITS;
	unsigned	thread		: L4_X0_THREADID_BITS;
	unsigned	task		: L4_X0_TASKID_BITS;
	unsigned	chief		: L4_X0_TASKID_BITS;
    } x0id;
    
    dword_t raw;
} l4_threadid_t;

/*
 * Some well known thread id's.
 */
#define L4_KERNEL_ID	((l4_threadid_t) { x0id : {0,1,0,0} })
#define L4_SIGMA0_ID	((l4_threadid_t) { x0id : {1,0,2,4} })
#define L4_ROOT_TASK_ID	((l4_threadid_t) { x0id : {1,0,4,4} })
#define L4_GRABKMEM_ID	L4_KERNEL_ID

/* 
 * be carefull - this has some context
 * interrupt-ids are always checked *AFTER* IS_KERNEL_ID
 * but makes sense for performance reasons
 */
#define IS_KERNEL_ID(x)	    (x.x0id.task == 0)
#define IS_INTERRUPT_ID(x)  (x.raw)
#define INTERRUPT_ID(x)	    (x.raw - 1)


#define L4_NIL_ID	((l4_threadid_t) { raw : 0x00000000 })
#define L4_INVALID_ID	((l4_threadid_t) { raw : 0xffffffff })

#define l4_is_nil_id(id)	((id).raw == L4_NIL_ID.raw)
#define l4_is_invalid_id(id)	((id).raw == L4_INVALID_ID.raw)

static inline int operator == (const l4_threadid_t & t1,
			       const l4_threadid_t & t2)
{
    return t1.raw == t2.raw;
}

static inline int operator != (const l4_threadid_t & t1,
			       const l4_threadid_t & t2)
{
    return t1.raw != t2.raw;
}


INLINE dword_t threadnum(tcb_t *tcb)
{
    return ((dword_t) tcb >> L4_NUMBITS_TCBS) & 
	((1 << L4_X0_THREADID_BITS) - 1);
}

INLINE dword_t tasknum(tcb_t *tcb)
{
    return ((dword_t) tcb >> (L4_NUMBITS_TCBS + L4_X0_THREADID_BITS)) & 
	((1 << L4_X0_TASKID_BITS) - 1);
}


void unwind_ipc(tcb_t *tcb);

/* several ways of returning to the user */
extern void abort_ipc();
extern void switch_to_user();

class space_t;
space_t * create_space (void);
void delete_space (space_t * space);


#if defined(CONFIG_ENABLE_SWITCH_TRACE)

typedef struct {
    dword_t	 f_uip;
    dword_t	 f_usp;
    dword_t	 t_uip;
    dword_t	 t_usp;
    tcb_t	*t_tcb;
} switch_trace_t;

enum switch_trace_type_t {
    SWTRACE_NONE,
    SWTRACE_ALL,
    SWTRACE_TASK,
    SWTRACE_THREAD
};

extern switch_trace_type_t kdebug_switch_trace_type;
extern switch_trace_t kdebug_switch_trace[];
extern dword_t kdebug_switch_trace_idx;
extern tcb_t *kdebug_switch_trace_id;

#define TRACE_THREAD_SWITCH(from, to)					 \
do {									 \
    if ((kdebug_switch_trace_type == SWTRACE_NONE)			 \
	|| (kdebug_switch_trace_type == SWTRACE_TASK &&			 \
	    tasknum (from) != tasknum (kdebug_switch_trace_id) &&	 \
	    tasknum (to) != tasknum (kdebug_switch_trace_id))		 \
	|| (kdebug_switch_trace_type == SWTRACE_THREAD &&		 \
	    from != kdebug_switch_trace_id &&				 \
	    to != kdebug_switch_trace_id))				 \
	break;								 \
									 \
    switch_trace_t *st =  kdebug_switch_trace + kdebug_switch_trace_idx; \
    st->f_uip = get_user_ip (from);					 \
    st->f_usp = get_user_sp (from);					 \
    st->t_uip = get_user_ip (to);					 \
    st->t_usp = get_user_sp (to);					 \
    st->t_tcb = to;							 \
									 \
    if (++kdebug_switch_trace_idx == CONFIG_SWITCH_TRACE_SIZE)		 \
	kdebug_switch_trace_idx = 0;					 \
} while (0)

#else /* !CONFIG_ENABLE_SWITCH_TRACE */
#define TRACE_THREAD_SWITCH(from, to)
#endif


#endif /* __THREAD_H__ */


