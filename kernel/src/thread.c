/*********************************************************************
 *                
 * Copyright (C) 1999-2002, 2004,  Karlsruhe University
 *                
 * File path:     thread.c
 * Description:   Thread creating and deletion.
 *                
 * @LICENSE@
 *                
 * $Id: thread.c,v 1.90 2004/01/22 13:52:22 ud3 Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <init.h>
#include <tracepoints.h>
#include INC_ARCH(syscalls.h)

#if defined(CONFIG_IO_FLEXPAGES)
#include INC_ARCH(io_mapping.h)
#endif


/* the idle thread has a dedicated tcb which is not in the paged area */
whole_tcb_t __idle_tcb L4_SECT_CPULOCAL;

/* tcb pointer to sigma0 */
tcb_t * sigma0;


static tcb_t * create_tcb(l4_threadid_t tid, space_t *space);

extern void switch_to_sigma0();
extern void switch_to_roottask();
extern ptr_t kernel_arg;

void create_nil_tcb(void)
{
    /* Make sure the NIL_ID thread contains an invalid thread id. */
    create_tcb(L4_NIL_ID, (space_t *) get_kernel_pagetable())->myself =
	L4_INVALID_ID;
}

void create_sigma0()
{
    if (kernel_info_page.sigma0_ip != 0xFFFFFFFF)
    {
	sigma0 = create_tcb(L4_SIGMA0_ID, NULL);
	if (!sigma0)
	    panic("create_sigma0: create_tcb() failed");
#if defined(CONFIG_DEBUG_TRACE_MISC)
	printf("sigma0: %p, %x\n", sigma0, sigma0->myself.raw);
#endif

	create_user_stack_frame(sigma0,
				kernel_info_page.sigma0_sp, kernel_info_page.sigma0_ip,
				switch_to_sigma0, &kernel_info_page);

#if defined(CONFIG_IO_FLEXPAGES)
	io_mdb_init_sigma0();
#endif
	sigma0->priority = 50;
	sigma0->thread_state = TS_RUNNING;
	
	
	thread_enqueue_ready(sigma0);
    }
    else
	panic("No sigma0 configured");
}

void create_root_task()
{
    if (kernel_info_page.root_ip != 0xFFFFFFFF)
    {
	tcb_t* tcb = create_tcb(L4_ROOT_TASK_ID, NULL);
	if (!tcb)
	    panic("create_roottask: create_tcb() failed");
#if defined(CONFIG_DEBUG_TRACE_MISC)
	printf("root_task: %p, %x\n", tcb, tcb->myself.raw);
#endif
	
	create_user_stack_frame(tcb,
				kernel_info_page.root_sp, kernel_info_page.root_ip,
				switch_to_roottask, kernel_arg);
	
#if defined(CONFIG_IO_FLEXPAGES)
	io_mdb_task_new(sigma0, tcb);
#endif
	tcb->priority = 50;
	tcb->pager = L4_SIGMA0_ID;
	tcb->thread_state = TS_RUNNING;
	
	thread_enqueue_ready(tcb);
    }
}


void unwind_ipc(tcb_t *tcb);

/* mark a tcb as free */
static void free_tcb(tcb_t* tcb);
/* remove a tcb from all queues */
static void free_thread(tcb_t* tcb);

#if defined(CONFIG_SMP)
int smp_delete_all_threads(space_t *);
#endif
void delete_all_threads(space_t *space)
{
#if defined(CONFIG_DEBUG_TRACE_MISC)
    printf("%s(%p)\n", __FUNCTION__, space);
#endif
    tcb_t *tcb, *next;
    //enter_kdebug("task delete");
#warning task_delete does not work over cpu boundaries
    /* walk along the present queue */
    for (tcb = get_idle_tcb()->present_next;
	 tcb != get_idle_tcb();
	 tcb = next)
    {
	next = tcb->present_next;
	if (tcb->space == space)
	{
	    /* remove the tcb from all queues */
	    free_thread(tcb);

	    /* free the tcb */
	    free_tcb(tcb);
	}
    }
}



/* mark a tcb as free */
static void free_tcb(tcb_t* tcb)
{
#if defined(CONFIG_DEBUG_TRACE_MISC)
    printf("  %s(%p)\n", __FUNCTION__, tcb);
#endif
    thread_dequeue_present(tcb);
    // this ensures that nobody can perform operations on that thread
    tcb->myself = L4_NIL_ID;
    tcb->thread_state = TS_ABORTED;

    free_resources(tcb);
}


/* remove a tcb from all queues */
static void free_thread(tcb_t* tcb)
{
#if defined(CONFIG_DEBUG_TRACE_MISC)
    printf("  %s(%p)\n", __FUNCTION__, tcb);
#endif
    unwind_ipc(tcb); /* dequeues from wakeup */
    while (tcb->send_queue)
        unwind_ipc(tcb->send_queue);
    thread_dequeue_ready(tcb);
    purge_resources(tcb);
}

/* on smp systems we hold the spinlock on tcb */
void unwind_ipc(tcb_t *tcb)
{
    dword_t ipc_error = 0;
    tcb_t* partner;

    TRACEPOINT(UNWIND_IPC,
	       printf("unwind_ipc - thread: %p, state: %x\n",
		      tcb, tcb->thread_state));

#if defined(CONFIG_SMP)
    /* xcpu unwinds are always done there */
    extern int smp_unwind_ipc(tcb_t *);
 retry_smp_unwind_ipc:
    if (tcb->cpu != get_cpu_id())
    {
	if (!smp_unwind_ipc(tcb))
	    goto retry_smp_unwind_ipc;
	return;
    }
#endif

    thread_dequeue_wakeup(tcb);

    switch(tcb->thread_state)
    {
    case TS_RUNNING:
    case TS_ABORTED:
	return;
	break;
    case TS_WAITING:
	ipc_error = IPC_ERR_CANCELED | IPC_ERR_RECV;
	break;
    case TS_POLLING:
#if CONFIG_DEBUG_SANITY
	if (l4_is_nil_id(tcb->partner))
	    enter_kdebug("unwind_ipc: partner = 0!!!");
#endif
	partner = tid_to_tcb(tcb->partner);
	ipc_error = IPC_ERR_CANCELED | IPC_ERR_SEND;
	/* we spinlock in ex_regs!!! */
	thread_dequeue_send(partner, tcb);
	break;
    case TS_LOCKED_WAITING:
	tcb->thread_state = TS_ABORTED;	/* avoid recursion */
#ifdef CONFIG_DEBUG_SANITY
	if (l4_is_nil_id(tcb->partner))
	    enter_kdebug("unwind_ipc: partner = 0!!!");
#endif
	if (!IS_KERNEL_ID(tcb->partner))
	{
	    partner = tid_to_tcb(tcb->partner);
	    unwind_ipc(partner);
	    partner->thread_state = TS_RUNNING;
	    thread_enqueue_ready(partner);
	}
	ipc_error = IPC_ERR_ABORTED | IPC_ERR_RECV;
	break;

#if defined(CONFIG_SMP)
    case TS_XCPU_LOCKED_WAITING:
	/* this is _really_ nasty:
	 * we have to deal with sillions of races and conditions
	 * like concurrent ex-regs on multiple cpu's 
	 * and loop-unwinding (cpu0:ex_regs(cpu1:ipc(cpu0))) 
	 */
	printf("unwind ipc of xcpu-ipc (%p)\n", tcb);
	enter_kdebug("unwind_ipc xcpu");
	break;

    case TS_XCPU_LOCKED_RUNNING:
	printf("unwind TS_XCPU_LOCKED_RUNNING %x\n", tcb);
	printf("partner = %x on cpu %d\n", tcb->partner, tcb->cpu);
	/* fall through */
#endif /* CONFIG_SMP */

    case TS_LOCKED_RUNNING:
	tcb->thread_state = TS_ABORTED;	/* avoid recursion */
#ifdef CONFIG_DEBUG_SANITY
	if (l4_is_nil_id(tcb->partner))
	    enter_kdebug("unwind_ipc: partner = 0!!!");
#endif
	if (!IS_KERNEL_ID(tcb->partner))
	{
	    partner = tid_to_tcb(tcb->partner);
	    unwind_ipc(partner);
#if defined(CONFIG_SMP)
	    /* only enqueue if we are on the same cpu! */
	    if (partner->cpu == get_cpu_id())
#endif
	    {
		partner->thread_state = TS_RUNNING;
		thread_enqueue_ready(partner);
	    }
	}
	ipc_error = IPC_ERR_ABORTED | IPC_ERR_SEND;
	break;
    default:
	ipc_error = 0;
	panic("unwind_ipc: unknown thread_state");
    }

    if (!tcb->unwind_ipc_sp)
    {
	create_user_stack_frame (tcb, get_user_sp (tcb), get_user_ip (tcb),
				 abort_ipc, (void *) ipc_error);
#if defined(CONFIG_IA32_FEATURE_SEP)
	/*
	 * If doing sysnter we need to pop off the sysenter/sysexit
	 * specific stuff from the user stack.
	 */
	if (is_ipc_sysenter (tcb))
	    set_user_sp (tcb, get_user_sp (tcb) + 16);
#endif
    }
    else
    {
	if ((*(dword_t*)tcb->unwind_ipc_sp) < 0xf0000000)
	    enter_kdebug("strange unwind ipc sp");
	tcb->stack = tcb->unwind_ipc_sp;
	tcb->unwind_ipc_sp = 0;
    }
}



void sys_task_new(l4_threadid_t task, dword_t mcp, l4_threadid_t pager, dword_t uip, dword_t usp)
{
#if defined(CONFIG_DEBUG_TRACE_SYSCALLS)
    printf("sys_task_new(tid: %x, mcp: %x, pager: %x, uip: %x, usp: %x)\n",
	   task.raw, mcp, pager.raw, uip, usp);
#endif
    TRACEPOINT_2PAR(SYS_TASK_NEW, task.raw, pager.raw,
   	            printf("sys_task_new (task: %p, mcp: %p, pager: %p, "
		           "uip: %p, usp: %p) - tcb/ip: %p/%x\n",
		           task.raw, mcp, pager.raw, uip, usp,
		           get_current_tcb(), get_user_ip(get_current_tcb())));

    tcb_t *current = get_current_tcb();
    
    if (task.x0id.task == 0)
	panic("sys_task_new: press b to retry :-)");

    task.x0id.thread = 0;	       /* thread 0 of the task */
    tcb_t * tcb = tid_to_tcb(task);

    if (current->myself.x0id.task == task.x0id.task)
    {
	enter_kdebug("sys_task_new: delete_myself not allowed");
	return_task_new(0);
    }
	
    /* if the task already exists - delete it first */
    if (tcb_exist(tcb))
    {
	space_t * space = tcb->space;		/* store space */

#if defined(CONFIG_IO_FLEXPAGES)
	io_mdb_delete_iospace(tcb);         /* remove the IO-Space */
#endif	    

#if defined(CONFIG_SMP)
	/* invoke remote task delete until remote partner signals ok. */
	while (!smp_delete_all_threads(space));
#endif
	delete_all_threads(space);		/* delete all threads */
	delete_space(space);			/* and remove the space */
    }

    if (l4_is_nil_id(pager))
	return_task_new(task); 	/* hand-over ? */
    
    /* create new task */
#warning REVIEWME: avoid tcb_exist
    allocate_tcb(tcb);

    tcb = create_tcb(task, NULL);
    if (!tcb)
	panic("sys_task_new: create_tcb() failed");

    tcb->pager = pager;
    create_user_stack_frame(tcb, usp, uip, switch_to_user, 0);

#if defined(CONFIG_IO_FLEXPAGES)
    io_mdb_task_new(tid_to_tcb(pager), tcb);
#endif
     
    tcb->priority = current->priority;
    tcb->thread_state = TS_RUNNING;
    thread_enqueue_ready(tcb);
    
    return_task_new(tcb->myself);
}

void sys_lthread_ex_regs(dword_t threadno, dword_t uip, dword_t usp, l4_threadid_t pager)
{
    tcb_t* current = get_current_tcb();
    l4_threadid_t tid = current->myself;

    tid.x0id.thread = threadno;
    tcb_t* tcb = tid_to_tcb(tid);

#if defined(CONFIG_DEBUG_TRACE_SYSCALLS)
    printf("lthread_ex_regs (%x,%x,%x,%x) by %p to %x - ip: %x\n",
	   threadno, uip, usp, pager.raw, current, tid.raw, get_user_ip(current));
#endif
    TRACEPOINT(SYS_LTHREAD_EX_REGS,
	       printf("sys_lthread_ex_regs (threadno: %p, uip: %p, usp: %p, "
		      "pager: %p)", threadno, uip, usp, pager.raw,
		      current, tid.raw);
	       printf(" - tcb/ip: %p/%x\n", current, get_user_ip(current)));
    
    if (~(uip & usp) && (tid != current->myself))
	/* allocate tcb on demand */
    {
#warning REVIEWME: avoid tcb_exist
	allocate_tcb(tcb);
	if (!tcb_exist(tcb))
	{
	    tcb = create_tcb(tid, current->space);
	    if (!tcb)
    		panic("sys_lthread_ex_regs: create_tcb() failed");
	    create_user_stack_frame(tcb, usp, uip, switch_to_user, 0);
	    tcb->pager = pager;
	    tcb->priority = current->priority;
	    tcb->thread_state = TS_RUNNING;
	    thread_enqueue_ready(tcb);
	    return_lthread_ex_regs(~0, ~0, L4_INVALID_ID, tid, 0);
	}
	else {
#if !defined(CONFIG_SMP)
	    unwind_ipc(tcb);
	    tcb->thread_state = TS_RUNNING;
#else /* CONFIG_SMP */
	retry_ex_regs:
	    spin_lock(&tcb->tcb_spinlock);
	    if (tcb->cpu == get_cpu_id())
	    {
		unwind_ipc(tcb);
		tcb->thread_state = TS_RUNNING;
		/* free spinlock after setting thread state */
		spin_unlock(&tcb->tcb_spinlock);
	    }
	    else 
	    {
		spin_unlock(&tcb->tcb_spinlock);
		dword_t flags;
		printf("cpu %d: ex-regs on cpu %d (%x)\n", get_cpu_id(), tcb->cpu, tcb);
		if (!smp_ex_regs(tcb, &uip, &usp, &pager, &flags))
		    goto retry_ex_regs;

		/* if we have a remote ex_regs we are done here */
		return_lthread_ex_regs(uip, usp, pager, tid, flags);
	    }
#endif /* !CONFIG_SMP */
	}
    }

#if defined(CONFIG_IA32_FEATURE_SEP)
    /*
     * If thread is returning through ipc_sysenter (i.e., sysexit) it
     * doesn't read the modified EIP from the kernel stack.  We
     * therefore redirect the thread through another exit stub.
     */
    extern dword_t alternate_sys_ipc_return;
    if (~(usp & uip) != 0 && is_ipc_sysenter (tcb))
    {
	get_tcb_stack_top(tcb)[IPCSTK_RETADDR] =
	    (dword_t) &alternate_sys_ipc_return;
	set_user_sp(tcb, get_user_sp(tcb) + 16);
    }
#endif

    if (usp == ~0U)
	usp = get_user_sp(tcb);
    else
	usp = set_user_sp(tcb, usp);

    if (uip == ~0U)
	uip = get_user_ip(tcb);
    else
	uip = set_user_ip(tcb, uip);

    if (pager == L4_INVALID_ID)
	pager = tcb->pager;
    else
    {
	l4_threadid_t oldpager;
	oldpager = tcb->pager;
	tcb->pager = pager;
	pager = oldpager;
    }

    if (tcb->thread_state == TS_RUNNING)
	thread_enqueue_ready(tcb);

    return_lthread_ex_regs(uip, usp, pager, tid, get_user_flags(tcb));
}


static tcb_t * create_tcb(l4_threadid_t tid, space_t *space)
{
    /* xxx sanity */
    tcb_t * tcb = tid_to_tcb(tid);

#if defined(CONFIG_DEBUG_TRACE_MISC)
    printf("create_tcb: tcb: %p, tid: %x, space %p, pdir=%x\n", tcb, tid.raw, space, get_current_pagetable());
#endif
    allocate_tcb(tcb);
    
#if defined(CONFIG_DEBUG_SANITY)
#warning REVIEW: clears tcb
    extern void zero_memory(dword_t * address, int size);
    zero_memory((dword_t*)tcb, L4_TOTAL_TCB_SIZE);
#endif
    tcb->myself = tid;
    tcb->magic = TCB_MAGIC;

    /* thread state *must* be aborted, otherwise IPC is possible
     * ending in a disaster... */
    tcb->thread_state = TS_ABORTED;
    tcb->queue_state = 0;
    tcb->send_queue = NULL;
    tcb->intr_pending = 0;
    tcb->timeslice = DEFAULT_TIMESLICE;
    tcb->current_timeslice = 0;
    tcb->resources = 0;
    tcb->copy_area1 = COPYAREA_INVALID;
    tcb->copy_area2 = COPYAREA_INVALID;
    tcb->unwind_ipc_sp = 0;
    tcb->intr_pending = 0;

    /* initialize resources */
    init_resources(tcb);

    /* no exception handling by default */
    tcb->excpt = 0;

    if (space == NULL)
	tcb->space = create_space ();
    else
	tcb->space = space;

#if !defined(CONFIG_SMP)
    tcb->pagedir_cache = tcb->space->pagedir_phys();
#else
    tcb->pagedir_cache = tcb->space->pagedir_phys(get_cpu_id());
    tcb->cpu = get_cpu_id();
    spin_lock_init(&tcb->tcb_spinlock);
#endif

    thread_enqueue_present(tcb);
    return tcb;
}




/* the famous null call */

void sys_id_nearest(l4_threadid_t tid)
{
    TRACEPOINT_2PAR(SYS_ID_NEAREST, get_current_tcb()->myself.raw, get_user_ip(get_current_tcb()));

#if defined(CONFIG_DEBUG_TRACE_SYSCALLS)
    printf("id_nearest(%x) by %x at %p\n", tid.raw,
	   get_current_tcb()->myself.raw, get_user_ip(get_current_tcb()));
#endif

    if (l4_is_nil_id(tid))
	return_id_nearest(get_current_tcb()->myself, 0);

    /* VU: Hazelnut does not support clans and chiefs 
     *     therefore we emulate the C&C behavior by returning
     *     the task-creator's thread id */
    l4_threadid_t chief = L4_NIL_ID;
    chief.x0id.task = get_current_tcb()->myself.x0id.chief;
    return_id_nearest(tid_to_tcb(chief)->myself, 4);
};


#if defined(NEED_FARCALLS)
#include INC_ARCH(farcalls.h)
#endif

tcb_t * create_idle_thread()
{
    tcb_t *idle = get_idle_tcb();
    if (idle) {
	idle->magic = TCB_MAGIC;
	idle->myself = (l4_threadid_t){ raw: 0x12345678 };
	idle->priority = 0;
	
	/* we use the idle tcb as start of the wakeup and present queue */
	idle->present_next = idle->present_prev = idle;
	idle->wakeup_next = idle->wakeup_prev = idle;
	idle->queue_state = TS_QUEUE_PRESENT | TS_QUEUE_WAKEUP;
	
	/* the idler never wakes up */
	idle->absolute_timeout = ~0;
	
	idle->stack = get_tcb_stack_top(idle);

	/* idler should have no timeslice at all */
	idle->timeslice = DEFAULT_TIMESLICE;
	idle->current_timeslice = 0;

	/* sanity */
	idle->space = NULL;

	notify_thread(idle, initialize_idle_thread);
	      
	return idle;
    }
    panic("can't create idle tcb");
    return NULL;
}


