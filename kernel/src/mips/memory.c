/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/memory.c
 * Description:   MIPS specific memory management handling.
 *                
 * @LICENSE@
 *                
 * $Id: memory.c,v 1.5 2001/11/22 13:27:53 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <mapping.h>

dword_t __pdir_root[PAGE_SIZE / 4] __attribute__((aligned(4096)));

/* context specific registers */
dword_t * current_stack;
dword_t * current_page_table;

#define EXC_CODE(x)		((x >> 2) & 0x1f)

#define PF_USERMODE(x)		(x < KERNEL_VIRT)
#define PF_WRITE(x)		(EXC_CODE(x) == 3)
#define PF_PROTECTION(x)	(EXC_CODE(x) == 1)

#define FP_NUM_PAGES(x)		(1 << (((x & 0xfc) >> 2) - PAGE_BITS))
#define FP_ADDR(x)		(x & PAGE_MASK)

//#define DOUBLE_PF_CHECK
#ifdef DOUBLE_PF_CHECK
static dword_t last_pf_addr = ~0;
static tcb_t * last_pf_tcb = NULL;
#endif

void init_paging()
{
    zero_memory(__pdir_root, PAGE_SIZE);
    printf("pdir_root: %x\n", __pdir_root);

    __asm__ __volatile__ (
	"mtc0 %0, $5\n" 
	:
	: "r"(0x00fffc000));

    current_page_table = __pdir_root;

}

ptr_t create_new_pagetable()
{
    printf("create_new_ptab\n");

    // allocate root ptab
    dword_t *pt = (dword_t*)kmem_alloc(PAGE_SIZE);
    
    // now copy the kernel area - assumption: one pagetable
    pt[KERNEL_VIRT >> PAGEDIR_BITS] = __pdir_root[KERNEL_VIRT >> PAGEDIR_BITS];

    /* and the tcb area. we need that, because otherwise we cannot create 
     * new tasks. we create a task and switch to it. the exception stack is 
     * on our own tcb. thus we need at least our own tcb mapped. 
     */
    for (dword_t i = (TCB_AREA >> PAGEDIR_BITS); i < ((TCB_AREA + TCB_AREA_SIZE) >> PAGEDIR_BITS); i++)
	pt[i] = __pdir_root[i];

    return pt;
}

/*
 * Function delete_pagetable (tcb)
 *
 *    Called from task_delete().
 *
 */
void delete_pagetable(tcb_t *tcb)
{
    /* Flush whole address space. */
    fpage_unmap((dword_t) tcb->page_dir, FPAGE_WHOLE_SPACE,
		(FPAGE_OWN_SPACE | FPAGE_FLUSH));

    /* Delete page directory. */
    kmem_free((ptr_t) phys_to_virt(tcb->page_dir), PAGE_SIZE*2);
}


void mips_map_kpage(dword_t addr, dword_t val)
{
    printf("mips_map_kpage %x, %x\n", addr, val);

    dword_t *ptab = (dword_t*)__pdir_root[addr >> PAGEDIR_BITS];

    /* first check if there is a second level pagetable */
    if (!ptab)
    {
	/* allocate second level */
	ptab = kmem_alloc(PAGE_SIZE);
	__pdir_root[addr >> PAGEDIR_BITS] = (dword_t)(ptab);
	printf("alloced 2nd level: %x\n", __pdir_root[addr >> PAGEDIR_BITS]);
    }
    ptab[(addr >> PAGE_BITS) & ~PAGE_MASK] = val;
} 

void mips_map_upage(ptr_t pdir, dword_t addr, dword_t val)
{
    printf("mips_map_upage %x, %x, %x\n", pdir, addr, val);
    enter_kdebug("map userpage");
#if 0
    dword_t *ptab = (dword_t*)pdir[addr >> PAGEDIR_BITS];
    dword_t *vptab;

    //printf("map_upage: pdir %x (%x), val %x\n", pdir, &pdir[addr >> 22], ptab);
    /* first check if there is a second level pagetable */
    if (!((dword_t)ptab & PAGE_VALID))
    {
	/* allocate second level */
	ptab = virt_to_phys(kmem_alloc(PAGE_SIZE));
	pdir[addr >> PAGEDIR_BITS] = ((dword_t)(ptab)) | PT_USER_TABLE;
    }
    vptab = phys_to_virt((dword_t*)((dword_t)ptab & PAGE_MASK));
    //printf("ptab: %x (%x), addr: %x, val: %x (%x)\n", ptab, vptab, addr, val, vptab[(addr >> 12) & 0x3ff]);
    vptab[(addr >> PAGE_BITS) & ~PAGE_MASK] = val;
#endif
}

dword_t lookup_page(ptr_t pdir, dword_t vaddr)
{
#if 0
    dword_t *ptab = (dword_t*)pdir[vaddr >> PAGEDIR_BITS];
    dword_t *vtab;
    if (!((dword_t)ptab & PAGE_VALID)) return NULL;
    vtab = phys_to_virt((dword_t*)((dword_t)ptab & PAGE_MASK));
    return vtab[(vaddr >> PAGE_BITS) & ~PAGE_MASK];
#else
    return 0;
#endif
}


/*
 * Function pg_flush (pgdir, vaddr, pgsize, mode)
 *
 *    Called from mdb_flush() whenever a page table entry needs to be
 *    changed.
 *
 */
void pg_flush(dword_t pgdir, dword_t vaddr, dword_t pgsize, dword_t mode)
{
    enter_kdebug("pg_flush not impl.");
#if 0
    dword_t *pdent, *ptent;


    if ( __kdebug_pf_tracing )
    {
	printf("pg_flush(%x, %x, %dK, %x)\n",
	       pgdir, vaddr, 1 << (mdb_pgshifts[pgsize]-10), mode);
    }

    pdent = ((dword_t *) pgdir) + pgdir_idx(vaddr);
    
    if ( !(*pdent & PAGE_VALID) )
    {
	printf("pg_flush: no pagetable present (pdir: %x, vaddr: %x, "
	       "pdent: %x)\n", pgdir, vaddr, *pdent);
	enter_kdebug();
	return;
    }

    /* Flush/remap regular page */
    ptent = (dword_t *) (*pdent & PAGE_MASK) + pgtab_idx(vaddr);

    if ( mode & MDB_FLUSH )
	*ptent = 0;
    else
	*ptent &= ~PAGE_WRITABLE;

    if ( pgdir ==  get_current_pagetable() )
	flush_tlbent((ptr_t) vaddr);
#endif
}


/**********************************************************************
 * temporary mapping for IPC
 * the resource bits are set in the pagefault handler!!!
 */

ptr_t get_copy_area(tcb_t * from, tcb_t * to, ptr_t addr)
{
#if 0
    //printf("get_map_area: %x->%x, %x\n", from, to, addr);
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
		printf("remap copy_area2\n");
		enter_kdebug();

		ptr_t pdir_from = phys_to_virt(from->page_dir);
		ptr_t pdir_to = phys_to_virt((tid_to_tcb(from->partner))->page_dir);
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
#endif
    return 0;
}

void free_copy_area(tcb_t * tcb)
{
    tcb->copy_area1 = COPYAREA_INVALID;
    tcb->copy_area2 = COPYAREA_INVALID;
}

void save_resources(tcb_t * current, tcb_t * dest)
{
#if 0
    printf("free thread resources (tcb: %x (%x))\n", current, current->resources);
    if (current->resources | TR_IPC_MEM)
    {
	ptr_t pdir = phys_to_virt(current->page_dir);
	pdir[MEM_COPYAREA1 >> PAGEDIR_BITS] = 0;
	pdir[(MEM_COPYAREA1 >> PAGEDIR_BITS) + 1] = 0;

	pdir[MEM_COPYAREA2 >> PAGEDIR_BITS] = 0;
	pdir[(MEM_COPYAREA2 >> PAGEDIR_BITS) + 1] = 0;

	if ((dest == get_idle_tcb()) || same_address_space(current, dest))
	    flush_tlb();
    }
	
    current->resources = 0;
#endif
}

void load_resources(tcb_t *current)
{
    /* currently no resources are loaded on MIPS */
}

void init_resources(tcb_t * tcb)
{
    /* currently no resources are initialized on MIPS */
}

void free_resources(tcb_t * tcb)
{
    /* currently no resources are freed on MIPS */
}

/**********************************************************************
 * pagefault handling
 */

dword_t get_current_stack()
{
    dword_t tmp;
    asm("move	%0, $sp\n"
	: "=r"(tmp));
    return tmp;
}

void pagefault(dword_t fault, dword_t cause, dword_t ip)
{
    tcb_t * current;

#ifdef DOUBLE_PF_CHECK
    if (last_pf_addr == fault && last_pf_tcb == get_current_tcb()) {
	printf("double pagefault (%x)\n", fault);
	enter_kdebug();
    }
    last_pf_addr = fault;
    last_pf_tcb = get_current_tcb();
#endif

    //printf("pagefault: %x ip: %x, cause: %x\n", fault, ip, cause);
    //printf("tcb: %x, stack: %x\n", get_current_tcb(), get_current_stack());

    /*---- user pagefault ------------------------------------------------*/
    if (PF_USERMODE(ip)) 
    {

	current = get_current_tcb();
	printf("upf (%x): %x @ %x, err: %x\n", current, fault, ip, cause);
	enter_kdebug();

	if (fault >= TCB_AREA)
	    panic("user touches kernel area\n");


	if (current->page_dir == sigma0->page_dir) 
	{
	    /* sigma0 pf are simple map operations */
	    ASSERT(fault < TCB_AREA);
	    mips_map_upage(phys_to_virt(current->page_dir), fault, 
			   (fault & PAGE_MASK) | PT_SIGMA0);
	    return;
	}
	else {
	    printf("do pf ipc to %x\n", current->pager);
	    panic("upf");
	    //do_pagefault_ipc(current, fault, ip);
	    return;
	}
	spin(78);
    }

    /*---- kernel pagefault ----------------------------------------------*/

    printf("kpf: %x @ %x, cause: %x\n", fault, ip, cause);


    /* kernel pagefault */
    if (PF_PROTECTION(cause)) {
	/* null-page remap? */
	dword_t phys = lookup_page(phys_to_virt(get_current_pagetable()), fault);
	if ((phys & PAGE_MASK) == (dword_t)virt_to_phys(__zero_page)) 
	{
	    // remap
	}
	panic("kernel protection error");
    }

    if (fault < TCB_AREA) {
	/* that is a string copy pf, registers are 
	 * already transfered - ipc_buffer[...] can be spilled
	 */
	current = get_current_tcb();
	l4_threadid_t saved_partner = current->partner;

	printf("string copy pf...");
	enter_kdebug();
	//do_pagefault_ipc(current, fault, 0xffffffff);
	//printf("...handled\n");

	current->partner = saved_partner;
	current->thread_state = TS_LOCKED_RUNNING;

	return;
    } 
    else if (fault < (TCB_AREA + TCB_AREA_SIZE))
    {
	/* is the area mapped in the kernel's pagetable ? */
	//tcb_t * current = get_current_tcb();
	ptr_t pdir = get_current_pagetable();

	/* do we run on the kernel ptab (idler...)? */
	if (pdir != __pdir_root) 
	{
	    if (__pdir_root[fault >> PAGEDIR_BITS] != pdir[fault >> PAGEDIR_BITS])
	    {
		/* sync pagetables */
		pdir[fault >> PAGEDIR_BITS] = __pdir_root[fault >> PAGEDIR_BITS];
		return;
	    }
	}
	/* tcb map request */
	if (PF_WRITE(cause)) {
	    ptr_t page = kmem_alloc(PAGE_SIZE);
	    printf("map new tcb: %x\n", page);
	    mips_map_kpage(fault, ((dword_t)virt_to_phys(page)) | PT_TCB);
	    return;
	}
	else
	{
	    printf("map zero page");
	    enter_kdebug();
	}
    }
    else 
    {
#if 0
	if (fault >= MEM_COPYAREA1 && fault < MEM_COPYAREA_END) 
	{
	    current = get_current_tcb();
	    tcb_t * partner = tid_to_tcb(current->partner);
	    ptr_t pdir_from = phys_to_virt(current->page_dir);
	    ptr_t pdir_to = phys_to_virt(partner->page_dir);

	    dword_t *saved_stack;

	    //printf("fault in copy area %x @ %x\n", current, fault);

	    if (fault < MEM_COPYAREA2)
	    {
		if (current->resources & TR_IPC_MEM1)
		{
		    /* pagefault */
		    //printf("tunneled pf ipc1\n");
		    //enter_kdebug();

		    saved_stack = partner->stack;
		    notify_thread_2(partner, copy_pagefault, 
				    current->copy_area1 + (fault - MEM_COPYAREA1), 
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
		//printf("fault in copy area %x @ %x\n", current, fault);

		if (current->resources & TR_IPC_MEM2)
		{
		    /* pagefault */
		    //printf("tunneled pf ipc2\n");
		    //enter_kdebug();

		    saved_stack = partner->stack;
		    notify_thread_2(partner, copy_pagefault, 
				    current->copy_area2 + (fault - MEM_COPYAREA2), 
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
#endif
	panic("kernel pagefault");
    }
    printf("%x", fault);
    spin(79);
}






