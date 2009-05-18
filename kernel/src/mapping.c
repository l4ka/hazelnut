/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     mapping.c
 * Description:   Generic mapping database implementation.
 *                
 * @LICENSE@
 *                
 * $Id: mapping.c,v 1.16 2001/12/07 19:08:52 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <mapping.h>
#include INC_ARCH(memory.h)
#include <init.h>


/*
 * UPLOCKS AND DOWNLOCKS
 *
 * There are two kinds of locks for mapping nodes: uplocks and
 * downlocks.  The uplock protects the prev pointer and page table
 * pointer and the downlock protects the next pointer.  If an uplock
 * is aquired, the thread is allowed to hold it as long as it wants.
 * If a downlock is aquired and the thread is not able to aquire its
 * child's uplock (given that it indeed has a child node), it must
 * release its own downlock and try acquiring it later.  Root nodes
 * contain only downlocks, and aquireing a downlock in a root node
 * follows the same algorithm as acquiring a downlock in a mapping
 * node.
 *
 * Using uplocks and downlocks, a node may only be deleted if it has
 * aquired both of its own locks, the downlock of its parent, and the
 * uplock of its child.  A new node may be added beneath a node if its
 * downlock, and its current child's uplock is aquired.
 *
 */


static dualnode_t *mdb_create_roots(dword_t size);


static inline dword_t mdb_arraysize(dword_t pgsize)
{
    return 1 << (mdb_pgshifts[pgsize+1] - mdb_pgshifts[pgsize]);
}

static inline dword_t mdb_get_index(dword_t size, dword_t addr)
{
    return (addr >> mdb_pgshifts[size]) & (mdb_arraysize(size) - 1);
}

static inline rnode_t *mdb_index_root(dword_t size, rnode_t *r, dword_t addr)
{
    return r + mdb_get_index(size, addr);
}

static inline int mdb_pgsize (int hw_pgsize)
{
    int s = 0;
    while (mdb_pgshifts[s] < hw_pgshifts[hw_pgsize])
	s++;
    return s;
}

static inline int hw_pgsize (int mdb_pgsize)
{
    int s = 0;
    while (hw_pgshifts[s] < mdb_pgshifts[mdb_pgsize])
	s++;
    return s;
}


/*
 * Function mdb_map (f_map, f_pg, f_pgsize, f_addr, t_pg, t_pgsize, t_space)
 *
 *    Insert the specified mapping.  We will have f_map->uplock upon
 *    entry.  If bit 0 in f_addr is set, it indicates that we are
 *    doing a grant operation.  Function returns new mapping node with
 *    the uplock acquired.
 *
 */
mnode_t *mdb_map(mnode_t *f_map, pgent_t *f_pg, dword_t f_pgsize,
		 dword_t f_addr, pgent_t *t_pg, dword_t t_pgsize,
		 space_t *t_space)
{
    dualnode_t *dual;
    rnode_t *root, *proot;
    mnode_t *newm, *nmap;
    dword_t newdepth = f_map->get_depth() + 1;

    if ( f_addr & 0x01 )
    {
	// Grant.
	if ( f_map->is_prev_root() )
	    f_map->set_backlink(f_map->get_prevr(f_pg), t_pg);
	else
	    f_map->set_backlink(f_map->get_prevm(f_pg), t_pg);
	f_map->set_space(t_space);

	return f_map;
    }

    // Convert to mapping database pagesizes.
    f_pgsize = mdb_pgsize (f_pgsize);
    t_pgsize = mdb_pgsize (t_pgsize);

    // Aquire f_map->downlock and nmap->uplock.
    for (;;)
    {
	f_map->down_lock();
	if ( f_map->is_next_root() )
	{
	    dual = (dualnode_t *) f_map->get_nextr();
	    nmap = dual->map;
	}
	else
	{
	    dual = NULL;
	    nmap = f_map->get_nextm();
	}
	if ( (! nmap) || nmap->up_trylock() )
	    break;
	f_map->down_unlock();
    }

    newm = (mnode_t *) mdb_alloc_buffer(sizeof(mnode_t));

    if ( f_pgsize == t_pgsize )
    {
	// Hook new node directly below mapping node.
	newm->set_backlink(f_map, t_pg);
	newm->set_space(t_space);
	newm->set_depth(f_map->get_depth() + 1);
	newm->clear_locks();
	newm->set_next(nmap);

	if ( dual )
	    dual->map = newm;
	else
	    f_map->set_next(newm);

	if ( nmap )
	{
	    nmap->set_backlink(newm, nmap->get_pgent(f_map));
	    nmap->up_unlock();
	}
	f_map->down_unlock();
    }
    else if ( f_pgsize > t_pgsize )
    {
	f_pgsize--;
	if ( nmap )
	    nmap->up_unlock(); // No longer needed.

	// Traverse into the root array below mappping node.
	if ( ! dual )
	{
	    // New array needs to be created.
	    dual = mdb_create_roots(f_pgsize);
	    dual->map = nmap;

	    root = mdb_index_root(f_pgsize, dual->root, f_addr);
	    root->down_lock();
	    f_map->set_next((rnode_t *) dual);
	    nmap = NULL;
	    dual = NULL;
	}
	else
	{
	    root = mdb_index_root(f_pgsize, dual->root, f_addr);

	    // Aquire root->downlock and nmap->uplock.
	    for (;;)
	    {
		root->down_lock();
		if ( root->is_next_root() )
		{
		    dual = (dualnode_t *) root->get_map();
		    nmap = dual->map;
		}
		else
		{
		    dual = NULL;
		    nmap = root->get_map();
		}
		if ( (! nmap) || nmap->up_trylock() )
		    break;
		root->down_unlock();
	    }
	}

	f_map->down_unlock(); // No longer needed.

	// Traverse into the root array below root arrays.
	while ( f_pgsize > t_pgsize )
	{
	    f_pgsize--;
	    if ( ! dual )
	    {
		// New array needs to be created.
		dual = mdb_create_roots(f_pgsize);
		dual->map = root->get_map();
		root->set_ptr((rnode_t *) dual);
	    }
	    proot = root;
	    root = mdb_index_root(f_pgsize, dual->root, f_addr);

	    if ( root->is_next_root() )
	    {
		dual = (dualnode_t *) root->get_map();
		nmap = dual->map;
		}
	    else
	    {
		dual = NULL;
		nmap = root->get_map();
	    }

	    root->down_lock();
	    proot->down_unlock();
	}

	newm->set_backlink(root, t_pg);
	newm->set_space(t_space);
	newm->set_depth(newdepth);
	newm->set_next(nmap);
	newm->down_unlock();

	if ( dual )
	    dual->map = newm;
	else
	    root->set_ptr(newm);

	if ( nmap )
	{
	    nmap->set_backlink(newm, nmap->get_pgent(root));
	    nmap->up_unlock();
	}
	root->down_unlock();
    }
    else
	enter_kdebug("mdb_map: f_pgsize < t_pgsize");

    return newm;
}



int mdb_flush(mnode_t *f_map, pgent_t *f_pg, dword_t f_pgsize,
	      dword_t f_addr, dword_t t_pgsize, dword_t mode)
{
    dualnode_t *dual = NULL;
    mnode_t *pmap = NULL, *nmap;
    rnode_t *root = NULL, *proot = NULL;
    dword_t rcnt, startdepth, vaddr;

    mnode_t *r_nmap[NUM_PAGESIZES-1];
    rnode_t *r_root[NUM_PAGESIZES-1];
    dword_t  r_rcnt[NUM_PAGESIZES-1];
    dword_t  r_prev[NUM_PAGESIZES-1];  /* Bit 0 set indicates mapping.
					  Bit 0 cleared indicates root. */

    rcnt = 0;
    startdepth = f_map->get_depth();

    // Convert to mapping database pagesizes.
    f_pgsize = mdb_pgsize (f_pgsize);
    t_pgsize = mdb_pgsize (t_pgsize);

    do {
	// Variables `f_map' and `f_pg' are valid at this point.

	// Aquire f_map->downlock and nmap->uplock.  We already
	// have f_map->uplock.
	for (;;)
	{
	    f_map->down_lock();
	    if ( f_map->is_next_root() )
	    {
		dual = (dualnode_t *) f_map->get_nextr();
		nmap = dual->map;
	    }
	    else
	    {
		dual = NULL;
		nmap = f_map->get_nextm();
	    }
	    if ( (! nmap) || nmap->up_trylock() )
		break;
	    f_map->down_unlock();
	}

	// Aquire either pmap->downlock or proot->downlock.
	if ( f_map->is_prev_root() )
	{
	    pmap = NULL;
	    proot = f_map->get_prevr(f_pg);
	    proot->down_lock();
	}
	else
	{
	    proot = 0;
	    pmap = f_map->get_prevm(f_pg);
	    pmap->down_lock();
	}

	// Variable contents at this point:
	//
	//   f_map - the current mapping node
	//   f_pg  - the current pgent node
	//   pmap  - the previous mapping node (or NULL if prev is root)
	//   proot - the previous root node (or NULL if prev is map)
	//   nmap  - the next mapping node (may be NULL)
	//   dual  - next dual node (or NULL if no such node)
	//   root  - Current root array pointer (or NULL)

//	printf("New: f_map=%p  dual=%p  nmap=%p  pmap=%p  proot=%p  "
//	       "fsize=%d   tsize=%d  root=%p\n",
//	       f_map, dual, nmap, pmap, proot, f_pgsize, t_pgsize, root);

	if ( mode & MDB_ALL_SPACES )
	{
	    space_t * space = f_map->get_space ();

	    if ( (f_pgsize > t_pgsize) && nmap )
	    {
		enter_kdebug("mdb_flush: trying to flush too small page");
		return -1;
	    }

	    int f_hwpgsize = hw_pgsize (f_pgsize);
	    vaddr = f_pg->vaddr(space, f_hwpgsize, f_map);

	    if ( mode & MDB_FLUSH )
	    {
		// Remove ourselves.
		if ( pmap )
		{
		    pmap->set_next(nmap);
		    if ( nmap )
			nmap->set_backlink(pmap, nmap->get_pgent(f_map));
		}
		else
		{
		    proot->set_ptr(nmap);
		    if ( nmap )
			nmap->set_backlink(proot, nmap->get_pgent(f_map));
		}

		mdb_free_buffer((dword_t *) f_map, sizeof(mnode_t));
		f_pg->clear(space, f_hwpgsize);
	    }
	    else
	    {
		f_pg->set_readonly(space, f_hwpgsize);
		pmap = f_map;
		proot = NULL;
	    }

	    // We might have to flush some TLB entries.
#if defined(CONFIG_ENABLE_SMALL_AS)
	    if (space->is_small ())
		// Page in small space.  Need to flush since we might
		// happen to swith to this space next (plus small
		// space TLB entries on x86 have global bit set).
 		flush_tlbent ((ptr_t) (vaddr + space->smallid ().offset ()));
	    else if (space == get_current_space ())
		// Page is in currently accessible page table.  We
		// have to flush.
 		flush_tlbent ((ptr_t) vaddr);
#else
	    if (space == get_current_space ())
 		flush_tlbent ((ptr_t) vaddr);
#endif
	}
	else
	{
	    pmap = f_map;
	    proot = NULL;
	}
	f_map = NULL;


	// Variables `f_map' and `f_pg' are no longer valid here.

	if ( dual )
	{
	    f_pgsize--;

	    if ( f_pgsize < t_pgsize )
	    {
		// Recurse into subarray before checking mappings.
		r_prev[f_pgsize] = pmap ? (dword_t) pmap | 0x01 : 
		    (dword_t) proot;
		r_nmap[f_pgsize] = nmap;
		r_root[f_pgsize] = root;
		r_rcnt[f_pgsize] = rcnt;

		root = dual->root - 1;
		rcnt = mdb_arraysize(f_pgsize);

		if ( (mode & MDB_FLUSH) & (mode & MDB_ALL_SPACES) )
		    mdb_free_buffer((dword_t *) dual, sizeof(dualnode_t));
	    }
	    else
	    {
		// We may use the virtual address `f_addr' for
		// indexing here since the alignment within the page
		// will be the same as with the physical address.
		root = mdb_index_root(f_pgsize, dual->root, f_addr) - 1;
		rcnt = 1;
	    }
	}
	else
	{
	    if ( nmap )
	    {
		if ( pmap )
		    f_pg = nmap->get_pgent(pmap);
		else
		    f_pg = nmap->get_pgent(proot);

		f_map = nmap;
	    }
	    else if ( f_pgsize < t_pgsize  && root )
	    {
		// Recurse up from subarray and delete it.
		if ( mode & MDB_FLUSH )
		{
		    root -= mdb_arraysize(f_pgsize) - 1;
		    mdb_free_buffer((dword_t *) root,
				    mdb_arraysize(f_pgsize) * sizeof(rnode_t));
		}

		f_map = r_nmap[f_pgsize];
		root  = r_root[f_pgsize];
		rcnt  = r_rcnt[f_pgsize];
		if ( r_prev[f_pgsize] & 0x01 )
		{
		    proot = NULL;
		    pmap = (mnode_t *) (r_prev[f_pgsize] & ~0x01);
		    if ( f_map )
			f_pg = f_map->get_pgent(pmap);
		}
		else
		{
		    proot = (rnode_t *) r_prev[f_pgsize];
		    pmap = NULL;
		    if ( f_map )
			f_pg = f_map->get_pgent(proot);
		}

		f_pgsize++;
	    }
	}

	// If f_map now is non-nil, the variables f_map, f_pg, pmap
	// and proot will all be valid.  Otherwise, root and rcnt will
	// be valid.

	while ( (! f_map) && (rcnt > 0)  )
	{
	    rcnt--;
	    root++;

	    // Aquire root->downlock and child->upmap (if child exists).
	    // There is no need to release the root->downlocks, since no one
	    // will be able to reference the root nodes when we have flushed
	    // them (we remove the parent node from the mapping tree before
	    // recursing into the root nodes).
	    for (;;)
	    {
		root->down_lock();
		if ( root->is_next_root() )
		{
		    dual = (dualnode_t *) root->get_map();
		    f_map = dual->map;
		}
		else
		{
		    dual = NULL;
		    f_map = root->get_map();
		}

		if ( (! f_map) || f_map->up_trylock() )
		    break;
	    }

	    if ( dual )
	    {
		// Recurse into subarray before checking mappings.
		f_pgsize--;

		if ( mode & MDB_FLUSH )
		    root->set_ptr(f_map); // Remove subarray.

		r_prev[f_pgsize] = (dword_t) root;
		r_nmap[f_pgsize] = f_map;
		r_root[f_pgsize] = root;
		r_rcnt[f_pgsize] = rcnt;

		f_map = NULL;
		root = dual->root - 1;
		rcnt = mdb_arraysize(f_pgsize);

		if ( mode & MDB_FLUSH )
		    mdb_free_buffer((dword_t *) dual, sizeof(dualnode_t));
	    }
	    else
	    {
		if ( f_map )
		{
		    f_pg = f_map->get_pgent(root);
		    pmap = NULL;
		    proot = root;
		}
	    }
	}

	if ( f_pgsize <= t_pgsize )
	    // From now on, we will remove all nodes.
	    mode |= MDB_ALL_SPACES;

    } while ( f_map && f_map->get_depth() > startdepth );

    // Allow next mapping node in tree to have it's prev counter modified.
    if ( f_map )
	f_map->up_unlock();

    return 0;
}



static dualnode_t * mdb_create_roots(dword_t size)
{
    rnode_t *newnodes, *n;

    size = mdb_arraysize(size);
    newnodes = (rnode_t *) mdb_alloc_buffer(sizeof(rnode_t) * size);

    for ( n = newnodes; size--; n++ )
    {
	n->set_ptr((mnode_t *) NULL);
	n->clear_locks();
    }

    n = (rnode_t *) mdb_alloc_buffer(sizeof(dualnode_t));
    ((dualnode_t *) n)->map = NULL;
    ((dualnode_t *) n)->root = newnodes;
    
    return (dualnode_t *) n;
}



// The sigma0_mapnode is initialized to own the whole address space.
static mnode_t __sigma0_mapnode;
mnode_t *sigma0_mapnode;

#if defined(NEED_FARCALLS)
#include INC_ARCH(farcalls.h)
#endif

void mdb_init(void)
{
    static dualnode_t *(*__mdb_create_roots)(dword_t) = mdb_create_roots;
    dualnode_t *dual;

    mdb_buflist_init();

    // Frame table for the complete address space.
    dual = __mdb_create_roots(NUM_PAGESIZES-1);

    // Let sigma0 own the whole address space.
    sigma0_mapnode = &__sigma0_mapnode;
    sigma0_mapnode->set_backlink((mnode_t *) NULL, (pgent_t *) NULL);
    sigma0_mapnode->set_space(SIGMA0_PGDIR_ID);
    sigma0_mapnode->set_depth(0);
    sigma0_mapnode->clear_locks();
    sigma0_mapnode->set_next((rnode_t *) dual);

    // Sanity checking of pgshift arrays.
    for (int i = 0, j; i < HW_NUM_PGSIZES; i++)
    {
	if (! ((1 << hw_pgshifts[i]) & HW_VALID_PGSIZES))
	    continue;
	for (j = 0; j < NUM_PAGESIZES; j++)
	    if (hw_pgshifts[i] == mdb_pgshifts[j])
		break;
	if (j == NUM_PAGESIZES)
	    panic("mdb_pgshifts[] is not a superset of hw_pgshifts[]");
    }
}


