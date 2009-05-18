/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     mapping.h
 * Description:   Declarations and interfaces for generic mapping
 *                database.
 *                
 * @LICENSE@
 *                
 * $Id: mapping.h,v 1.6 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __MAPPING_H__
#define __MAPPING_H__

#include INC_ARCH(mapping.h)


#define MDB_ALL_SPACES		(1<<31)
#define MDB_FLUSH		(1<<1)


/*
 * The mdb_pgshifts[] is an architecture specific arrray defining the
 * page sizes that should be supported by the mapping databse.
 * Actually, it defines the shift numbers rather than the page sizes.
 * The last entry in the array should define the whole address space.
 * E.g if page sizes of 1MB, 64KB, 4KB, and 1KB are supported, the
 * array would contain:
 *
 *    dword_t mdb_pgshifts[5] = { 10, 12, 16, 20, 32 };
 *
 */
extern dword_t mdb_pgshifts[];



/*
 * The mdb_buflistsp[] array defines the buffer sizes that will be
 * needed by the mapping database, these sizes depend on the page
 * sizes to be supported, and also on the size of the rnode_t and
 * mnode_t structures.  For efficiency reasons, the first entry should
 * define the buffer size that will be allocated most frequently (i.e.
 * the size of mapping nodes).  The array should be terminated by
 * defining a zero buffer size.  A buffer size declaration for the ARM
 * architecture would for instance be declared like:
 *
 *    mdb_buflist_t mdb_buflists[5] = { {12}, {8}, {64}, {16}, {0} };
 *
 */
typedef struct {
    dword_t	size;
    ptr_t	list_of_lists;
    dword_t	max_free;
} mdb_buflist_t;

extern mdb_buflist_t mdb_buflists[];


/* Top level mapping node for sigma0. */
extern mnode_t *sigma0_mapnode;


typedef struct {
    mnode_t	*map;
    rnode_t	*root;
} dualnode_t;



/*
 * Prototypes
 */

/* From mapping.c */
mnode_t *mdb_map(mnode_t *f_map, pgent_t *f_pg, dword_t f_pgsize,
		 dword_t f_addr, pgent_t *t_pg, dword_t t_pgsize,
		 space_t *t_space);
int mdb_flush(mnode_t *f_map, pgent_t *f_pg, dword_t f_pgsize,
	      dword_t t_addr, dword_t t_pgsize, dword_t mode);


/* From mapping_alloc.c */
ptr_t mdb_alloc_buffer(dword_t size);
void mdb_free_buffer(ptr_t addr, dword_t size);



/*
 * Helper functions
 */

static inline dword_t page_size(dword_t pgsize)
{
    return 1 << hw_pgshifts[pgsize];
}

static inline dword_t page_mask(dword_t pgsize)
{
    return (~0U) >> (32 - hw_pgshifts[pgsize]);
}

static inline dword_t table_size(dword_t pgsize)
{
    return 1 << (hw_pgshifts[pgsize+1] - hw_pgshifts[pgsize]);
}

static inline dword_t table_index(dword_t addr, dword_t pgsize)
{
    return (addr >> hw_pgshifts[pgsize]) & (table_size(pgsize) - 1);
}


#endif /* !__MAPPING_H__ */
