/*********************************************************************
 *                
 * Copyright (C) 2000, 2001, 2002,  Karlsruhe University
 *                
 * File path:     memory.c
 * Description:   Architecture independent multilevel page table
 *                access functions.
 *                
 * @LICENSE@
 *                
 * $Id: memory.c,v 1.20 2002/06/07 17:03:41 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <mapping.h>
#include <tracepoints.h>
#if defined(CONFIG_IO_FLEXPAGES)
#include INC_ARCH(io_mapping.h)
#endif

#if defined(CONFIG_ENABLE_SMALL_AS)
#define USER_LIMIT	SMALL_SPACE_START
#else
#define USER_LIMIT	TCB_AREA
#endif


extern int __kdebug_pf_tracing;


/*
 * Function sys_fpage_unmap (fpage, mapmask)
 *
 *    The system call interface for fpage_unmap().
 *
 */
void sys_fpage_unmap(fpage_t fpage, dword_t mapmask)
{
    TRACEPOINT_2PAR(SYS_FPAGE_UNMAP, fpage.raw, mapmask);
#if defined(CONFIG_DEBUG_TRACE_SYSCALLS)
    printf("%s(%x, %x) - %p\n", __FUNCTION__, fpage.raw, mapmask,
	   get_current_tcb());
#endif
#if defined(CONFIG_IO_FLEXPAGES)
    
    /* Before doing anything related to main memory, check, if send fpage is
     * actually a IO-Flexpage
     */
    if (EXPECT_FALSE(IOFP_LO_BOUND <= fpage.raw && fpage.raw <= IOFP_HI_BOUND)){
	unmap_io_fpage(get_current_tcb(), fpage, mapmask);
	return;
    }	

#endif   
    fpage_unmap(get_current_tcb()->space, fpage, mapmask);
}



/*
 * Helper functions.
 */

static inline dword_t base_mask(fpage_t fp, dword_t num)
{
    return ((~0U) >> (32 - fp.fpage.size)) & ~((~0U) >> (32 - num));
}

static inline dword_t address(fpage_t fp, dword_t num)
{
    return fp.raw & ~((~0U) >> (32 - num));
}

static inline int is_pgsize_valid (dword_t pgsize)
{
    return (1 << hw_pgshifts[pgsize]) & HW_VALID_PGSIZES;
}

/*
 * the mapping database is made SMP save by a code lock
 */
DEFINE_SPINLOCK(mdb_spinlock);
#define SPIN_LOCK_MDB	spin_lock(&mdb_spinlock);
#define SPIN_UNLOCK_MDB spin_unlock(&mdb_spinlock);


/*
 * Function map_fpage (from, to, base, snd_fp, rcv_fp)
 *
 *    Map/grant a flexpage from one thread to another.
 *
 */
int map_fpage(tcb_t *from, tcb_t *to, dword_t base,
	      fpage_t snd_fp, fpage_t rcv_fp)
{
    dword_t offset, f_addr, t_addr, f_num, t_num;
    dword_t pgsize, f_size, t_size;
    pgent_t *fpg, *tpg;
    mnode_t *newmap, *map;

    pgent_t *r_fpg[HW_NUM_PGSIZES-1], *r_tpg[HW_NUM_PGSIZES-1];
    dword_t r_fnum[HW_NUM_PGSIZES-1], r_tnum[HW_NUM_PGSIZES-1];
#if defined(CONFIG_SMP)
    int tlb_shootdown = 0;
#endif

#if defined(CONFIG_IO_FLEXPAGES)
    
    /* 
     * Before doing anything related to main memory, check, if send fpage is
     * actually a IO-Flexpage
     */
    if (EXPECT_FALSE(IOFP_LO_BOUND <= snd_fp.raw && snd_fp.raw <= IOFP_HI_BOUND)){
	return map_io_fpage(from, to, snd_fp);
    }	

#endif    
#if defined(CONFIG_DEBUG_TRACE_MDB)
    if ( __kdebug_mdb_tracing )
    {
	printf("map_fpage(sender=%p, receiver=%p, base=%x, "
	       "sndfp=%x, rcvfp=%x)\n", from, to, base,
	       snd_fp.raw, rcv_fp.raw);
    }
#endif

    /*
     * Calculate the actual send and receive address to use.  This is
     * almost magic.  Please THINK before you consider changing this.
     */
    if ( snd_fp.fpage.size <= rcv_fp.fpage.size )
    {
	f_num = snd_fp.fpage.size;
	base &= base_mask(rcv_fp, f_num);
	f_addr = address(snd_fp, f_num);
	t_addr = (address(rcv_fp, f_num) & ~base_mask(rcv_fp, f_num)) + base;
    }
    else
    {
	enter_kdebug("map_fpage(): Larger send window");
	return 0;
    }

    if ( f_num < hw_pgshifts[0] )
    {
	enter_kdebug("map_fpage(): Invalid fpage size");
	return 0;
    }

    /*
     * Setting bit 0 in the from address indicates to mdb_map() that
     * we are doing a grant operation.
     */
    if ( snd_fp.fpage.grant )
	f_addr |= 0x1;


    /*
     * Find pagesize to use, and number of pages to map.
     */
    for (pgsize = HW_NUM_PGSIZES-1;
	 (hw_pgshifts[pgsize] > f_num) || (!is_pgsize_valid (pgsize));
	 pgsize--) {}
    f_num = t_num = 1 << (f_num - hw_pgshifts[pgsize]);
    f_size = t_size = HW_NUM_PGSIZES-1;

    fpg = from->space->pgent (table_index (f_addr, f_size));
    tpg = to->space->pgent (table_index (t_addr, t_size));

    SPIN_LOCK_MDB;

    while ( f_num > 0 || t_num > 0 )
    {
	if ( (f_addr >= USER_LIMIT && from != sigma0) || t_addr >= USER_LIMIT )
	{
	  /* Do not mess with the kernel area. */
	  break;
	}

	if ( from == sigma0 )
	{
	    /*
	     * When mapping from sigma0 we bypass the page table lookup.
	     */
	    f_size = pgsize;
	}
	else
	{
	    if ( !fpg->is_valid(from->space, f_size) )
	    {
		while ( t_size < f_size )
		{
		    /* Recurse up. */
		    tpg = r_tpg[t_size];
		    t_num = r_tnum[t_size];
		    t_size++;
		}

		if ( t_size == f_size )
		    goto Next_receiver_entry;

		/* t_size > f_size */
		goto Next_sender_entry;
	    }

	    if ( (f_size > pgsize) && fpg->is_subtree(from->space, f_size) )
	    {
		/*
		 * We are currently working on to large page sizes.
		 */
		f_size--;
		fpg = fpg->subtree (from->space, f_size+1)->next
		    (from->space, table_index(f_addr, f_size), f_size);
		continue;
	    }
	    else if ( fpg->is_subtree(from->space, f_size) )
	    {
		/*
		 * The mappings in the senders address space are to small.
		 * We have to map each single entry in the subtree.
		 */
		f_size--;
		r_fpg[f_size] = fpg->next(from->space, 1, f_size+1);
		r_fnum[f_size] = f_num - 1;

		fpg = fpg->subtree(from->space, f_size+1);
		f_num = table_size(f_size);
		continue;
	    }
	    else if ( f_size > pgsize )
	    {
		f_num = 1;
	    }
	}

	/*
	 * If we get here `fpg' is a valid mapping in the senders
	 * address space.
	 */


	if ( (t_size > f_size) || ((t_size > pgsize) && (f_size > pgsize)) )
	{
	    /*
	     * We are currently working on larger receive pages than
	     * send pages.
	     */
	    t_size--;
	    r_tpg[t_size] = tpg->next(to->space, 1, t_size+1);
	    r_tnum[t_size] = t_num - 1;

	    if ( !tpg->is_valid(to->space, t_size+1) )
	    {
		/*
		 * Subtree does not exist.  Create one.
		 */
		tpg->make_subtree(to->space, t_size+1);
	    }
	    else if ( !tpg->is_subtree(to->space, t_size+1) )
	    {
		/*
		 * There alredy exists a larger mapping.  Just
		 * continue.  BEWARE: This may cause extension of
		 * access rights to be refused even though they are
		 * perfectly legal.  I.e. if all the mappings in the
		 * subtree of the sender's address space are valid.
		 */
		printf("map_fpage(): Larger mapping already exists.\n");
		enter_kdebug();
		goto Next_receiver_entry;
	    }

	    if ( t_size >= pgsize )
	    {
		tpg = tpg->subtree (to->space, t_size+1)->next
		    (to->space, table_index(t_addr, t_size), t_size);
		continue;
	    }
	    else
	    {
		tpg = tpg->subtree(to->space, t_size+1);
		t_num = table_size(t_size);
	    }

	    /* Adjust destination according to where source is located. */
	    tpg = tpg->next(to->space, table_index(f_addr, t_size), t_size);
	    t_num -= table_index(f_addr, t_size);
	    t_addr += table_index(f_addr, t_size) << hw_pgshifts[t_size];
	    continue;
	}
	else if ( tpg->is_valid(to->space, t_size) &&
		  tpg->is_subtree(to->space, t_size) )
	{
	    /*
	     * Target mappings are of smaller page size.  We have to
	     * map each single entry in the subtree.
	     */
	    t_size--;
	    r_tpg[t_size] = tpg->next(to->space, 1, t_size+1);
	    r_tnum[t_size] = t_num - 1;

	    tpg = tpg->subtree(to->space, t_size+1);
	    t_num = table_size(t_size);
	}
	else if ( !is_pgsize_valid (t_size) )
	{
	    /*
	     * Pagesize is ok but is not a valid hardware pagesize.
	     * Need to create mappings of smaller size.
	     */
	    t_size--;
	    r_tpg[t_size] = tpg->next(to->space, 1, t_size+1);
	    r_tnum[t_size] = t_num - 1;
	    tpg->make_subtree(to->space, t_size+1);

	    tpg = tpg->subtree(to->space, t_size+1);
	    t_num = table_size(t_size);
	    continue;
	}


	/*
	 * If we get here `tpg' will be the page table entry that we
	 * are going to change.
	 */

	offset = f_addr & page_mask(f_size) & ~page_mask(t_size);

	if ( tpg->is_valid(to->space, t_size) )
	{
	    /*
	     * If a mapping already exists, it might be that we are
	     * just extending the current access rights.
	     */
	    if ( (from == sigma0) ? (tpg->address(to->space, t_size) != f_addr) :
		 (tpg->address(to->space, t_size) !=
		  fpg->address(from->space, f_size) + offset) )
	    {
		printf("map_fpage(sender=%p, receiver=%p, base=%x, "
		       "sndfp=%x, rcvfp=%x)\n", from, to, base,
		       snd_fp.raw, rcv_fp.raw);
		enter_kdebug("map_fpage(): Mapping already exists.");
		goto Next_receiver_entry;
	    }

	    /* Extent access rights. */
	    if ( (fpg->is_writable(from->space, t_size) || from == sigma0) &&
		 snd_fp.fpage.write )
	    {
		tpg->set_writable(to->space, t_size);
#if defined(CONFIG_ENABLE_SMALL_AS)	    
		if (to->space->is_small ())
		    flush_tlbent ((ptr_t) (t_addr +
					   to->space->smallid ().offset ()));
		else if (from->space->is_small ())
		    flush_tlbent((ptr_t) t_addr);
#endif	
#if defined(CONFIG_SMP)
		tlb_shootdown = 1;
#endif
	    }
	}
	else
	{
	    /*
	     * This is where the real work is done.
	     */

	    if  ( from == sigma0 )
	    {
		/*
		 * If mapping from sigma0, fpg will not be a valid
		 * page table entry.
		 */
#if defined(CONFIG_DEBUG_TRACE_MDB)
		if ( __kdebug_mdb_tracing )
		{
		    printf("paddr=%p mdb_map(from=sigma0,4G,%p, "
			   "to=%p,%dk,%p)\n",
			   f_addr, f_addr, (dword_t) to->space,
			   1 << (hw_pgshifts[t_size] - 10), t_addr);
		    if ( __kdebug_mdb_tracing > 1 )
			enter_kdebug("mdb_map");
		}
#endif

		newmap = mdb_map(sigma0_mapnode, fpg, HW_NUM_PGSIZES,
				 f_addr + offset, tpg, t_size,
				 to->space);

		tpg->set_entry(to->space, f_addr, snd_fp.fpage.write, t_size);
		tpg->set_mapnode(to->space, t_size, newmap, t_addr);

#if defined(CONFIG_CACHEABILITY_BITS)
		if (snd_fp.fpage.uncacheable)
		    tpg->set_uncacheable(to->space, t_size);
		if (snd_fp.fpage.unbufferable)
		    tpg->set_unbufferable(to->space, t_size);
#endif
	    }
	    else
	    {
#if defined(CONFIG_DEBUG_TRACE_MDB)
		if ( __kdebug_mdb_tracing )
		{
		    printf("mdb_map(paddr=%p, from=%p,%dK,%p, to=%p,%dk,%p)\n",
			   fpg->address(from->space, f_size) + offset,
			   (dword_t) from->space,
			   1 << (hw_pgshifts[f_size] - 10),
			   f_addr, to->space,
			   1 << (hw_pgshifts[t_size] - 10), t_addr);
		    if ( __kdebug_mdb_tracing > 1 )
			enter_kdebug("mdb_map");
		}
#endif

		map = fpg->mapnode(from->space, f_size,
				   f_addr & ~page_mask(f_size));

		// Make sure that we are not flushed.
		while ( ! map->up_trylock() )
		    if ( ! fpg->is_valid(from->space, f_size) )
			goto Next_receiver_entry;
		    else
			enter_kdebug("map_fpage: waiting for uplock");

		newmap = mdb_map(map, fpg, f_size, f_addr, tpg, t_size,
				 to->space);

		tpg->set_entry(to->space,
			       fpg->address(from->space, f_size) + offset,
			       fpg->is_writable(from->space, f_size) &&
			       snd_fp.fpage.write,
			       t_size);
		tpg->set_mapnode(to->space, t_size, newmap, t_addr);

#if defined(CONFIG_CACHEABILITY_BITS)
		if (snd_fp.fpage.uncacheable)
		    tpg->set_uncacheable(to->space, t_size);
		if (snd_fp.fpage.unbufferable)
		    tpg->set_unbufferable(to->space, t_size);
#endif

		if ( f_addr & 0x1 )
		{
		    /* Grant operation.  Remove mapping from current space. */
		    fpg->clear(from->space, f_size);
#if defined(CONFIG_ENABLE_SMALL_AS)	    
		    if (from->space->is_small ())
			flush_tlbent ((ptr_t) (f_addr + from->space->smallid
					       ().offset ()));
		    else
#endif
			flush_tlbent((ptr_t) f_addr);
		}
		newmap->up_unlock();
		map->up_unlock();
	    }
	}

    Next_receiver_entry:

	t_addr += page_size(t_size);
	t_num--;

	if ( t_num > 0 )
	{
	    /* Go to next entry */
	    tpg = tpg->next(to->space, 1, t_size);
	    if ( t_size < f_size )
		continue;
	}
	else if ( t_size < f_size && f_size < pgsize )
	{
	    /* Recurse up */
	    tpg = r_tpg[t_size];
	    t_num = r_tnum[t_size];
	    t_size++;
	    continue;
	}
	else if ( t_size > f_size )
	{
	    /* Skip to next fpg entry.  Happens if tpg is already mapped. */
	    f_addr += page_size(t_size) - page_size(f_size);
	    f_num = 1;
	}

    Next_sender_entry:

	f_addr += page_size(f_size);
	f_num--;

	if ( f_num > 0 )
	{
	    /* Go to next entry */
	    if ( from != sigma0 )
		fpg = fpg->next(from->space, 1, f_size);
	    continue;
	}
	else if ( f_size < pgsize )
	{
	    /* Recurse up */
	    fpg = r_fpg[f_size];
	    f_num = r_fnum[f_size];
	    f_size++;
	}
	else
	{
	    /* Finished */
	    t_num = 0;
	}
    }
    SPIN_UNLOCK_MDB;
#if defined (CONFIG_SMP)
    if (tlb_shootdown)
	smp_flush_tlb();
#endif
    return 1;
}


/*
 * Function fpage_unmap (space, fpage, mapmask)
 *
 *    Unmap FPAGE from SPACE.  MAPMASK indicates whether to flush or
 *    remap, and if current pgdir should also be flushed.
 *
 */
void fpage_unmap(space_t *space, fpage_t fpage, dword_t mapmask)
{
    dword_t vaddr, num, size, pgsize;
    pgent_t *pg;
    mnode_t *map;
    int r;

    pgent_t *r_pg[HW_NUM_PGSIZES-1];
    dword_t r_num[HW_NUM_PGSIZES-1];
#if defined(CONFIG_SMP)
    int tlb_shootdown = 0;
#endif

#if defined(CONFIG_DEBUG_TRACE_MDB)
    if ( __kdebug_mdb_tracing )
    {
	printf("fpage_unmap(%x, %x, %x)\n", space, fpage.raw, mapmask);
    }
#endif

    num = fpage.fpage.size;
    vaddr = address(fpage, num);

    if ( num < hw_pgshifts[0] )
    {
	printf("map_fpage(): Invalid fpage size (%d).\n", num);
	enter_kdebug();
	return;
    }

    /*
     * Find pagesize to use, and number of pages to map.
     */
    for (pgsize = HW_NUM_PGSIZES-1;
	 (hw_pgshifts[pgsize] > num) || (!is_pgsize_valid (pgsize));
	 pgsize-- ) {}
    num = 1 << (num - hw_pgshifts[pgsize]);

    size = HW_NUM_PGSIZES-1;
    pg = space->pgent (table_index (vaddr, size));

    SPIN_LOCK_MDB;

    while ( num )
    {
	if ( vaddr >= USER_LIMIT )
	    /* Do not mess with the kernel area. */
	    break;

	if ( size > pgsize )
	{
	    /* We are operating on to large page sizes. */
	    if ( ! pg->is_valid(space, size) )
		break;
	    else if ( pg->is_subtree(space, size) )
	    {
		size--;
		pg = pg->subtree(space, size+1)->next
		    (space, table_index(vaddr, size), size);
		continue;
	    }
	    else if ( mapmask & MDB_ALL_SPACES )
	    {
		printf("fpage_unmap(%x, %x, %x)\n", space, fpage.raw, mapmask);
		enter_kdebug("fpage_unmap: page is to large");
		break;
	    }
	}

	if ( ! pg->is_valid(space, size) )
	    goto Next_entry;

	if ( pg->is_subtree(space, size) )
	{
	    /* We have to flush each single page in the subtree. */
	    size--;
	    r_pg[size] = pg;
	    r_num[size] = num - 1;

	    pg = pg->subtree(space, size+1);
	    num = table_size(size);
	    continue;
	}

#if defined(CONFIG_DEBUG_TRACE_MDB)
	if ( __kdebug_mdb_tracing )
	{
	    printf("mdb_flush(space=%p, vaddr=%p, %dK:%p, %dk:%p, mode=%p)\n",
		   space, vaddr,
		   1 << (hw_pgshifts[size] - 10),
		   pg->address(space, size),
		   1 << (hw_pgshifts[pgsize] - 10),
		   pg->address(space, size) + (vaddr & page_mask(size)),
		   mapmask);
	    if ( __kdebug_mdb_tracing > 1)
		enter_kdebug("mdb_flush");
	}
#endif
	map = pg->mapnode(space, size, vaddr & ~page_mask(size));

	// Make sure that we are not flushed.
	while ( ! map->up_trylock() )
	    if ( ! pg->is_valid(space, size) )
		goto Next_entry;
	    else
		enter_kdebug("fpage_unmap: waiting for uplock");

	r = mdb_flush(pg->mapnode(space, size, vaddr & ~page_mask(size)),
		      pg, size, vaddr, pgsize, mapmask);
	if ( r == -1 )
	    enter_kdebug("fpage_unmap: page already flushed");
#if defined(CONFIG_SMP)
	else
	    tlb_shootdown |= r;
#endif
    Next_entry:

	pg = pg->next(space, 1, size);
	vaddr += page_size(size);
	num--;

	if ( num == 0 && size < pgsize )
	{
	    /* Recurse up */
	    pg = r_pg[size];
	    num = r_num[size];
	    size++;

	    if ( (mapmask & MDB_ALL_SPACES) && (mapmask & MDB_FLUSH) )
		pg->remove_subtree(space, size);
	    pg = pg->next(space, 1, size);	
	}
    }
    SPIN_UNLOCK_MDB;
#if defined(CONFIG_SMP)
    smp_flush_tlb();
#endif    
}











