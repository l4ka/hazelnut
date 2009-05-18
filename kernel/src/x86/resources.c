/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     x86/resources.c
 * Description:   Thread resource management for x86.
 *                
 * @LICENSE@
 *                
 * $Id: resources.c,v 1.5 2002/05/13 13:04:31 stoess Exp $
 *                
 ********************************************************************/

#include <universe.h>
#include <thread.h>
#include <tracepoints.h>

void save_resources(tcb_t *current, tcb_t *dest)
{
    TRACEPOINT(SAVE_RESOURCES, printf("save_resources: tcb=%p  rc=%p\n",
				      current, current->resources));

    /*
     * Debug registers must be saved upon each context switch and all
     * current breakpoints must be disabled.
     */
    if ( current->resources & TR_DEBUG_REGS )
    {
	save_debug_regs((ptr_t) &current->priv[0].dbg);
	asm ("mov %0, %%dr7" : :"r"(0));
    }

    /*
     * Floating point state can be saved lazily (i.e., next time
     * floating point unit is used).
     */
    if ( current->resources & TR_FPU )
    {
	disable_fpu();
	current->resources &= ~TR_FPU;
    }

    /*
     * Remove any temporary copy-area mapppings.
     */
    if ( current->resources & TR_IPC_MEM )
    {
	dword_t * pgdir = current->space->pagedir ();

	pgdir[pgdir_idx(MEM_COPYAREA1)] = 0;
	pgdir[pgdir_idx(MEM_COPYAREA1)+1] = 0;
	pgdir[pgdir_idx(MEM_COPYAREA2)] = 0;
	pgdir[pgdir_idx(MEM_COPYAREA2)+1] = 0;

	if ( dest == get_idle_tcb () ||
#if defined(CONFIG_ENABLE_SMALL_AS)
	     dest->space->is_small () ||
#endif
	     same_address_space(current, dest) )
	    flush_tlb();

	current->resources &= ~TR_IPC_MEM;
    }
#if defined(CONFIG_ENABLE_PVI)    
    
    if ( current->resources & TR_PVI )
    {
	// printf("PVI resources\n");
	dword_t dest_ufl = get_user_flags(dest);
	dword_t src_ufl = get_user_flags(current);
	
	current->resources &= ~TR_PVI;
	dest->resources |= TR_PVI;	
	src_ufl &= ~X86_EFL_VIP;
	dest_ufl |= X86_EFL_VIP;

	set_user_flags(dest, dest_ufl);
	set_user_flags(current, src_ufl);
		
    }
#endif
}


/* 
 * Purges all resources that the thread holds, including lazily stored
 * resources.  Needed for, e.g., thread migration.
 */
void purge_resources(tcb_t * tcb)
{
    TRACEPOINT(PURGE_RESOURCES, printf("purge_resources: tcb=%p\n", tcb));

    /* the current FPU owner resides in the cpu-local section */
    extern tcb_t* fpu_owner;
    if (fpu_owner == tcb)
    {
#if defined(X86_FLOATING_POINT_ALLOC_SIZE)
	save_fpu_state((byte_t*) tcb->priv->fpu_ptr);
#else
	save_fpu_state((byte_t*) &tcb->priv->fpu);
#endif
	fpu_owner = NULL;
    }
}


void load_resources(tcb_t *current)
{
    TRACEPOINT(LOAD_RESOURCES, printf("load_resources: tcb=%p  rc=%p\n",
				      current, current->resources));

#if defined(CONFIG_ENABLE_SMALL_AS)
    if ( current->resources & TR_LONG_IPC_PTAB )
    {
	/*
	 * Current thread (in small address space) is doing long IPC
	 * and needs to use the pagetable of its partner for copying.
	 */
	enter_kdebug("setting live ptab");
	ptr_t pgdir = tid_to_tcb (current->partner)->space->pagedir_phys ();
	if ( get_current_pagetable() != pgdir )
	    set_current_pagetable(pgdir);
    }
#endif

    if ( current->resources & TR_DEBUG_REGS )
	load_debug_regs((ptr_t) &current->priv[0].dbg);
#if defined(CONFIG_ENABLE_PVI)    
    if ( current->resources & TR_PVI )
    {
	dword_t ufl = get_user_flags(current);
	
	// if ((ufl & X86_EFL_VIF) || (current == get_idle_tcb())){
	if ((current == get_idle_tcb() || current == sigma0)){

	    extern void handle_pending_irq(void);
	    current->resources &= ~TR_PVI;
	    ufl &= ~X86_EFL_VIP;
	    set_user_flags(current, ufl);
	    handle_pending_irq();
	    
	}
    }
#endif

}


void init_resources(tcb_t * tcb)
{ 
#if defined(X86_FLOATING_POINT_ALLOC_SIZE)
    tcb->priv->fpu_ptr = NULL;
#else
    tcb->priv->fpu_used = 0;		// fpu not used
#endif
}

void free_resources(tcb_t * tcb)
{
#if defined(X86_FLOATING_POINT_ALLOC_SIZE)
    if (tcb->priv->fpu_ptr)
    {
	kmem_free ((ptr_t) tcb->priv->fpu_ptr, X86_FLOATING_POINT_ALLOC_SIZE);
	tcb->priv->fpu_ptr = NULL;
    }
#endif

    extern tcb_t *fpu_owner;
    if (fpu_owner == tcb)
	fpu_owner = NULL;
}

