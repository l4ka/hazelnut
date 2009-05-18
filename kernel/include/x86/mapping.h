/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86/mapping.h
 * Description:   Mapping database specification for x86.
 *                
 * @LICENSE@
 *                
 * $Id: mapping.h,v 1.4 2001/12/05 20:49:51 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __X86__MAPPING_H__
#define __X86__MAPPING_H__

#define SIGMA0_PGDIR_ID		0
#define NUM_PAGESIZES		2

#if !defined(ASSEMBLY)

#ifdef CONFIG_SMP
#define __LOCK	"lock;"
#else
#define __LOCK
#endif

#if 0
#define MDB_BIT_LOCK(word, bit)			\
    __asm__ __volatile__ (			\
	"1: 			\n"		\
	__LOCK					\
	"   btsl  %1, %0	\n"		\
	"   jc    2f		\n"		\
	".section .spinlock	\n"		\
	"2: 			\n"		\
	"   jmp   1b		\n"		\
	".previous		\n"		\
	:					\
	:"m"(word), "I"(bit))

#define MDB_BIT_TRYLOCK(word, bit)		\
({						\
    int ret = 0;				\
						\
    __asm__ __volatile__ (			\
	__LOCK					\
	"   btsl  %2, %1	\n"		\
	"   setnc %b0		\n"		\
	:					\
	:"r"(ret), "m"(word), "I"(bit));	\
						\
    ret;					\
})

#define MDB_BIT_UNLOCK(word, bit)		\
    __asm__ __volatile__ (			\
	__LOCK "btrl %1, %0"			\
	:					\
	:"m"(word), "I"(bit))

#else

#define MDB_BIT_LOCK(word, bit)
#define MDB_BIT_TRYLOCK(word, bit) (1)
#define MDB_BIT_UNLOCK(word, bit)

#endif


//#define MDB_SANITY_CHECK


class space_t;
class pgent_t;
class rnode_t;


#define MDB_NEXT_ROOT		0x01
#define MDB_PREV_ROOT		0x01

#define MDB_SPACE_MASK		0xfffff000
#define MDB_DEPTH_MASK		0x000000ff
#define MDB_UPLOCK_BIT		8
#define MDB_DOWNLOCK_BIT	9
#define MDB_UPLOCK		(1 << MDB_UPLOCK_BIT)
#define MDB_DOWNLOCK		(1 << MDB_DOWNLOCK_BIT)


class mnode_t
{
 public:
    volatile unsigned long w1;	// Prev pointer, pgent_t pointer
    volatile unsigned long w2;	// Space, tree depth, type bits
    volatile unsigned long w3;	// Next pointer

    inline void down_lock(void)
	{ MDB_BIT_LOCK(w2, MDB_DOWNLOCK_BIT); }

    inline int down_trylock(void)
	{ return MDB_BIT_TRYLOCK(w2, MDB_DOWNLOCK_BIT); }

    inline void down_unlock(void)
	{ MDB_BIT_UNLOCK(w2, MDB_DOWNLOCK_BIT); }

    inline void up_lock(void)
	{ MDB_BIT_LOCK(w2, MDB_UPLOCK_BIT); }

    inline int up_trylock(void)
	{ return MDB_BIT_TRYLOCK(w2, MDB_UPLOCK_BIT); }

    inline void up_unlock(void)
	{ MDB_BIT_UNLOCK(w2, MDB_UPLOCK_BIT); }

    inline void clear_locks(void)
	{ w2 &= ~(MDB_UPLOCK | MDB_DOWNLOCK); }


    // Previous pointer and backing pointer

    inline pgent_t *get_pgent(mnode_t *prev)
	{
#ifdef MDB_SANITY_CHECK
	    if ( w1 & MDB_PREV_ROOT )
		panic("mnode::get_pgent - Not prev map\n");
#endif
	    return (pgent_t *) (w1 ^ (dword_t) prev);
	}

    inline pgent_t *get_pgent(rnode_t *prev)
	{
#ifdef MDB_SANITY_CHECK
	    if ( !(w1 & MDB_PREV_ROOT) )
		panic("mnode::get_pgent - Not prev root\n");
#endif
	    return (pgent_t *) ((w1 ^ (dword_t) prev) & ~MDB_PREV_ROOT);
	}

    inline mnode_t *get_prevm(pgent_t *pg)
	{
#ifdef MDB_SANITY_CHECK
	    if ( w1 & MDB_PREV_ROOT )
		panic("mnode::get_prevm - Not prev map\n");
#endif
	    return (mnode_t *) ((w1 ^ (dword_t) pg) & ~MDB_PREV_ROOT);
	}

    inline rnode_t *get_prevr(pgent_t *pg)
	{
#ifdef MDB_SANITY_CHECK
	    if ( !(w1 & MDB_PREV_ROOT) )
		panic("mnode::get_prevr - Not prev root\n");
#endif
	    return (rnode_t *) ((w1 ^ (dword_t) pg) & ~MDB_PREV_ROOT);
	}

    inline void set_backlink(mnode_t *prev, pgent_t *pg)
	{
	    w1 = (dword_t) prev ^ (dword_t) pg;
	}

    inline void set_backlink(rnode_t *prev, pgent_t *pg)
	{
	    w1 = ((dword_t) prev ^ (dword_t) pg) | MDB_PREV_ROOT;
	}

    inline int is_prev_root(void)
	{
	    return w1 & MDB_PREV_ROOT;
	}


    // Next pointer

    inline mnode_t *get_nextm(void)
	{
	    return (mnode_t *) (w3 & ~MDB_NEXT_ROOT);
	}

    inline rnode_t *get_nextr(void)
	{
	    return (rnode_t *) (w3 & ~MDB_NEXT_ROOT);
	}

    inline void set_next(mnode_t *map)
	{
	    w3 = (dword_t) map;
	}

    inline void set_next(rnode_t *root)
	{
	    w3 = (dword_t) root | MDB_NEXT_ROOT;
	}

    inline int is_next_root(void)
	{
	    return w3 & MDB_NEXT_ROOT;
	}


    // Address space identifier

    inline space_t * get_space(void)
	{
	    return (space_t *) (w2 & MDB_SPACE_MASK);
	}

    inline void set_space(space_t * space)
	{
	    w2 = (w2 & ~MDB_SPACE_MASK) | ((dword_t) space & MDB_SPACE_MASK);
	}


    // Tree depth

    inline dword_t get_depth(void)
	{
	    return w2 & MDB_DEPTH_MASK;
	}

    inline void set_depth(dword_t depth)
	{
	    w2 = (w2 & ~MDB_DEPTH_MASK) | (depth & MDB_DEPTH_MASK);
	}

} __attribute__ ((packed));


#define MDB_RNEXT_ROOT		0x01
#define MDB_RDOWNLOCK_BIT	1
#define MDB_RDOWNLOCK		(1 << MDB_RDOWNLOCK_BIT)
#define MDB_RPTR_MASK		0xfffffffc


class rnode_t
{
 public:
    unsigned long w1;

    inline void down_lock(void)
	{ MDB_BIT_LOCK(w1, MDB_RDOWNLOCK_BIT); }

    inline int down_trylock(void)
	{ return MDB_BIT_TRYLOCK(w1, MDB_RDOWNLOCK_BIT); }

    inline void down_unlock(void)
	{ MDB_BIT_UNLOCK(w1, MDB_RDOWNLOCK_BIT); }

    inline void clear_locks(void)
	{ w1 &= ~MDB_RDOWNLOCK;	}

    inline mnode_t *get_map(void)
	{
	    return (mnode_t *) (w1 & MDB_RPTR_MASK);
	}

    inline void set_ptr(mnode_t *map)
	{
	    w1 = (dword_t) map | MDB_RDOWNLOCK;
	}

    inline void set_ptr(rnode_t *root)
	{
	    w1 = (dword_t) root | MDB_RNEXT_ROOT | MDB_RDOWNLOCK;
	}

    inline int is_next_root(void)
	{
	    return w1 & MDB_RNEXT_ROOT;
	}
};


#endif /* !ASSEMBLY */

#endif /* !__X86__MAPPING_H__ */
