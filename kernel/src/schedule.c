/*********************************************************************
 *                
 * Copyright (C)  1999, 2000, 2001, 2002,  Karlsruhe University
 *                
 * File path:     schedule.c
 * Description:   Code dealing with scheduling and thread switching.
 *                
 * @LICENSE@
 *                
 * $Id: schedule.c,v 1.64 2002/05/13 13:04:30 stoess Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <init.h>
#include <tracepoints.h>
#include INC_ARCH(syscalls.h)

int current_max_prio L4_SECT_CPULOCAL;
tcb_t *prio_queue[MAX_PRIO + 1] L4_SECT_CPULOCAL;


void dispatch_thread(tcb_t * tcb)
{
    tcb_t * current = get_current_tcb();

    if (tcb == current)
	return;

    if (tcb == get_idle_tcb()) {
	printf("dispatch: switch to idle, current=%x, state=%x\n", 
	       current, current->thread_state);
	enter_kdebug("dispatch: switch to idle");
	switch_to_idle(current);
	return;
    }

    //printf("dispatch: %p -> %p\n", current, tcb);
    /* Do not insert TCBS not in the ready queue into the prio_queue */
    if (tcb->queue_state & TS_QUEUE_READY)
	prio_queue[tcb->priority] = tcb;

    //printf("switch_to %x->%x (%x)\n", current, tcb, tcb->thread_state);
    switch_to_thread(tcb, current);
}


/* the scheduler code */
tcb_t * find_next_thread()
{

    for (int i = current_max_prio; i >= 0; i--)
    {
	tcb_t *tcb = prio_queue[i];
	while (tcb) {
	    //printf("find_next1 (%d) %x (%x, %x)\n", i, tcb, tcb->ready_next, tcb->thread_state);
#if defined(CONFIG_DEBUG_SANITY)
	    ASSERT(tcb->queue_state & TS_QUEUE_READY);
	    ASSERT(tcb->ready_next != NULL);
#endif
	    tcb = tcb->ready_next;
	    //printf("find_next2 %x (%x, %x)\n", tcb, tcb->ready_next, tcb->thread_state);

	    if ( IS_RUNNING(tcb) && (tcb->timeslice > 0) ) 
	    {
		prio_queue[i] = tcb;
		current_max_prio = i;
		//printf("find_next: returns %p\n", tcb);
		return tcb;
	    }
	    else {
		//printf("dequeueing %p state=%x, snd_queue=%p\n", tcb, tcb->thread_state, tcb->send_queue);
		//enter_kdebug();
		thread_dequeue_ready(tcb);
		tcb = prio_queue[i];
	    }
	}
    }
    /* if we can't find a schedulable thread - switch to idle */
    //printf("find_next: returns idle\n");
    current_max_prio = -1;
    return get_idle_tcb();
}


tcb_t * parse_wakeup_queue(dword_t current_prio, qword_t current_time)
{
    tcb_t * highest_wakeup = NULL;
    tcb_t * tcb = get_idle_tcb()->wakeup_next;

    while(tcb != get_idle_tcb())
    {
	if (tcb->absolute_timeout <= current_time)
	{
	    /* we have to wakeup the guy */
	    if (tcb->priority > current_prio)
	    {
		current_prio = tcb->priority;
		highest_wakeup = tcb;
	    }
	    tcb_t * tmp = tcb->wakeup_next;
	    thread_dequeue_wakeup(tcb);
	    
	    /* set it running and enqueue into ready-queue */
	    tcb->thread_state = TS_RUNNING;
	    thread_enqueue_ready(tcb);

	    tcb = tmp;
	}
	else
	    tcb = tcb->wakeup_next;
    }
    return highest_wakeup;
}


/* this function is called from the hardware dependend 
 * interrupt service routine for a timer tick
 */
void handle_timer_interrupt()
{
#if (defined(CONFIG_DEBUGGER_KDB) || defined(CONFIG_DEBUGGER_GDB)) && defined(CONFIG_DEBUG_BREAKIN)
    kdebug_check_breakin();
#endif
    
    TRACEPOINT(HANDLE_TIMER_INTERRUPT,
	       printf("timer interrupt (curr=%p)\n", get_current_tcb()));

#if defined(CONFIG_SMP)
    /* only cpu 0 maintains the clock */
    if (get_cpu_id() == 0)
#endif
    kernel_info_page.clock += TIME_QUANTUM;

    
    tcb_t * current = get_current_tcb();
    tcb_t * wakeup = parse_wakeup_queue(current->priority, kernel_info_page.clock);

    current->current_timeslice -= TIME_QUANTUM;

    /* make sure runable threads are in the run queue */
    if (current != get_idle_tcb())
	thread_enqueue_ready(current);

    if (wakeup)
    {
	dispatch_thread(wakeup);
	return;
    }

    if (current->current_timeslice <= 0)
    {
	spin1(78);
	current->current_timeslice = current->timeslice;
	dispatch_thread(find_next_thread());
    }
}



/* this is the initial startup function of the idle
 * thread. The idle thread initializes the main task (sigma0)
 * and does all the stuff where a valid tcb is necessary.
 */
void final_init()
{
#if defined(CONFIG_SMP)
    printf("starting idler - cpu %d\n", get_apic_cpu_id());
    if (!is_boot_cpu()) {
	/* tell the boot CPU we're there */
	set_cpu_online(get_apic_cpu_id());
	return;
    }
#endif
    
    /* we initialize SMP processors here
       and have to do that BEFORE we create all the other tasks
       otherwise we don't have cpu-local kernel pagetables */
    init_arch_3();

    /* do initialization */
    create_nil_tcb();
    create_sigma0();
    create_root_task();
}


notify_procedure(initialize_idle_thread)
{
#if defined(CONFIG_DEBUG_TRACE_MISC)
    printf("idler started\n");
#endif

    /* prio queues are processor local - clear here */
    for(int i = 0; i < MAX_PRIO; i++)
	prio_queue[i] = (tcb_t *)NULL;
    current_max_prio = -1;

    /* do the final init. now we have a valid tcb and a virtual stack */
    final_init();

    /* and now simply schedule */
    while(1)
    {
	spin1(79);
#if defined(CONFIG_DEBUG_SANITY)
	if (get_idle_tcb()->queue_state & (TS_QUEUE_READY | TS_QUEUE_SEND))
	    enter_kdebug("idler has broken queue state");
#endif

	//enable_interrupts();
	tcb_t * next = find_next_thread();
	if (next != get_idle_tcb())
	    dispatch_thread(next);
	else 
	    system_sleep();
    }
}


void sys_thread_switch(l4_threadid_t tid)
{
    TRACEPOINT_1PAR(SYS_THREAD_SWITCH, tid.raw);

#if defined (CONFIG_DEBUG_TRACE_SYSCALLS)
    if (tid == L4_NIL_ID)
	spin1(75);
    else
	printf("sys_thread_switch(tid: %x)\n", tid);
#endif
    
    /* Make sure we are in the ready queue to 
     * find at least ourself and ensure that the thread 
     * is rescheduled */
    thread_enqueue_ready(get_current_tcb());

    tcb_t * tcb = tid_to_tcb(tid);
    if (!(!l4_is_nil_id(tid) && (tcb->myself == tid ) && (IS_RUNNING(tcb))))
	tcb = find_next_thread();

    /* do dispatch only if necessary */
    if (tcb != get_current_tcb())
	dispatch_thread(tcb);

    return_thread_switch();
}


void sys_schedule(schedule_param_t param, l4_threadid_t tid)
{
#if defined (CONFIG_DEBUG_TRACE_SYSCALLS)
    printf("sys_schedule (%p) tid: %x, param: %x - ip:%x\n",
	   get_current_tcb(), tid.raw, param.raw, get_user_ip(get_current_tcb()));
#endif
    TRACEPOINT(SYS_SCHEDULE,
	       printf("sys_schedule (param: %p, tid: %p) - tcb/ip: %p/%x\n",
		      param.raw, tid.raw, get_current_tcb(),
		      get_user_ip(get_current_tcb())));


    tcb_t * tcb = tid_to_tcb(tid);
    schedule_param_t oldparam;

    if (tcb->myself != tid)
	return_thread_schedule(INVALID_SCHED_PARAM);

    /* save current values for return */
    oldparam.param.prio = tcb->priority;
#if defined(CONFIG_ENABLE_SMALL_AS)
    oldparam.param.small_as = tcb->space->smallid ().value ();
#elif defined(CONFIG_SMP)
    oldparam.param.small_as = tcb->cpu;
#endif
    if (param.raw == ~0U)
    {
	/* only read out */
    }
    else
    {
	if (param.param.prio != tcb->priority)
	{
	    TRACEPOINT(SET_PRIO,
		       printf("%s: setting prio of %x %d -> %d\n",
			      __FUNCTION__, tid.raw, tcb->priority,
			      param.param.prio));
	    if (tcb->queue_state & TS_QUEUE_READY)
	    {
		thread_dequeue_ready(tcb);
		tcb->priority = (param.param.prio);
		thread_enqueue_ready(tcb);
	    }
	    else
		tcb->priority = (param.param.prio);
	}

#if defined(CONFIG_ENABLE_SMALL_AS)
	if ( (param.param.small_as != 0) &&
	     (param.param.small_as != SMALL_SPACE_INVALID) &&
	     (tcb->space != NULL) )
	{
	    smallid_t small = tcb->space->smallid ();

	    TRACEPOINT(SET_SMALL_SPACE,
		       printf("%s: setting small_as of %p %2x -> %2x\n",
			      __FUNCTION__, tid.raw, small,
			      param.param.small_as));

	    if (param.param.small_as != small.value ())
		make_small_space (tcb->space, param.param.small_as);
	}
#endif

#if defined(CONFIG_SMP)
	if (dword_t cpu = param.param.cpu)
	{
	    if (tcb != get_current_tcb() && ((cpu - 1) < CONFIG_SMP_MAX_CPU))
		smp_move_thread(tcb, cpu - 1);
	};
#endif
    }

    /* return the old values to the user */
    return_thread_schedule(oldparam);
}
