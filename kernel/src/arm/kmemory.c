/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/kmemory.c
 * Description:   ARM specific kernel memory allocator
 *                
 * @LICENSE@
 *                
 * $Id: kmemory.c,v 1.16 2001/11/22 13:18:48 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

/* Just in case someone wants to use the generic allocator. */
#ifdef CONFIG_HAVE_ARCH_KMEM

#include <init.h>
#include INC_PLATFORM(platform.h)
#include INC_ARCH(farcalls.h)


/*
 * From linker scripts.
 */
extern dword_t kmem_start[];
extern dword_t kmem_end[];
extern dword_t cpaddr[];


/*
 * The layout of the first words in free pages.
 */
typedef struct pglink_t pglink_t;
struct pglink_t {
    pglink_t	*next;		/* This MUST be the first field */
    pglink_t	*prev;
    pglink_t	*next_cluster;
};


/*
 * Keep one freelists for each of the page sizes.
 */
static pglink_t	*freelist_16k;
static dword_t	numfree_16k;

static pglink_t	*freelist_4k;
static pglink_t	*cluster_4k;
static dword_t	numfree_4k;

static pglink_t	*freelist_1k;
static pglink_t	*cluster_1k;
static dword_t	numfree_1k;



/*
 * Bitarray indicating which pages are allocated.  Granularity is 1KB.
 */
static byte_t alloc_array[PHYS_MEMSIZE/1024/8];



/*
 * Uncomment to turn on debugging output.
 */
//#define DEBUG_KMEM 1

#ifdef DEBUG_KMEM
# define DBG_PRINTF(fmt, args...) printf(fmt , ## args)
# define PRINT_STATUS() 						\
({									\
    DBG_PRINTF("Kmem status: 16k(%d,%p)", numfree_16k, freelist_16k);	\
    DBG_PRINTF(" 4k(%d,%p,%p)", numfree_4k, freelist_4k, cluster_4k);	\
    DBG_PRINTF(" 1k(%d,%p,%p)\n", numfree_1k, freelist_1k, cluster_1k);	\
})
#else
# define DBG_PRINTF(fmt, args...)
# define PRINT_STATUS()
#endif


/*
 * Uncomment to turn on tracing output.
 */
//#define TRACE_KMEM 1

#ifdef TRACE_KMEM
# define TRACE_ALLOC()							\
({									\
    dword_t lr;								\
    __asm__ ("mov %0,lr\n":"=r"(lr));					\
    printf("0x%x: %s() -> 0x%x\n", lr, __FUNCTION__, pg);		\
})
# define TRACE_FREE()							\
({									\
    dword_t lr;								\
    __asm__ ("mov %0,lr\n":"=r"(lr));					\
    printf("0x%x: %s(0x%x)\n", lr, __FUNCTION__, addr);			\
})
#else
# define TRACE_ALLOC()
# define TRACE_FREE()
#endif



/*
 *              KERNEL MEMORY ALLOCATOR DESIGN
 *
 * BASICS
 *
 * The kernel memory allocator works as follows: Have one freelist
 * each for page sizes of 16k, 4k, and 1k.  The freelists are doubly
 * linked, with the next and prev pointers within freelists residing
 * in the beginning of the page memory (see pglink_t structure).  When
 * allocating a page of a given size, the head of the corresponding
 * freelist is popped off.  This is all peachy keen.  However, if
 * there are no pages available in the freelist, things get a little
 * bit messier.  For allocating 1k and 4k pages, there is a
 * possibility of splitting up a 4k page, or a 16k page respectively.
 * This will make more and more smaller pages though, eventually
 * leaving only 1k pages left.
 *
 * DEALING WITH SPLIT PAGES
 *
 * We must be able to cluster several small pages to make a larger
 * page.  So, in addition to the freelist pointers, we also keep
 * cluster pointers for the 1k and 4k freelists.  These pointers will
 * always point to a position in the freelist where we can find 4
 * consecutive pages (ie. four 1K pages making up a 4K page, or four
 * 4K pages making up a 16K page).  When we need to concatenete
 * smaller pages to make a larger one, we may simply retrieve 4 pages
 * from where the cluster pointer is pointing.
 *
 * KEEPING CLUSTER POINTERS VALID
 *
 * Ensuring that cluster pointers are valid is the responsibility of
 * the free functions (we assume that freeing pages is less time
 * critical than allocating pages).  This is accomplished in the
 * following manner: The page which is freed will fall into one of
 * four locations in a larger page.  Eg. a 4K page may be located in
 * the beginning of a 16K page (page 0 in the figure below), it might
 * be located in the middle (page 1 or 2), or in the end (page 3).
 *
 *        +-----+-----+-----+-----+
 *        |  0  |  1  |  2  |  3  |      (four 4K pages)
 *        +-----+-----+-----+-----+
 *
 *        +-----------------------+
 *        |                       |      (one 16K page)
 *        +-----------------------+
 *
 * If it is the case that the page was in the beginning (ie. page 0),
 * we inspect the memory location for the next page (page 1).  If this
 * page is NOT allocated, we insert the freed page in front of this
 * page, and also checks to see if the next pointer in this page
 * points to page 2.  If this is the case, we check the next pointer
 * of page 2 to see if it points to page 3.  If all test turn out
 * positive, we have discovered that we have 4 consecutive pages.
 * These 4 pages are referred to as a cluster, which is inserted into
 * the end of the freelist (see below).  If some of the tests above
 * did not turn out positive, we have at least started to build a
 * cluster (by inserting the freed page 0 in front of page 1).  Of
 * course, the example above is just valid when we a freeing page 0.
 * If the freed page was not page 0, the steps taken above would be
 * somewhat different (the basic concept would remain the same).
 *
 * In order to prevent clustered pages to be split up unnecessarily,
 * we move clustered pages to the end of the freelist rather than
 * keeping them in arbitrary positions.  A cluster pointer is
 * associated with both the 4K and 1K freelists, pointing to the first
 * page belonging to a cluster.  The invariant is that all pages
 * following the cluster pointer will belong to a cluster, and the 4
 * pages within each cluster will be ordered.
 *
 * Using cluster pointers, moving four 1K or four 4K pages to the 4K
 * or 16K freelists should prove efficient.  The policy for when to
 * move pages between freelists need to be investigated though.
 * Currently, the alloc functions tries to move pages in between
 * freelists if they find that there are no available pages in their
 * respective freelist.
 *
 */


/*
 * The allocator is a bit large (codewise).  The first 230 lines of
 * code however (700 bytes or so), is just for the initalization.  It
 * will as such be removed from the kernel image once the
 * initialization has completed.
 */


/*
 * Function is not needed after initialization.
 */
void kmem_init(void) L4_SECT_INIT;


/*
 * Function kmem_init ()
 *
 *    Initialize freelists for 16K, 4K, and 1K pages.
 *
 */
void kmem_init(void)
{
    pglink_t *pg, *pgn, *pgp;
    dword_t *p, num16, num4, num1, i, j;
    dword_t start16k, end16k, addr;

    /*
     * Calculate where to start and stop allocating 16K pages.
     */
    start16k = (dword_t) kmem_start - ((dword_t) kmem_start & KMEM_16K_MASK);
    	+ (((dword_t) kmem_start & KMEM_16K_MASK) ? KMEM_16K_MASK+1 : 0);
    end16k = (dword_t) kmem_end - ((dword_t) kmem_end & KMEM_16K_MASK);

    /*
     * First, put all kmem into 16K pages.
     */
    pgp = (pglink_t *) &freelist_16k;
    freelist_16k = (pglink_t *) start16k;

    for (pg = (pglink_t *) start16k, num16 = 0;
	 pg < (pglink_t *) end16k;
	 pg = pgn)
    {
	pgn = (pglink_t *) ((dword_t) pg + 16384);

	/* Zero out pages */
	for (p = (dword_t *) pg; p < (dword_t *) pgn; p++)
	    *p = 0;

	pg->next = pgn;
	pg->prev = pgp;
	pgp = pg;

	/* Count pages */
	num16++;
    }
    pgp->next = (pglink_t *) NULL;


    /*
     * Calculate number of pages to put into other freelists.
     */
    num4 = num16 * KMEM_INIT_4K_RATIO;
    num1 = num16 * KMEM_INIT_1K_RATIO;
    num16 -= (num4 + num1);

    /*
     * Move pages from 16K freelist to 4K freelist.
     */
    if ( num4 ) {
	pgp = (pglink_t *) &freelist_4k;
	freelist_4k = freelist_16k;

	for (i = 0, pg = (pglink_t *) freelist_4k;
	     i < num4;
	     i++)
	{
	    /* Check for clustering */
	    if ( i+1 < num4 )
		pg->next_cluster = (pglink_t *) ((dword_t) pg + 16384);

	    for (j = 0; j < 4; j++, pg = pgn) {
		pgn = (pglink_t *) ((dword_t) pg + 4096);

		pg->next = pgn;
		pg->prev = pgp;
		pgp = pg;
	    }
	}
	num4 *= 4;
	pgp->next = (pglink_t *) NULL;
	cluster_4k = freelist_4k;

	/* Cleanup 16K freelist */
	if ( (dword_t) pg < end16k ) {
	    freelist_16k = pg;
	    pg->prev = (pglink_t *) &freelist_16k;
	} else {
	    freelist_16k = NULL;
	}
    } else {
	freelist_4k = NULL;
	numfree_4k = 0;
    }


    /*
     * Move pages from 16K freelist to 1K freelist.
     */
    if ( num1 ) {
	pgp = (pglink_t *) &freelist_1k;
	freelist_1k = freelist_16k;

	for (i = 0, pg = (pglink_t *) freelist_1k;
	     i < num1;
	     i++)
	{
	    for (j = 0; j < 16; j++, pg = pgn) {
		pgn = (pglink_t *) ((dword_t) pg + 1024);

		/* Check for clustering */
		if ( !(j%4) && (j < 12 || i+1  < num1) )
		    pg->next_cluster = (pglink_t *) ((dword_t) pg + 4096);

		pg->next = pgn;
		pg->prev = pgp;
		pgp = pg;
	    }
	}
	num1 *= 16;
	pgp->next = (pglink_t *) NULL;
	cluster_1k = freelist_1k;

	/* Cleanup 16K freelist */
	if ( (dword_t) pg < end16k ) {
	    freelist_16k = pg;
	    pg->prev = (pglink_t *) &freelist_16k;
	} else {
	    freelist_16k = NULL;
	}
    } else {
	freelist_1k = NULL;
	numfree_1k = 0;
    }


    /*
     * If kmem_start was not aligned on a 16K boundary, put all memory
     * from kmem_start to the first 16K boundary into the 1K freelist
     * (ie. max 15 pages).
     */
    if ( (dword_t) kmem_start < start16k ) {
	pgp = (pglink_t *) &freelist_1k;

	for (pg = (pglink_t *) kmem_start;
	     pg < (pglink_t *) start16k;
	     pg = pgn)
	{
	    pgn = (pglink_t *) ((dword_t) pg + 1024);

	    /* Zero out page */
	    for (p = (dword_t *) pg; p < (dword_t *) pgn; p++)
		*p = 0;

	    pg->next = pgn;
	    pg->prev = pgp;
	    pgp = pg;

	    num1++;
	}

	/* Insert into start of freelist (before clusters) */
	pgp->next = (pglink_t *) freelist_1k;
	if ( pgp->next )
	    pgp->next->prev = pgp;
	pgp->prev = (pglink_t *) &freelist_1k;
	freelist_1k = (pglink_t *) kmem_start;
    }


    /*
     * If kmem_end was not aligned on a 16K boundary, put all memory
     * from the previous 16K boundary upto kmem_end into the 1K
     * freelist (ie. max 15 pages).
     */
    if ( end16k < (dword_t) kmem_end ) {
	pgp = (pglink_t *) &freelist_1k;

	for (pg = (pglink_t *) end16k;
	     pg < (pglink_t *) kmem_end;
	     pg = pgn)
	{
	    pgn = (pglink_t *) ((dword_t) pg + 1024);

	    /* Zero out page */
	    for (p = (dword_t *) pg; p < (dword_t *) pgn; p++)
		*p = 0;

	    pg->next = pgn;
	    pg->prev = pgp;
	    pgp = pg;

	    num1++;
	}

	/* Insert into start of freelist (before clusters) */
	pgp->next = (pglink_t *) freelist_1k;
	if ( pgp->next )
	    pgp->next->prev = pgp;
	pgp->prev = (pglink_t *) &freelist_1k;
	freelist_1k = (pglink_t *) end16k;
    }


    /*
     * Initialize accounting info
     */
    numfree_16k = num16;
    numfree_4k = num4;
    numfree_1k = num1;

    /*
     * Mark all memory as allocated.
     */
    for ( i = 0; i < sizeof(alloc_array); i++ )
    {
	alloc_array[i] = 0xff;
    }

    /*
     * Mark kernel memory as free.
     */
    addr = ((dword_t) kmem_start + KMEM_1K_MASK) & ~KMEM_1K_MASK;
    for ( i = (addr - ((dword_t) cpaddr & ~KMEM_1K_MASK)) >> KMEM_1K_SHIFT;
	  addr < ((dword_t) kmem_end & ~KMEM_1K_MASK);
	  addr += KMEM_1K_MASK+1, i++ )
    {
	alloc_array[i>>3] &= ~(0x80 >> (i&3));
    }

    printf("Freelists initialized: %d*16K,  %d*4K,  %d*1K\n",
	   num16, num4, num1);
}


/*
 * Function kmem_move_by_splitting (num, smaller_size,
 *	freelist_bigger, clusters_bigger, numfree_bigger,
 *	freelist_smaller, clusters_smaller, numfree_smaller)
 *
 *    Move `num' frames from framelist of bigger frames into framelist
 *    of smaller frames.  `smaller_size' is the size of the smaller
 *    frames.
 *
 */
static int kmem_move_by_splitting(dword_t num, dword_t smaller_size,
				  pglink_t **freelist_bigger,
				  pglink_t **clusters_bigger,
				  dword_t *numfree_bigger,
				  pglink_t **freelist_smaller,
				  pglink_t **clusters_smaller,
				  dword_t *numfree_smaller)
{
    pglink_t *pg, *op, *pp;
    int i, ret = 0;

    /*
     * Loop until we have move `num' new pages of smaller size.
     */
    while (num-- > 0)
    {
	pg = *freelist_bigger;
	if ( pg ) {
	    /* Update freelist for bigger frames */
	    *freelist_bigger = pg->next;
	    if ( pg->next )
		pg->next->prev = (pglink_t *) freelist_bigger;

	    /* Update clusterlists */
	    if ( clusters_bigger ) {
		if ( *clusters_bigger == pg )
		    *clusters_bigger = pg->next_cluster;
	    }
	    pg->next_cluster = *clusters_smaller;
	    *clusters_smaller = pg;

	    /* Update freelist for smaller frames */
	    op = *freelist_smaller;
	    pp = (pglink_t *) freelist_smaller;
	    for (i = 0; i < 4; i++, pg = pg->next) {
		pg->next = (pglink_t *) ((dword_t) pg + smaller_size);
		pg->prev = pp;
		pp->next = pg;
		pp = pg;
	    }
	    pp->next = op;
	    if ( op )
		op->prev = pp;

	    /* Do accounting */
	    *numfree_bigger -= 1;
	    *numfree_smaller += 4;
	    
	    ret++;

	} else {
	    /*
	     * No more free pages.  Return number of new pages.
	     */
	    return ret*4;
	}
    }

    return ret*4;
}

#define kmem_move_16k_to_4k(num) 					\
    kmem_move_by_splitting((num), 4096,					\
			   (pglink_t **) &freelist_16k,			\
			   (pglink_t **) NULL,				\
			   (dword_t *)   &numfree_16k,			\
			   (pglink_t **) &freelist_4k,			\
			   (pglink_t **) &cluster_4k,			\
			   (dword_t *)   &numfree_4k)

#define kmem_move_4k_to_1k(num) 					\
    kmem_move_by_splitting((num), 1024,					\
			   (pglink_t **) &freelist_4k,			\
			   (pglink_t **) &cluster_4k,			\
			   (dword_t *)   &numfree_4k,			\
			   (pglink_t **) &freelist_1k,			\
			   (pglink_t **) &cluster_1k,			\
			   (dword_t *)   &numfree_1k)



/*
 * Function kmem_move_by_concatenating (num,
 *	freelist_bigger, numfree_bigger,
 *	freelist_smaller, clusters_smaller, numfree_smaller)
 *
 *    Move frames from freelist of smaller frames into freelist of
 *    bigger frames until we have `num' new bigger frames.
 *
 */
static int kmem_move_by_concatenating(dword_t num,
				      pglink_t **freelist_bigger,
				      dword_t *numfree_bigger,
				      pglink_t **freelist_smaller,
				      pglink_t **clusters_smaller,
				      dword_t *numfree_smaller)
{
    pglink_t *pg, *pgn;
    int ret = 0;

    /*
     * Loop until we have moved `num' larger pages.
     */
    while (num-- > 0)
    {
	pg = *clusters_smaller;
	if ( pg ) {
	    /* Update smaller clusterlist */
	    *clusters_smaller = pg->next_cluster;

	    /* Update smaller freelist */
	    if ( pg == *freelist_smaller )
		*freelist_smaller = *clusters_smaller;
	    pgn = pg->next_cluster;
	    if ( pgn )
		pgn->prev = pg->prev;
	    pg->prev->next = pgn;

	    /* Update bigger freelist */
	    pg->next = *freelist_bigger;
	    pg->prev = (pglink_t *) freelist_bigger;
	    *freelist_bigger = pg;
	    if ( pg->next )
		pg->next->prev = pg;
	    pg->next_cluster = (pglink_t *) NULL;

	    /* Do accounting */
	    *numfree_smaller -= 4;
	    *numfree_bigger += 1;

	    ret++;

	} else {
	    /*
	     * No more clusters.  Return number of new 16k pages.
	     */
	    return ret;
	}
    }

    return ret;
}

#define kmem_move_4k_to_16k(num) 					\
    kmem_move_by_concatenating((num),					\
			       (pglink_t **) &freelist_16k,		\
			       (dword_t *)   &numfree_16k,		\
			       (pglink_t **) &freelist_4k,		\
			       (pglink_t **) &cluster_4k,		\
			       (dword_t *)   &numfree_4k)

#define kmem_move_1k_to_4k(num) 					\
    kmem_move_by_concatenating((num),					\
			       (pglink_t **) &freelist_4k,		\
			       (dword_t *)    &numfree_4k,		\
			       (pglink_t **) &freelist_1k,		\
			       (pglink_t **) &cluster_1k,		\
			       (dword_t *)    &numfree_1k)


/*
 * Function kmem_alloc_16k ()
 *
 *    Allocate a 16 page and return it.
 *
 */
ptr_t kmem_alloc_16k(void)
{
    pglink_t *pg, *np;
    dword_t cpsr;

    cpsr = DISABLE_INTERRUPTS();

    DBG_PRINTF("Allocated 16k\nE-");
    PRINT_STATUS();

    /*
     * Grab head of freelist.
     */
    pg = (pglink_t *) freelist_16k;
    if ( pg == NULL ) {
	/*
	 * Ooops.  No more free pages.  Try to grab some pages from
	 * the 4k freelist.
	 */
	int num = numfree_4k > 20 ? 2 : 1;

	if ( kmem_move_4k_to_16k(num) == 0 ) {
	    panic("AIEEE!  Unable to allocate 16k page.\n");
	}

	pg = (pglink_t *) freelist_16k;
    }
    np = pg->next;
    freelist_16k = np;
    if ( np != NULL )
	np->prev = (pglink_t *) &freelist_16k;

    /*
     * Accounting.
     */
    numfree_16k--;

    /*
     * Mark page as allocated.
     */
    ((uint16 *) alloc_array)
	[ ((dword_t) pg - (dword_t)cpaddr) >> KMEM_16K_SHIFT ] = 0xffff;

    ENABLE_INTERRUPTS(cpsr);

    DBG_PRINTF("L-");
    PRINT_STATUS();

    TRACE_ALLOC();
    return (ptr_t) pg;
}


/*
 * Function kmem_free_16k (addr)
 *
 *    Free the 16k page given in `addr'.
 *
 */
void kmem_free_16k(dword_t addr)
{
    pglink_t *pg;
    dword_t cpsr;

    cpsr = DISABLE_INTERRUPTS();

    TRACE_FREE();
    DBG_PRINTF("Freed 16k\n");

    /*
     * Get addresses of the 16k frame.
     */
    pg = (pglink_t *) (addr & KMEM_16K_MASK);

    /*
     * Mark page as not allocated.
     */
    ((uint16 *) alloc_array)
	[ ((dword_t) pg - (dword_t) cpaddr) >> KMEM_16K_SHIFT ] = 0x0000;

    /*
     * Insert into head of freelist.
     */
    pg->next = (pglink_t *) freelist_16k;
    pg->prev = (pglink_t *) &freelist_16k;
    if ( pg->next )
	pg->next->prev = pg;
    freelist_16k = pg;

    /*
     * Accounting.
     */
    numfree_4k++;

    ENABLE_INTERRUPTS(cpsr);
}


/*
 * Function kmem_alloc_4k ()
 *
 *    Allocate a 4k page and return it.
 *
 */
ptr_t kmem_alloc_4k(void)
{
    pglink_t *pg, *np;
    dword_t cpsr;

    cpsr = DISABLE_INTERRUPTS();

    DBG_PRINTF("Allocated 4k\nE-");
    PRINT_STATUS();

    /*
     * Grab head of freelist.
     */
    pg = (pglink_t *) freelist_4k;
    if ( pg == NULL ) {
	/*
	 * Ooops.  No more free pages.  Try to grab some pages from
	 * the 16k or 1k freelists.
	 */
	int ok = 0;

	/*
	 * We prefer to split a 16k page (if the number of 1k pages is
	 * not extremely high, that is).
	 */
	if ( numfree_16k > 20*numfree_1k ) {
	    int num = numfree_16k > 5 ? 2 : 1;
	    ok = kmem_move_16k_to_4k(num);
	}

	/*
	 * Did not split a 16k page.  Grab some from the 1k freelist.
	 */
	if ( !ok ) {
	    int num = numfree_1k > 12 ? 2 : 1;
	    ok =  kmem_move_1k_to_4k(num);
	}

	/*
	 * As a last resort, we try to split a single 16k page.
	 */
	if ( !ok ) {
	    ok = kmem_move_16k_to_4k(1);
	}

	if ( !ok ) {
	    panic("AIEEE!  Unable to allocate 4k page.\n");
	}

	pg = (pglink_t *) freelist_4k;
    }
    np = pg->next;
    freelist_4k = np;
    if ( np != NULL )
	np->prev = (pglink_t *) &freelist_4k;

    /*
     * Check if we have split a cluster.
     */
    if ( cluster_4k == pg )
	cluster_4k = pg->next_cluster;

    /*
     * Accounting.
     */
    numfree_4k--;

    /*
     * Mark page as allocated.
     */
    alloc_array[ ((dword_t) pg - (dword_t) cpaddr) >> (KMEM_16K_SHIFT-1) ] |=
	(dword_t) pg & (1 << KMEM_4K_SHIFT) ? 0xf0 : 0x0f;

    ENABLE_INTERRUPTS(cpsr);

    DBG_PRINTF("L-");
    PRINT_STATUS();
    printf("\n");

    TRACE_ALLOC();
    return (ptr_t) pg;
}


/*
 * Function kmem_free_4k (addr)
 *
 *    Free the 4k page given in `addr'.  Try to cluster consecutive 4k
 *    pages into a cluster of 16k.  It is an error to free a 4k page
 *    within a page that was allocated using kmem_alloc_16k().
 *
 */
void kmem_free_4k(dword_t addr)
{
    pglink_t *pg, *np, *pp, *cp;
    dword_t pg16, cpsr;
    uint16 allocs;

    cpsr = DISABLE_INTERRUPTS();

    TRACE_FREE();
    DBG_PRINTF("Freed 4k\n");

    /*
     * Get addresses of the 4k frame, and the 16k frame which the page
     * might belong to (when clustered).
     */
    pg = (pglink_t *) (addr & KMEM_4K_MASK);
    pg16 = addr & KMEM_16K_MASK;

    /*
     * Mark page as not allocated.
     */
    alloc_array[ ((dword_t) pg - (dword_t) cpaddr) >> (KMEM_16K_SHIFT-1) ] &=
	(dword_t) pg & (1 << KMEM_4K_SHIFT) ? ~0xf0 : ~0x0f;

    allocs = ((uint16 *) alloc_array)
	[ ((dword_t) pg - (dword_t) cpaddr) >> KMEM_16K_SHIFT ];


    /*
     * Check if we have 4 consecutive pages (i.e. a 16k page).
     */
    switch (((dword_t) pg >> KMEM_16K_SHIFT) & 0x02) {
    default:
    case 0:
	/*
	 * Page is first in cluster.
	 */
	np = (pglink_t *) (pg16 + 4096);
	if ( ~allocs & 0x0f00 )
	    goto Hook_before_next;
	goto Hook_into_head;

    case 1:
	/*
	 * Page is second in cluster.
	 */
	pp = (pglink_t *) pg16;
	if ( ~allocs & 0xf000 )
	    goto Hook_after_previous;
	np = (pglink_t *) (pg16 + 2*4096);
	if ( ~allocs & 0x00f0 ) {
	    goto Hook_before_next;
	} 
	goto Hook_into_head;

    case 2:
	/*
	 * Page is third in cluster.
	 */
	pp = (pglink_t *) (pg16 + 4096);
	if ( ~allocs & 0x0f00 )
	    goto Hook_after_previous;
	np = (pglink_t *) (pg16 + 3*4096);
	if ( ~allocs & 0x000f )
	    goto Hook_before_next;
	goto Hook_into_head;

    case 3:
	/*
	 * Page is last in cluster.
	 */
	pp = (pglink_t *) (pg16 + 2*4096);
	if ( ~allocs & 0x00f0 )
	    goto Hook_after_previous;
	goto Hook_into_head;

    Hook_after_previous:
	pg->next = pp->next;
	pg->prev = pp;
	if ( pg->next )
	    pg->next->prev = pg;
	pp->next = pg;
	break;

    Hook_before_next:
	pg->next = np;
	pg->prev = np->prev;
	np->prev = pg;
	if ( freelist_4k == np )
	    freelist_4k = pg;
	else
	    pg->prev->next = pg;
	break;

    Hook_into_head:
	pg->next = (pglink_t *) freelist_4k;
	pg->prev = (pglink_t *) &freelist_4k;
	if ( pg->next )
	    pg->next->prev = pg;
	freelist_4k = pg;
	break;
    }

    /*
     * Update cluster (if all four frames in cluster has been freed).
     */
    if ( allocs == 0x0000 ) {
	cp = (pglink_t *) pg16;
	cp->next_cluster = (pglink_t *) cluster_4k;
	cluster_4k = cp;
    }

    /*
     * Accounting.
     */
    numfree_4k++;

    ENABLE_INTERRUPTS(cpsr);
}


/*
 * Function kmem_alloc_1k ()
 *
 *    Allocate a 1k page and return it.
 *
 */
ptr_t kmem_alloc_1k(void)
{
    pglink_t *pg, *np;
    dword_t cpsr;

    cpsr = DISABLE_INTERRUPTS();

    DBG_PRINTF("Allocated 1k\nE-");
    PRINT_STATUS();

    /*
     * Grab head of freelist.
     */
    pg = (pglink_t *) freelist_1k;
    if ( pg == NULL ) {
	/*
	 * Ooops.  No more free pages.  Try to grab some pages from
	 * the 4k freelists.
	 */
	int num = numfree_4k > 15 ? 2 : 1;

	if ( kmem_move_4k_to_1k(num) == 0 ) {
	    /*
	     * It might be that there are some free 16k pages.
	     */
	    kmem_move_16k_to_4k(2);
	    if ( kmem_move_4k_to_1k(2) == 0 ) {
		panic("AIEEE!  Unable to allocate 1k page.\n");
	    }
	}

	pg = (pglink_t *) freelist_1k;
    }
    np = pg->next;
    freelist_1k = np;
    if ( np != NULL )
	np->prev = (pglink_t *) &freelist_1k;

    /*
     * Check if we have split a cluster.
     */
    if ( cluster_1k == pg )
	cluster_1k = pg->next_cluster;

    /*
     * Accounting.
     */
    numfree_1k--;

    /*
     * Mark page as allocated.
     */
    alloc_array[ ((dword_t) pg - (dword_t) cpaddr) >> (KMEM_16K_SHIFT-1) ] |=
	0x80 >> (((dword_t) pg >> KMEM_1K_SHIFT) & 0x03);

    ENABLE_INTERRUPTS(cpsr);

    DBG_PRINTF("L-");
    PRINT_STATUS();

    TRACE_ALLOC();
    return (ptr_t) pg;
}


/*
 * Delay this implementation until 4k free function has been sanity
 * checked.
 */
void kmem_free_1k(dword_t addr)
{
    pglink_t *pg;

    TRACE_FREE();

    pg = (pglink_t *) (addr & KMEM_1K_MASK);

    /*
     * Mark page as not allocated.
     */
    alloc_array[ ((dword_t) pg - (dword_t) cpaddr) >> (KMEM_16K_SHIFT-1) ] |=
	~(0x80 >> (((dword_t) pg >> KMEM_1K_SHIFT) & 0x03));

    printf("Freeing 1k page at 0x%x [Not implemented yet]\n", (int) addr);
}


#endif /* CONFIG_HAVE_ARCH_KMEM */
