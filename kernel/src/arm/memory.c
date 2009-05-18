/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/memory.c
 * Description:   ARM specific memory management.
 *                
 * @LICENSE@
 *                
 * $Id: memory.c,v 1.37 2001/12/12 23:47:06 ud3 Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include INC_ARCH(memory.h)
#include INC_PLATFORM(platform.h)
#include <mapping.h>
#include INC_ARCH(syscalls.h)
#include INC_ARCH(farcalls.h)
#include <tracepoints.h>

//#define DOUBLE_PF_CHECK

dword_t* kernel_ptdir = NULL;

dword_t hw_pgshifts[HW_NUM_PGSIZES+1] = { 12, 16, 20, 32 };


#ifdef DOUBLE_PF_CHECK
static dword_t last_pf = ~0;
#endif


void arm_map_page (space_t * space, dword_t vaddr, dword_t paddr,
		   dword_t ap, dword_t pgsize)
{
//    printf("arm_map_page(space=%p, vaddr=%p, paddr=%p, ap=%x, pgsize=%x)\n",
//	   space, vaddr, paddr,  ap, pgsize);

    dword_t remap = 0;
    dword_t pgsz = HW_NUM_PGSIZES-1;
    pgent_t *pg = space->pgent (vaddr >> PAGE_SECTION_BITS);

    while (pgsz > pgsize)
    {
	if (!pg->is_valid (space, pgsz))
	    pg->make_subtree (space, pgsz);
	else if (!pg->is_subtree (space, pgsz))
		panic("arm_map_page(): Mapping is no subtree.\n");

	pgsz--;
	pg = pg->subtree (space, pgsz+1)->next (space, table_index (vaddr, pgsz), pgsz);
    }

    remap = pg->is_valid (space, pgsize);
    pg->set_entry (space, paddr, 0, pgsize);
    pg->set_access_bits (space, pgsize, ap, 1, 1);

    if (remap)
	flush_tlbent ((ptr_t) vaddr);
}

space_t * create_space (void)
{
    // allocate root ptab
    fld_t* new_ptdir = (fld_t*) kmem_alloc(FLD_SIZE*2);
    
    /* copy kernel area (TCBs, cont. phys mem, kernel code/data */
    for (dword_t i = vaddr_to_fld(TCB_AREA);
	 i <= vaddr_to_fld(0xFFFFFFFF);
	 i++)
	new_ptdir[i] = ((fld_t *) kernel_ptdir)[i];

#ifndef EXCEPTION_VECTOR_RELOCATED
#warning HACK

    /* allocate second level pagetable */
    sld_t* ptbl = (sld_t*) kmem_alloc(SLD_SIZE);
    /* copy section entry from kernel pagetable */
    new_ptdir[0] = ((fld_t *) kernel_ptdir)[0];
    /* install second level pagetable instead */
    new_ptdir[0].page.ptba = paddr_to_ptba(virt_to_phys((dword_t)ptbl));
    /* copy entry for exception vector page */
    *((sld_t*) phys_to_virt(ptba_to_paddr(new_ptdir[0].page.ptba))) = *((sld_t*) phys_to_virt(ptba_to_paddr(((fld_t *) kernel_ptdir)[0].page.ptba)));
#endif


//    printf("%s: new ptgable at %p\n", __FUNCTION__, new_ptdir);
    return (space_t *) virt_to_phys(new_ptdir);
}


void delete_space (space_t * space)
{
    printf("   %s: %p\n", __FUNCTION__, space);

    fpage_unmap (space, FPAGE_WHOLE_SPACE, (FPAGE_OWN_SPACE|FPAGE_FLUSH));

    fld_t* pd = (fld_t*) space->pagedir ();

    /* kmem_free all 2nd level pagetables */
    for (dword_t i = 0; i < TCB_AREA; i += (1 << FLD_SHIFT))
	if (fld_type (&pd[vaddr_to_fld (i)]) == FLD_PAGE)
	    kmem_free ((dword_t *) phys_to_virt
		       (ptba_to_paddr (pd[vaddr_to_fld (i)].page.ptba)),
		       SLD_SIZE);

    /* kmem_free the page table directory */
    kmem_free ((ptr_t) pd, FLD_SIZE*2);
}



/*
 * Function kernel_data_abort (fault_status, fault_address)
 *
 *    Handle data aborts within the kernel.  Usually this means that
 *    we have to handle some TLB mapping.  If this is not the case, we
 *    should panic big time.
 *
 */
void kernel_pagefault(dword_t fault_status, dword_t fault_address, dword_t ip)
{
#ifdef CONFIG_DEBUG_TRACE_KPF
    if (__kdebug_pf_tracing)
    {
	printf("kpf: thread: %x @ %x ip: %x, fsr: %x, ptab: %x\n", 
	       get_current_tcb(), fault_address, ip,
	       fault_status, get_current_pagetable());
	if (__kdebug_pf_tracing > 1)
	    enter_kdebug("kpf");
    }
#endif
#ifdef DOUBLE_PF_CHECK
    if (last_pf == fault_address) {
	enter_kdebug("double pagefault");
    }
    last_pf = fault_address;
#endif

    space_t * kspace = get_kernel_space ();

    /* Only by reading the instruction and checking for the L-bit (20)
       we can find out whether we had a read or a write pagefault.
       Inside the kernel we'll never have instruction fetch faults. */
    dword_t read_fault = *((ptr_t) ip) & (1 << 20);

    /* Keep only the interesting bits. */
    fault_status &= STATUS_MASK;
    
    if ( (fault_address >= TCB_AREA) && 
	 (fault_address < (TCB_AREA + TCB_AREA_SIZE)) )
    {
	/*
	 * Fault_address is in TCB region.
	 */
	if ( fault_status == TRANSLATION_FAULT_S )
	{
	    pgent_t *kpg = (pgent_t *) kernel_ptdir;
	    pgent_t *cpg = (pgent_t *) phys_to_virt(get_current_pagetable());
	    pgent_t *pg;

	    kpg += fault_address >> PAGE_SECTION_BITS;
	    cpg += fault_address >> PAGE_SECTION_BITS;
	    
	    /* First level fault - lookup in kernel pagetable. */
	    if ( kpg->is_valid (kspace, PGSIZE_1MB) )
	    {
		/* Found in kernel pagetable. */
		cpg->x.raw = kpg->x.raw;

		/* If a 4K mapping exists, just return. */
		pg = (pgent_t *) phys_to_virt(kpg->x.raw & PAGE_TABLE_MASK);
		pg += (fault_address & ~PAGE_SECTION_MASK) >> PAGE_SMALL_BITS;

		if ( (pg->x.raw & PAGE_TYPE_MASK) == 2 )
		    return;
	    }
	    else
	    {
		/* Have to allocate 2nd level array. */
		pg = (pgent_t *) kmem_alloc(1024);

		cpg->x.raw = (dword_t) virt_to_phys(pg) | 0x01;
		for( int i = 0; i < 256; i++ )
		    pg[i].x.raw = PAGE_64K_VALID;
		
		/* Update kernel pagetable. */
		kpg->x.raw = cpg->x.raw;
	    }
	}

	if (read_fault)
	{
	    TRACEPOINT (MAP_ZERO_PAGE,
			printf ("map zero page (%p @ %p)\n",
				fault_address, ip));

	    /* Map in the zero page READ-ONLY */
	    arm_map_page (kspace, fault_address,
			  (dword_t) virt_to_phys (__zero_page),
			  PAGE_AP_RESTRICTED, PGSIZE_4KB);
	    return;
	}
	else
	{
	    TRACEPOINT (MAP_NEW_TCB,
			printf ("map TCB page (%p @ %p)\n",
				fault_address, ip));

	    /* Map in a fresh tcb page. */
	    arm_map_page (kspace, fault_address,
			  (dword_t) virt_to_phys(kmem_alloc(PAGE_SMALL_SIZE)),
			  PAGE_AP_NOACCESS, PGSIZE_4KB);

	    /* Get rid of zero-page entries. */
	    flush_tlbent ((ptr_t) fault_address);
#warning Avoid Flush_ID_Cache
	    Flush_ID_Cache();
	    return;
	}

    }

    if (fault_address >= MEM_COPYAREA1 && fault_address < MEM_COPYAREA_END) 
    {
	tcb_t* current = get_current_tcb();
	tcb_t * partner = tid_to_tcb(current->partner);
	ptr_t pdir_from = current->space->pagedir ();
	ptr_t pdir_to = partner->space->pagedir ();
	
	dword_t *saved_stack;
	
	printf("fault in copy area %x @ %x\n", current, fault_address);
	
	if (fault_address < MEM_COPYAREA2)
	{
	    if (current->resources & TR_IPC_MEM1)
	    {
		/* pagefault */
		//printf("tunneled pf ipc1\n");
		//enter_kdebug();
		
		saved_stack = partner->stack;
		notify_thread_2(partner, copy_pagefault, 
				current->copy_area1 + (fault_address - MEM_COPYAREA1), 
				(dword_t)current);
		partner->thread_state = TS_LOCKED_RUNNING;
		current->thread_state = TS_LOCKED_WAITING;
		/* xxx: should we: thread_enqueue_ready(partner); */
		dispatch_thread(partner);
		//printf("tunneled pf done\n");
		partner->stack = saved_stack;
		    
		    /* we did an IPC here with a thread switch - the pagetable 
		     * entries of the copy area are gone -> we would directly 
		     * pagefault -> so run into the pagedir update...
		     */
		}

		pdir_from[MEM_COPYAREA1 >> PAGEDIR_BITS] = pdir_to[current->copy_area1 >> PAGEDIR_BITS];
		pdir_from[(MEM_COPYAREA1 >> PAGEDIR_BITS) + 1] = pdir_to[(current->copy_area1 >> PAGEDIR_BITS) + 1];
		current->resources |= TR_IPC_MEM1;
		return;
	    }
	    else 
	    {
		printf("fault in copy area %x @ %x\n", current, fault_address);

		if (current->resources & TR_IPC_MEM2)
		{
		    /* pagefault */
		    //printf("tunneled pf ipc2\n");
		    //enter_kdebug();

		    saved_stack = partner->stack;
		    notify_thread_2(partner, copy_pagefault, 
				    current->copy_area2 + (fault_address - MEM_COPYAREA2), 
				    (dword_t)current);
		    partner->thread_state = TS_LOCKED_RUNNING;
		    current->thread_state = TS_LOCKED_WAITING;
		    /* xxx: should we: thread_enqueue_ready(partner); */
		    dispatch_thread(partner);
		    //printf("tunneled pf2 done\n");
		    partner->stack = saved_stack;
		}

		/* see comment above */
		pdir_from[MEM_COPYAREA2 >> PAGEDIR_BITS] = pdir_to[current->copy_area2 >> PAGEDIR_BITS];
		pdir_from[(MEM_COPYAREA2 >> PAGEDIR_BITS) + 1] = pdir_to[(current->copy_area2 >> PAGEDIR_BITS) + 1];
		current->resources |= TR_IPC_MEM2;
		return;
	    }
    }
    panic( "Whooiips.  Data abort within kernel.\n" );
}


/*
 * Function user_pagefault (fault_status, fault_address, ip)
 *
 *    Handle data aborts within user level.  This will usually
 *    generate a pagefault IPC.  The exception is pagefaults within
 *    sigma0 which will cause a 1MB page to be mapped.
 *
 */
void user_pagefault(dword_t fault_status, dword_t fault_address, dword_t ip)
{
    tcb_t *current = get_current_tcb();

#ifdef CONFIG_DEBUG_TRACE_UPF
    if (__kdebug_pf_tracing)
    {
	printf("upf: thread: %x @ %x ip: %x, fsr: %x, ptab: %x\n", 
	       current, fault_address, ip,
	       fault_status, get_current_pagetable());
	if (__kdebug_pf_tracing > 1)
	    enter_kdebug("upf");
    }
#endif

    TRACEPOINT_2PAR(KERNEL_USER_PF, fault, ip,
		    printf("upf: thread: %x @ %x ip: %x, fsr: %x, ptab: %x\n", 
			   current, fault_address, ip,
			   fault_status, get_current_pagetable()));

#ifdef DOUBLE_PF_CHECK
    if (last_pf == fault_address) {
	enter_kdebug("double pagefault");
    }
    last_pf = fault_address;
#endif

    /* Figure out whether we're dealing with a write fault or not. */
    fault_status &= STATUS_MASK;
    fault_address &= ~3;
    fault_address |= ((fault_status == PERMISSION_FAULT_S) ||
		      (fault_status == PERMISSION_FAULT_P)) ? 2 : 0;

    if ( !IN_KERNEL_AREA(fault_address) )
    {
	if ( same_address_space(sigma0, current) )
	{
	    /*
	     * Sigma0 pagefaults will just cause the page tables to be
	     * populated with 1MB pages.  We bypass the mdb_map()
	     * because sigma0 is already defined to own the whole
	     * address space.
	     */
	    arm_map_page (current->space, fault_address, fault_address,
			  PAGE_AP_WRITABLE, PGSIZE_1MB);
	    return;
	}

	/* Other tasks will generate a pagefault IPC to their pager. */
	current->ipc_timeout = TIMEOUT_NEVER;
	do_pagefault_ipc(current, fault_address, ip);
	return;
    }

    panic(__FUNCTION__);
}







/**********************************************************************
 * temporary mapping for IPC
 * the resource bits are set in the pagefault handler!!!
 */

ptr_t get_copy_area(tcb_t * from, tcb_t * to, ptr_t addr)
{
    //printf("%s: %x->%x, %x\n", __FUNCTION__, from, to, addr);
    //enter_kdebug();

    if (from->copy_area1 == COPYAREA_INVALID)
    {
	/* nothing mapped yet */
	from->copy_area1 = ((dword_t)addr & PAGEDIR_MASK);
	return (ptr_t)(((dword_t)addr & ~PAGEDIR_MASK) + MEM_COPYAREA1);
    }
    else {
	if (from->copy_area1 == ((dword_t)addr & PAGEDIR_MASK))
	    return (ptr_t)((((dword_t)addr) & ~PAGEDIR_MASK) + MEM_COPYAREA1);
	else if (from->copy_area2 == ((dword_t)addr & PAGEDIR_MASK))
	{
	    /* works for invalid too :) */
	    return (ptr_t)((((dword_t)addr) & ~PAGEDIR_MASK) + MEM_COPYAREA2);
	} 
	else 
	{
	    if (from->resources & TR_IPC_MEM2)
	    {
		/* resource area 2 in use -> worst case
		 * we have to manipulate the pagetable 
		 */
		enter_kdebug("remap copy_area2");

		ptr_t pdir_from = from->space->pagedir ();
		ptr_t pdir_to = tid_to_tcb (from->partner)->space->pagedir ();
		pdir_from[MEM_COPYAREA2 >> PAGEDIR_BITS] = pdir_to[(dword_t)addr >> PAGEDIR_BITS];
		pdir_from[(MEM_COPYAREA2 >> PAGEDIR_BITS) + 1] = pdir_to[((dword_t)addr >> PAGEDIR_BITS) + 1];
		
		flush_tlb();
	    }
	    
	    from->copy_area2 = ((dword_t)addr) & PAGEDIR_MASK;
	    //printf("copy_area2: %x\n",from->copy_area2);
	    //from->resources |= TR_IPC_MEM2;

	    return (ptr_t)((((dword_t)addr) & ~PAGEDIR_MASK) + MEM_COPYAREA2);
	}
    }

    return 0;
}

void free_copy_area(tcb_t * tcb)
{
    tcb->copy_area1 = COPYAREA_INVALID;
    tcb->copy_area2 = COPYAREA_INVALID;
}
