/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips/memory.h
 * Description:   MIPS specific page table manipulation code.
 *                
 * @LICENSE@
 *                
 * $Id: memory.h,v 1.6 2001/11/22 14:56:36 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __MIPS_MEMORY_H__
#define __MIPS_MEMORY_H__

#include INC_ARCH(config.h)

#define PAGEDIR_BITS		22
#define PAGEDIR_SIZE		(dword_t)(1 << PAGEDIR_BITS)
#define PAGEDIR_MASK		(dword_t)(~(PAGEDIR_SIZE - 1))

#define PAGE_BITS		12
#define PAGE_SIZE		(1 << PAGE_BITS)
#define PAGE_MASK		(~(PAGE_SIZE - 1))

#define PAGE_UNCACHED		(2 << 5)
#define PAGE_CACHE_NONCOHERENT	(3 << 5)
#define PAGE_CACHE_EXCLUSIVE	(4 << 5)
#define PAGE_CACHE_SHARABLE	(5 << 5)
#define PAGE_CACHE_UPDATE	(6 << 5)

#define PAGE_READONLY		0x00
#define PAGE_WRITABLE		0x10

#define PAGE_GLOBAL		0x04
#define PAGE_VALID		0x08

#define PT_ZEROPAGE		(PAGE_VALID | PAGE_UNCACHED)
#define PT_TCB			(PAGE_VALID | PAGE_WRITABLE | PAGE_GLOBAL | PAGE_UNCACHED)
#define PT_KERNELMEM		(PAGE_VALID | PAGE_WRITABLE | PAGE_GLOBAL | PAGE_UNCACHED)
#define PT_SIGMA0		(PAGE_VALID | PAGE_WRITABLE | PAGE_CACHE_SHARABLE)

#define PAGE_USER_BITS		(PAGE_VALID)

#ifndef ASSEMBLY

/* declared in mips/init.c */
extern ptr_t __zero_page;


#define virt_to_phys(x) \
({ typeof(x) _x = (typeof(x))((dword_t) (x) - KERNEL_OFFSET); _x;})

#define phys_to_virt(x) \
({ typeof(x) _x = (typeof(x))((dword_t) (x) + KERNEL_OFFSET); _x;})

INLINE void set_ptab_entry(ptr_t ptab, ptr_t virt, dword_t val)
{
    ptab[(((dword_t)virt) >> 12) & 0x3ff] = val;
}

INLINE void set_pdir_entry(ptr_t pdir, ptr_t virt, dword_t val)
{
    pdir[((dword_t)virt) >> 22] = val;
}

INLINE dword_t get_pagefault_address()
{
    return 0;
}

INLINE ptr_t get_kernel_pagetable()
{
    extern dword_t  __pdir_root[];
    return &__pdir_root[0];
}

INLINE ptr_t get_current_pagetable()
{
    return current_page_table;
}

INLINE void set_current_pagetable(ptr_t pdir)
{
    current_page_table = pdir;
}

void init_paging();

class pgent_t 
{
 public:
    union {
	struct {
	    unsigned zero		:2;
	    unsigned global		:1;
	    unsigned valid		:1;
	    unsigned rw			:1;
	    unsigned cache		:3;
	    unsigned base		:24;
	} pg;

	dword_t raw;
    } x;

    // Predicates

    inline dword_t is_valid(dword_t pgsize)
	{ return x.raw & PAGE_VALID; }

    inline dword_t is_writable(dword_t pgsize)
	{ return x.raw & PAGE_WRITABLE; }

    inline dword_t is_subtree(dword_t pgsize)
	{ return 1; }

    // Retrieval operations

    inline dword_t address(dword_t pgsize)
	{ return x.raw & PAGE_MASK; }
	
    inline pgent_t * subtree(dword_t pgsize)
	{ return (pgent_t *) phys_to_virt(x.raw & PAGE_MASK); }

    // Change operations

    inline void clear(dword_t pgsize)
	{ x.raw = 0; }

    inline void make_subtree(dword_t pgsize)
	{ x.raw = virt_to_phys((dword_t) kmem_alloc((dword_t) PAGE_SIZE))
	      | PAGE_VALID; 
	}

    inline void remove_subtree(dword_t pgsize)
	{
	    kmem_free((ptr_t) phys_to_virt(x.raw & PAGE_MASK), PAGE_SIZE);
	    x.raw = 0;
	}
    inline void split(dword_t pgsize)
	{
#if 0
	    pgent_t *n = (pgent_t *) kmem_alloc((dword_t) PAGE_SIZE);
	    pgent_t t;

	    x.raw &= ~PAGE_SUPER;
	    t.x.raw = x.raw;

	    for ( int i = 0; i < 1024; i++ )
	    {
		n[i].x.raw = t.x.raw;
		t.x.pg.base++;
	    }

	    x.raw = (dword_t) virt_to_phys(n) | PAGE_USER_BITS | PAGE_WRITABLE; 
#endif
	}
    inline void set_entry(dword_t addr, dword_t writable, dword_t pgsize)
	{ x.raw = (addr & PAGE_MASK) | PAGE_USER_BITS |
	      (writable ? PAGE_WRITABLE : 0);
	}

    inline void set_writable(dword_t pgsize)
	{ x.raw |= PAGE_WRITABLE; }

    inline void set_readonly(dword_t pgsize)
	{ x.raw &= ~PAGE_WRITABLE; }

    // Movement

    inline pgent_t * next(dword_t num, dword_t pgsize)
	{ return this + num; }

};


#endif /* ASSEMBLY */

#endif /* __MIPS_MEMORY_H__ */
