/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     schedule.h
 * Description:   Inline functions and declarations dealing with
 *                sceduling.
 *                
 * @LICENSE@
 *                
 * $Id: schedule.h,v 1.20 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#include INC_ARCH(notify.h)
#include <thread.h>

#define MAX_PRIO	255

extern notify_procedure(initialize_idle_thread);

/* priority queue */
extern tcb_t * prio_queue[];
extern int current_max_prio;

/*
** ready queue handling
*/

INLINE void thread_enqueue_ready(tcb_t * tcb)
{
    if (!(tcb->queue_state & TS_QUEUE_READY))
    {
	int prio = tcb->priority;
	if (prio_queue[prio])
	{
	    tcb_t *tmp = prio_queue[prio];
	    tcb->ready_next = tmp;
	    tcb->ready_prev = tmp->ready_prev;
	    tcb->ready_prev->ready_next = tcb;
	    tmp->ready_prev = tcb;
	}
	else {
	    tcb->ready_next = tcb->ready_prev = tcb;
	    prio_queue[prio] = tcb;
	}
	tcb->queue_state |= TS_QUEUE_READY;
	if (current_max_prio < prio) current_max_prio = prio;
    }
}

INLINE void thread_dequeue_ready(tcb_t * tcb)
{
    if (tcb->queue_state & TS_QUEUE_READY)
    {
	if (tcb->ready_next == tcb) {
	    prio_queue[tcb->priority] = NULL;
#ifdef CONFIG_DEBUG_SANITY
	    tcb->ready_next = tcb->ready_prev = NULL;
#endif
	}
	else {
	    if (prio_queue[tcb->priority] == tcb)
		prio_queue[tcb->priority] = tcb->ready_next;
	    tcb->ready_prev->ready_next = tcb->ready_next;
	    tcb->ready_next->ready_prev = tcb->ready_prev;
#ifdef CONFIG_DEBUG_SANITY
	    tcb->ready_next = tcb->ready_prev = NULL;
#endif
	}
	tcb->queue_state &= ~TS_QUEUE_READY;
    }
}


/* 
 * wakup queue 
 */
INLINE void thread_enqueue_wakeup(tcb_t * tcb)
{
    if (!(tcb->queue_state & TS_QUEUE_WAKEUP))
    {
	tcb_t * tmp = get_idle_tcb();
	tcb->wakeup_next = tmp->wakeup_next;
	tmp->wakeup_next = tcb;
	tcb->wakeup_prev = tmp;
	tcb->wakeup_next->wakeup_prev = tcb;	
	tcb->queue_state |= TS_QUEUE_WAKEUP;
    }
}

INLINE void thread_dequeue_wakeup(tcb_t * tcb)
{
    if (tcb->queue_state & TS_QUEUE_WAKEUP)
    {
	tcb->wakeup_next->wakeup_prev = tcb->wakeup_prev;
	tcb->wakeup_prev->wakeup_next = tcb->wakeup_next;
#ifdef CONFIG_DEBUG_SANITY
	tcb->wakeup_next = tcb->wakeup_prev = NULL;
#endif
	tcb->queue_state &= ~TS_QUEUE_WAKEUP;
    }
}


/* 
 * send queue handling 
 */
INLINE void thread_enqueue_send(tcb_t * dest, tcb_t * from)
{
    if (!dest->send_queue)
    {
	dest->send_queue = from;
	from->send_next = from->send_prev = from;
    }
    else 
    {
	tcb_t * first = dest->send_queue;
	from->send_prev = first->send_prev;
	from->send_next = first;
	first->send_prev = from;
	from->send_prev->send_next = from;
    }
}

INLINE tcb_t * thread_dequeue_send(tcb_t * dest, tcb_t * tcb)
{
    if (tcb->send_next == tcb)
    {
	/* we are the only one :-) */
	dest->send_queue = NULL;

#ifdef CONFIG_DEBUG_SANITY
	/* and sanity for debugging */
	tcb->send_next = tcb->send_prev = NULL;
#endif
    }
    else
    {
	if (dest->send_queue == tcb)
	    dest->send_queue = tcb->send_next;

	tcb->send_prev->send_next = tcb->send_next;
	tcb->send_next->send_prev = tcb->send_prev;

#ifdef CONFIG_DEBUG_SANITY
	/* and sanity for debugging */
	tcb->send_next = tcb->send_prev = NULL;
#endif
    }

    /* coding convenience */
    return tcb;
}


/*
 * scheduler specific functions 
 */

typedef union {
    struct {
	unsigned prio		: 8;
	unsigned small_as	: 8;
	unsigned cpu		: 4;
	unsigned time_exp	: 4;
	unsigned time_man	: 8;
    } param;
    dword_t raw;
} schedule_param_t;
#define INVALID_SCHED_PARAM ((schedule_param_t){raw:0xffffffff})

tcb_t * find_next_thread();

void dispatch_thread(tcb_t * tcb);

tcb_t * parse_wakeup_queue(dword_t current_prio, qword_t current_time);

/*
 * time
 */

INLINE qword_t get_current_time()
{
    return kernel_info_page.clock;
}


#endif /* __SCHEDULE_H__ */
