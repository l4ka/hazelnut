/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     kmemory.c
 * Description:   Kernel memory allocator.
 *                
 * @LICENSE@
 *                
 * $Id: kmemory.c,v 1.27 2001/11/22 15:10:32 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

#ifndef CONFIG_HAVE_ARCH_KMEM

#include <init.h>




dword_t *kmem_free_list = NULL;
dword_t free_chunks = 0;

/* kernel memory allocator semaphore */
DEFINE_SPINLOCK(kmem_spinlock);

/* the stupid version */
void kmem_free(ptr_t address, dword_t size)
{
    ptr_t p;
    ptr_t prev, curr;

    spin_lock(&kmem_spinlock);

    //printf("kmem_free(%p, %x)\n", address, size);

    for (p = address;
	 p < ((ptr_t)(((dword_t)address) + size - KMEM_CHUNKSIZE));
	 p = (ptr_t) *p)
	*p = (dword_t) p + KMEM_CHUNKSIZE; /* write next pointer */
    
    /* find the place to insert */
    for (prev = (ptr_t) &kmem_free_list, curr = kmem_free_list;
	 curr && (address > curr);
	 prev = curr, curr = (dword_t*) *curr);
    /* and insert there */
    //printf("prev %p/%p, curr %p, p: %p, \n", prev, *prev, curr, p); 
    *prev = (dword_t) address; *p = (dword_t) curr;

    /* update counter */
    free_chunks += (size/KMEM_CHUNKSIZE);

    spin_unlock(&kmem_spinlock);
}

#if defined(NEED_FARCALLS)
#include INC_ARCH(farcalls.h)
#endif


dword_t kmem_end   =  0;
dword_t kmem_start = ~0;

void kmem_init()
{

/*
 * From linker scripts.
 */
    extern dword_t text_paddr[];
    extern dword_t _end_text_p[];

#if defined(CONFIG_DEBUG_SANITY)
    if (kmem_end < kmem_start)
	printf("%s(%x,%x) is a silly config\n", __FUNCTION__, kmem_start, kmem_end);
#endif

#if defined(CONFIG_DEBUG_TRACE_MISC)
    printf("%s(%x,%x)\n", __FUNCTION__, kmem_start, kmem_end);
#endif

    kmem_free((ptr_t) kmem_start, kmem_end-kmem_start);
    kernel_info_page.reserved_mem0_low  = (dword_t) text_paddr;
    kernel_info_page.reserved_mem0_high = (dword_t) _end_text_p;
    kernel_info_page.reserved_mem1_low  = virt_to_phys((dword_t) kmem_start);
    kernel_info_page.reserved_mem1_high = (virt_to_phys((dword_t) kmem_end)  + (PAGE_SIZE-1)) & PAGE_MASK;
}

/* the stupid version */
ptr_t kmem_alloc(dword_t size)
{
    spin_lock(&kmem_spinlock);

    dword_t*	prev;
    dword_t*	curr;
    dword_t*	tmp;
    dword_t	i;
    
    //printf("%s(%d) kfl: %p\n", __FUNCTION__, size, kmem_free_list);
    for (prev = (dword_t*) &kmem_free_list, curr = kmem_free_list;
	 curr;
	 prev = curr, curr = (dword_t*) *curr)
    {
//	printf("curr=%x\n", curr);
	/* properly aligned ??? */
	if (!((dword_t) curr & (size - 1)))
	{
//	    printf("%s(%d):%d: curr=%x\n", __FUNCTION__, size, __LINE__, curr);
	    tmp = (dword_t*) *curr;
	    for (i = 1; tmp && (i < (size / KMEM_CHUNKSIZE)); i++)
	    {
//		printf("%s():%d: i=%d, tmp=%x\n", __FUNCTION__, __LINE__, i, tmp);
		if ((dword_t) tmp != ((dword_t) curr + KMEM_CHUNKSIZE*i))
		{
//		    printf("skip: %x\n", curr);
		    tmp = 0;
		    break;
		};
		tmp = (dword_t*) *tmp;
	    }
	    if (tmp)
	    {
		/* dequeue */
		*prev = (dword_t) tmp;

		/* zero the page */
		for (dword_t i = 0; i < (size / sizeof(dword_t)); i++)
		    curr[i] = 0;

		/* update counter */
		free_chunks -= (size/KMEM_CHUNKSIZE);

		/* successful return */
		spin_unlock(&kmem_spinlock);
		//printf("kmalloc: %x->%p (%d), kfl: %p\n", size, curr, free_chunks, kmem_free_list);
		return curr;
	    }
	}
    }
#if 0
    dword_t * tmp1 = kmem_free_list;
    while(tmp1) {
	printf("%p -> ");
	tmp1 = (dword_t*)*tmp1;
    }
#endif
    enter_kdebug("kmem_alloc: out of kernel memory");
    spin_unlock(&kmem_spinlock);
    return NULL;
}

#endif /* !CONFIG_HAVE_ARCH_KMEM */

