/*********************************************************************
 *                
 * Copyright (C) 1999-2002,  Karlsruhe University
 *                
 * File path:     tcb.h
 * Description:   The Thread Control Block structure.
 *                
 * @LICENSE@
 *                
 * $Id: tcb.h,v 1.16 2002/05/29 15:51:54 ud3 Exp $
 *                
 ********************************************************************/
#ifndef __TCB_H__
#define __TCB_H__


#include INC_ARCH(tcb.h)

#if defined(CONFIG_SMP)
# include INC_ARCH(sync.h)
#endif

/* From <arch/space.h> */
class space_t;


/*
  
  Thread Control Block (TCB)

  A TCB contains the state of a thread. However this is true for all but
  the currently executing threads.
  Every thread has its own kernel stack located behind the TCB. Most people
  use the term TCB for the combination of both the TCB data structure and
  the associated kernel stack.
  
*/

typedef struct tcb_t {
/* TCB_START_MARKER - do NOT delete */
    dword_t		ipc_buffer[3];
    timeout_t		ipc_timeout;
    dword_t		msg_desc;

    /* don't touch above - hard coded in asm!!! */

    /*
     * Task state.
     */
    space_t		*space;
    dword_t		*pagedir_cache;

    /*
     * Thread state.
     */
    l4_threadid_t	myself;
    l4_threadid_t	pager;
    dword_t		excpt;
    ptr_t		stack;
    dword_t		thread_state;
    dword_t		queue_state;
    dword_t		priority;
    dword_t		flags;

    sdword_t		timeslice;
    sdword_t		current_timeslice;
    qword_t		absolute_timeout;
	
    /*
     * Kernel links.
     */
    tcb_t		*present_prev;
    tcb_t		*present_next;
    tcb_t		*ready_prev;
    tcb_t		*ready_next;

    /*
     * Wakeup queues
     */
    tcb_t		*wakeup_prev;
    tcb_t		*wakeup_next;

    /*
     * resource management
     */
    dword_t		resources;
    dword_t		copy_area1;
    dword_t		copy_area2;

    /*
     * ipc links
     */
    tcb_t		*send_queue;
    tcb_t		*send_prev;
    tcb_t		*send_next;
    l4_threadid_t	partner;

    /*
     * interrupts
     */
    dword_t		intr_pending;

    /*
     *
     */
    ptr_t		unwind_ipc_sp;

#if defined(CONFIG_SMP)
    /*
     * SMP cpu ownership/control
     */
    dword_t		cpu;
    spinlock_t		tcb_spinlock;
#endif

    /*
     * This field is not evaluated -> it can be written
     * f.i. to force a write-pgfault in allocate_tcb
     */
    dword_t		scratch;

    /*
     * TCB magic number (easy to check stack overrun).
     */
    dword_t		magic;

    /*
     * Architecture-defined layout
     */
    tcb_priv_t		priv[0] __attribute__((aligned(16)));

/* TCB_END_MARKER - do NOT delete */
} tcb_t;

typedef union {
    tcb_t tcb __attribute__((aligned(L4_TOTAL_TCB_SIZE)));
    char pad[L4_TOTAL_TCB_SIZE];
} whole_tcb_t __attribute__((aligned(L4_TOTAL_TCB_SIZE)));


#define TCB_MAGIC	0x3F594857

/* The offset from the start of the tcb to the scratch field. */
#define TCB_SCRATCH_OFFSET ({tcb_t *t; ((dword_t) &t->scratch - (dword_t) t);})






/* the bit fields for thread states */
#define TSB_READY		0x01
#define TSB_POLLING		0x02
#define TSB_LOCKED		0x04

/*
 * Thread states use negative logic -> eases testing on x86
 */

#if !defined(CONFIG_SMP)

/* Running.  Present in ready-queue and ready to be scheduled.	*/
#define TS_RUNNING		(TSB_LOCKED | TSB_POLLING	     )

/* Sending, waiting for partner to receive my message.		*/
#define TS_POLLING		(TSB_LOCKED |	            TSB_READY)

/* found someone to send me a message				*/
#define TS_LOCKED_WAITING	(	      TSB_POLLING | TSB_READY)

/* Running, sending or receiving a message.			*/
#define TS_LOCKED_RUNNING	(	      TSB_POLLING	     )

/* dead								*/
#define TS_ABORTED		(TSB_LOCKED | TSB_POLLING | TSB_READY)

/* waiting for someone to send me a message			*/
#define TS_WAITING		(~0U)

#else /* CONFIG_SMP */

#define TSB_XCPU		0x08

#define TS_RUNNING		(TSB_XCPU | TSB_LOCKED | TSB_POLLING	        )
#define TS_POLLING		(TSB_XCPU | TSB_LOCKED |               TSB_READY)
#define TS_LOCKED_WAITING	(TSB_XCPU |              TSB_POLLING | TSB_READY)
#define TS_LOCKED_RUNNING	(TSB_XCPU | 	         TSB_POLLING		)
#define TS_ABORTED		(TSB_XCPU | TSB_LOCKED | TSB_POLLING | TSB_READY)
#define TS_XCPU_LOCKED_WAITING	(	                 TSB_POLLING | TSB_READY)
#define TS_XCPU_LOCKED_RUNNING	(                        TSB_POLLING            )
#define TS_WAITING		(~0U)

#endif /* CONFIG_SMP */

/* macros for checking thread states in IPC */
#define IS_WAITING(x)		(!(~((x)->thread_state)))
#define IS_POLLING(x)		(!((x)->thread_state & ~TS_POLLING))
#define IS_RUNNING(x)		(!((x)->thread_state & ~TS_RUNNING))





/* Queue membership - these are the bits in the queue_state member of a TCB.
   A set bit indicates that the TCB is in the respective queue, i.e. its
   member pointers for this queue are valid */
#define TS_QUEUE_READY		0x01
#define TS_QUEUE_PRESENT	0x02
#define TS_QUEUE_WAKEUP		0x04
#define TS_QUEUE_SEND		0x08




/*
 * Resource bits for resources that threads can use.
 */

void save_resources(tcb_t *current, tcb_t *dest);
void load_resources(tcb_t *current);

void init_resources(tcb_t * tcb);
void free_resources(tcb_t * tcb);
void purge_resources(tcb_t * tcb);

#define TR_IPC_MEM1		(1 << 0)
#define TR_IPC_MEM2		(1 << 1)
#define TR_IPC_MEM		(TR_IPC_MEM1|TR_IPC_MEM2)
#define TR_FPU			(1 << 2)
#define TR_LONG_IPC_PTAB	(1 << 3)
#define TR_DEBUG_REGS		(1 << 4)

#if defined(CONFIG_ENABLE_PVI)
#define TR_PVI                  (1 << 5)
#endif

#define COPYAREA_INVALID	((dword_t)~0)


/* Various thread flags. */
#define TF_SENDING_IPC		0x01	/* Set during IPC send phase. */


/* some globally used TCBs */
extern whole_tcb_t __idle_tcb;
extern tcb_t * sigma0;




/* check, whether the threads belonging to two TCBs are in the same
   address space by comparing the MMU translation table pointers */
INLINE int same_address_space(tcb_t * tcb1, tcb_t * tcb2)
{
    return (tcb1->space == tcb2->space);
}



/*
 * idle tcb stuff 
 */
INLINE tcb_t * get_idle_tcb() {
    return &__idle_tcb.tcb;
};



/*
  Present List Handling
  
  The present list contains all existing TCBs in a doubly linked list.
  Initially, only the idle_tcb is in the present list. Newly created TCBs
  are added. There is a present list per CPU.
*/

/* enqueue the TCB in the present list */
INLINE void thread_enqueue_present(tcb_t * tcb)
{
    /* check if not already in the list */
    if (!(tcb->queue_state & TS_QUEUE_PRESENT))
    {
	/* add the TCB next to the idle_tcb */
	tcb_t * idle = get_idle_tcb();

	tcb->present_prev = idle;
	tcb->present_next = idle->present_next;
	idle->present_next = tcb;
	tcb->present_next->present_prev = tcb;

	tcb->queue_state |= TS_QUEUE_PRESENT;
    }
}

INLINE void thread_dequeue_present(tcb_t * tcb)
{
    if (tcb->queue_state & TS_QUEUE_PRESENT)
    {
	tcb->present_prev->present_next = tcb->present_next;
	tcb->present_next->present_prev = tcb->present_prev;
	tcb->queue_state &= ~TS_QUEUE_PRESENT;
    }
}



#endif /* __TCB_H__ */
