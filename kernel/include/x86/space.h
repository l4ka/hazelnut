/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     x86/space.h
 * Description:   Address space definition for the x86
 *                
 * @LICENSE@
 *                
 * $Id: space.h,v 1.5 2001/12/04 20:15:46 uhlig Exp $
 *                
 ********************************************************************/
#ifndef __X86__SPACE_H__
#define __X86__SPACE_H__

#if defined(CONFIG_ENABLE_SMALL_AS)

/*
 * Small space IDs on x86 are contained within a single byte.  The
 * least significant bit which is set inidicates the size of the space
 * and the remaining most significant bits indicate the position.
 * Examples:
 *
 *	0001 1011	- Bit 0 set, space size = 4MB * 2^0 = 4MB
 *			  Position 0001101b = 13, 13th 4MB space
 *	0100 0100	- Bit 2 set, space size = 4MB * 2^2 = 16MB
 *			  Position 01000b = 8, 8th 16MB space
 */

#define MAX_SMALL_SPACES	128
#define SMALL_SPACE_INVALID	0xff

#if defined(CONFIG_SMALL_AREA_2GB)
# define SMALL_SPACE_MIN	16
# define SMALL_SPACE_SHIFT	(PAGEDIR_BITS+2)
#elif defined(CONFIG_SMALL_AREA_1GB)
# define SMALL_SPACE_MIN	8
# define SMALL_SPACE_SHIFT	(PAGEDIR_BITS+1)
#else
# define SMALL_SPACE_MIN	4
# define SMALL_SPACE_SHIFT	PAGEDIR_BITS
#endif


class smallid_t
{
    dword_t x;

public:

    inline dword_t value (void)
	{ return x; }

    inline void set_value (dword_t value)
	{ x = value & 0xff; }

    inline int is_valid (void)
	{ return (x != SMALL_SPACE_INVALID) && (x != 0); }

    inline dword_t size (void)
	{ return (1 << lsb (x)); }

    inline dword_t bytesize (void)
	{ return (size () << SMALL_SPACE_SHIFT); }

    inline dword_t num (void)
	{ return x >> (lsb (x) + 1); }

    inline dword_t idx (void)
	{ return (x >> (lsb (x) + 1)) << lsb (x); }

    inline dword_t pgdir_idx (void)
	{ return (idx () * (SMALL_SPACE_MIN >> 2))
	      + (SMALL_SPACE_START >> PAGEDIR_BITS); }

    inline dword_t offset (void)
	{ return (pgdir_idx () << PAGEDIR_BITS); }
};

#endif /* CONFIG_ENABLE_SMALL_AS */




/*
 * On x86, some words in the upper parts of the page directory are
 * defined to contain a small space id and the GDT entries which are
 * valid for this space.  No auxiliary data structures are needed.
 */

class space_t
{
    union {
#if !defined(CONFIG_SMP)
	// Normal page directory.
	dword_t pgdir[1024];
#else
	// multiple page directories
	dword_t pgdir[CONFIG_SMP_MAX_CPU][1024];
#endif

#if defined(CONFIG_ENABLE_SMALL_AS)
	// Page directory containg various variables.
	struct {
	    dword_t	pgdir[SPACE_ID_IDX];
	    smallid_t	smallid;
	    seg_desc_t	gdt_entry;
	    dword_t	pgdir_kern[1024-SPACE_ID_IDX-3];
	} space;
#endif /* CONFIG_ENABLE_SMALL_AS */
    } u;

public:
#if !defined(CONFIG_SMP)
    inline dword_t * pagedir_phys (void)
	{ return (dword_t *) this; }

    inline dword_t * pagedir (void)
	{ return (dword_t *) phys_to_virt (this); }

    inline pgent_t * pgent (int n)
	{ return (pgent_t *) phys_to_virt (&u.pgdir[n]); }
#else /* CONFIG_SMP */
    inline dword_t * pagedir_phys (int cpu = 0)
	{ return (dword_t *) &u.pgdir[cpu][0]; }

    inline dword_t * pagedir (int cpu = 0)
	{ return (dword_t *) phys_to_virt (&u.pgdir[cpu][0]); }

    inline pgent_t * pgent (int n, int cpu = 0)
	{ return (pgent_t *) phys_to_virt (&u.pgdir[cpu][n]); }
#endif /* CONFIG_SMP */

#if defined(CONFIG_ENABLE_SMALL_AS)
#define SMALLID  (*(phys_to_virt (&u.space.smallid)))
#define GDT_DESC (*(phys_to_virt (&u.space.gdt_entry)))

    inline smallid_t smallid (void)
	{ return SMALLID; }

    inline void set_smallid (dword_t value)
	{
	    SMALLID.set_value (value);
	    if (SMALLID.is_valid ())
		GDT_DESC.set_seg ((void *) SMALLID.offset (),
				  SMALLID.bytesize () - 1, 3, 3);
	}

    inline void set_smallid (smallid_t small)
	{ set_smallid (small.value ()); }
    
    inline int is_small (void)
	{ return SMALLID.is_valid (); }

#undef SMALLID
#undef GDT_DESC
#endif /* CONFIG_ENABLE_SMALL_AS */
};


INLINE space_t * get_current_space (void)
{
    space_t * s;
    __asm__ __volatile__ ("movl %%cr3, %0	\n" 
			  "andl %1, %0		\n"
			  :"=r" (s)
			  :"i"(PAGE_MASK));
#if defined(CONFIG_SMP)
    s = (space_t*)((dword_t)s & ~(PAGE_SIZE * CONFIG_SMP_MAX_CPU - 1));
#endif
    return s;
}

INLINE space_t * get_kernel_space (void)
{
    extern ptr_t kernel_ptdir_root;
    return (space_t*)virt_to_phys (kernel_ptdir_root);
}


#endif /* !__X86__SPACE_H__ */

