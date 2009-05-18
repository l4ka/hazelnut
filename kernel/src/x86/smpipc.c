/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86/smpipc.c
 * Description:   IPC code for x86 SMP.
 *                
 * @LICENSE@
 *                
 * $Id: smpipc.c,v 1.5 2001/12/04 20:38:37 uhlig Exp $
 *                
 ********************************************************************/
#include <universe.h>

#if defined(CONFIG_SMP)

#include <init.h>
#include <tracepoints.h>
#include INC_ARCH(syscalls.h)

#define IS_SEND		(snd_desc != (dword_t)~0)
#define IS_RECEIVE	(rcv_desc != (dword_t)~0)
#define IS_OPEN_WAIT	((rcv_desc & 1) == 1)

#define IRQ_IN_SERVICE	(1<<31)

#define IS_SHORT_IPC(x)	(!(x & ~0x3))
#define IS_MAP(x)	(x & 0x2)

#if defined(CONFIG_DEBUG_TRACE_IPC)
extern dword_t __kdebug_ipc_tracing;
extern dword_t __kdebug_ipc_tr_mask;
extern dword_t __kdebug_ipc_tr_thread;
extern dword_t __kdebug_ipc_tr_this;
extern dword_t __kdebug_ipc_tr_dest;
#endif


/* we can make some assumptions here:
 * - we are not the idle thread
 * - we know that we don't activate ourself
 * --> save time :-)))
 */
INLINE void ipc_switch_to_thread(tcb_t * current, tcb_t * tcb)
{
#if defined(CONFIG_DEBUG_TRACE_MISC)
    //printf("ipc_switch_to_thread(%x->%x)\n", current, tcb);
#endif
    ASSERT(current->cpu == tcb->cpu);
    switch_to_thread(tcb, current);
}

INLINE void ipc_switch_to_idle(tcb_t * current)
{
#if defined(CONFIG_DEBUG_TRACE_MISC)
    //printf("ipc_switch_to_idle: curr: %x\n", current);
#endif
    switch_to_idle(current);
}


INLINE void transfer_message(tcb_t* const from, tcb_t* const to, const dword_t snd_desc)
{
    
    /* UD: use temporary variables to give gcc a hint
           to->ipc_buffer = from->ipc_buffer resulted in only one register
	   being used for all three dwords -> stalls	*/
    dword_t w0, w1, w2;
    w0 = from->ipc_buffer[0];
    w1 = from->ipc_buffer[1];
    w2 = from->ipc_buffer[2];
    to->ipc_buffer[0] = w0;
    to->ipc_buffer[1] = w1;
    to->ipc_buffer[2] = w2;

#if 1
#warning REVIEWME: p-bit
	/* What's up with the p-bit?? We don't support autopropagation,
	   but we can't simply assume the bit to be zero !!! */
#else
    if (snd_desc & 1)
	enter_kdebug("propagate");
#endif

    /* IPC optimization - register ipc. */
    if (EXPECT_FALSE( snd_desc & ~0x1 ))
    {
	/*
	 * Make sure that we are marked as doing IPC (in case we are
	 * descheduled).  Also set the SENDING_IPC flag while we are
	 * in the send phase.
	 */
	from->thread_state = TS_LOCKED_RUNNING;
	from->flags |= TF_SENDING_IPC;
	extended_transfer(from, to, snd_desc);
	from->thread_state = TS_RUNNING;
	from->flags &= ~TF_SENDING_IPC;
    }
    else
	to->msg_desc = 0;
}


extern struct last_ipc_t
{
    l4_threadid_t myself;
    l4_threadid_t dest;
    dword_t snd_desc;
    dword_t rcv_desc;
    dword_t ipc_buffer[3];
    dword_t ip;
} last_ipc;

/*
 * the system call
 */

/**********************************************************************
 *                       MULTI-PROCESSOR                              *
 **********************************************************************/

//#define CHECK_LOCK(l) do { if ((l)->lock) {printf("locked (%x)\n", l); enter_kdebug("check_lock");} } while(0)
#if 1
#define IPC_SPIN_LOCK(x)	spin_lock(x)
#define IPC_SPIN_UNLOCK(x)	spin_unlock(x)
#else
#define IPC_SPIN_LOCK(x)
#define IPC_SPIN_UNLOCK(x)
#endif
void sys_ipc(const l4_threadid_t dest, const dword_t snd_desc, const dword_t rcv_desc)
{
    tcb_t * current = get_current_tcb();
    tcb_t * to_tcb = NULL;

#if defined(CONFIG_DEBUG_TRACE_IPC)
    {
	last_ipc.myself = current->myself;
	last_ipc.dest = dest;
	last_ipc.snd_desc = snd_desc;
	last_ipc.rcv_desc = rcv_desc;
	last_ipc.ipc_buffer = (dword_t[]){current->ipc_buffer[0],
			       current->ipc_buffer[1], current->ipc_buffer[2]};
	last_ipc.ip = get_user_ip(current);
    }
    if ( __kdebug_ipc_tracing && 
	 /* Do we trace spcific threads? */
	 ((! __kdebug_ipc_tr_thread) ||
	  /* Do we trace IPCs invoked by this thread? */
	  (__kdebug_ipc_tr_thread == (__kdebug_ipc_tr_mask & 
				      (dword_t) current) &&
	   __kdebug_ipc_tr_this) ||
	  /* Do we trace sends _to_ this thread? */
	  (__kdebug_ipc_tr_thread == (__kdebug_ipc_tr_mask &
				      (dword_t) tid_to_tcb(dest)) &&
	   __kdebug_ipc_tr_dest)) )
    {
	printf("ipc: %x -> %x snd_desc: %x rcv_desc: %x "
	       "(%x, %x, %x) - ip=%x\n",  current->myself.raw, dest.raw,
	       snd_desc, rcv_desc, current->ipc_buffer[0],
	       current->ipc_buffer[1], current->ipc_buffer[2],
	       get_user_ip(current));
	if (__kdebug_ipc_tracing > 1)
	    enter_kdebug("ipc");
    }
#endif

    TRACEPOINT_2PAR(SYS_IPC, current->myself.raw, dest.raw,
 	            printf("ipc: %x -> %x snd_desc: %x rcv_desc: %x "
		      "(%x, %x, %x) - ip=%x\n",
		      current->myself.raw, dest.raw,
		      snd_desc, rcv_desc, current->ipc_buffer[0],
		      current->ipc_buffer[1], current->ipc_buffer[2],
		      get_user_ip(current)));

#if defined(CONFIG_DEBUG)
    if (current->queue_state & TS_QUEUE_WAKEUP)
    {
	printf("ipc: current in wakeup-queue (%x)\n", current);
	enter_kdebug("ipc error");
    }
#endif

    if (EXPECT_TRUE( IS_SEND ))
    {
	to_tcb = tid_to_tcb(dest);
//	printf("ipc: send branch (%p)\n", current);
restart_ipc_send:
	if (EXPECT_TRUE( to_tcb->myself == dest ))
	{
	    IPC_SPIN_LOCK(&to_tcb->tcb_spinlock);
	    current->partner = dest;
//	    printf("exist (%p, %x, %x)\n", to_tcb, to_tcb->partner.raw,
//		   to_tcb->thread_state );

	    if (EXPECT_FALSE( !IS_WAITING(to_tcb) ) ||
		EXPECT_FALSE( ((to_tcb->partner != current->myself)
			       && (!l4_is_nil_id(to_tcb->partner)) )))
	    {
//		printf("ipc (%p->%p): partner not ready(%x->%x,%x)\n",
//		       current, to_tcb, current->myself.raw,
//		       to_tcb->partner.raw, to_tcb->thread_state);

		/*
		 * Is either not receiving or is receiving but not from
		 * me or any.
		 */

		/* special case: send to ourself fails (JOCHEN-compatible) */
		if (to_tcb == current)
		    return_ipc(IPC_ERR_EXIST);
		    
		if ( !IS_INFINITE_SEND_TIMEOUT(current->ipc_timeout) )
		{
		    /*
		     * We have a send timeout.  Enqueue into wakeup queue.
		     */
		    qword_t absolute_timeout =
			TIMEOUT(current->ipc_timeout.timeout.snd_exp, 
				current->ipc_timeout.timeout.snd_man);

		    if ( !absolute_timeout )
		    {
			IPC_SPIN_UNLOCK(&to_tcb->tcb_spinlock);
			return_ipc(IPC_ERR_SENDTIMEOUT);
		    }
		    
		    current->absolute_timeout = absolute_timeout +
			get_current_time();

		    thread_enqueue_wakeup(current);
		}
	    		    
		thread_enqueue_send(to_tcb, current);
		current->thread_state = TS_POLLING;
		IPC_SPIN_UNLOCK(&to_tcb->tcb_spinlock);

		/* Start waiting for send to complete. */
		ipc_switch_to_idle(current);
		
		if ( current->thread_state == TS_RUNNING )
		    /* Reactivated because of a timeout. */
		    return_ipc(IPC_ERR_SENDTIMEOUT);

		/* retake spinlock - improves critical path */
		IPC_SPIN_LOCK(&to_tcb->tcb_spinlock);

		thread_dequeue_wakeup(current);
	    }

	    /* 
	     * at this point we _always_ hold the spinlock 
	     */

	    /* do message transfer */

	    if (EXPECT_TRUE( to_tcb->cpu == get_cpu_id() )) 
	    {
#if 0
#warning debug hack
		if (!l4_is_nil_id(to_tcb->partner) && 
		    to_tcb->partner != current->myself)
		{
		    printf("current=%x (%p), to=%x, ",
			   current->myself.raw, get_user_ip(current), to_tcb->myself.raw);
		    printf("to->partner=%x (ip=%p), state=%x\n",
			   to_tcb->partner, get_user_ip(to_tcb), to_tcb->thread_state);
		    
		    if (to_tcb->thread_state == TS_WAITING)
		    {
			printf("eval bool: %x\n", (!IS_WAITING(to_tcb) ) ||
			       ((to_tcb->partner != current->myself) && (!l4_is_nil_id(to_tcb->partner)) ));
			enter_kdebug("weird thread state");
		    }
		}
		current->scratch = 0;
#endif
		/* same cpu, we run with disabled interrupts */
		spin_unlock(&to_tcb->tcb_spinlock);
		transfer_message(current, to_tcb, snd_desc);

		/* what can happen here ??? */
		to_tcb->partner = current->myself;

		if (EXPECT_FALSE( !IS_RECEIVE ))
		{
		    /* set ourself running and enqueue to ready */
		    current->thread_state = TS_RUNNING;
		    thread_enqueue_ready(current);

		    ipc_switch_to_thread(current, to_tcb);
		    return_ipc(0);
		}
	    }
	    else {
		//printf("inter-processor ipc: %x->%x\n", current, to_tcb);

		/* if a send is performed, we have to signal the other cpu
		 * for a receive the partner thread is already waiting
		 * and we can perform the IPC 
		 */
		if ( current->thread_state != TS_XCPU_LOCKED_RUNNING )
		{
		    /* active send */
		    current->thread_state = TS_LOCKED_RUNNING;
		    spin_unlock(&to_tcb->tcb_spinlock);
		    if (EXPECT_TRUE( IS_SHORT_IPC(snd_desc) ))
		    {
			if (!smp_start_short_ipc(to_tcb, current)) 
			{
			    current->thread_state = TS_RUNNING;
			    goto restart_ipc_send;
			}
			transfer_message(current, to_tcb, snd_desc);
			to_tcb->partner = current->myself;
			smp_end_short_ipc(to_tcb);
		    }
		    else
		    {	
			if (!smp_start_ipc(to_tcb, current))
			{
			    current->thread_state = TS_RUNNING;
			    goto restart_ipc_send;
			}
			transfer_message(current, to_tcb, snd_desc);
			to_tcb->partner = current->myself;
			smp_end_ipc(to_tcb, current);
		    }

		}
		else /* activated by a receive */
		{
		    /* partner is already waiting -> transfer the message */
		    spin_unlock(&to_tcb->tcb_spinlock);
		    transfer_message(current, to_tcb, snd_desc);
		    to_tcb->partner = current->myself;
		    smp_end_ipc(to_tcb, current);
		}
		    
		if (EXPECT_FALSE( !IS_RECEIVE ))
		{
		    /* set ourself running and enqueue to ready */
		    current->thread_state = TS_RUNNING;
		    return_ipc(0);
		}

		/* we do not want to switch to the partner if 
		 * he resides on a different cpu
		 */
		to_tcb = NULL;
	    }
	}
	else
	{
	    /* ERROR - Destination is invalid. */
	    return_ipc(IPC_ERR_EXIST);
	}

    } /* IS_SEND */


    /*----------------------------------------------------------------
     *
     * We are receiving (perhaps in conjunction with send).
     *
     */

    /* UD: We can drop this check. If there was no send phase then there
       MUST BE a receive phase. Confirmed by Jochen.
       //if ( IS_RECEIVE )
    */
    {
restart_ipc_receive:

	tcb_t * from_tcb;
	//printf("ipc: receive branch (%x)\n", current);

	if (EXPECT_FALSE( current->intr_pending & IRQ_IN_SERVICE ))
	{
	    unmask_interrupt((current->intr_pending & ~IRQ_IN_SERVICE) - 1);
	    current->intr_pending = 0;
	}

	if (EXPECT_TRUE( !IS_OPEN_WAIT ))
	{
	    
	    /*--- receive from ---*/
	    from_tcb = tid_to_tcb(dest);
	    
	    /* special cases... taskid == 0 */
	    if (EXPECT_FALSE( IS_KERNEL_ID(dest) ))
	    {
		current->msg_desc = 0;
		return_ipc_args(dest, ipc_handle_kernel_id(current, dest), current);
	    }

	    if (EXPECT_FALSE( from_tcb->myself.raw != dest.raw ))
	    {
		/*
		 * Destination is invalid.  This can happen if we do a
		 * receive-only.  Otherwise, the send branch has
		 * tested this already.
		 */
		return_ipc(IPC_ERR_EXIST);
	    }
	    IPC_SPIN_LOCK(&current->tcb_spinlock);
	    //printf("ipc: receive from: %x\n", from_tcb);
	} 
	else /* OPEN WAIT */ 
	{
	    if (EXPECT_FALSE( current->intr_pending ))
	    {
		dword_t irq;
		/* Interrupt is pending.  Receive it. */
		irq = current->intr_pending;
		current->intr_pending = IRQ_IN_SERVICE | irq;

		if ( to_tcb )
		{
		    /*
		     * If we did a send, we must make sure that the
		     * receiver is woken up.  We can not simply make
		     * him RUNNING since that will cause him to return
		     * with an IPC_ERR_TIMEOUT.
		     */
		    current->thread_state = TS_RUNNING;
		    thread_enqueue_ready(current);

		    ipc_switch_to_thread(current, to_tcb);
		}
		/* empty msg desc */
		current->msg_desc = 0;
		return_ipc_args(irq, 0, current);
	    }

	    IPC_SPIN_LOCK(&current->tcb_spinlock);
	    from_tcb = current->send_queue;
	    //printf("ipc: wait sq: %x\n", from_tcb);
	}
	    
	current->msg_desc = rcv_desc;

	/* 
	 * no partner || partner does not exist || 
	 * partner does not receive from me 
	 */
	if ( EXPECT_FALSE(!from_tcb) || 
	     EXPECT_TRUE(!IS_POLLING(from_tcb)) ||
	     (from_tcb->partner != current->myself) )
	{
	    /*
	     * Partner is not ready to send to me yet or no partner.
	     */

	    if (EXPECT_FALSE( !IS_INFINITE_RECV_TIMEOUT(current->ipc_timeout) ))
	    {
		/*
		 * We have a recv timeout.  Enqueue into wakeup queue.
		 */
		qword_t absolute_timeout =
		    TIMEOUT(current->ipc_timeout.timeout.rcv_exp, 
			    current->ipc_timeout.timeout.rcv_man);

		if ( !absolute_timeout )
		    return_ipc(IPC_ERR_RECVTIMEOUT);
		
		current->absolute_timeout = absolute_timeout +
		    get_current_time();

		thread_enqueue_wakeup(current);
//		    printf("enqueue %p into wakeup; timeout: 0x%x",
//			   current, current->ipc_timeout.raw);
	    }

	    // this is not obvious - from_tcb can be only zero in one
	    // case - if we have an open wait. Otherwise the tcb does
	    // not exist (and we do not reach this path) or we have
	    // a valid tcb. thus, by checking for an open wait we are safe.
	    current->partner = EXPECT_FALSE(IS_OPEN_WAIT) ? L4_NIL_ID : from_tcb->myself;
	    current->thread_state = TS_WAITING;
	    IPC_SPIN_UNLOCK(&current->tcb_spinlock);

	    /*
	     * Start waiting for message to arrive.  If we are
	     * also sending to the partner, switch to him.
	     */

	    if (EXPECT_TRUE( to_tcb != NULL ))
		ipc_switch_to_thread(current, to_tcb);
	    else
		ipc_switch_to_idle(current);

	    if (EXPECT_FALSE( current->thread_state == TS_RUNNING ))
		/* Reactivated because of a timeout. */
		return_ipc(IPC_ERR_RECVTIMEOUT);

	    /* ok, got a message... */
	    /* we could - but this is no clean solution - do
            if (EXPECT_FALSE( current->queue_state & TS_QUEUE_WAKEUP ))
               here to avoid calling the function if not needed */
	    thread_dequeue_wakeup(current);
	}
	else
	{ 
	    /*
	     * Partner is ready to send to me.
	     */

	    if (EXPECT_TRUE (from_tcb->cpu == get_cpu_id()) )
	    {
		/* 
		 * if we don't switch to from_tcb we must
		 * enqueue from_tcb into the ready queue
		 */
		from_tcb->thread_state = TS_LOCKED_RUNNING;
		thread_enqueue_ready(from_tcb);

		/*
		 * The sender is waiting for us.  As such, he is in
		 * our send-queue.  Dequeue him.
		 */

		thread_dequeue_send(current, from_tcb);
		current->thread_state = TS_LOCKED_WAITING;
		
		/* this lock secured our thread state as well as 
		 * the send queue */
		IPC_SPIN_UNLOCK(&current->tcb_spinlock);


		/*
		 * Switch to our waiting partner.
		 * If we don't switch to the woken up we have
		 * to dequeue him from the wakeup queue
		 */
		thread_dequeue_wakeup(from_tcb);

		if ( to_tcb )
		    ipc_switch_to_thread(current, to_tcb);
		else
		    ipc_switch_to_thread(current, from_tcb);
		if (from_tcb->myself != current->partner)
		    enter_kdebug("oops partner invalid");
	    } 
	    else
	    {
		/* other cpu */
		current->thread_state = TS_LOCKED_WAITING;
		IPC_SPIN_UNLOCK(&current->tcb_spinlock);

		/* the partner dequeues himself from the send 
		 * queue on an ipi */
		if (!smp_start_receive_ipc(from_tcb, current)) {
		    current->thread_state = TS_LOCKED_RUNNING;
		    goto restart_ipc_receive;
		}

		/* ok, ipc send starts on other cpu now - switch 
		 * to other thread and wait for re-activation */

		if ( to_tcb )
		    ipc_switch_to_thread(current, to_tcb);
		else
		    switch_to_idle(current);
	    }
	}

	current->thread_state = TS_RUNNING;
	return_ipc_args(current->partner, 0, current);
    }
enter_kdebug("endof_ipc");
    return_ipc(0);
}
#endif /* defined(CONFIG_SMP) */
