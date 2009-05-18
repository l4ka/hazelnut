/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/mapping.h
 * Description:   MIPS specifications for the mapping database.
 *                
 * @LICENSE@
 *                
 * $Id: mapping.h,v 1.3 2001/11/22 14:56:36 skoglund Exp $
 *                
 ********************************************************************/
#ifdef CONFIG_ENABLE_NEWMDB
#include INC_ARCH(mapping_new.h)
#else
#ifndef __ARM__MAPPING_H__
#define __ARM__MAPPING_H__

#define SIGMA0_PGDIR_ID		0

#define NUM_PAGESIZES		1


/*
**
** Mapping node definition and associated macros.
**
*/
typedef struct {
    unsigned pgdir	:30;
    unsigned next_type	:2;
    unsigned depth	:6;
    unsigned		:4;
    unsigned offset	:22;
    unsigned next	:32;
} __attribute__ ((packed)) mnode_t;

#define MNODE_SET_NEXT(n, addr) \
    ((n)->next = (dword_t) (addr))

#define MNODE_GET_NEXT(n) \
    ((mnode_t *) (n)->next)

#define MNODE_SET_NEXT_TYPE_MAP(n) \
      ((n)->next_type = 0)

#define MNODE_SET_NEXT_TYPE_ROOT(n) \
      ((n)->next_type = 1)

#define MNODE_SET_PARTLY_INVALID(n) \
      ((n)->next_type = 2)

#define MNODE_SET_INVALID(n) \
      ((n)->next_type = 3)

#define MNODE_IS_NEXT_TYPE_MAP(n) \
      ((n)->next_type == 0)

#define MNODE_IS_NEXT_TYPE_ROOT(n) \
      ((n)->next_type == 1)

#define MNODE_IS_PARTLY_INVALID(n) \
      ((n)->next_type == 2)

#define MNODE_SET_PGDIR(n, pgd) \
    ((n)->pgdir = (pgd) >> 2)

#define MNODE_GET_PGDIR(n) \
    ((n)->pgdir << 2)

#define MNODE_SET_OFFSET(n, off) \
    ((n)->offset = (off) >> 10)

#define MNODE_GET_OFFSET(n) \
    ((n)->offset << 10)

#define MNODE_OFFSET_EQ(n, off) \
    ((n)->offset << 10 == (off))

#define MNODE_SET_DEPTH(n, dpt) \
    ((n)->depth = (dpt))

#define MNODE_GET_DEPTH(n) \
    ((n)->depth)


/*
**
** Root node definition and associated macros.
**
*/
typedef struct {
    unsigned pgsize	:1;
    unsigned		:3;
    unsigned maptree	:26;
    unsigned next_type	:2;
} rnode_t;

#define RNODE_SET_NEXT_TYPE_MAP(n) \
    ((n)->next_type = 0)

#define RNODE_SET_NEXT_TYPE_ROOT(n) \
    ((n)->next_type = 1)

#define RNODE_SET_PARTLY_INVALID(n) \
    ((n)->next_type = 2)

#define RNODE_SET_INVALID(n) \
    ((n)->next_type = 3)

#define RNODE_IS_NEXT_TYPE_MAP(n) \
      ((n)->next_type == 0)

#define RNODE_IS_NEXT_TYPE_ROOT(n) \
      ((n)->next_type == 1)

#define RNODE_IS_PARTLY_INVALID(n) \
      ((n)->next_type == 2)

#define RNODE_IS_INVALID(n) \
      ((n)->next_type == 3)

#define RNODE_SET_PGSIZE(n, size) \
    ((n)->pgsize = size)

#define RNODE_GET_PGSIZE(n) \
    ((n)->pgsize)

#define RNODE_SET_MAPTREE(n, tree)				\
({								\
    ((n)->maptree = ((((dword_t) (tree)) >> 2) & 0x03ffffff));	\
    if ( (dword_t) RNODE_GET_MAPTREE(n) != (dword_t) tree &&	\
	 (dword_t) tree != 0 ) {				\
	printf("Maptree = 0x%x\n", tree);			\
	panic("Maptree setting has lost some bits.");		\
    }								\
})

#define RNODE_GET_MAPTREE(n) \
    ((n)->maptree ? ((mnode_t *) (((n)->maptree << 2) + 0xf0000000)) : \
     (mnode_t *) 0)



typedef struct {
    mnode_t	*map;
    rnode_t	*root;
    unsigned	num_occupied:16;
    unsigned	num_invalid:16;
} dualnode_t;


/*
**
** Macros not operating on neither mapping nodes nor root nodes.
**
*/
#define MDB_IS_KERNEL_PGDIR(p) \
    ((p) == kernel_pgdir)

#define MDB_OFFSET_TO_ADDR(offset) \
    ((offset) << 10)

#define MDB_ADDR_TO_OFFSET(addr) \
    ((addr) >> 10)




#endif /* !__ARM__MAPPING_H__ */
#endif
