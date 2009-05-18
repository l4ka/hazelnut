/*********************************************************************
 *                
 * Copyright (C) 2001-2002,  Karlsruhe University
 *                
 * File path:     new-kdb.c
 * Description:   New kernel debugger implementation.
 *                
 * @LICENSE@
 *                
 * $Id: new-kdb.c,v 1.4 2002/02/19 18:03:39 ud3 Exp $
 *                
 ********************************************************************/
#include <universe.h>

#if defined(CONFIG_DEBUGGER_NEW_KDB)
#include <mapping.h>
#include "kdebug.h"
#include "kdebug_keys.h"
#include "kdebug_help.h"

static int exit_kdb; 

extern int num_lines;
extern int name_tab_index;

extern void dump_names();
extern void set_name();

extern void kdebug_ipc_tracing();
extern void kdebug_exception_tracing();
extern void kdebug_pagefault_tracing();

extern void kdebug_arch_exit(exception_frame_t * frame);
extern int  kdebug_arch_entry(exception_frame_t * frame);
extern void use_cache();

extern void strcpy(char *dest, char *src);
extern void kdebug_dump_trace();


//name table
extern struct {
    dword_t tid;
    char name[9];
} name_tab[32];           

#if defined(CONFIG_DEBUG_TRACE_MDB)
int __kdebug_mdb_tracing = 0;
#endif

void panic(const char *msg)
{
    printf("panic: %s", msg);
    enter_kdebug("panic");
    while(1);
}

void kdebug_reset() {
    char c;
    printf("reset computer? [n] ");
    c = getc();
    if (c == 'y') {
	putc(c);
	kdebug_hwreset();
    }
    else {
	putc('n');
	putc('\n');
    }
}

//taken from mini_kdb.c for small spaces

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

void kdebug_print_help(struct kdb_help_t *h) {
    for ( dword_t i = 0; h[i].key; i++ )
    {
	printf(h[i].key == ' ' ? " spc" :
	       h[i].alternate_key ? " %c/%c" : " %c  ",
	       h[i].key, h[i].alternate_key);
	printf(" %s\n", h[i].help);
    }
}

void dump_mem(dword_t* p) {//output in other formats than hex
    int i,j;
    printf("\n  memory dump:  %x - %x", p, p+(4*(num_lines-3)));
    for (i = 0; i < num_lines - 2; i++) {
	printf("\n    %p: ", &p[i*4]);
	for (j = 0; j < 4; j++)
	    p[i*4+j] == 0x00000000 ? printf("       0 ") : printf("%x ", p[i*4+j]);
	printf("  ");
	for (j = 0; j < 16; j++)
	    printf("%c", ((unsigned char* )p)[i*16+j] >= ' ' ? ((unsigned char* )p)[i*16+j] < 0x80 ? ((unsigned char* )p)[i*16+j] : '.' : '.');
    };
    printf("\n\n      q: exit to debugger,  pg up:scroll up,  pg dn: scroll down "); 
}

#define CHUNKSIZE 0x400

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
	if ( (dword_t) prev + CHUNKSIZE == (dword_t) cur )
	    num++;
	else
	    num = 1;

	for ( i = j = 1, idx = 0; i <= num; i++ )
	{
	    if ( i == j )
	    {
		/* Number of chunks is power of 2. */
		dword_t mask = (j * CHUNKSIZE)-1;
		dword_t tmp = (dword_t) cur - ((j-1) * CHUNKSIZE);

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
	   free_chunks, (free_chunks*CHUNKSIZE)/1024);
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
	   tcb->thread_state == TS_RUNNING        ? "RUNNING" :
	   tcb->thread_state == TS_POLLING        ? "POLLING" :
	   tcb->thread_state == TS_LOCKED_RUNNING ? "LCKRUNN" :
	   tcb->thread_state == TS_LOCKED_WAITING ? "LCKWAIT" :
	   tcb->thread_state == TS_ABORTED        ? "ABORTED" :
	   tcb->thread_state == TS_WAITING        ?
	             ((tcb->partner == L4_NIL_ID) ? "OWAIT  " :
                                                    "WAITING"):
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
    printf("partner:\t%T\ttimeout %x (cur. time: %x)\n", 
	   tcb->partner.raw, (dword_t)tcb->ipc_timeout.raw, (dword_t)kernel_info_page.clock);
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


/**
 * outputs the interrupt association
 **/
void kdebug_interrupt_association() {
    printf("\nint association\n");
    int num = 0;
    for (int i = 0; i < MAX_INTERRUPTS; i++)
	if (interrupt_owner[i])
	    printf("interrupt %2d: tcb %p (%t)%c",
		   i, interrupt_owner[i], interrupt_owner[i],
		   (num++) & 1 ? '\n' : '\t');
}

void common_menu(char c, struct kdb_help_t *h, exception_frame_t* frame) {

    tcb_t *current = ptr_to_tcb((ptr_t) frame);
    space_t *space = NULL;

    switch (c) {
	
	case K_CPU_BREAKPOINT:
	    kdebug_breakpoint();
	    break;
	    
	case K_DUMP_TRACE:
	    kdebug_dump_trace();
	    break;
	    
	case K_FRAME_DUMP:
	    putc('\n');
	    kdebug_dump_frame(frame);
	    break;
	    
	case K_GO:
	    putc(c);
	    putc('\n');
	    exit_kdb = 1;
	    return;
	    break;

	case K_HELP:
	case K_HELP2:
	    kdebug_print_help(h);
	    printf("\nPress any key to go back.\n");
	    break;
	    
	case K_KERNEL_TCB_DUMP:
	    kdebug_dump_tcb(kdebug_get_thread(current));
	    break;
	    
	case K_MEM_DUMP:
	    space = get_current_space ();
	    /* FALLTHROUGH */
	case K_MEM_DUMP_OTHER:
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
	
	
	case K_MEM_PTAB_DUMP:
	    putc(c);
	    kdebug_dump_pgtable(kdebug_get_pgtable(current));
	    break;
	    
	case K_RESET:
	case K_RESET2:
	    kdebug_reset();
	    break;
    }
}

/* the configuration submenu */
void config_debugger(exception_frame_t* frame) {

    char c;

    while ( ((c = getc()) != K_LEAVE_MENU) && (!exit_kdb) ) {
	printf("Debugger configuration: ");
	switch (c) {
	    
	case K_CONFIG_CACHE:
	    use_cache();
	    break;
	    
	case K_CONFIG_KNOWN_NAMES:
	    
	    name_tab[0].tid = 0x04020001;
	    strcpy(name_tab[0].name, "sigma0");
	    
	    name_tab[1].tid = 0x04040001;
	    strcpy(name_tab[1].name, "roottask");
	    
	    name_tab[2].tid = 0x00000000;
	    strcpy(name_tab[2].name, "nil");
	    
	    name_tab[3].tid = 0xffffffff;
	    strcpy(name_tab[3].name, "invalid");

	    name_tab[4].tid = 0x12345678;
	    strcpy(name_tab[4].name, "idler");

	    name_tab_index = 5;
	    dump_names();
	    break;
	    
	case K_CONFIG_NAMES:
	    set_name();
	    break;
	   
	case K_CONFIG_DUMP_NAMES:
	    dump_names();
	    break;
 
	default:
	    //here are the shortcuts
	    common_menu(c, kdb_config_help, frame);
	}
    }
}

/* the submenu for all memory stuff */
void kdebug_memory_stuff(exception_frame_t* frame) {

    char c;

    while( ((c = getc()) != K_LEAVE_MENU) && (!exit_kdb) ) {

	printf("Memory menu:\n");

	switch (c) {
  
	case K_MEM_MDB_DUMP:
	    putc(c);
	    kdebug_dump_map(kdebug_get_hex());
	    break;

#if defined(CONFIG_DEBUG_TRACE_MDB)
	    case K_MEM_MDB_TRACE:
		c = kdebug_get_choice("mdb traing", "+/*/-");
		switch (c)
		{
		case '+': __kdebug_mdb_tracing = 2; break;
		case '*': __kdebug_mdb_tracing = 1; break;
		case '-': __kdebug_mdb_tracing = 0; break;
		}
		break;
#endif
	case K_MEM_PF_TRACE:
	case K_MEM_PF_TRACE2:
//	    printf("Pagefault tracing: ");
	    kdebug_pagefault_tracing();
	    break;
	default:
	    //here are some undocumented shortcuts
	    common_menu(c, kdb_mem_help, frame);
	}
    }
}

/* the submenu for all the cpu dependent stuff */
void kdebug_cpu_stuff(exception_frame_t* frame) {
    char c;
    tcb_t *current = ptr_to_tcb((ptr_t) frame);

    while( ((c = getc()) != K_LEAVE_MENU) && (!exit_kdb) ) {

	printf("CPU menu:\n");

	switch (c) {
    
#if defined(CONFIG_DEBUG_DISAS)
	case K_CPU_DISASSEMBLE:
//	    printf("disassembler\n");
	    kdebug_disassemble(frame);
	    break;
#endif
	    
	case K_CPU_PERFCTR:
	    kdebug_perfmon();
	    break;
	    
#if defined(CONFIG_ENABLE_PROFILING)
	case K_CPU_PROFILE:
	    kdebug_profiling(current);
	    break;
	    
	case K_CPU_PROFILE_DUMP:
	    kdebug_profile_dump(current);
	    break;
#endif

#if 0 //i think i don't need this togeter with step instruction  
	case K_CPU_SINGLE_STEPPING:
	    c = kdebug_get_choice("single stepping", "+/-");
	    switch (c)
	    {
	    case '+': kdebug_single_stepping(frame, 1); break;
	    case '-': kdebug_single_stepping(frame, 0); break;
	    }
	    break;
#endif
	    
	case K_CPU_STATE:
	    printf("cpu state for");
	    kdebug_cpustate(current);
	    break;
	default:
	    //here are the shortcuts
	    common_menu(c, kdb_cpu_help, frame);
	}
    }
}

/* submenu for all kernel things */
void kdebug_kernel_stuff( exception_frame_t* frame) {
    char c;
    tcb_t *current = ptr_to_tcb((ptr_t) frame);
	
    while ( ((c = getc()) != K_LEAVE_MENU) && (!exit_kdb) ) {
	printf("Kernel menu:\n");

	switch (c) {
	    
	case K_KERNEL_EXCPTN_TRACE:
//	    printf("exception tracing: \n");
	    kdebug_exception_tracing();
	    break;
	    
	case K_KERNEL_INTERRUPT:
	    kdebug_interrupt_association();
	    break;
	    
	case K_KERNEL_IPC_TRACE:
//	    printf("ipc tracing: \n");
	    kdebug_ipc_tracing();
	    break;
	    
	case K_KERNEL_QUEUES:
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
	    
	case K_KERNEL_STATS:
	    c = kdebug_get_choice("stats", "Kmem/Mdb");
	    switch (c)
	    {
	    case 'm': kdebug_mdb_stats(0x0, 0xffffffff); break;
	    case 'k': kdebug_kmem_stats(); break;
	    }
	    break;
	    
	case K_KERNEL_TASK_DUMP:
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

#if defined(CONFIG_ENABLE_TRACEPOINTS)
	    case K_KERNEL_TRACEPOINT:
		kdebug_tracepoint();
		break;
#endif

	default:
	    //here are the shortcuts
	    common_menu(c, kdb_kernel_help, frame);
	}
    }
}


/**
 * the enter point to the debugger
 **/  
int kdebug_entry(exception_frame_t* frame) {

    /* kdebug_arch_entry can decide about whether
       to enter the interactive KDB (0) or not (1) */
    if (kdebug_arch_entry(frame) == 0)
    {
	
#if defined(CONFIG_SMP)
	smp_enter_kdebug();
#endif
	
	exit_kdb = 0;
	char c; 

	/* Works because frame resides on the kernel stack. */
	tcb_t *current = ptr_to_tcb((ptr_t) frame);

	extern dword_t _kdebug_stack_bottom[];
	extern dword_t _kdebug_stack_top[];
	if ( (dword_t) current < TCB_AREA ||
	     ((dword_t) frame >= (dword_t) &_kdebug_stack_bottom &&
	      (dword_t) frame <  (dword_t) &_kdebug_stack_top) )
	    /* In case we entered kdebug on startup, or caught an
	       exception inside KDB. */
	    current = get_idle_tcb();

#if !defined(CONFIG_SMP)
	printf("\n--------------------kernel-debugger--------------------\n");
#else
	extern dword_t __cpu_id;
	printf("\n----------------kernel-debugger-cpu-%d-----------------\n", __cpu_id);
#endif
	while (! exit_kdb) {

	    printf("\nL4KD: ");
	    c = getc();

	    switch(c) {
		
	    case K_CONFIG_STUFF:
		printf("Configuration of the debugger:\n");
		config_debugger(frame);
		break;

	    case K_CPU_STUFF:
		printf("cpu dependent part\n");
		kdebug_cpu_stuff(frame);
		break;

	    case K_HELP:
	    case K_HELP2:
		printf("\nL4Ka Kernel Debugger Help:\n");
		kdebug_print_help(kdb_general_help);
		printf("\nPress '%c', '%c', '%c' or '%c' for the Memory, CPU, Kernel\n"
		       " or cOnfiguration submenu,\n"
		       "any other key to go back to the main menu\n\n",
		       K_MEM_STUFF, K_CPU_STUFF, K_KERNEL_STUFF, K_CONFIG_STUFF);
		
		int back;
		back = 0;
		
		while (!back) {
		    c = getc();
		    if ( (c == K_CPU_STUFF) || (c == K_KERNEL_STUFF) || 
			 (c == K_MEM_STUFF) || (c == K_CONFIG_STUFF) ) {
			switch (c) {
			case K_CONFIG_STUFF:
			    printf("Configuration submenu:\n");
			    kdebug_print_help(kdb_config_help);
			    break;
			case K_CPU_STUFF:
			    printf("CPU submenu:\n");
			    kdebug_print_help(kdb_cpu_help);
			    break;
			case K_KERNEL_STUFF:
			    printf("Kernel submenu:\n");
			    kdebug_print_help(kdb_kernel_help);
			    break;
			case K_MEM_STUFF:
			    printf("Memory submenu:\n");
			    kdebug_print_help(kdb_mem_help);
			    break;
			}
		    }
		    else {
			back = 1;
		    }
		}
		// I want an other Helpmenu as in the common function here.
		c = NULL;
		break;

	    case K_KERNEL_STUFF:
		printf("kernel things\n");
		kdebug_kernel_stuff(frame);
		break;

	    case K_MEM_STUFF:
		printf("memory dependent part:\n");
		kdebug_memory_stuff(frame);
		break;

	    case K_RESET:
	    case K_RESET2:
		kdebug_reset();
		break;

	    case K_STEP_BLOCK:
		exit_kdb = kdebug_step_block(frame);
		break;
		
	    case K_STEP_INSTR:
		exit_kdb = kdebug_step_instruction(frame);
		break;
		
	    default:
		//here are the shortcuts
		common_menu(c, kdb_general_help, frame);
	    } /* end of switch */
	} /* end of while (! exit_kdb) */

#if defined(CONFIG_SMP)
	smp_leave_kdebug();
#endif
    }
    kdebug_arch_exit(frame);
    return -1;
}


/*
**
** Functions for safely accessing memory without causing page faults.
**
*/

extern dword_t mdb_pgshifts[];

int lookup_mapping(space_t * space, dword_t vaddr, ptr_t * paddr)
{
    pgent_t *pg = space->pgent (0);
    dword_t pgsz = NUM_PAGESIZES-1;

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

#endif /* CONFIG_DEBUGGER_NEW_KDB */
