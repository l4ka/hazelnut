/*********************************************************************
 *                
 * Copyright (C) 2000, 2001, 2002,  Karlsruhe University
 *                
 * File path:     arm/memory.h
 * Description:   ARM specific pagetable manipulation code.
 *                
 * @LICENSE@
 *                
 * $Id: memory.h,v 1.7 2002/06/07 17:04:26 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __ARM__MEMORY_H__
#define __ARM__MEMORY_H__

#include INC_ARCH(config.h)
#include INC_ARCH(mapping.h)


#define PAGE_BITS		12
#define PAGE_SIZE		(1 << PAGE_BITS)
#define PAGE_MASK		(~(PAGE_SIZE - 1))


#define PAGEDIR_BITS		20
#define PAGEDIR_SIZE		(1 << PAGEDIR_BITS)
#define PAGEDIR_MASK		(~(PAGEDIR_SIZE - 1))


/*
 * Generic page table configuration
 */

#define HW_NUM_PGSIZES		3
#define HW_VALID_PGSIZES	((1<<20) + (1<<16) + (1<<12))
extern dword_t hw_pgshifts[];



#define USER_DOMAIN	0x1


/*
**
** First level descriptors.
**
*/

/* First-Level Formats */

#define FLD_INVALID	0	/* Fault Entry */
#define FLD_PAGE	1	/* Page Entry (has 2nd-level discriptor) */
#define FLD_SECTION	2	/* Section Entry (1MB mapping) */
#define FLD_RESERVED	3	/* Reserved Entry */

/* First-Level Shifts */

#define FLD_SH_DOMAIN	5	/* Domain Shift, 4bits */
#define FLD_SH_AP	10	/* Access Permissions, 2bits */
#define FLD_SH_PTBA	10	/* Page Table Base Address, 22bits */
#define FLD_SH_SBA	20	/* Section Base Address, 12bits */

typedef struct {
    unsigned type	:2;
    unsigned		:30;
} fld_invalid_t;

typedef struct {
    unsigned type	:2;
    unsigned		:3;
    unsigned domain	:4;
    unsigned		:1;
    unsigned ptba	:22;
} fld_page_t;

typedef struct {
    unsigned type	:2;
    unsigned buf	:1;
    unsigned cache	:1;
    unsigned		:1;
    unsigned domain	:4;
    unsigned		:1;
    unsigned ap		:2;
    unsigned		:8;
    unsigned sba	:12;
} fld_section_t;

typedef fld_invalid_t fld_reserved_t;

typedef union fld_t {
    dword_t		init;
    fld_invalid_t	invalid;
    fld_page_t		page;
    fld_section_t	section;
    fld_reserved_t	reserved;
} fld_t;

#define fld_type(fld)	(((fld_t *) fld)->invalid.type)


/*
**
** Second level descriptors.
**
*/

/* Second-Level Formats */

#define SLD_INVALID	0	/* Fault Entry */
#define SLD_LARGE	1	/* Large Page Entry (64KB mapping) */
#define SLD_SMALL	2	/* Small Page Entry (4KB mapping) */
#define SLD_RESERVED	3	/* Reserved Entry */

/* Second-Level Shifts */

#define SLD_SH_AP0	4	/* 1st Sub-Page Access Permissions, 2bits */
#define SLD_SH_AP1	6	/* 2nd Sub-Page Access Permissions, 2bits */
#define SLD_SH_AP2	8	/* 3rd Sub-Page Access Permissions, 2bits */
#define SLD_SH_AP3	10	/* 4th Sub-Page Access Permissions, 2bits */
#define SLD_SH_LPBA	16	/* Large Page Base Address, 16bits */
#define SLD_SH_SPBA	12	/* Small Page Base ADdress, 20bits */

typedef struct {
    unsigned type	:2;
    unsigned		:30;
} sld_invalid_t;

typedef struct {
    unsigned type	:2;
    unsigned buf	:1;
    unsigned cache	:1;
    unsigned ap0	:2;
    unsigned ap1	:2;
    unsigned ap2	:2;
    unsigned ap3	:2;
    unsigned		:4;
    unsigned lpba	:16;
} sld_largepage_t;

typedef struct {
    unsigned type	:2;
    unsigned buf	:1;
    unsigned cache	:1;
    unsigned ap0	:2;
    unsigned ap1	:2;
    unsigned ap2	:2;
    unsigned ap3	:2;
    unsigned spba	:20;
} sld_smallpage_t;

typedef sld_invalid_t sld_reserved_t;

typedef union {
  dword_t		init;
  sld_invalid_t		invalid;
  sld_largepage_t	large;
  sld_smallpage_t	small;
  sld_reserved_t	reserved;
} sld_t;

#define sld_type(sld)	(((sld_t *) sld)->invalid.type)


/*
**
** MMU defines.
**
*/

#define FLD_SHIFT	(32 - 12) 	/* Bitshift for index into 1st level */
#define SLD_SHIFT	(32 - 20)	/* Bitshift for Index into 2nd level */
#define SLD_MASK	(0x000000FF)
#define FLD_SIZE	(1 << 14)	/* Size of 1st level PT in bytes */
#define SLD_SIZE	(1 << 10)	/* Size of 2nd level PT in bytes */
#define FLD_MAX		(FLD_SIZE >> 2)	/* Number of 1st level entries */
#define SLD_MAX		(SLD_SIZE >> 2)	/* Number of 2nd level entries */
#define LRG_ENTS	16		/* Number of 2nd level entries for
					   a single large page */

/* Caching Attributes */

#define CB_CACHABLE	(1 << 3)	/* Entry is Cachable */
#define CB_BUFFERABLE	(1 << 2)	/* Entry is Bufferable */
#define CB_IDEPENDENT	(1 << 4)	/* Implimentation Dependent */

/* Access Permitions */

#define AP_RESTRICTED	0	/* Supervisor: Restricted, User: Restricted */
#define AP_NOACCESS	1	/* Supervisor: Read/Write, User: No Access */
#define AP_READACCESS	2	/* Supervisor: Read/Write, User: Read Only */
#define AP_ALLACCESS	3	/* Supervisor: Read/Write, User: Read/Write */

/* Domain Access Types */

#define DAT_NOACCESS	0	/* No Access (acceses cause domain fault) */
#define DAT_CLIENT	1	/* Client Domain (access permitions checked) */
#define DAT_RESERVED	2	/* Reserved Domain Type */
#define DAT_MANAGER	3	/* Manager Domain (no access checks) */

/* Fault Types */

#define TRANSLATION_FAULT_S	0x00000005
#define TRANSLATION_FAULT_P	0x00000007
#define DOMAIN_FAULT_S		0x00000009
#define DOMAIN_FAULT_P		0x0000000B
#define PERMISSION_FAULT_S	0x0000000D
#define PERMISSION_FAULT_P	0x0000000F

#define STATUS_MASK		0x0000000F


/*
**
** Macros
**
*/

/* Virtual Address to FLD Index */
#define vaddr_to_fld(vaddr) ((vaddr) >> FLD_SHIFT)

/* FLD Index to Virtual Address */
#define fld_to_vaddr(fld) ((fld) << FLD_SHIFT)

/* Physical Address to Section Base Address */
#define paddr_to_sba(paddr) ((paddr) >> FLD_SH_SBA)

/* Section Base Address to Physical Address */
#define sba_to_paddr(sba) ((sba) << FLD_SH_SBA)

/* Page Table Base Address to Physical Address */
#define ptba_to_paddr(ptba) ((ptba) << FLD_SH_PTBA)

/* Physical Address to Page Table Base Address */
#define paddr_to_ptba(paddr) ((paddr) >> FLD_SH_PTBA)

/* Virtual Address to SLD Index */
#define vaddr_to_sld(vaddr) (SLD_MASK & ((vaddr) >> SLD_SHIFT))

/* SLD Index to Partial Virtual Address */
#define sld_to_vaddr(sld) ((sld) << SLD_SHIFT)

/* Physical Address to Large Page Base Address */
#define paddr_to_lpba(paddr) ((paddr) >> SLD_SH_LPBA)

/* Large Page Base Address to Physical Address */
#define lpba_to_paddr(lpba) ((lpba) << SLD_SH_LPBA)

/* Physical Address to Small Page Base Address */
#define paddr_to_spba(paddr) ((paddr) >> SLD_SH_SPBA)

/* Small Page Base Address to Physical Address */
#define spba_to_paddr(spba) ((spba) << SLD_SH_SPBA)

#if 0
/* Physical Address to Virtual Address (bank0 and kernel only) */
#define paddr_to_vaddr0(paddr) (0xF0000000 | ((paddr) - DRAM0_PBASE))

/* Virtual Address to Physical Address (bank0 and kernel only) */
#define vaddr_to_paddr0(vaddr) ((0x0FFFFFFF & (vaddr)) + DRAM0_PBASE)


/* Contigous physical address to real physical address */
#define cpaddr_to_paddr(addr)					\
({								\
    dword_t off;						\
								\
    off = ((dword_t) (addr) - (dword_t) cpaddr);		\
    off = ((off >> MEM_BANKSHIFT) << DRAM_BANKSHIFT) +		\
	DRAM0_PBASE + (off & ((1 << MEM_BANKSHIFT)-1));		\
    off;							\
})

/* Real physical address to contigous physical address */
#define paddr_to_cpaddr(addr)					\
({								\
    dword_t off;						\
								\
    off = ((dword_t) (addr) - DRAM0_PBASE);			\
    off = ((off >> DRAM_BANKSHIFT) << MEM_BANKSHIFT) +		\
	(dword_t) cpaddr + (off & ((1 << MEM_BANKSHIFT)-1));	\
    off;							\
})

#if 0
#define virt_to_phys(x) cpaddr_to_paddr(x)
#else
#define virt_to_phys(x) ({ typeof(x) _x = (typeof(x)) (cpaddr_to_paddr((dword_t) (x))); _x;})
#endif

#if 0
#define phys_to_virt(x) paddr_to_cpaddr(x)
#else
#define phys_to_virt(x) ({ typeof(x) _x = (typeof(x)) (paddr_to_cpaddr((dword_t) (x))); _x;})
#endif

#else

typedef union {
    dword_t r;
    struct {
	dword_t o:DRAM_BANKSIZE_BITS;		/* offset */
	dword_t s:32-DRAM_BANKSIZE_BITS;	/* rest   */
    } x;
} _addr_t;


#define _p2v_(_x_) ({_addr_t y = (_addr_t) {r : ((dword_t) (_x_)) - DRAM_BASE}; y.x.s >>= (DRAM_BANKDIST_BITS-DRAM_BANKSIZE_BITS); y.r + cpaddr;})
#define _v2p_(_x_) ({_addr_t y = (_addr_t) {r : ((dword_t) (_x_)) - cpaddr}; y.x.s <<= (DRAM_BANKDIST_BITS-DRAM_BANKSIZE_BITS); y.r + DRAM_BASE;})

#define phys_to_virt(x) ({ typeof(x) _x = (typeof(x)) (_p2v_((dword_t) (x))); _x;})
#define virt_to_phys(x) ({ typeof(x) _x = (typeof(x)) (_v2p_((dword_t) (x))); _x;})
#endif


/*
 * Macro get_ttb ()
 *
 *    Return Translation Table Base.
 *
 */
#define get_ttb()						\
({								\
    dword_t x;							\
								\
    __asm__ __volatile__ (					\
	"	mrc	p15, 0, %0, c2, c0, 0	\n"		\
	:"=r"(x));						\
								\
    (x & ~((1<<14) - 1));					\
})


/*
 * Macro set_ttb (x)
 *
 *    Set Translation Table Base to `x'.
 *
 */
#define set_ttb(x)						\
({								\
    __asm__ __volatile__ (					\
	"	mcr	p15, 0, %0, c2, c0, 0	\n"		\
	:							\
	:"r"(x));						\
})

INLINE ptr_t get_current_pagetable()
{
    return (ptr_t) get_ttb();
}


extern dword_t* kernel_ptdir;
extern fld_t kernel_ptdir_p[];
extern sld_t kernel_pgtable_p[];
extern ptr_t __zero_page;

INLINE ptr_t get_kernel_pagetable()
{
    return kernel_ptdir;
}


#define PGSIZE_1MB	(HW_NUM_PGSIZES-1)
#define PGSIZE_64KB	(HW_NUM_PGSIZES-2)
#define PGSIZE_4KB	(HW_NUM_PGSIZES-3)
#define PGSIZE_1KB	(HW_NUM_PGSIZES-4)

#define NUM_PAGESIZES		4

#define PAGE_SMALL_BITS		12
#define PAGE_SMALL_SIZE		(1 << PAGE_SMALL_BITS)
#define PAGE_SMALL_MASK		(~(PAGE_SMALL_SIZE - 1))

#define PAGE_LARGE_BITS		16
#define PAGE_LARGE_SIZE		(1 << PAGE_LARGE_BITS)
#define PAGE_LARGE_MASK		(~(PAGE_LARGE_SIZE - 1))

#define PAGE_SECTION_BITS	20
#define PAGE_SECTION_SIZE	(1 << PAGE_SECTION_BITS)
#define PAGE_SECTION_MASK	(~(PAGE_SECTION_SIZE - 1))

#define PAGE_TABLE_BITS		10
#define PAGE_TABLE_MASK		(~((1 << PAGE_TABLE_BITS) - 1))

#define PAGE_TYPE_MASK		0x03

#define PAGE_AP_RESTRICTED	0	/* S: Restricted, U: Restricted */
#define PAGE_AP_NOACCESS	1	/* S: Read/Write, U: No Access */
#define PAGE_AP_READONLY	2	/* S: Read/Write, U: Read Only */
#define PAGE_AP_WRITABLE	3	/* S: Read/Write, U: Read/Write */

/*
 * This bit marks the 64K page valid even if the type field is 0.  It
 * indicates that the 64K page has a subtree (i.e. valid 4K pages).
 */
#define PAGE_64K_VALID		0x80000000


INLINE dword_t pgdir_idx(dword_t addr)
{
    return addr >> PAGE_SECTION_BITS;
}

INLINE dword_t pgtab_idx(dword_t addr)
{
    /* Assumption: 4k pages only */
    return (addr >> PAGE_SMALL_BITS) & ~(PAGE_SMALL_MASK >> 2);
}






class pgent_t 
{
 public:
    union {
	struct {
	    unsigned type	:2;	// 0
	    unsigned buf	:1;	// 2
	    unsigned cache	:1;	// 3
	    unsigned		:1;	// 4
	    unsigned domain	:4;	// 5
	    unsigned		:1;	// 9
	    unsigned ap		:2;	// 10
	    unsigned		:8;	// 12
	    unsigned sba	:12;	// 20
	} section;

	struct {
	    unsigned type	:2;	// 0
	    unsigned		:3;	// 2
	    unsigned domain	:4;	// 5
	    unsigned		:1;	// 9
	    unsigned base	:22;	// 10
	} table;

	struct {
	    unsigned type	:2;	// 0
	    unsigned buf	:1;	// 2
	    unsigned cache	:1;	// 3
	    unsigned ap0	:2;	// 4
	    unsigned ap1	:2;	// 6
	    unsigned ap2	:2;	// 8
	    unsigned ap3	:2;	// 10
	    unsigned		:4;	// 12
	    unsigned lpba	:16;	// 16
	} large;

	struct {
	    unsigned type	:2;	// 0
	    unsigned buf	:1;	// 2
	    unsigned cache	:1;	// 3
	    unsigned ap0	:2;	// 4
	    unsigned ap1	:2;	// 6
	    unsigned ap2	:2;	// 8
	    unsigned ap3	:2;	// 10
	    unsigned spba	:20;	// 12
	} small;

	dword_t raw;
    } x;

#define __MNOFF ((pgsize == PGSIZE_1MB) ? 16384 : 1024)

#define GET_MAPNODE()	(*(dword_t*) ((dword_t)this + __MNOFF))
#define SET_MAPNODE(x)	(*(dword_t*) ((dword_t)this + __MNOFF)) = (x)

    // Predicates

    inline dword_t is_valid (space_t * s, dword_t pgsize)
	{
	    dword_t type = (x.raw & PAGE_TYPE_MASK);
	    return type == 1 || type == 2 ||
		(pgsize == PGSIZE_64KB && type == 0 &&
		 (x.raw & PAGE_64K_VALID));
	}

    inline dword_t is_writable (space_t * s, dword_t pgsize)
	{
	    return x.section.ap == PAGE_AP_WRITABLE;
	}

    inline dword_t is_subtree (space_t * s, dword_t pgsize)
	{
	    dword_t type = (x.raw & PAGE_TYPE_MASK);
	    return (pgsize == PGSIZE_1MB && type == 1) ||
		(pgsize == PGSIZE_64KB &&
		 (type == 2 || (type == 0 && (x.raw & PAGE_64K_VALID))));
	}

    // Retrieval operations

    inline dword_t address (space_t * s, dword_t pgsize)
	{
	    return pgsize == PGSIZE_1MB ?
		(x.raw & PAGE_SECTION_MASK) :
		(x.raw & PAGE_SMALL_MASK);
	}
	
    inline pgent_t * subtree (space_t * s, dword_t pgsize)
	{
	    return pgsize == PGSIZE_1MB ?
		(pgent_t *) phys_to_virt(x.raw & PAGE_TABLE_MASK) :
		this;
	}

    inline mnode_t * mapnode (space_t * s, dword_t pgsize, dword_t vaddr)
	{ return (mnode_t *) (GET_MAPNODE() ^ vaddr);	}

    inline dword_t vaddr (space_t * s, dword_t pgsize, mnode_t *map)
	{ return (GET_MAPNODE() ^ (dword_t) map); }

    // Change operations

    inline void clear (space_t * s, dword_t pgsize)
	{
	    if ( pgsize == PGSIZE_4KB )
		x.raw = PAGE_64K_VALID;
	    else {
		x.raw = 0;
		if ( pgsize == PGSIZE_64KB )
		    for ( int i = 1; i < 16; i++ )
			this[i].x.raw = 0;
	    }
	    SET_MAPNODE(0);
	}

    inline void make_subtree (space_t * s, dword_t pgsize)
	{
	    if ( pgsize == PGSIZE_1MB ) {
		x.raw = (dword_t) virt_to_phys(kmem_alloc(1024*2)) | 0x01;
	    } else {
		for ( int i = 0; i < 16; i++ ) {
		    this[i].x.raw = PAGE_64K_VALID;
		}
	    }
	}

    inline void remove_subtree (space_t * s, dword_t pgsize)
	{
	    if ( pgsize == PGSIZE_64KB ) {
		for ( int i = 0; i < 16; i++ )
		    this[i].x.raw = 0;
	    } else {
		kmem_free((ptr_t) phys_to_virt(x.raw & PAGE_TABLE_MASK),
			  1024*2);
		x.raw = 0;
	    }
	}

    inline void set_entry (space_t * s, dword_t addr,
			   dword_t writable, dword_t pgsize)
	{
	    if ( pgsize == PGSIZE_1MB ) {
		x.raw = (addr & PAGE_SECTION_MASK) | 0x02;
		x.section.ap = writable ? PAGE_AP_WRITABLE : PAGE_AP_READONLY;
	    } else {
		x.raw = pgsize == PGSIZE_64KB ?
		    (addr & PAGE_LARGE_MASK) | 0x1 :
		    (addr & PAGE_SMALL_MASK) | 0x2;
		x.small.ap0 = x.small.ap1 = x.small.ap2 = x.small.ap3 =
		    writable ? PAGE_AP_WRITABLE : PAGE_AP_READONLY;
		if ( pgsize == PGSIZE_64KB )
		    for ( int i = 1; i < 16; i++ )
			this[i].x.raw = x.raw;
	    }
	}

    inline void set_access_bits (space_t * s, dword_t pgsize, dword_t ap, int c, int b)
	{
	    if (pgsize == PGSIZE_1MB) {
		x.section.ap = ap;
		x.section.cache = c;
		x.section.buf = b;
	    } else {
		x.small.ap0 = x.small.ap1 = x.small.ap2 = x.small.ap3 = ap;
		x.section.cache = c;
		x.section.buf = b;
		if (pgsize == PGSIZE_64KB)
		    for (int i = 1; i < 16; i++)
			this[i].x.raw = x.raw;
	    }
	}

    inline void set_writable (space_t * s, dword_t pgsize)
	{
	    if ( pgsize == PGSIZE_1MB ) {
		x.section.ap = PAGE_AP_WRITABLE;
	    } else {
		x.small.ap0 = x.small.ap1 = x.small.ap2 = x.small.ap3 =
		    PAGE_AP_WRITABLE;
		if ( pgsize == PGSIZE_64KB )
		    for ( int i = 1; i < 16; i++ )
			this[i].x.raw = x.raw;
	    }
	}

    inline void set_readonly (space_t * s, dword_t pgsize)
	{
	    if ( pgsize == PGSIZE_1MB ) {
		x.section.ap = PAGE_AP_READONLY;
	    } else {
		x.small.ap0 = x.small.ap1 = x.small.ap2 = x.small.ap3 =
		    PAGE_AP_READONLY;
		if ( pgsize == PGSIZE_64KB )
		    for ( int i = 1; i < 16; i++ )
			this[i].x.raw = x.raw;
	    }
	}

    inline void set_uncacheable (space_t * s, int pgsize)
	{ }

    inline void set_unbufferable (space_t * s, int pgsize)
	{ }

    inline void set_mapnode (space_t * s, dword_t pgsize,
			     mnode_t *map, dword_t vaddr)
	{ SET_MAPNODE((dword_t) map ^ vaddr); }

    // Movement

    inline pgent_t * next (space_t * s, dword_t num, dword_t pgsize)
	{
	    return pgsize == PGSIZE_64KB ?
		this + (16 * num) : this + num;
	}

};


#endif /* !__ARM__MEMORY_H__ */
