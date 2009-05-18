/*********************************************************************
 *                
 * Copyright (C) 1999-2002,  Karlsruhe University
 *                
 * File path:     x86/memory.c
 * Description:   X86 specific memory management functions.
 *                
 * @LICENSE@
 *                
 * $Id: memory.c,v 1.101 2002/05/14 13:08:53 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <x86/memory.h>
#include <x86/syscalls.h>
#include <mapping.h>
#include <ipc.h>
#include <tracepoints.h>

#define PF_USERMODE(x)		(x & PAGE_USER)
#define PF_WRITE(x)		(x & PAGE_WRITABLE)
#define PF_PROTECTION(x)	(x & 0x1)

#define FP_NUM_PAGES(x)		(1 << (((x & 0xfc) >> 2) - PAGE_BITS))
#define FP_ADDR(x)		(x & PAGE_MASK)
#define FP_BASE_MASK(x)		((~0U) >> (32 - ((x & 0xfc) / 4)))

#ifdef CONFIG_DEBUG_DOUBLE_PF_CHECK
static dword_t last_pf_addr = ~0;
static tcb_t * last_pf_tcb = NULL;
#endif


/*
 * Page sizes handled by the hardware.
 */

dword_t hw_pgshifts[HW_NUM_PGSIZES+1] = { 12, 22, 32 };

/*
 * Page sizes handled in the mapping database.
 */

dword_t mdb_pgshifts[NUM_PAGESIZES+1] = { 12, 22, 32 };

/*
 * Allocation sizes handled by the mapping database allocator.
 */
mdb_buflist_t mdb_buflists[] = {
    { 12 },	// Mapping nodes
    { 8 },	// ptrnode_t
    { 4096 },	// 4KB array
#if defined(CONFIG_IO_FLEXPAGES)
    { 28 },     // IO-Mapping-Nodes
#endif
    { 0 }
};

#if defined(CONFIG_SMP)
# define SPACE_ALLOC_SIZE	(PAGE_SIZE * CONFIG_SMP_MAX_CPU)
#else
# define SPACE_ALLOC_SIZE	(PAGE_SIZE)
#endif


/*
 * Function create_space ()
 *
 *    Called from create_tcb() when a new address space is created.
 *
 */
space_t * create_space (void)
{
    space_t *nspace = (space_t *) virt_to_phys 
	(kmem_alloc (SPACE_ALLOC_SIZE * 2));
    space_t *kspace = get_kernel_space ();
    pgent_t *pg_n = nspace->pgent (TCB_AREA >> PAGEDIR_BITS);
    pgent_t *pg_k = kspace->pgent (TCB_AREA >> PAGEDIR_BITS);
    
    /* Copy TCB area and kernel area. */
    for (int i = (TCB_AREA >> PAGEDIR_BITS); i < 1024; i++)
    {
	pg_n->copy (nspace, 1, pg_k, kspace, 1);
	pg_n = pg_n->next (nspace, 1, 1);
	pg_k = pg_k->next (kspace, 1, 1);
    }

#if defined(CONFIG_ENABLE_SMALL_AS)
    /* Set to large space. */
    nspace->set_smallid (SMALL_SPACE_INVALID);
#endif

#if defined(CONFIG_SMP)
    /* Free up unused shadow page directories. */
    dword_t tmp = (dword_t) nspace->pagedir () + SPACE_ALLOC_SIZE;
    for (int cpu = 1; cpu < CONFIG_SMP_MAX_CPU; cpu++)
	kmem_free ((ptr_t) (tmp + PAGE_SIZE * cpu), PAGE_SIZE);

    /* fixup the cpu-local mappings */
    for (int cpu = 0; cpu < CONFIG_SMP_MAX_CPU; cpu++)
    {
	extern char _start_cpu_local;
	pg_n = nspace->pgent(pgdir_idx((dword_t)&_start_cpu_local), cpu);
	pg_k = kspace->pgent(pgdir_idx((dword_t)&_start_cpu_local), cpu);
	pg_n->copy_nosync(nspace, 1, pg_k, kspace, 1);
    }
#endif

    return nspace;
}


/*
 * Function delete_space (space)
 *
 *    Called from task_delete ().
 *
 */
void delete_space (space_t * space)
{
#if defined(CONFIG_ENABLE_SMALL_AS)
    if (space->is_small ())
	small_space_to_large (space);

    /*
     * VU: If we run in a small space we better switch to our own
     * pagetable, because otherwise it may disappear.
     */
    if (get_current_tcb ()->space->is_small ())
	set_current_pagetable (get_current_tcb ()->space->pagedir_phys ());
#endif

    /* Flush whole address space. */
    fpage_unmap (space, FPAGE_WHOLE_SPACE, FPAGE_OWN_SPACE + FPAGE_FLUSH);

    /* Delete page directory. */
    kmem_free ((ptr_t) space->pagedir (), SPACE_ALLOC_SIZE);

    /* Free up shadow page directory. */
    kmem_free ((ptr_t) ((dword_t) space->pagedir () + SPACE_ALLOC_SIZE),
	       PAGE_SIZE);
}


#if defined(CONFIG_ENABLE_SMALL_AS)

extern "C" void ipc_sysenter_small (void);
static int restart_ipc (tcb_t *);

#if defined(CONFIG_GLOBAL_SMALL_SPACES)
static void modify_global_bits (space_t *, pgent_t *, int, int);
#endif


/*
 * Array containing pagedir pointers for the spaces who've allocated
 * the space slots.
 */

space_t * small_space_owner[MAX_SMALL_SPACES];


/*
 * Function make_small_space (space, smallid)
 *
 *    Make a small space of SPACE.  We also allow for extending a
 *    small space, but only if the space starts at the same pgdir
 *    position.  Making a space smaller is currently not supported.
 *    Return 0 upon success, -1 upon error.
 *
 */
int make_small_space (space_t * space, byte_t smallid)
{
    smallid_t small;
    small.set_value (smallid);

    dword_t size = small.size ();
    dword_t idx  = small.idx ();
    int i;

    /* Check for previoulsy allocated slots. */
    for ( i = 0; i < MAX_SMALL_SPACES; i++ )
    {
	if ( small_space_owner[i] == space &&
	     ((dword_t) i < idx || (dword_t) i >= idx+size) )
	    goto Space_failed;
    }

    /* Try allocating space slots. */
    for ( i = 0; i < (int) size; i++ )
    {
	if ( small_space_owner[idx + i] == 0 )
	    small_space_owner[idx + i] = (space_t *) ((dword_t) space | 1);
	else if ( small_space_owner[idx + i] != space )
	    break;
    }

    /* Did we manage to allocate all slots? */
    if ( i < (int) size )
    {
	while ( i > 0 )
	    /* Do not deallocate previously allocated slots. */
	    if ( small_space_owner[idx + i] != space )
		small_space_owner[idx + i] = 0;

    Space_failed:
	TRACEPOINT(SET_SMALL_SPACE_FAILED,
		   printf("Creating small space %2x failed [size=%d idx=%d]\n",
			  small, size, idx));
	return -1;
    }

    /* Fixup newly allocated slots. */
    for ( i = 0; i < (int) size; i++ )
	small_space_owner[idx + i] = space;

    tcb_t *t = get_idle_tcb();
    do {
	/*
	 * If a thread in the same address space is in the middle of a
	 * long IPC, the memory pointers used in the IPC will be
	 * incorrect in a small space context.  We must therefore
	 * restart any such ongoing (and active) IPC operations.
	 */
	if ((t->space == space) && (~t->thread_state & TSB_LOCKED))
	    restart_ipc(t);

	t = t->present_next;
    } while ( t != get_idle_tcb() );


#if defined(CONFIG_GLOBAL_SMALL_SPACES)
    modify_global_bits (space, space->pgent (0), small.bytesize (), 1);
#endif

#if defined(CONFIG_IA32_FEATURE_SEP)
    static int uses_small_spaces = 0;

    if ( ! uses_small_spaces )
    {
	/*
	 * Once we start using small spaces, we must switch to the
	 * version of ipc_sysenter that saves/restores segment
	 * registers.
	 */
	asm ("	sub	%%edx, %%edx			\n"
	     "	wrmsr	/* SYSENTER_EIP_MSR */		\n"
	     :
	     :"c"(0x176), "a"(ipc_sysenter_small)
	     :"edx");

	extern dword_t return_from_sys_ipc;
	extern dword_t return_from_sys_ipc_small;
	t = get_idle_tcb();
	do {
	    /*
	     * Make sure that ongoing ipc_sysenter operations are
	     * returned through the `ipc_sysenter_small' handler (so
	     * that segment registers are loaded properly).
	     */
	    if ( t->stack < (ptr_t) &get_tcb_stack_top(t)[IPCSTK_RETADDR] &&
		 (get_tcb_stack_top(t)[IPCSTK_RETADDR] ==
		  (dword_t) &return_from_sys_ipc) )
	    {
		get_tcb_stack_top(t)[IPCSTK_RETADDR] =
		    (dword_t) &return_from_sys_ipc_small;
	    }

	    t = t->present_next;
	} while ( t != get_idle_tcb() );

	uses_small_spaces = 1;
    }
#endif /* CONFIG_IA32_FEATURE_SEP */

    space->set_smallid (small);

    if (space == get_current_space ())
    {
	/* Reset GDT entries to have proper limits. */
	extern seg_desc_t gdt[];
	gdt[X86_UCS >> 3].set_seg ((void *) small.offset (),
				   small.bytesize () - 1,
				   3, GDT_DESC_TYPE_CODE);
	gdt[X86_UDS >> 3].set_seg ((void *) small.offset (),
				   small.bytesize () - 1,
				   3, GDT_DESC_TYPE_DATA);
	reload_user_segregs ();

	/* Inform thread switch code that we run in a small space. */
	extern dword_t __is_small;
	__is_small = 1;
    }

    return 0;
}


/*
 * Function small_space_to_large (space)
 *
 *    Promote the small space into a large one.  This is an expensive
 *    operation since all page tables in the system has to be modified
 *    so that they no longer contain any pagedir entries from the
 *    previous small space.
 *
 *    It is possible to make this operation cheaper by keeping some
 *    versioning info in all pagetables and let the padedir entry
 *    deletion happen lazily upon context switches.  Such an approach
 *    would, however, make context switch times more unpredictable.
 *
 */
void small_space_to_large (space_t * space)
{
    smallid_t small = space->smallid ();
    dword_t idx = small.pgdir_idx ();
    dword_t size = small.size ();
    dword_t *p, i;
    tcb_t *t;
    int restarted_current = 0;

    space->set_smallid (SMALL_SPACE_INVALID);

#if defined(CONFIG_GLOBAL_SMALL_SPACES)
    modify_global_bits (space, space->pgent (0), small.bytesize (), 0);
#endif

    t = get_idle_tcb ();
    do {
	/* Remove stale pagedir entries. */
	p = t->space->pagedir ();
	for (i = 0; i < (size * SMALL_SPACE_MIN/4); i++)
	    p[idx + i] = 0;

	/*
	 * If a thread in the same address space is in the middle of a
	 * long IPC, the memory pointers used in the IPC will be
	 * incorrect when enlarging the space.  We must therefore
	 * restart any such ongoing (and active) IPC operations.
	 */
	if ((t->space ==  space) && (~t->thread_state & TSB_LOCKED))
	    restarted_current |= restart_ipc (t);

	t = t->present_next;
    } while (t != get_idle_tcb ());

    TRACEPOINT (ENLARGE_SPACE);

    /* Deallocate pagedir slots.  Must be done last. */
    idx = small.idx ();
    for (i = 0; i < size; i++)
	small_space_owner[idx + i] = NULL;

    if (get_current_tcb ()->space == space)
    {
	/* Reset GDT entries to 3GB limit. */
	extern seg_desc_t gdt[];
	gdt[X86_UCS >> 3].set_seg (0, USER_AREA_MAX-1, 3, GDT_DESC_TYPE_CODE);
	gdt[X86_UDS >> 3].set_seg (0, USER_AREA_MAX-1, 3, GDT_DESC_TYPE_DATA);
	reload_user_segregs ();

	/* Make sure that we run on our own page table. */
	set_current_pagetable (space->pagedir_phys ());

	/* Inform thread switch code that we run in a large space. */
	extern dword_t __is_small;
	__is_small = 0;
    }

    /*
     * Flush complete TLB (including global pages) by modifying the
     * PGE bit in cr4.
     */
    asm volatile (
	"	movl	%%cr4, %%eax	\n"
	"	andl	%0, %%eax	\n"
	"	movl	%%eax, %%cr4	\n"
	"	movl	%%cr3, %%ebx	\n"
	"	movl	%%ebx, %%cr3	\n"
	"	orl	%1, %%eax	\n"
	"	movl	%%eax, %%cr4	\n"
	:
	: "i" (~X86_CR4_PGE), "i" (X86_CR4_PGE)
	: "eax", "ebx");

    if (restarted_current)
    {
	/*
	 * If an IPC in the current thread was restarted, we must
	 * reenter the sys_ipc function instead of returning.
	 */
	asm volatile (
	    "	movl	%0, %%esp	\n"
	    "	ret			\n"
	    :
	    : "r" (get_current_tcb ()->stack));
	/* NOTREACHED */
    }
}


/*
 * Function restart_ipc (tcb)
 *
 *    Restart the IPC operation in which `tcb' is involved.  It is
 *    assumed that the `tcb' is involved in an IPC with its partner.
 *    Return non-nil if `tcb' or its partner is the current thread,
 *    otherwise nil.
 *
 */
static int restart_ipc(tcb_t *tcb)
{
    tcb_t *current = get_current_tcb();
    tcb_t *partner = tid_to_tcb(tcb->partner);

    enter_kdebug("Restart ipc --- press `g' and tell Espen if this works");
    TRACEPOINT(RESTART_IPC,
	       printf("Restart IPC (tcb %p, partner %p)\n", tcb, partner));

    /*
     * Cancel ongoing IPC operations.  Set unwind_ipc_sp so that stack
     * contents are not overwritten.
     */

    tcb->unwind_ipc_sp = get_tcb_stack_top(tcb) - 9;
    partner->unwind_ipc_sp = get_tcb_stack_top(partner) - 9;
    unwind_ipc(tcb);

    /*
     * If thread is inside send phase, restart whole IPC.  If thread
     * is inside receive phase, cancel send phase by setting the send
     * descriptor to nil.
     */

    if ( tcb->flags & TF_SENDING_IPC )
	tcb->flags &= ~TF_SENDING_IPC;
    else
	get_tcb_stack_top(tcb)[IPCSTK_SNDDESC] = 0xffffffff;

    if ( partner->flags & TF_SENDING_IPC )
	partner->flags &= ~TF_SENDING_IPC;
    else
	get_tcb_stack_top(partner)[IPCSTK_SNDDESC] = 0xffffffff;

    /*
     * Push sys_ipc on the stacks, making thread_switch reenter the
     * sys_ipc function when thread is rescheduled.
     */

    void sys_ipc(l4_threadid_t, dword_t, dword_t);
    tcb_stack_push(tcb, (dword_t) sys_ipc);
    tcb_stack_push(partner, (dword_t) sys_ipc);

    tcb->thread_state = partner->thread_state = TS_RUNNING;
    tcb->resources &= ~TR_LONG_IPC_PTAB;
    partner->resources &= ~TR_LONG_IPC_PTAB;

    return (tcb == current) || (partner == current);
}

#if defined(CONFIG_GLOBAL_SMALL_SPACES)

/*
 * Function modify_global_bits (space, pg, size, onoff)
 *
 *    Set the global bit in page directory and page table entries as
 *    specified by ONOFF.  SIZE indicates the number of entries to
 *    modify.
 *
 */
static void modify_global_bits (space_t * space, pgent_t * pg,
				int size, int onoff)
{
    while (size > 0)
    {
	if (pg->is_valid (space))
	{
	    if (onoff == 0) pg->x.raw &= ~PAGE_GLOBAL;
	    else pg->x.raw |= PAGE_GLOBAL;

	    if (pg->is_subtree (space, 1))
	    {
		pgent_t *pg2 = pg->subtree (space, 1);
		for (int i = 1024; i > 0; i--)
		{
		    if (pg2->is_valid (space))
		    {
			if (onoff == 0) pg2->x.raw &= ~PAGE_GLOBAL;
			else pg2->x.raw |= PAGE_GLOBAL;
		    }
		    pg2 = pg2->next (space, 1);
		}
	    }
	}

	size -= PAGEDIR_SIZE;
	pg = pg->next (space, 1);
    }
}

#endif /* CONFIG_GLOBAL_SMALL_SPACES */
#endif /* CONFIG_ENABLE_SMALL_AS */


/**********************************************************************
 * temporary mapping for IPC
 * the resource bits are set in the pagefault handler!!!
 */

ptr_t get_copy_area(tcb_t * from, tcb_t * to, ptr_t addr)
{
#if defined(CONFIG_ENABLE_SMALL_AS)
    smallid_t small_from = from->space->smallid ();
    smallid_t small_to = to->space->smallid ();

    if (small_to.is_valid ())
    {
	/*
	 * Use small space area of target address space.
	 */
	TRACEPOINT(SMALL_SPACE_RECEIVER);
	return (ptr_t) ((dword_t) addr + small_to.offset ());
    }
    else if (small_from.is_valid ())
    {
	/*
	 * Use normal address for target address space and make sure
	 * that target address spaces' page tables are used.
	 */
	TRACEPOINT(SMALL_SPACE_SENDER);
	from->resources |= TR_LONG_IPC_PTAB;
	if (get_current_space() != to->space)
	    set_current_pagetable (to->space->pagedir_phys ());
	return addr;
    }
#endif

    dword_t *pgdir_from = from->space->pagedir ();
    dword_t *pgdir_to = tid_to_tcb(from->partner)->space->pagedir ();

    if ( from->copy_area1 == COPYAREA_INVALID )
    {
	/* Nothing mapped yet.  Use copy area 1. */
	from->copy_area1 = (dword_t) addr & PAGEDIR_MASK;
	
	pgdir_from[pgdir_idx(MEM_COPYAREA1)] =
	    pgdir_to[pgdir_idx((dword_t) addr)];
	pgdir_from[pgdir_idx(MEM_COPYAREA1)+1] =
	    pgdir_to[pgdir_idx((dword_t) addr)+1];

	from->resources |= TR_IPC_MEM1;
	return (ptr_t) (((dword_t) addr & ~PAGEDIR_MASK) + MEM_COPYAREA1);
    }
    else
    {
	if ( from->copy_area1 == ((dword_t) addr & PAGEDIR_MASK) )
	{
	    /* Copy area 1 contains the correct mapping. */
	    return (ptr_t) (((dword_t) addr & ~PAGEDIR_MASK) + MEM_COPYAREA1);
	}
	else if ( from->copy_area2 == ((dword_t) addr & PAGEDIR_MASK) )
	{
	    /*
	     * Copy area 2 contians the correct mapping.  Also works if
	     * area is invalid.
	     */
	    return (ptr_t) (((dword_t) addr & ~PAGEDIR_MASK) + MEM_COPYAREA2);
	} 
	else 
	{
	    /* Use copy area 2. */
	    pgdir_from[pgdir_idx(MEM_COPYAREA2)] =
		pgdir_to[pgdir_idx((dword_t) addr)];
	    pgdir_from[pgdir_idx(MEM_COPYAREA2)+1] =
		pgdir_to[pgdir_idx((dword_t) addr)+1];
		
	    if ( from->resources & TR_IPC_MEM2 )
	    {
		/*
		 * Copy area 2 in use (i.e., worst case).  We have to
		 * flush the existing TLB entries.
		 */
		enter_kdebug("remap copy_area2");
		flush_tlb();
	    }
	    else
		from->resources |= TR_IPC_MEM2;

	    from->copy_area2 = (dword_t) addr & PAGEDIR_MASK;
	    return (ptr_t) (((dword_t) addr & ~PAGEDIR_MASK) + MEM_COPYAREA2);
	}
    }

    /* NOTREACHED */
    return 0;
}

void free_copy_area(tcb_t * tcb)
{
    tcb->copy_area1 = COPYAREA_INVALID;
    tcb->copy_area2 = COPYAREA_INVALID;
}



/**********************************************************************
 * pagefault handling
 */

void pagefault(dword_t edx, dword_t ecx, dword_t eax, dword_t errcode, dword_t ip)
{
    /* The first three arguments of this function are dummy arguments - they
       have been pushed to the stack by the exception handler to save them
       over the C function call and restore them afterwards. */

    // fault is already pf-protocol compliant
    dword_t fault = (get_pagefault_address() & (~PAGE_WRITABLE)) | (errcode & PAGE_WRITABLE);
    // we need it in 99% of the interesting cases...
    tcb_t * current = get_current_tcb();

    /* Correct `current' for pagefaults happening on the kdebug stack. */
    if ( (dword_t) current > TCB_AREA + TCB_AREA_SIZE )
    {
	extern dword_t _kdebug_saved_esp;
	current = ptr_to_tcb((ptr_t) _kdebug_saved_esp);
    }


#ifdef CONFIG_DEBUG_DOUBLE_PF_CHECK
    if (last_pf_addr == fault && last_pf_tcb == current) {
	printf("double pagefault (%x)\n", fault);
	enter_kdebug();
    }
    last_pf_addr = fault;
    last_pf_tcb = current;
#endif

#if defined(CONFIG_DEBUGGER_KDB) && defined(CONFIG_DEBUG_BREAKIN)
    // if we have a kernel pagefault loop no timer interrupts are raised :( 
    kdebug_check_breakin();
#endif

    /*---- user pagefault ------------------------------------------------*/
    if ( PF_USERMODE(errcode) ) 
    {
#ifdef CONFIG_DEBUG_TRACE_UPF
	if (__kdebug_pf_tracing) {
	    printf("upf: thread: %x @ %x ip: %x, err: %x, cr3: %x\n", 
		   current, fault, ip, errcode, get_current_pagetable());
	    if (__kdebug_pf_tracing > 1)
		enter_kdebug("upf");
	}
#endif

	if ( fault >= TCB_AREA )
	{
	    printf("upf (%x): %x @ %x, err: %x\n",
		   current, fault, ip, errcode);
	    enter_kdebug("user touches kernel area");
#warning: REVIEWME: thread shoot-down is experimental
	    extern void unwind_ipc(tcb_t*);
	    unwind_ipc(current);
	    thread_dequeue_ready(current);
	    thread_dequeue_wakeup(current);
	    current->thread_state = TS_ABORTED;
	    while (1)
		switch_to_idle(current);
	}

	if (same_address_space (current, sigma0))
	{
	    /*
	     * Sigma0 pagefaults are simple map operations.  Bypass
	     * the mdb_map() operation.
	     */
	    ASSERT(fault < TCB_AREA);

	    pgent_t *pg = current->space->pgent (pgdir_idx (fault));
	    pg->set_address (current->space, 1, fault & PAGEDIR_MASK);
	    pg->set_flags (current->space, 1, PAGE_SIGMA0_BITS);
	}
	else
	{
	    /*
	     * Other pagefaults generate pagefault IPCs.
	     */
	    current->ipc_timeout = TIMEOUT_NEVER;

#if defined(CONFIG_ENABLE_SMALL_AS)
	    space_t * mspace = current->space;
	    if (mspace->is_small ())
	    {
		/*
		 * Check if we can simply copy a valid pagedir entry
		 * from the master page table.  If not, fall through
		 * and perform pagefault IPC (with translated fault
		 * address).
		 */
		space_t *fspace = get_current_space ();
		pgent_t *pg_f = fspace->pgent (pgdir_idx (fault));
		fault -= mspace->smallid ().offset ();
		pgent_t *pg_m = mspace->pgent (pgdir_idx (fault));

		if (pg_m->is_valid (mspace) && (! pg_f->is_valid (fspace)))
		{
		    /* Copy valid pagedir entry and try again. */
		    TRACEPOINT (COPY_SMALL_PDIRENT);
		    pg_f->copy (fspace, 1, pg_m, mspace, 1);
		    return;
		}
	    }
#endif

#if defined(CONFIG_DEBUG_TRACE_UPF)
	    if (__kdebug_pf_tracing) 
		printf("do_pf_ipc(%x, %x, %x)\n", current, fault, ip);
#endif
	    do_pagefault_ipc(current, fault, ip);
	}

	return;
    }

    /*---- kernel pagefault ----------------------------------------------*/

#ifdef CONFIG_DEBUG_TRACE_KPF
    if (__kdebug_pf_tracing) {
	printf("kpf: thread: %x @ %x ip: %x, err: %x, cr3: %x\n", 
	       current, fault, ip, errcode, get_current_pagetable());
#ifdef CONFIG_SMP
	printf("cpu: %d/%d\n", get_cpu_id(), get_apic_cpu_id());
#endif
	if (__kdebug_pf_tracing > 1)
	    enter_kdebug("kpf");
    }
#endif

    /* Pointer to the pgent we faulted on. */
    space_t *fspace = get_current_space ();
    pgent_t *pg_f = (pgent_t *) fspace->pgent (pgdir_idx (fault));

    if ( fault < TCB_AREA )
    {
	/*
	 * This is most likely a string copy pf.  Registers are
	 * already transfered, ipc_buffer[...] can therefore be
	 * spilled.  Alternatively, we might have caught a pagefault
	 * in the instruction parsing or LIDT emulation of the GP
	 * exception handler -- we may treat those faults in the same
	 * manner as string copy faults.
	 */

#if defined (CONFIG_DEBUG_SANITY)
	if (current->space == sigma0->space)
	    panic("Sigma0 - kpf in user area");
#endif

	TRACEPOINT_2PAR(KERNEL_USER_PF, fault, ip,
		        printf("kpf: thread: %x @ %x ip: %x, err: %x, cr3: %x\n", 
			       current, fault, ip, errcode, get_current_pagetable()));

#if defined(CONFIG_ENABLE_SMALL_AS)
	if ( fault >= SMALL_SPACE_START )
	{
	    tcb_t *partner = tid_to_tcb(current->partner);
	    space_t *mspace = current->space;
	    space_t *pspace = partner->space;
	    smallid_t small_m = mspace->smallid ();
	    smallid_t small_p = partner->space->smallid ();

	    if (small_m.is_valid () &&
		fault >= small_m.offset () &&
		fault < (small_m.offset () + small_m.bytesize ()))
	    {
		/*
		 * Pagefault in current small space.
		 */
		fault -= small_m.offset ();
		pgent_t *pg_m = mspace->pgent (pgdir_idx (fault));

		if (pg_m->is_valid (mspace) && (!pg_f->is_valid (fspace)))
		{
		    /* Copy valid pagedir entry and try again. */
		    pg_f->copy (fspace, 1, pg_m, mspace, 1);
		    return;
		}
	    }
	    else if ((~current->thread_state & TSB_LOCKED) &&
		     small_p.is_valid () &&
		     fault >= small_p.offset () &&
		     fault < (small_p.offset () + small_p.bytesize ()))
	    {
		/*
		 * Pagefault in partner's small space (i.e., during
		 * long IPC).
		 */
		fault -= small_p.offset ();
		pgent_t *pg_p = pspace->pgent (pgdir_idx (fault));

		if ((!pg_p->is_valid (pspace)) || pg_f->is_valid (fspace))
		{
		    /*
		     * Master pagedir or pagetab entry not present,
		     * tunnel pagefault through partner.
		     */
		    dword_t *saved_stack = partner->stack;
		    notify_thread_2(partner, copy_pagefault, fault,
				    (dword_t) current);
		    partner->thread_state = TS_LOCKED_RUNNING;
		    current->thread_state = TS_LOCKED_WAITING;
		    dispatch_thread(partner);
		    partner->stack = saved_stack;
		}

		/* Copy valid pagedir entry. */
		pg_f->copy (fspace, 1, pg_p, pspace, 1);
		return;
	    }
	    else
		panic("kernel pagefault in small space");
	}

	/* IPC buffer must be saved in case we have to restart IPC. */
	dword_t ipc_buf[3] =  { current->ipc_buffer[0],
				current->ipc_buffer[1],
				current->ipc_buffer[2] };
#endif

	l4_threadid_t saved_partner = current->partner;
	dword_t saved_state = current->thread_state;
	timeout_t to = current->ipc_timeout;

	TRACEPOINT(STRING_COPY_PF,
		   printf("String copy pagefault (%p @ %p) ip=%p\n",
			  current, fault, ip);
		   kdebug_dump_frame((exception_frame_t *)
				     get_tcb_stack_top(current) - 1));

	current->ipc_timeout =
	    GET_SEND_PF_TIMEOUT(tid_to_tcb(current->partner)->ipc_timeout);
	do_pagefault_ipc(current, fault, 0xffffffff /*ip*/);

	current->partner = saved_partner;
	current->ipc_timeout = to;
	current->thread_state = saved_state;
#if defined(CONFIG_ENABLE_SMALL_AS)
	current->ipc_buffer[0] = ipc_buf[0];
	current->ipc_buffer[1] = ipc_buf[1];
	current->ipc_buffer[2] = ipc_buf[2];
#endif
	return;
    } 


    if (fault < (TCB_AREA + TCB_AREA_SIZE))
    {
	space_t *kspace = get_kernel_space ();
	pgent_t *pg_k = kspace->pgent (pgdir_idx (fault));

	/*
	 * Try syncing TCB area, but not if running on kernel ptab.
	 */

	if (pg_f != pg_k)
	{
	    if (!pg_f->is_valid (fspace, 1) && pg_k->is_valid (kspace, 1))
	    {
		TRACEPOINT (SYNC_TCB_AREA,
			    printf ("Sync TCB area:  "
				    "kernel: %p/%x   this: %p/%x\n",
				    kspace, pg_k->x.raw,
				    fspace, pg_f->x.raw));

		pg_f->copy (fspace, 1, pg_k, kspace, 1);
		return;
	    }
	}

	/*
	 * TCB map request.
	 */

	if ((! pg_f->is_valid (fspace, 1)) || (! pg_f->is_subtree (fspace, 1)))
	{
	    pg_f->make_subtree (fspace, 1);
	    /* sync kernel pagedir */
	    pg_k->copy(kspace, 1, pg_f, fspace, 1);
	}

	pg_f = pg_f->subtree (fspace, 1);
	pg_f = pg_f->next (fspace, pgtab_idx (fault), 0);

	if (PF_WRITE (errcode))
	{
	    TRACEPOINT (MAP_NEW_TCB,
			printf ("map TCB page (%p @ %p)\n", fault, ip));

	    pg_f->set_address (fspace, 0, (dword_t)
			       virt_to_phys (kmem_alloc (PAGE_SIZE)));
	    pg_f->set_flags (fspace, 1, PAGE_TCB_BITS);
	    flush_tlbent ((ptr_t) fault); /* Get rid of zero-page entries. */
	    return;
	}
	else
	{
	    TRACEPOINT (MAP_ZERO_PAGE,
			printf ("map zero page (%p @ %p)\n", fault, ip));

	    pg_f->set_address (fspace, 0, (dword_t)
			       virt_to_phys (__zero_page));
	    pg_f->set_flags (fspace, 0, PAGE_ZERO_BITS);
	    return;
	}
    }
    else 
    {

#if !defined(CONFIG_DISABLE_LONGIPC)
	if ( fault >= MEM_COPYAREA1 && fault < MEM_COPYAREA_END ) 
	{
	    tcb_t * partner = tid_to_tcb(current->partner);
	    ptr_t pdir_to   = partner->space->pagedir ();
	    ptr_t pdir_from = current->space->pagedir ();

	    dword_t *saved_stack;

	    if ( fault < MEM_COPYAREA2 )
	    {
		if ( current->resources & TR_IPC_MEM1 )
		{
		    /*
		     * Page has not been mapped. Tunnel pagefault to
		     * the receiving thread.
		     */
		    saved_stack = partner->stack;
		    notify_thread_2(partner, copy_pagefault, 
				    current->copy_area1 +
				    (fault - MEM_COPYAREA1), 
				    (dword_t) current);

		    partner->thread_state = TS_LOCKED_RUNNING;
		    current->thread_state = TS_LOCKED_WAITING;

		    /* Receiver is already present in the ready queue. */
		    dispatch_thread(partner);
		    partner->stack = saved_stack;
		
		    /*
		     * We did an IPC (i.e., thread switch), implying
		     * that the pagetable entires of copy area are
		     * possibly gone.  Fall through so that the
		     * pagedir entries are updated again.
		     */
		}

		pdir_from[pgdir_idx(MEM_COPYAREA1)] =
		    pdir_to[pgdir_idx(current->copy_area1)] & ~PAGE_USER;
		pdir_from[pgdir_idx(MEM_COPYAREA1) + 1] =
		    pdir_to[pgdir_idx(current->copy_area1) + 1] & ~PAGE_USER;

		current->resources |= TR_IPC_MEM1;
		return;
	    }
	    else 
	    {
		if ( current->resources & TR_IPC_MEM2 )
		{
		    /*
		     * Page has not been mapped. Tunnel pagefault to
		     * the receiving thread.
		     */
		    saved_stack = partner->stack;
		    notify_thread_2(partner, copy_pagefault, 
				    current->copy_area2 +
				    (fault - MEM_COPYAREA2), 
				    (dword_t)current);

		    partner->thread_state = TS_LOCKED_RUNNING;
		    current->thread_state = TS_LOCKED_WAITING;

		    /* Receiver is already present in the ready queue. */
		    dispatch_thread(partner);
		    partner->stack = saved_stack;
		}

		pdir_from[pgdir_idx(MEM_COPYAREA2)] =
		    pdir_to[pgdir_idx(current->copy_area2)] & ~PAGE_USER;
		pdir_from[pgdir_idx(MEM_COPYAREA2) + 1] =
		    pdir_to[pgdir_idx(current->copy_area2) + 1] & ~PAGE_USER;

		current->resources |= TR_IPC_MEM2;
		return;
	    }
	}
#endif /* !CONFIG_DISABLE_LONGIPC */

#if 0
	else if (fault >= phys_to_virt(0U) && fault < phys_to_virt(kernel_info_page.main_mem_high))
	{
	    /* is the area mapped in the kernel's pagetable ? */
	    ptr_t pdir = get_current_pagetable();
	    ptr_t pkdir = get_kernel_pagetable();
	    /* do we run on the kernel ptab (idler...)? */
	    if (pdir != virt_to_phys((ptr_t) pkdir)) 
	    {
		if (pkdir[fault >> PAGEDIR_BITS] != phys_to_virt(pdir)[fault >> PAGEDIR_BITS])
		{
		    /* sync pagetables */
		    phys_to_virt(pdir)[fault >> PAGEDIR_BITS] = pkdir[fault >> PAGEDIR_BITS];
		    return;
		}
	    }
	    printf("map physmem\n");
	    x86_map_kpage(fault & PAGE_MASK, ((dword_t)virt_to_phys(fault & PAGE_MASK)) | PAGE_KERNEL_BITS);
	    return;
	}
#endif	
#if defined (CONFIG_SMP)
	printf("kernel pf: %x ip: %x cpu: %x\n", fault, ip, get_cpu_id());
#else
	printf("kernel pf: %x ip: %x\n", fault, ip);	
#endif
	panic("kernel pagefault");
    }
    printf("%x", fault);
    spin(79);
}
