/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mapping_alloc.c
 * Description:   Memory management for mapping database.
 *                
 * @LICENSE@
 *                
 * $Id: mapping_alloc.c,v 1.10 2001/11/22 13:04:52 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <mapping.h>
#include <init.h>


/*
 * Size of chunks allocated from kmem.
 */
#define MDB_ALLOC_CHUNKSZ	4096



#define enable_interrupts()
#define disable_interrupts()


//#define MDB_DEBUG 

#ifdef MDB_DEBUG
#define ASSERT_BL(_bl_)							\
{									\
    mdb_mng_t *p = NULL, *m = (mdb_mng_t *) (_bl_)->list_of_lists;	\
    mdb_link_t *l;							\
    int cnt;								\
									\
    if ( m ) {								\
	ASSERT( m->prev_freelist == NULL );				\
	do {								\
	    ASSERT( m->prev_freelist == p );				\
	    ASSERT( m->freelist != NULL );				\
	    ASSERT( m->num_free > 0 );					\
	    ASSERT( m->bl == (_bl_) );					\
	    for ( cnt = 0, l = m->freelist; l; cnt++ )			\
		l = l->next;						\
	    ASSERT( cnt == m->num_free );				\
	    p = m;							\
	    m = m->next_freelist;					\
	} while ( m );							\
    }									\
}

#else
# define ASSERT_BL(_bl_)
#endif



/*
 * Structure used for freelists.
 */
typedef struct mdb_link_t mdb_link_t;
struct mdb_link_t {
    mdb_link_t	*next;
};

/*
 * Structure used for managing contents of a 4kb page.
 */
struct mdb_mng_t {
    mdb_link_t	*freelist;
    dword_t	num_free;
    mdb_mng_t	*next_freelist;
    mdb_mng_t	*prev_freelist;
    mdb_buflist_t *bl;
};


void mdb_buflist_init(void)
{
    mdb_buflist_t *bl;
    dword_t i;

    /*
     * Initialize the remaining fields in the buflists array.  The max_
     * free field is calculated here so that we don't have to use
     * division in the mdb_alloc_buffer() function.
     */
    for ( bl = mdb_buflists; bl->size; bl++ )
    {
	bl->list_of_lists = NULL;
	if ( bl->size >= MDB_ALLOC_CHUNKSZ )
	    continue;
	for ( bl->max_free = 0, i = MDB_ALLOC_CHUNKSZ - bl->size;
	      i >= sizeof(mdb_mng_t);
	      i -= bl->size, bl->max_free++ ) {}
    }
}



/*
 * Function mdb_alloc_buffer (size)
 *
 *    Allocate a buffer of the indicated size.  It is assumed the size
 *    is at least 8, and a multiple of 4.
 *
 */
ptr_t mdb_alloc_buffer(dword_t size)
{
    mdb_buflist_t *bl;
    mdb_link_t *buf;
    mdb_mng_t *mng;

    /*
     * Find the correct buffer list.
     */
    for ( bl = mdb_buflists; bl->size != size; bl++ )
    {
	if ( bl->size == 0 )
	{
	    printf("MDB alloc size = %d\n", size);
	    panic("Illegal MDB buffer allocation size.\n");
	}
    }

    /*
     * Forward large buffer requests directly to the kmem allocator.
     */
    if ( size >= MDB_ALLOC_CHUNKSZ )
	return kmem_alloc(size);

    disable_interrupts();

    /*
     * Get pool of available buffers.
     */
    mng = (mdb_mng_t *) bl->list_of_lists;
    if ( mng == NULL )
    {
	mdb_link_t *b, *n, *p, *ed;

	enable_interrupts();

	/*
	 * First slot of page is dedicated to management strucures.
	 */
	mng = (mdb_mng_t *) kmem_alloc(MDB_ALLOC_CHUNKSZ);
	mng->freelist		= (mdb_link_t *)
	    ((dword_t) mng + MDB_ALLOC_CHUNKSZ - bl->max_free*size);
	mng->num_free		= bl->max_free;
	mng->prev_freelist	= (mdb_mng_t *) NULL;
	mng->bl			= bl;

	/*
	 * Initialize buffers.
	 */
	ed = (mdb_link_t *) ((dword_t) mng + MDB_ALLOC_CHUNKSZ - size);
	for ( b = mng->freelist; b <= ed; b = n )
	{
	    n = (mdb_link_t *) ((dword_t) b + size);
	    b->next = n;
	}
	p = (mdb_link_t *) ((dword_t) b - size);
	p->next = (mdb_link_t *) NULL;

	disable_interrupts();

	/*
	 * Update list of freelists.
	 */
	mng->next_freelist = (mdb_mng_t *) bl->list_of_lists;
	bl->list_of_lists = (ptr_t) mng;
	if ( mng->next_freelist )
	{
	    mng->next_freelist->prev_freelist = mng;
	}
    }

    ASSERT_BL(bl);

    /*
     * Remove buffer from freelist inside page.
     */
    buf = mng->freelist;
    mng->freelist = buf->next;
    mng->num_free--;

    if ( mng->num_free == 0 )
    {
	/*
	 * All buffers in page have been used.  Remove freelist from
	 * list_of_lists.
	 */
	bl->list_of_lists = (ptr_t) mng->next_freelist;
	if ( bl->list_of_lists != NULL )
	    ((mdb_mng_t *) bl->list_of_lists)->prev_freelist =
		(mdb_mng_t *) NULL;
    }

    ASSERT_BL(bl);
    enable_interrupts();

    return (ptr_t) buf;
}


/*
 * Function mdb_free_buffer (addr)
 *
 *    Free buffer at address `addr'.
 *
 */
void mdb_free_buffer(ptr_t addr, dword_t size)
{
    mdb_buflist_t *bl;
    mdb_mng_t *mng;
    mdb_link_t *buf;

    /*
     * Forward large buffer requests directly to the kmem allocator.
     */
    if ( size >= MDB_ALLOC_CHUNKSZ )
    {
	kmem_free(addr, size);
	return;
    }

    buf = (mdb_link_t *) addr;
    mng = (mdb_mng_t *) ((dword_t) addr & ~(MDB_ALLOC_CHUNKSZ-1));
    bl = mng->bl;

    ASSERT_BL(bl);
    disable_interrupts();

    /*
     * Put buffer into local pool of buffers.
     */
    buf->next = mng->freelist;
    mng->freelist = buf;
    mng->num_free++;

    if ( mng->num_free == 1 )
    {
	/*
	 * Buffer pool has gone from empty to non-empty.  Put pool back
	 * into list of pools.
	 */
	if ( bl->list_of_lists ) 
	    ((mdb_mng_t *) bl->list_of_lists)->prev_freelist = mng;
	mng->next_freelist = (mdb_mng_t *) bl->list_of_lists;
	mng->prev_freelist = (mdb_mng_t *) NULL;
	bl->list_of_lists = (ptr_t) mng;

    }
    else if ( mng->num_free == bl->max_free )
    {
	/*
	 * We have freed up all buffers in the frame.  Give frame back
	 * to kernel memory allocator.
	 */
	if ( mng->next_freelist )
	    mng->next_freelist->prev_freelist = mng->prev_freelist;

	if ( bl->list_of_lists == (ptr_t) mng )
	    bl->list_of_lists = (ptr_t) mng->next_freelist;
	else if ( mng->prev_freelist )
	    mng->prev_freelist->next_freelist = mng->next_freelist;

	kmem_free((ptr_t) mng, MDB_ALLOC_CHUNKSZ);
    }

    ASSERT_BL(bl);
    enable_interrupts();
}

