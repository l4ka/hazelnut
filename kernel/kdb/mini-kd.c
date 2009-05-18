/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mini-kd.c
 * Description:   Main entry pint for kernel debugger (rather
 *                misleading filename).
 *                
 * @LICENSE@
 *                
 * $Id: mini-kd.c,v 1.71 2001/12/12 22:48:31 ud3 Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <mapping.h>

#if defined(CONFIG_DEBUGGER_KDB) && !defined(CONFIG_DEBUGGER_NEW_KDB)
#include "kdebug.h"
#include "kdebug_keys.h"

dword_t __kdebug_ipc_tracing = 0;
dword_t __kdebug_ipc_tr_mask = 0;
dword_t __kdebug_ipc_tr_thread = 0;
dword_t __kdebug_ipc_tr_dest = 0;
dword_t __kdebug_ipc_tr_this = 0;
int __kdebug_pf_tracing = 0;

struct last_ipc_t
{
    l4_threadid_t myself;
    l4_threadid_t dest;
    dword_t snd_desc;
    dword_t rcv_desc;
    dword_t ipc_buffer[3];
    dword_t ip;
} last_ipc;


#if defined(CONFIG_DEBUG_TRACE_MDB)
int __kdebug_mdb_tracing = 0;
#endif

#if defined(CONFIG_ENABLE_SWITCH_TRACE)
switch_trace_type_t kdebug_switch_trace_type = SWTRACE_NONE;
switch_trace_t kdebug_switch_trace[CONFIG_SWITCH_TRACE_SIZE];
dword_t kdebug_switch_trace_idx;
tcb_t *kdebug_switch_trace_id;
#endif

void panic(const char *msg)
{
    printf("panic: %s", msg);
    enter_kdebug("panic");
    while(1);
}

void kdebug_interrupt_association()
{
    printf("\nint association\n");
    int num = 0;
    for (int i = 0; i < MAX_INTERRUPTS; i++)
	if (interrupt_owner[i])
	    printf("interrupt %2d (%x): tcb %p (%t)%c",
		   i, interrupt_owner[i]->intr_pending,
		   interrupt_owner[i], interrupt_owner[i],
		   (num++) & 1 ? '\n' : '\t');
}

#define MAX_KMEM_SIZECNT 10
static dword_t num_chunks[MAX_KMEM_SIZECNT+1];

void kdebug_kmem_stats(void)
{
    extern dword_t *kmem_free_list;
    extern dword_t free_chunks;
    dword_t num, *cur, *prev, i, j, idx, max_chunks;

    /* Clear out statistics. */
    for ( idx = 0; idx <= MAX_KMEM_SIZECNT; idx++ )
	num_chunks[idx] = 0;
    max_chunks = 0;

    for ( num = 1, cur = kmem_free_list, prev = NULL;
	  cur;
	  prev = cur, cur = (dword_t *) *cur )
    {
	/* Are chunks contigous? */
	if ( (dword_t) prev + KMEM_CHUNKSIZE == (dword_t) cur )
	    num++;
	else
	    num = 1;

	for ( i = j = 1, idx = 0; i <= num; i++ )
	{
	    if ( i == j )
	    {
		/* Number of chunks is power of 2. */
		dword_t mask = (j * KMEM_CHUNKSIZE)-1;
		dword_t tmp = (dword_t) cur - ((j-1) * KMEM_CHUNKSIZE);

		/* Check if chunks are properly aligned. */
		if ( (tmp & mask) == 0 )
		{	
		    if ( idx < MAX_KMEM_SIZECNT )
			num_chunks[idx]++;
		    else
			num_chunks[MAX_KMEM_SIZECNT]++;
		    if ( idx > max_chunks )
			max_chunks = idx;
		}

		j <<= 1;
		idx++;
	    }
	}
    }

    printf("\nKmem statistics.  Free chunks = %d (%dKB)\n",
	   free_chunks, (free_chunks*KMEM_CHUNKSIZE)/1024);
    printf("Chunks\tNum.\n");
    for ( i = 0; i <= MAX_KMEM_SIZECNT; i++ )
    {
	printf(" %d%c\t %d\n", (1 << i),
	       i == MAX_KMEM_SIZECNT ? '+' : ' ',
	       num_chunks[i]);
    }
    printf("\n Max:\t %d\n", 1 << max_chunks);
}


void kdebug_dump_tcb(tcb_t* tcb)
{
    /* max width 79 !!! */
#if !defined(CONFIG_SMP)
    printf("=== TCB: %p === TID: %p === PRIO: 0x%2x ===============",
	   tcb, (void*) tcb->myself.raw, tcb->priority);
#else
    printf("=== TCB: %p === TID: %p === PRIO: 0x%2x === CPU: %2x ===",
	   tcb, (void*) tcb->myself.raw,
	   tcb->priority, tcb->cpu);
#endif
    printf("%s ===\n", tcb->magic == TCB_MAGIC ? "====== OK" : " *BROKEN*");
    printf("UIP: %x   queues: %c%c%c%c       present: %p:%p   space: %p\n",
	   get_user_ip(tcb),
	   'R' + 0x20*!(tcb->queue_state & TS_QUEUE_READY),
	   'P' + 0x20*!(tcb->queue_state & TS_QUEUE_PRESENT),
	   'W' + 0x20*!(tcb->queue_state & TS_QUEUE_WAKEUP),
	   'S' + 0x20*!(tcb->queue_state & TS_QUEUE_SEND),
	   tcb->present_prev, tcb->present_next, tcb->space);
    printf("USP: %x   tstate: %s    ready:   %p:%p   pager: %x\n",
	   get_user_sp(tcb),
	   tcb->thread_state == TS_RUNNING		? "RUNNING" :
	   tcb->thread_state == TS_POLLING		? "POLLING" :
	   tcb->thread_state == TS_LOCKED_RUNNING	? "LCKRUNN" :
	   tcb->thread_state == TS_LOCKED_WAITING	? "LCKWAIT" :
	   tcb->thread_state == TS_ABORTED		? "ABORTED" :
	   tcb->thread_state == TS_WAITING		?
	             ((tcb->partner == L4_NIL_ID)	? "OWAIT  " :
							  "WAITING"):
#if defined(CONFIG_SMP)
	   tcb->thread_state == TS_XCPU_LOCKED_RUNNING	? "XLKRUNN" :
	   tcb->thread_state == TS_XCPU_LOCKED_WAITING	? "XLKWAIT" :
#endif
							  "???????",
	   tcb->ready_prev, tcb->ready_next, tcb->pager.raw);
    printf("KSP: %p   resour: %c%c%c%c       wakeup:  %p:%p   excpt: %x\n",
	   tcb->stack,
	   tcb->resources & TR_IPC_MEM		? 'M' : 'm',
	   tcb->resources & TR_FPU		? 'F' : 'f',
	   tcb->resources & TR_LONG_IPC_PTAB	? 'P' : 'p',
	   tcb->resources & TR_DEBUG_REGS	? 'D' : 'd',
	   tcb->wakeup_prev, tcb->wakeup_next, tcb->excpt);
    printf("KIP: %p                      send:    %p:%p\n",
	   (ptr_t)((((dword_t)tcb->stack >= TCB_AREA) &&
		    ((dword_t)tcb->stack < TCB_AREA+TCB_AREA_SIZE)) ?
		   *tcb->stack : (dword_t)-1),
	   tcb->send_prev, tcb->send_next);
    printf("\n");
    printf("timeslc:\t%d/%d\n", tcb->current_timeslice, tcb->timeslice);
    printf("partner:\t%T\ttimeout %x/%x (cur. time: %x)\n", 
	   tcb->partner.raw, (dword_t)tcb->ipc_timeout.raw, 
	   (dword_t)tcb->absolute_timeout, (dword_t)kernel_info_page.clock);
    printf("sndqueue:\t%p\n", tcb->send_queue);
    printf("IPC:  %x:%x:%x\n", tcb->ipc_buffer[0], tcb->ipc_buffer[1],
	   tcb->ipc_buffer[2]);
    printf("unwind:\t%p\n", tcb->unwind_ipc_sp);
#ifdef CONFIG_SMP
    printf("tcb spinlock:\t%x\n", tcb->tcb_spinlock.lock);
#endif
#if defined(CONFIG_ENABLE_SMALL_AS)
    smallid_t id = tcb->space->smallid ();
    printf("Small space: ");
    if (id.is_valid ())
	printf("%02x [size=%dMB, idx=%d]\n", id.value (),
	       id.bytesize () >> 20, id.idx ());
    else
	printf("no\n");
#endif
}


#if defined(CONFIG_ENABLE_SWITCH_TRACE)
static void kdebug_thread_switch_trace (tcb_t * current)
{
    char c = kdebug_get_choice ("\nThread switch trace",
				"All/None/taSk/thRead", 'a');
    switch (c)
    {
    case 'a':
	kdebug_switch_trace_type = SWTRACE_ALL;
	break;
    case 'n':
	kdebug_switch_trace_type = SWTRACE_NONE;
	break;
    case 's':
	kdebug_switch_trace_type = SWTRACE_TASK;
	kdebug_switch_trace_id = kdebug_get_task (current);
	break;
    case 'r':
	kdebug_switch_trace_type = SWTRACE_THREAD;
	kdebug_switch_trace_id = kdebug_get_thread (current);
	break;
    }

    zero_memory ((ptr_t) kdebug_switch_trace, sizeof (kdebug_switch_trace));
    kdebug_switch_trace_idx = 0;
}
				   
static void kdebug_thread_switch_trace_dump (void)
{
    printf ("\nThread switch trace ");
    switch (kdebug_switch_trace_type)
    {
    case SWTRACE_NONE:
	printf ("(none):\n");
	break;
    case SWTRACE_ALL:
	printf ("(all):\n");
	break;
    case SWTRACE_TASK:
	printf ("(task %02x):\n", tasknum (kdebug_switch_trace_id));
	break;
    case SWTRACE_THREAD:
	printf ("(thread %p):\n", kdebug_switch_trace_id);	
	break;
    }

    tcb_t *otcb = NULL;
    for (int i = kdebug_switch_trace_idx, n = 0;
	 n < CONFIG_SWITCH_TRACE_SIZE; n++)
    {
	switch_trace_t *st = kdebug_switch_trace + i;

	if (st->t_tcb != NULL)
	{
	    if (otcb == NULL)
		printf ("???????? (??.?"/* fuck trigraphs */"?) eip=???????? esp=????????  ->  ");
	    else if (otcb == get_idle_tcb ())
		printf ("%p (idler) eip=XXXXXXXX esp=XXXXXXXX  ->  ", otcb);
	    else
		printf ("%p (%t) eip=%p esp=%p  ->  ", otcb,
			otcb->myself, st->f_uip, st->f_usp);
	    if (st->t_tcb == get_idle_tcb ())
		printf ("%p (idler) eip=XXXXXXXX esp=XXXXXXXX\n", st->t_tcb);
	    else
		printf ("%p (%t) eip=%p esp=%p\n", st->t_tcb,
			st->t_tcb->myself, st->t_uip, st->t_usp);
	}

	if (++i == CONFIG_SWITCH_TRACE_SIZE)
	    i = 0;
	otcb = st->t_tcb;
    }
}
#endif


tcb_t *kdebug_find_task_tcb(dword_t tasknum, tcb_t *current)
{
    tcb_t *tcb;
    int first_time;

    for ( tcb = current, first_time = 1;
	  tcb != current || first_time;
	  tcb = tcb->present_next, first_time = 0 )
	if ( tcb->myself.x0id.task == tasknum)
	    return tcb;

    return NULL;
}


int kdebug_entry(exception_frame_t* frame)
{
    /* kdebug_arch_entry can decide about whether
       to enter the interactive KDB (0) or not (1) */
    if (kdebug_arch_entry(frame) == 0)
    {
	
#if defined(CONFIG_SMP)
	smp_enter_kdebug();
#endif
	
	int exit_kdb = 0;
	char c; 

	/* Works because frame resides on the kernel stack. */
	tcb_t *current = ptr_to_tcb((ptr_t) frame);
	space_t *space = NULL;

	extern dword_t _kdebug_stack_bottom[];
	extern dword_t _kdebug_stack_top[];
	if ( (dword_t) current < TCB_AREA ||
	     ((dword_t) frame >= (dword_t) &_kdebug_stack_bottom &&
	      (dword_t) frame <  (dword_t) &_kdebug_stack_top) )
	    /* In case we entered kdebug on startup, or caught an
	       exception inside KDB. */
	    current = get_idle_tcb();
	
	printf("\n--------------------------------------------------------\n");
	
	while ( ! exit_kdb )
	{
	    printf("\nKD: ");

	    /*
	     * Keep case targets sorted alphabetically.
	     */
	    switch(c = getc())
	    {
	    case K_BREAKPOINT:
		kdebug_breakpoint();
		break;

	    case K_CPUSTATE:
		kdebug_cpustate(current);
		break;

#if defined(CONFIG_DEBUG_DISAS)
	    case K_DISASSEMBLE:
		kdebug_disassemble(frame);
		break;
#endif

	    case K_FRAME_DUMP:
		putc('\n');
		kdebug_dump_frame(frame);
		break;

	    case K_GO:
		putc(c); putc('\n');
		exit_kdb = 1;
		break;

	    case K_HELP:
	    case K_HELP2:
		printf("\nL4Ka Kernel Debugger Help:\n");
		for ( dword_t i = 0; kdb_help[i*2].key; i++ )
		{
		    struct kdb_help_t *h = kdb_help + i;
		    printf(h->key == ' ' ? " spc" :
			   h->alternate_key ? " %c/%c" : " %c  ",
			   h->key, h->alternate_key);
		    dword_t n = printf("  %s", h->help);

		    h += sizeof(kdb_help)/sizeof(kdb_help_t)/2;
		    if ( ! h->key ) break;
		    for ( n = 38-n; n--; ) putc(' ');
		    printf(h->key == ' ' ? "spc" :
			   h->alternate_key ? "%c/%c" : "%c  ",
			   h->key, h->alternate_key);
		    printf("  %s\n", h->help);
		}
		//kdebug_arch_help();
		putc('\n');
		break;

	    case K_INTERRUPTS:
		kdebug_interrupt_association();
		break;

	    case K_IPC_TRACE:
	    {
		c = kdebug_get_choice("ipc tracing", "+/*/-/r", 0, 1);
		__kdebug_ipc_tr_mask = __kdebug_ipc_tr_thread = 0;
		switch (c)
		{
		case '+': __kdebug_ipc_tracing = 2; break;
		case '*': __kdebug_ipc_tracing = 1; break;
		case '-': __kdebug_ipc_tracing = 0; break;
		case 'r':
		    __kdebug_ipc_tracing = 0;
		    __kdebug_ipc_tr_this = __kdebug_ipc_tr_dest = 0;
		    while ( __kdebug_ipc_tracing == 0 )
		    {
			switch (c = getc())
			{
			case '>': __kdebug_ipc_tr_dest = 1; putc(c); break;
			case '.': __kdebug_ipc_tr_this = 1; putc(c); break;
			case '+': __kdebug_ipc_tracing = 2; putc(c); break;
			case '*': __kdebug_ipc_tracing = 1; putc(c); break;
			}
		    }
		    if ( (__kdebug_ipc_tr_this + __kdebug_ipc_tr_dest) == 0 )
			__kdebug_ipc_tr_this = 1;
		    printf("\nThread mask [ffffffff]: ");
		    __kdebug_ipc_tr_mask = kdebug_get_hex(0xffffffff);
		    __kdebug_ipc_tr_thread = (dword_t)
			kdebug_get_thread(current) & __kdebug_ipc_tr_mask;
		    break;
		}
		putc('\n');
		break;
	    }

	    case K_LAST_IPC:
		printf("\nlast_ipc: %x -> %x snd_desc: %x rcv_desc: %x "
		       "(%x, %x, %x) - ip=%x\n",
		       last_ipc.myself.raw, last_ipc.dest.raw,
		       last_ipc.snd_desc, last_ipc.rcv_desc,
		       last_ipc.ipc_buffer[0], last_ipc.ipc_buffer[1],
		       last_ipc.ipc_buffer[2], last_ipc.ip);
		break;

	    case K_MDB_DUMP:
		putc(c);
		kdebug_dump_map(kdebug_get_hex());
		break;

#if defined(CONFIG_DEBUG_TRACE_MDB)
	    case K_MDB_TRACE:
		c = kdebug_get_choice("mdb traing", "+/*/-");
		switch (c)
		{
		case '+': __kdebug_mdb_tracing = 2; break;
		case '*': __kdebug_mdb_tracing = 1; break;
		case '-': __kdebug_mdb_tracing = 0; break;
		}
		break;
#endif

	    case K_MEM_DUMP:
		space = get_current_space ();
		/* FALLTHROUGH */
	    case K_MEM_OTHER_DUMP:
	    {
		dword_t *p, dw;
		byte_t dc;
		int i, j;

		putc(c);
		p = (dword_t *) kdebug_get_hex();

		if (space == NULL)
		    space = (space_t *) kdebug_get_pgtable(
			current, "Address space (page-table/task-id/thread-id)"
			" [current]: ");

		for ( i = 0; i < 16; i++ )
		{
		    printf("    %p: ", &p[i*4]);
		    for ( j = 0; j < 4; j++ )
		    {
			if ( kdebug_get_mem(space, &p[i*4+j], &dw) )
			    printf("%p ", dw);
			else
			    printf("######## ");
		    }
		    printf("  ");
		    for ( j = 0; j < 16; j++ )
		    {
			if ( kdebug_get_mem(space, (byte_t *) p+i*16+j, &dc) )
			    printf("%c", dc >= ' ' ? dc < 0x80 ? dc :'.':'.');
			else
			    putc('#');
		    }
		    putc('\n');
		}

		space = NULL;
		break;
	    }

	    case K_PERFMON:
		kdebug_perfmon();
		break;

#if defined(CONFIG_DEBUG_TRACE_UPF) || defined(CONFIG_DEBUG_TRACE_KPF)
	    case K_PF_TRACE:
	    case K_PF_TRACE2:
		c = kdebug_get_choice("pf tracing", "+/*/-");
		switch (c)
		{
		case '+': kdebug_pf_tracing(2); break;
		case '*': kdebug_pf_tracing(1); break;
		case '-': kdebug_pf_tracing(0); break;
		}
		break;
#endif

#if defined(CONFIG_ENABLE_PROFILING)
	    case K_PROFILE:
		kdebug_profiling(current);
		break;

	    case K_PROFILE_DUMP:
		kdebug_profile_dump(current);
		break;
#endif

	    case K_PTAB_DUMP:
		putc(c);
		kdebug_dump_pgtable(kdebug_get_pgtable(current));
		break;

	    case K_QUEUES:
		printf("\n");
		for (int i = MAX_PRIO; i >= 0; i--)
		{
		    /* check whether we have something for this prio */
		    tcb_t* walk = get_idle_tcb();
		    do {
			if ((int) walk->priority == i)
			{
			    /* if so, print */
			    printf("[%d]:", i);
			    walk = get_idle_tcb();
			    do {
				if ((int) walk->priority == i) {
				    if (walk->queue_state & TS_QUEUE_READY)
					printf(" %p", walk);
				    else
					printf(" (%p)", walk);
				}
				walk = walk->present_next;
			    } while (walk != get_idle_tcb());
			    printf("\n");
			    break;
			}
			walk = walk->present_next;
		    } while (walk != get_idle_tcb());
		}
		printf("\n");
		break;

	    case K_RESET:
	    case K_RESET2:
		printf("reset ... bye\n\n");
		kdebug_hwreset();
		break;

	    case K_SINGLE_STEPPING:
		c = kdebug_get_choice("single stepping", "+/-");
		switch (c)
		{
		case '+': kdebug_single_stepping(frame, 1); break;
		case '-': kdebug_single_stepping(frame, 0); break;
		}
		break;

	    case K_STATISTICS:
		c = kdebug_get_choice("stats", "Kmem/Mdb");
		switch (c)
		{
		case 'm': kdebug_mdb_stats(0x0, 0xffffffff); break;
		case 'k': kdebug_kmem_stats(); break;
		}
		break;

	    case K_STEP_BLOCK:
		exit_kdb = kdebug_step_block(frame);
		break;

	    case K_STEP_INSTR:
		exit_kdb = kdebug_step_instruction(frame);
		break;

#if defined(CONFIG_ENABLE_SWITCH_TRACE)
	    case K_SWITCH_TRACE:
		kdebug_thread_switch_trace (current);
		break;

	    case K_SWITCH_TRACE_DUMP:
		kdebug_thread_switch_trace_dump ();
		break;
#endif

	    case K_TASK_DUMP:
	    {
		tcb_t *tcb, *task = kdebug_get_task(current);
		dword_t task_id = task->myself.x0id.task;
		dword_t first_time = 1;
		int n;

		printf("Task %02x:\n", task_id);
		for ( tcb = current;
		      (tcb != current) || first_time;
		      tcb = tcb->present_next, first_time = 0 )
		{
		    if ( tcb->myself.x0id.task != task_id )
			continue;

		    printf("%p (%t): ", tcb, tcb->myself);

		    printf(" queues=[%c%c%c%c]",
			   'R' + 0x20*!(tcb->queue_state & TS_QUEUE_READY),
			   'P' + 0x20*!(tcb->queue_state & TS_QUEUE_PRESENT),
			   'W' + 0x20*!(tcb->queue_state & TS_QUEUE_WAKEUP),
			   'S' + 0x20*!(tcb->queue_state & TS_QUEUE_SEND));
		    printf("  state=%s",
			   tcb->thread_state == TS_RUNNING        ? "RUNNING" :
			   tcb->thread_state == TS_POLLING        ? "POLLING" :
			   tcb->thread_state == TS_LOCKED_RUNNING ? "LCKRUNN" :
			   tcb->thread_state == TS_LOCKED_WAITING ? "LCKWAIT" :
			   tcb->thread_state == TS_ABORTED        ? "ABORTED" :
			   tcb->thread_state == TS_WAITING        ?
			   ((tcb->partner == L4_NIL_ID) ? "OWAIT  " :
			    "WAITING") : "???????");
		    n = printf("  partner=%p (%t)", tcb->partner,
			       tcb->partner);
		    for ( n = 29-n; n > 0; n-- ) putc(' ');
		    printf("  uip=%p", get_user_ip(tcb));
		    putc('\n');
		}
		break;
	    }

	    case K_TCB_DUMP:
		putc(c); putc('\n');
		kdebug_dump_tcb(kdebug_get_thread(current));
		break;

#if defined(CONFIG_TRACEBUFFER)
            case K_TRACE_DUMP:
            {
                putc(c); putc('\n');
                kdebug_dump_tracebuffer();
                break;
            }

            case K_TRACE_FLUSH:
            {
                putc(c); putc('\n');
                kdebug_flush_tracebuffer();
                break;
            }    
#endif

#if defined(CONFIG_ENABLE_TRACEPOINTS)
	    case K_TRACEPOINT:
		kdebug_tracepoint();
		break;
#endif

#if defined(CONFIG_SMP)
	    case K_SMP_SWITCH_CPU:
	    {
		printf("current cpu: %d, switch to cpu: ", get_cpu_id());
		int cpu = kdebug_get_dec(0);
		kdebug_arch_exit(frame);
		return cpu;
	    }

	    case K_SMP_INFO:
		kdebug_smp_info();
		break;
#endif
	    }
	}

#if defined(CONFIG_SMP)
	smp_leave_kdebug();
#endif
    }
    kdebug_arch_exit(frame);
    return -1;
}


/*
**
** Function for safely accessing memory without causing page faults.
**
*/

int lookup_mapping(space_t * space, dword_t vaddr, ptr_t * paddr)
{
    pgent_t *pg = space->pgent (0);
    dword_t pgsz = HW_NUM_PGSIZES-1;

    /* Look for valid page table entry. */
    pg = pg->next(space, table_index(vaddr, pgsz), pgsz);
    while ( pg->is_valid(space, pgsz) && pg->is_subtree(space, pgsz) )
    {
	pg = pg->subtree(space, pgsz--);
	pg = pg->next(space, table_index(vaddr, pgsz), pgsz);
    }

    if ( ! pg->is_valid(space, pgsz) )
	/* Return value of 0 indicates invalid address. */
	return 0;

    /* Store physical address into return parameter. */
    if (paddr)
	*paddr = (ptr_t)(pg->address(space, pgsz) + (vaddr & page_mask(pgsz)));

    return 1;
}




#endif /* CONFIG_DEBUGGER_KDB */
