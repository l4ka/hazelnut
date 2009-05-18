/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001, 2002,  Karlsruhe University
 *                
 * File path:     ipc.c
 * Description:   Implementation of the L4 IPC.
 *                
 * @LICENSE@
 *                
 * $Id: ipc.c,v 1.87 2002/06/14 20:17:53 haeber Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <init.h>
#include <tracepoints.h>
#include INC_ARCH(syscalls.h)
#include INC_ARCH(memory.h)

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

#if defined(CONFIG_MEASURE_INT_LATENCY)
extern dword_t interrupted_time;
extern dword_t interrupted_rdtsc;
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
    switch_to_thread(tcb, current);
}

INLINE void ipc_switch_to_idle(tcb_t * current)
{
    TRACEPOINT(SWITCH_TO_IDLE, 
               printf("Switch to idle\n"));

#if defined(CONFIG_DEBUG_TRACE_MISC)
    //printf("ipc_switch_to_idle: curr: %x\n", current);
#endif
    switch_to_idle(current);
}




/* 
 * interrupt ownership and interrupt ipc stuff
 */

tcb_t * interrupt_owner[MAX_INTERRUPTS];

void interrupts_init()
{
    for (int i = 0; i < MAX_INTERRUPTS; i++)
	interrupt_owner[i] = NULL;
}

void handle_interrupt(dword_t number)
{
    spin1(77);    
    tcb_t * tcb = interrupt_owner[number];
    tcb_t * current = get_current_tcb();

    //printf("handle_interrupt(%d) ass. tcb: %p\n", number, tcb);
    //enter_kdebug();

    /* if no one is associated - we simply drop the interrupt */
    if (tcb)
    {
	if (IS_WAITING(tcb) && 
	    ((tcb->partner.raw == (number + 1)) || l4_is_nil_id(tcb->partner)))
	{
	    tcb->intr_pending = (number + 1) | IRQ_IN_SERVICE;

#if defined(CONFIG_MEASURE_INT_LATENCY)
	    tcb->ipc_buffer[0] = interrupted_time;
	    tcb->ipc_buffer[1] = interrupted_rdtsc;
#endif	    
	    
	    if (current->priority >= tcb->priority)
	    {
		tcb->thread_state = TS_LOCKED_RUNNING;
		thread_enqueue_ready(tcb);
	    }
	    else {
		/* make sure runnable threads are in the ready queue */
		if (current != get_idle_tcb())
		    thread_enqueue_ready(current);

		ipc_switch_to_thread(current, tcb);
	    }

	    //printf("deliver irq-ipc (%d) to %p\n", number, tcb);
	}
	else 
	    tcb->intr_pending = number + 1;
    }
}



/*
 * long and long-long ipc
 */

// ???: Perhaps we should have a generic memcpy() instead.

#if !defined(HAVE_ARCH_IPC_COPY)
static void ipc_copy(dword_t * to, dword_t * from, dword_t len)
{
#if defined(CONFIG_DEBUG_TRACE_MISC)
    printf("ipc_copy(%p, %p, %d)\n", to, from, len);
    //enter_kdebug();
#endif

    int i = len / 4;
    while ( i-- )
	*(to++) = *(from++);

    if ( len & 2 )
    {
	*((word_t *) to) = *((word_t *) from);
	to =   (dword_t *) ((word_t *) to + 1);
	from = (dword_t *) ((word_t *) from + 1);
    }

    if ( len & 1 )
	*(byte_t *) to = *(byte_t *) from;
}
#endif /* defined(HAVE_ARCH_IPC_COPY) */


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

/* avoids inlining in ipc path :-) */
void extended_transfer(tcb_t * from, tcb_t * to, dword_t snd_desc)
{
    memmsg_t *snd_msg;
    memmsg_t *rcv_msg = NULL;
    fpage_t rcv_fpage;
    msgdope_t msgdope = (msgdope_t) { raw: {0} };
#if defined(CONFIG_ENABLE_SMALL_AS)
#   define IFSMALL(x) do { x; } while (0)
    dword_t snd_msg_limit, rcv_msg_limit = 0;
#else
#   define IFSMALL(x)
#endif

    /* first dequeue thread from wakeup queue */
    thread_dequeue_wakeup(to);

    TRACEPOINT(EXTENDED_TRANSFER,
	       printf("Extended transfer: snd_desc %p, rcv_desc %p\n",
		      snd_desc, to->msg_desc));

    if ( IS_MAP(snd_desc) && !same_address_space(from, to) )
    {
	if ( !IS_SHORT_IPC(to->msg_desc) ) 
	{
	    if ( IS_MAP(to->msg_desc) )
	    {
		/* Receive fpage is contained in receive descriptor. */
		rcv_fpage.raw = to->msg_desc & ~0x3;
	    }
	    else 
	    {
		/* Receive fpage(s) is contained in message buffer. */
		rcv_msg = (memmsg_t *) 
		    get_copy_area(from, to, (ptr_t) (to->msg_desc & ~0x3));

#if defined(CONFIG_ENABLE_SMALL_AS)
		rcv_msg_limit = get_ipc_copy_limit((dword_t) rcv_msg, to);
		check_limit((dword_t) &rcv_msg->dwords[0], rcv_msg_limit, to);
#endif

		if ( !rcv_msg )
		    enter_kdebug("!rcv_msg");

		rcv_fpage = rcv_msg->rcv_fpage;
	    }

	    if ( map_fpage(from, to, 
			   from->ipc_buffer[0],				/* snd base  */
			   (fpage_t) {raw:{from->ipc_buffer[1]}},	/* snd fpage */
			   rcv_fpage) )					/* rcv fpage */
		msgdope.raw |= IPC_MAP_MESSAGE;
	    else
		msgdope.raw |= IPC_ERR_CUTMSG;
	}
	else
	    /* Receiver is only doing short-receive ipc. */
	    msgdope.raw |= IPC_ERR_CUTMSG;
    }

    if ( IS_SHORT_IPC(snd_desc) )
    {
	IFSMALL(from->resources &= ~TR_LONG_IPC_PTAB;);
	to->msg_desc = msgdope.raw;
	if (EXPECT_FALSE(rcv_msg))
	    free_copy_area(from);
	return;
    }


    /*
     * We have a long-send ipc.
     */
    snd_msg = (memmsg_t *) (snd_desc & ~0x3);

#if defined(CONFIG_ENABLE_SMALL_AS)
    /*
     * We might need to translate snd_msg address.  We must also check
     * if message descriptor stays within its segment boundaries.
     */
    smallid_t small = from->space->smallid ();
    if (small.is_valid ())
	snd_msg = (memmsg_t *) ((dword_t) snd_msg + small.offset ());

    snd_msg_limit = get_ipc_copy_limit((dword_t) snd_msg, from);
    check_limit((dword_t) &snd_msg->dwords[0], snd_msg_limit, from);
#endif

    dword_t num_dwords_src = snd_msg->send_dope.msgdope.dwords;
    dword_t num_strings_src = snd_msg->send_dope.msgdope.strings;

    TRACEPOINT(LONG_SEND_IPC,
	       printf("Long-Send IPC: "
		      "snd_msg %p, snd_dw %d, snd_str %d, rcv_msg %p\n", 
		      snd_msg, num_dwords_src, num_strings_src, to->msg_desc));

    /* Long send but boiling down to a short. */
    if ( (num_dwords_src <= IPC_DWORD_GAP) && (num_strings_src == 0) )
    {
	IFSMALL(from->resources &= ~TR_LONG_IPC_PTAB;);
	to->msg_desc = msgdope.raw;
	if (EXPECT_FALSE(rcv_msg))
	    free_copy_area(from);
	return;
    }
    
    /* Long send but only short receive. */
    if ( IS_SHORT_IPC(to->msg_desc) )
    {
	IFSMALL(from->resources &= ~TR_LONG_IPC_PTAB;);
	to->msg_desc = msgdope.raw | IPC_ERR_CUTMSG;
	return;
    }

    /*
     * From here on we handle long-to-long IPC.
     */

#if defined(CONFIG_ENABLE_SMALL_AS)
    check_limit((dword_t) &snd_msg->dwords[0] +
		(num_dwords_src * sizeof(dword_t)) +
		(num_strings_src * sizeof(stringdope_t)),
		snd_msg_limit-3, from);
#endif

    if ( !rcv_msg )
    {
	rcv_msg = (memmsg_t *)
	    get_copy_area(from, to, (ptr_t) (to->msg_desc & ~0x3));
	IFSMALL(rcv_msg_limit = get_ipc_copy_limit((dword_t) rcv_msg, to););
    }

    TRACEPOINT(LONG_LONG_IPC,
	       printf("Long-Long IPC: "
		      "snd_dw %d, rcv_dw %d, snd_str %d, rcv_str %d\n",
		      num_dwords_src, rcv_msg->size_dope.msgdope.dwords,
		      num_strings_src, rcv_msg->size_dope.msgdope.strings));

#if defined(CONFIG_ENABLE_SMALL_AS)
    check_limit((dword_t) &rcv_msg->dwords[0] +
		(rcv_msg->size_dope.msgdope.dwords * sizeof(dword_t)) +
		(rcv_msg->size_dope.msgdope.strings * sizeof(stringdope_t)),
		rcv_msg_limit-3, to);
#endif

    /*
     * Copy dword area.
     */
    if (num_dwords_src > IPC_DWORD_GAP)
    {
	
	if ( num_dwords_src > rcv_msg->size_dope.msgdope.dwords )
	{
	    num_dwords_src = rcv_msg->size_dope.msgdope.dwords;
#if !defined(CONFIG_JOCHEN_BUGS)
	    msgdope.raw |= IPC_ERR_CUTMSG;
#endif
	}
	
#if defined(CONFIG_JOCHEN_BUGS)
	ipc_copy(&rcv_msg->dwords[IPC_DWORD_GAP],
		 &snd_msg->dwords[IPC_DWORD_GAP], 
		 (num_dwords_src - IPC_DWORD_GAP + 1) * 4);
#else                 
	ipc_copy(&rcv_msg->dwords[IPC_DWORD_GAP],
		 &snd_msg->dwords[IPC_DWORD_GAP], 
		 (num_dwords_src - IPC_DWORD_GAP) * 4);
#endif
    }

    msgdope.msgdope.dwords = num_dwords_src;


    /*
     * TODO: Handle multiple send flexpages.
     */
    if ( IS_MAP(snd_desc) )
    {
	if ( snd_msg->dwords[IPC_DWORD_GAP + 1] != 0 )
	    enter_kdebug("send multiple fpages not implemented");
    }


    /*
     * Copy indirect strings.
     */
    if ( num_strings_src )
    {
	dword_t num_strings_dest = rcv_msg->size_dope.msgdope.strings;

#if defined(CONFIG_JOCHEN_BUGS)
	/* the "[... + 1]" is for Jochen-compatibility */
	stringdope_t *dope_src = (stringdope_t *)
	    &snd_msg->dwords[snd_msg->size_dope.msgdope.dwords + 1];

	stringdope_t *dope_dest = (stringdope_t *)
	    &rcv_msg->dwords[rcv_msg->size_dope.msgdope.dwords + 1];
#else
	stringdope_t *dope_src = (stringdope_t *)
	    &snd_msg->dwords[snd_msg->size_dope.msgdope.dwords];

	stringdope_t *dope_dest = (stringdope_t *)
	    &rcv_msg->dwords[rcv_msg->size_dope.msgdope.dwords];
#endif

	char *p_src = (char *) dope_src->send_address;
	char *p_dest = (char *) dope_dest->rcv_address;

	dword_t size_src = dope_src->send_size;
	dword_t size_dest = dope_dest->rcv_size;
	dword_t size_total = 0;
	dword_t copy_size;

	for (;;)
	{
	    if ( num_strings_dest == 0 )
	    {
		msgdope.raw |= IPC_ERR_CUTMSG;
		break;
	    }
	    if ( num_strings_src == 0 )
		break;

#if defined(CONFIG_ENABLE_SMALL_AS)
	    if ( (dword_t) snd_msg >= SMALL_SPACE_START &&
		 (dword_t) snd_msg <  TCB_AREA )
		p_src += small.offset ();
	    else if ( (dword_t) rcv_msg >= SMALL_SPACE_START &&
		      (dword_t) rcv_msg <  TCB_AREA )
		p_dest += to->space->smallid ().offset ();
#endif

	    TRACEPOINT(STRING_COPY_IPC,
		       printf("snd_dope @ %p: %p, %d\n"
			      "rcv_dope @ %p: %p, %d (%p)\n", 
			      dope_src, p_src, dope_src->send_size,
			      dope_dest, p_dest, dope_dest->rcv_size,
			      msgdope.raw));

	    copy_size = size_src < size_dest ? size_src : size_dest;

#if defined(CONFIG_ENABLE_SMALL_AS)
	    check_limit((dword_t) p_src  + copy_size, snd_msg_limit, from);
	    check_limit((dword_t) p_dest + copy_size, rcv_msg_limit, to);
#endif

// ???: May not work (depending on arch) if p_dest and p_src are not
// aligned.
	    ipc_copy(get_copy_area(from, to, (ptr_t) p_dest),
		     (dword_t *) p_src, copy_size);

	    size_src -= copy_size;
	    size_dest -= copy_size;
	    size_total += copy_size;

#if defined(CONFIG_DEBUG_TRACE_MISC)
	    printf("size_src: %d, size_dest: %d, size_total: %d\n",
		   size_src, size_dest, size_total);
#endif

	    if ( size_src == 0 )
	    {
		num_strings_src--;

		if ( num_strings_src == 0 )
		{
		    /* No more strings to send. */
		    dope_dest->send_size = size_total;
		    dope_dest->send_continue = 0;
		    dope_dest->send_address = dope_dest->rcv_address;
		    msgdope.msgdope.strings++;
		    goto string_abort;
		}

		dope_src++;

		if ( dope_src->send_continue )
		{
		    /* Continue copying from next send string. */
		    p_src = (char *) dope_src->send_address;
		    size_src = dope_src->send_size;
		    p_dest += copy_size;
		    continue;
		    /* we ignore the case that dest_size is 0 -->
		       one iteration more... */
		}
		
		dope_dest->send_address = dope_dest->rcv_address;

		/*
		 * Skip to next receive dope which is not part of
		 * the current scatter gather string.
		 */

		do {
		    dope_dest->send_size = size_total;
		    dope_dest->send_continue = 0;
		    dope_dest++;
		    num_strings_dest--;
		    msgdope.msgdope.strings++;

		    if ( num_strings_dest == 0 )
		    {
			msgdope.raw |= IPC_ERR_CUTMSG;
			goto string_abort;
		    }

		    /*
		     * This will make the remaining scatter gather
		     * receive strings marked as empty.
		     */
		    size_total = 0;
			
		} while( dope_dest->rcv_continue );

		/* Found next new rcv_dope. */
		size_dest = dope_dest->rcv_size;
		p_dest = (char *) dope_dest->rcv_address;
		size_src = dope_src->send_size;
		p_src = (char *) dope_src->send_address;
	    }
	    else /* size_dest == 0 */
	    { 
		dope_dest->send_size = size_total;
		dope_dest->send_address = dope_dest->rcv_address;

		num_strings_dest--;
		msgdope.msgdope.strings++;

		if ( num_strings_dest == 0 )
		{
		    msgdope.raw |= IPC_ERR_CUTMSG;
		    goto string_abort;
		}

		dope_dest++;

		if ( dope_dest->rcv_continue )
		{
		    /* Continue copying into next receive string. */
		    p_dest = (char *) dope_dest->rcv_address;
		    size_dest = dope_dest->rcv_size;
		    p_src += copy_size;
		    size_total = 0;
		    continue;
		}
		else
		{
		    /* Receive string was too small. */
		    msgdope.raw |= IPC_ERR_CUTMSG;
		    goto string_abort;
		}
		    

		/*
		 * Skip to next send dope which is not part of the
		 * current scatter gather string.
		 */
		do { 
		    dope_src++;
		    num_strings_src--;

		    if ( num_strings_src == 0 )
			goto string_abort;
			
		} while( dope_src->send_continue );
		
		/* Found next new send_dope. */
		size_dest = dope_dest->rcv_size;
		p_dest = (char *) dope_dest->rcv_address;
		size_src = dope_src->send_size;
		p_src = (char *) dope_src->send_address;
		size_total = 0;
	    }
	}
    }

string_abort:
    free_copy_area(from);

    to->msg_desc = msgdope.raw;
    IFSMALL(from->resources &= ~TR_LONG_IPC_PTAB;);
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
#if !defined(CONFIG_SMP)

/**********************************************************************
 *                      SINGLE PROCESSOR                              *
 **********************************************************************/

//void sys_ipc(const l4_threadid_t dest, const dword_t snd_desc, const dword_t rcv_desc) __attribute__ ((section(".ipc-c")));
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

    if (EXPECT_TRUE( IS_SEND ))
    {
	to_tcb = tid_to_tcb(dest);
//	printf("ipc: send branch (%p)\n", current);

	if (EXPECT_TRUE( to_tcb->myself == dest ))
	{
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
			return_ipc(IPC_ERR_SENDTIMEOUT);
		    
		    current->absolute_timeout = absolute_timeout +
			get_current_time();

		    thread_enqueue_wakeup(current);
//		    printf("enqueue %p into wakeup; to: %d, abs timeout: %d\n",
//			   current, (dword_t) absolute_timeout,
//			   (dword_t) current->absolute_timeout);
		}
	    		    
		thread_enqueue_send(to_tcb, current);
		current->thread_state = TS_POLLING;

		/* Start waiting for send to complete. */
		ipc_switch_to_idle(current);
		
		if ( current->thread_state == TS_RUNNING )
		    /* Reactivated because of a timeout. */
		    return_ipc(IPC_ERR_SENDTIMEOUT);
		
		thread_dequeue_wakeup(current);
	    }

//	    printf("ipc: performing transfer (%p->%p) state (%x->%x)\n",
//		   current, to_tcb, current->thread_state,
//		   to_tcb->thread_state);

	    transfer_message(current, to_tcb, snd_desc);

	    /* If partner does open wait, he needs to know who we are. */
	    to_tcb->partner = current->myself;


	    /* 
	     * Since the other guy activates itself - we *must* switch.
	     */
	    if (EXPECT_FALSE( !IS_RECEIVE ))
	    {
		/* set ourself running and enqueue to ready */
		current->thread_state = TS_RUNNING;
		thread_enqueue_ready(current);

		ipc_switch_to_thread(current, to_tcb);
		return_ipc(0);
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
	tcb_t * from_tcb;
	//printf("ipc: receive branch (%x)\n", current);

	if ( current->intr_pending & IRQ_IN_SERVICE )
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
	    //printf("ipc: receive from: %x\n", from_tcb);
	} 
	else /* OPEN WAIT */ 
	{
	    if (current->intr_pending)
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
	    
	}

	current->thread_state = TS_RUNNING;
	return_ipc_args(current->partner, 0, current);
    }
enter_kdebug("endof_ipc");
    return_ipc(0);
}
#endif /* !defined(CONFIG_SMP) */


notify_procedure(copy_pagefault, dword_t fault, tcb_t * partner)
{
    tcb_t * current = get_current_tcb();

    dword_t save[3];
    timeout_t save_timeout;

    /* Save current ipc state. */
    save[0] = current->ipc_buffer[0];
    save[1] = current->ipc_buffer[1];
    save[2] = current->ipc_buffer[2];
    save_timeout = current->ipc_timeout;

    /* Perform the tunneled pagefault. */
    current->ipc_timeout = GET_RECV_PF_TIMEOUT(partner->ipc_timeout);
    do_pagefault_ipc(current, fault, 0xffffffff);

    /* Restore old ipc state. */
    current->partner = partner->myself;
    current->ipc_buffer[0] = save[0];
    current->ipc_buffer[1] = save[1];
    current->ipc_buffer[2] = save[2];
    current->ipc_timeout = save_timeout;

    current->thread_state = TS_LOCKED_WAITING;
    partner->thread_state = TS_LOCKED_RUNNING;
    thread_enqueue_ready(partner);

    /*
     * Switch back to sender.  He will restore our old stack pointer.
     * It can NEVER happen that our partner is rescheduled because we run
     * with disabled interrupts, Espen.
     */
    switch_to_thread(partner, current);
    spin(43);
}
    
    

    
int ipc_handle_kernel_id(tcb_t* current, l4_threadid_t dest)
{
#if 0
    printf("ipc sleep to: %x, %d:%d\n", current->ipc_timeout,
	   current->ipc_timeout.timeout.rcv_exp, 
	   current->ipc_timeout.timeout.rcv_man);
#endif
    
    if ( IS_INTERRUPT_ID(dest) )
    {

	dword_t irq = INTERRUPT_ID(dest) + 1;
	
	if ( current->intr_pending == irq )
	{
	    /* Interrupt is pending.  Receive it. */
	    current->intr_pending = IRQ_IN_SERVICE | irq;
#if defined(CONFIG_MEASURE_INT_LATENCY)
	    current->ipc_buffer[2] = rdtsc();
#endif	    
	    return(0);
	}
    }


    
    if ( !IS_INFINITE_RECV_TIMEOUT(current->ipc_timeout) )
    {
	qword_t absolute_timeout =
	    TIMEOUT(current->ipc_timeout.timeout.rcv_exp, 
		    current->ipc_timeout.timeout.rcv_man);
	
	if ( !absolute_timeout )
	{
	    /*
	     * Interrupt association.
	     */
	    
	    /* Detach from all attached interrupts. */
	    for (int i = 0; i < MAX_INTERRUPTS; i++)
		if ( interrupt_owner[i] == current )
		    interrupt_owner[i] = NULL;
	    
	    if ( IS_INTERRUPT_ID(dest) ) 
	    {
		if ( interrupt_owner[INTERRUPT_ID(dest)] == NULL )
		{
#if defined(CONFIG_DEBUG_TRACE_MISC)
		    printf("associate interrupt %x\n",
			   INTERRUPT_ID(dest));
#endif
		    interrupt_owner[INTERRUPT_ID(dest)] = current;
		} 
		else
		{
		    printf("interrupt association failed (%p irq: "
			   "%d)\n", current, INTERRUPT_ID(dest));
		    return(IPC_ERR_EXIST);
		}
		
		mask_interrupt(INTERRUPT_ID(dest));
	    }
	    
	    return(IPC_ERR_RECVTIMEOUT);
	}
	
	/* Wait for interrupt to be trigered. */
	current->absolute_timeout = absolute_timeout +
	    get_current_time();
	
	thread_enqueue_wakeup(current);
#if 0
	printf("sleep timeout: %d us, current time: %x, wakeup: "
	       "%x\n", (dword_t) absolute_timeout,
	       (dword_t) get_current_time(),
	       current->absolute_timeout);
#endif
    }

    /* automatic interrupt acking where available */
    if ( IS_INTERRUPT_ID(dest) )
    {
	unmask_interrupt(INTERRUPT_ID(dest));
    }
    
    /* VU: Huuuugh - next time better read the manual!!!
     *     Anyone who did a sleep-call did an open wait
     *     in Hazelnut. L4Linux really did not like that...
     */
    if (l4_is_nil_id(dest))
	dest = L4_INVALID_ID;

    current->partner = dest;
    current->thread_state = TS_WAITING;
    
    /* Start waiting for interrupt to be triggered. */
    ipc_switch_to_idle(current);
    
    if ( current->thread_state == TS_RUNNING )
	/* Reactivated because of a timeout. */
	return(IPC_ERR_RECVTIMEOUT);
    
    thread_dequeue_wakeup(current);

#if defined(CONFIG_MEASURE_INT_LATENCY)
    current->ipc_buffer[2] = rdtsc();
#endif	    

    current->thread_state = TS_RUNNING;
    return(0);
}

