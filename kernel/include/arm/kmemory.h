/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/kmemory.h
 * Description:   ARM specific kernel memory allocator.
 *                
 * @LICENSE@
 *                
 * $Id: kmemory.h,v 1.4 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __ARM__KMEMORY_H__
#define __ARM__KMEMORY_H__

void panic(const char *msg) L4_SECT_KDEBUG;


/*
 * Prototypes.
 */
ptr_t kmem_alloc_1k(void);
ptr_t kmem_alloc_4k(void);
ptr_t kmem_alloc_16k(void);
void kmem_free_1k(ptr_t);
void kmem_free_4k(ptr_t);
void kmem_free_16k(ptr_t);


/*
 * Macro kmem_alloc (size)
 *
 *    Allocate an aligned buffer of the indicated size (1K, 4K, or
 *    16K).  Gcc should be able to optimize away the if's.
 *
 */
#define kmem_alloc(size)					\
({								\
    ptr_t r;							\
								\
    if ( size == 1024 )						\
	r = kmem_alloc_1k();					\
    else if ( size == 4096 )					\
	r = kmem_alloc_4k();					\
    else if ( size == 16384 )					\
	r = kmem_alloc_16k();					\
    else							\
	panic("kmem_alloc() -- unsopported block size.\n");	\
								\
    r;								\
})


/*
 * Function kmem_free (addr, size)
 *
 *    Free the indicated buffer (of 1K, 4K, or 16K size).
 *
 */
INLINE void kmem_free(ptr_t addr, dword_t size)
{
    if ( size == 1024 )
	return kmem_free_1k(addr);
    else if ( size == 4096 )
	return kmem_free_4k(addr);
    else if ( size == 16384 )
	return kmem_free_16k(addr);
    else
	panic("kmem_free() -- unsopported block size.\n");
}


/*
 * Masks and shift numbers for kmem pages.
 */
#define KMEM_16K_MASK	0x3fff
#define KMEM_4K_MASK	0x0fff
#define KMEM_1K_MASK	0x03ff

#define KMEM_16K_SHIFT	14
#define KMEM_4K_SHIFT	12
#define KMEM_1K_SHIFT	10


/*
 * Amount of initial memory to allocate to the different freelists.
 */
#define KMEM_INIT_4K_RATIO	1 / 1
#define KMEM_INIT_1K_RATIO	0


#endif /* !__ARM__KMEMORY_H__ */
