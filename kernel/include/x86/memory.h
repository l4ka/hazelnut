/*********************************************************************
 *                
 * Copyright (C) 2000, 2001, 2002,  Karlsruhe University
 *                
 * File path:     x86/memory.h
 * Description:   x86 page table manipulation code.
 *                
 * @LICENSE@
 *                
 * $Id: memory.h,v 1.20 2002/07/18 13:09:07 haeber Exp $
 *                
 ********************************************************************/
#ifndef __X86__MEMORY_H__
#define __X86__MEMORY_H__

#include INC_ARCH(config.h)
#include INC_ARCH(mapping.h)

#if defined(ASSEMBLY)
# define __DWORD_CAST
#else
# define __DWORD_CAST (dword_t)
#endif

#define PAGEDIR_BITS		22
#define PAGEDIR_SIZE		__DWORD_CAST (1 << PAGEDIR_BITS)
#define PAGEDIR_MASK		__DWORD_CAST (~(PAGEDIR_SIZE - 1))

#define PAGE_BITS		12
#define PAGE_SIZE		__DWORD_CAST (1 << PAGE_BITS)
#define PAGE_MASK		__DWORD_CAST (~(PAGE_SIZE - 1))

#define PAGE_VALID		(1<<0)
#define PAGE_WRITABLE		(1<<1)
#define PAGE_USER		(1<<2)
#define PAGE_WRITE_THROUGH	(1<<3)
#define PAGE_CACHE_DISABLE	(1<<4)
#define PAGE_ACCESSED		(1<<5)
#define PAGE_DIRTY		(1<<6)
#define PAGE_SUPER		(1<<7)
#if defined(CONFIG_IA32_FEATURE_PGE) && !defined(CONFIG_SMP)
# define PAGE_GLOBAL		(1<<8)
#else
# define PAGE_GLOBAL		(0)
#endif
#define PAGE_FLAGS_MASK		(0x017f)

#define PAGE_USER_BITS		(PAGE_VALID | PAGE_USER)
#define PAGE_USER_TBITS		(PAGE_VALID | PAGE_USER | PAGE_WRITABLE)
#define PAGE_SIGMA0_BITS	(PAGE_VALID | PAGE_USER | PAGE_WRITABLE)

#define PAGE_KERNEL_INIT_SECT	(PAGE_VALID | PAGE_WRITABLE)
#define PAGE_KERNEL_BITS	(PAGE_VALID | PAGE_WRITABLE | PAGE_GLOBAL)
#define PAGE_KERNEL_TBITS	(PAGE_VALID | PAGE_WRITABLE | PAGE_GLOBAL | PAGE_ACCESSED)
#define PAGE_TCB_BITS		(PAGE_VALID | PAGE_WRITABLE | PAGE_GLOBAL)
#define PAGE_ZERO_BITS		(PAGE_VALID)
#define PAGE_APIC_BITS		(PAGE_VALID | PAGE_WRITABLE | PAGE_GLOBAL | PAGE_CACHE_DISABLE)

#define IS_PAGE_VALID(x)	(x & PAGE_VALID)


#if !defined(ASSEMBLY)

/*
 * Generic page table configuration
 */

#define HW_NUM_PGSIZES		2

#if defined(CONFIG_IA32_FEATURE_PSE)
# define HW_VALID_PGSIZES	((1<<22) + (1<<12))
#else
# define HW_VALID_PGSIZES	(1<<12)
#endif

extern dword_t hw_pgshifts[];



/* declared in x86/init.c */
extern ptr_t __zero_page;


#if 0
INLINE ptr_t virt_to_phys(ptr_t p)
{
    return (ptr_t)((dword_t)p - KERNEL_OFFSET);
}
#else
#define virt_to_phys(x) \
({ typeof(x) _x = (typeof(x))((dword_t) (x) - KERNEL_OFFSET); _x;})
#endif

#if 0
INLINE ptr_t phys_to_virt(ptr_t p)
{
    return (ptr_t)((dword_t)p + KERNEL_OFFSET);
}
#else
#define phys_to_virt(x) \
({ typeof(x) _x = (typeof(x))((dword_t) (x) + KERNEL_OFFSET); _x;})
#endif


INLINE dword_t pgdir_idx(dword_t addr)
{
    return addr >> PAGEDIR_BITS;
}

INLINE dword_t pgtab_idx(dword_t addr)
{
    return (addr >> PAGE_BITS) & ~(PAGE_MASK >> 2);
}

INLINE ptr_t get_kernel_pagetable()
{
    extern dword_t * kernel_ptdir_root;
    return kernel_ptdir_root;
}

#if defined(CONFIG_SMP)
INLINE ptr_t get_cpu_pagetable(int cpu)
{
    extern ptr_t cpu_ptdir_root[CONFIG_SMP_MAX_CPU];
    return cpu_ptdir_root[cpu];
}
#endif


INLINE dword_t get_pagefault_address(void)
{
    register dword_t tmp;

    asm ("movl	%%cr2, %0\n"
	 :"=r" (tmp));

    return tmp;
}

INLINE ptr_t get_current_pagetable(void)
{
    ptr_t tmp;

    asm volatile ("movl %%cr3, %0	\n"
		  :"=a" (tmp));

    return tmp;
}

INLINE void set_current_pagetable(ptr_t pdir)
{
    asm volatile ("mov %0, %%cr3\n"
		  :
		  : "r"(pdir));
}

#include INC_ARCH(space.h)

#if defined(CONFIG_ENABLE_SMALL_AS)

#if defined(CONFIG_SMP)
#error SMP does not work with small address spaces.
#endif

void small_space_to_large (space_t *);
int make_small_space (space_t *, byte_t);

INLINE dword_t get_ipc_copy_limit(dword_t addr, tcb_t *receiver)
{
    if ( addr < SMALL_SPACE_START )
	/* Sender is in small space.  Use receivers pagetab. */
	return SMALL_SPACE_START;
    else if ( addr < TCB_AREA )
    {
	/* Receiver is in small space. */
	smallid_t small = receiver->space->smallid ();
	return small.offset () + small.bytesize ();
    }
    else
	/* Neither sender or receiver in small space.  Use copy area. */
	return MEM_COPYAREA_END;
}

INLINE void check_limit(dword_t addr, dword_t limit, tcb_t *tcb)
{
    if ( addr >= limit )
	if ( limit <= SMALL_SPACE_START )
	    __asm__ __volatile__ (
		"int $3;"
		"jmp 1f;"
		".ascii \"KD# limit crosses 3GB boundary\";"
		"1:");
	else	
	    small_space_to_large (tcb->space);
}

#endif /* CONFIG_ENABLE_SMALL_AS */

class pgent_t 
{
 public:
    union {
	struct {
	    unsigned present		:1;
	    unsigned rw			:1;
	    unsigned privilege		:1;
	    unsigned write_through	:1;

	    unsigned cache_disabled	:1;
	    unsigned accessed		:1;
	    unsigned dirty		:1;
	    unsigned size		:1;

	    unsigned global		:1;
	    unsigned avail		:3;

	    unsigned base		:20;
	} pg;

	dword_t raw;
    } x;

#if defined(CONFIG_SMP)

# define SPACE_BASE(_x) ((dword_t)(_x) & ~(PAGE_SIZE*(CONFIG_SMP_MAX_CPU-1)))

# define SYNC_PGENT()							\
do {									\
    if ( pgsize == NUM_PAGESIZES-1 )					\
	for ( int i = 0; i < CONFIG_SMP_MAX_CPU; i++ )			\
	    *(dword_t*) ( SPACE_BASE(this) + (i*PAGE_SIZE)) = x.raw;	\
} while (0)

# define GET_MAPNODE()							\
    ((pgsize == 0) ?							\
	(*(dword_t *) ((dword_t) this + PAGE_SIZE)) :			\
	(*(dword_t *) ((dword_t) SPACE_BASE(this) +			\
			PAGE_SIZE*CONFIG_SMP_MAX_CPU)))

# define SET_MAPNODE(x)							\
    if ( pgsize == 0 )							\
	(*(dword_t *) ((dword_t) this + PAGE_SIZE)) = (x);		\
    else								\
	(*(dword_t *) ((dword_t) SPACE_BASE(this) +			\
			PAGE_SIZE*CONFIG_SMP_MAX_CPU)) = (x)

#else
# define SYNC_PGENT()
# define GET_MAPNODE()	(*(dword_t*) ((dword_t) this + PAGE_SIZE))
# define SET_MAPNODE(x)	(*(dword_t*) ((dword_t) this + PAGE_SIZE)) = (x)
#endif	    

    // Predicates

    inline int is_valid(space_t * s, int pgsize = 0)
	{ return x.raw & PAGE_VALID; }

    inline int is_writable(space_t * s, int pgsize = 0)
	{ return x.raw & PAGE_WRITABLE; }

    inline int is_subtree(space_t * s, int pgsize)
	{ return (pgsize == 1) && !(x.raw & PAGE_SUPER); }

    // Retrieval operations

    inline dword_t address(space_t * s, int pgsize)
	{ return x.raw & PAGE_MASK; }
	
    inline pgent_t * subtree(space_t * s, int pgsize)
	{ return (pgent_t *) phys_to_virt(x.raw & PAGE_MASK); }

    inline mnode_t * mapnode(space_t * s, int pgsize, dword_t vaddr)
	{ return (mnode_t *) (GET_MAPNODE() ^ vaddr);	}

    inline dword_t vaddr(space_t * s, int pgsize, mnode_t *map)
	{ return (GET_MAPNODE() ^ (dword_t) map); }

    // Change operations

    inline void clear(space_t * s, int pgsize)
	{ x.raw = 0; SYNC_PGENT(); SET_MAPNODE(0); }

    inline void make_subtree(space_t * s, int pgsize)
	{
	    x.raw = virt_to_phys((dword_t) kmem_alloc((dword_t) PAGE_SIZE*2))
		| PAGE_USER_TBITS;
	    SYNC_PGENT();
#if defined(CONFIG_ENABLE_SMALL_AS)
	    if (s->is_small ())
	    {
		dword_t idx = s->smallid ().pgdir_idx ();
		idx += ((dword_t) this & ~PAGE_MASK) >> 2;
		((dword_t *) phys_to_virt (get_current_pagetable ()))[idx] =
		    ((dword_t *) get_kernel_pagetable ())[idx] = x.raw;
	    }
#endif
	}

    inline void remove_subtree(space_t * s, int pgsize)
	{
	    kmem_free((ptr_t) phys_to_virt(x.raw & PAGE_MASK), PAGE_SIZE*2);
	    x.raw = 0;
	    SYNC_PGENT();
#if defined(CONFIG_ENABLE_SMALL_AS)
	    if (s->is_small ())
		__asm__ __volatile__ (
		    "int $3;"
		    "jmp 1f;"
		    ".ascii \"KD# removing subtree from small as\";"
		    "1:");
#endif
	}

    inline void set_entry(space_t * s, dword_t addr, dword_t writable,
			  int pgsize)
	{
	    x.raw = (addr & PAGE_MASK) | PAGE_USER_BITS |
		(writable ? PAGE_WRITABLE : 0) |
		(pgsize == 1 ? PAGE_SUPER : 0);
#if defined(CONFIG_GLOBAL_SMALL_SPACES)
	    if (s->is_small ())
		x.raw |= PAGE_GLOBAL;
#endif
	    SYNC_PGENT();
	}

    inline void set_writable(space_t * s, int pgsize)
	{ x.raw |= PAGE_WRITABLE; SYNC_PGENT(); }

    inline void set_readonly(space_t * s, int pgsize)
	{ x.raw &= ~PAGE_WRITABLE; SYNC_PGENT(); }

    inline void set_uncacheable(space_t * s, int pgsize)
	{ x.raw |= PAGE_CACHE_DISABLE; SYNC_PGENT(); }

    inline void set_unbufferable(space_t * s, int pgsize)
	{ }

    inline void set_mapnode(space_t * s, int pgsize, mnode_t *map,
			    dword_t vaddr)
	{ SET_MAPNODE((dword_t) map ^ vaddr); }

    // Movement

    inline pgent_t * next(space_t * s, dword_t num, int pgsize = 0)
	{ return this + num; }


    // Architecture specific

    inline void copy (space_t * s, int pgsize, pgent_t *from,
		      space_t * f_s, int f_pgsize)
	{ x.raw = from->x.raw; SYNC_PGENT(); }

    inline void set_address (space_t * s, int pgsize, dword_t addr)
	{ x.raw = (x.raw & ~PAGE_MASK) | (addr & PAGE_MASK) |
	      (pgsize == 1 ? PAGE_SUPER : 0); SYNC_PGENT(); }

    inline void set_flags (space_t * s, int pgsize, dword_t flags)
	{ x.raw = (x.raw & ~PAGE_FLAGS_MASK) | flags; SYNC_PGENT(); }

#if defined(CONFIG_SMP)
    inline void copy_nosync (space_t * s, int pgsize, pgent_t * from,
			     space_t * f_s, int f_pgsize)
	{ x.raw = from->x.raw; }

    inline void sync (space_t * s, int pgsize)
	{ SYNC_PGENT(); }
#endif

};

#define HAVE_ARCH_IPC_COPY
INLINE void ipc_copy(dword_t * to, dword_t * from, dword_t len)
{
    int i = len / 4;
    unsigned dummy;
    
    __asm__ __volatile__ ( 
      "rep movsl\n\t" 
      : "=S" (from), "=D" (to), "=c" (dummy)
      : "S" (from), "D" (to), "c" (i)
    );
    
    if ( len & 2 )
    {
	*((word_t *) to) = *((word_t *) from);
	to =   (dword_t *) ((word_t *) to + 1);
	from = (dword_t *) ((word_t *) from + 1);
    }

    if ( len & 1 )
	*(byte_t *) to = *(byte_t *) from;
}

#endif /* !ASSEMBLY */

#endif /* __X86__MEMORY_H__ */
