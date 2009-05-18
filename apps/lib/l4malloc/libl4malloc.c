/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     lib/l4malloc/libl4malloc.c
 * Description:   A simple malloc(3)/free(3) interface using the
 *                sigma0 protocol for allocating pages.  It is assumed
 *                that the current application's pager will NOT unmap
 *                any of the allocated memory.  Also note that this
 *                implementation can not release memory back to its
 *                pager since no such mechanism exists within the
 *                sigma0 protocol.
 *                
 * @LICENSE@
 *                
 * $Id: libl4malloc.c,v 1.2 2001/11/30 14:24:23 ud3 Exp $
 *                
 ********************************************************************/
#include <l4/l4.h>
#include <l4io.h>


#define PAGE_BITS	(12)
#define PAGE_SIZE	(1 << PAGE_BITS)
#define PAGE_MASK	(~(PAGE_SIZE-1))

/* Number of extra pages to allocate when allocating memory. */
#define EXTRA_ALLOC	1


/*
 * Structure used to hold list of allocated/free memory areas.
 */

struct node_t {
    char *base;
    int size;
    struct node_t *next;
    struct node_t *prev;
};


/* List of free memory areas. */
static struct node_t head;
static struct node_t tail;

/* List of allocated memory areas. */
static struct node_t a_head;
static struct node_t a_tail;

static int memory_in_pool;
static dword_t heap_start;
static dword_t heap_end;
static l4_threadid_t my_pager;



static void *malloc_get_pages(int num)
{
    l4_msgdope_t result;
    dword_t dummy;
    void *ret = (void *) heap_end;

    while ( num-- )
    {
	/* Allocate a page using the sigma0 protocol. */
	l4_ipc_call(my_pager, 0,
		    0xfffffffc, 0, 0,
		    (void *) l4_fpage(heap_end, PAGE_BITS, 1, 0).fpage,
		    &dummy, &dummy, &dummy,
		    L4_IPC_NEVER, &result);

	if ( result.md.error_code || (! result.md.fpage_received) )
	    enter_kdebug("malloc: could not grab more pages");

	heap_end += PAGE_SIZE;
    }

    return ret;
}


static void malloc_free_pages(void *ptr, int num)
{
}



/*
 * Function malloc (size)
 *
 *    Allocate size bytes worth of memory.  Return a pointer to the
 *    newly allocated memory, or NULL if memory could not be allocated.
 *
 */
void *malloc(int size)
{
    struct node_t *tmp, *tmp2;
    int pages_allocated;
    static int init = 0;

    /*
     * If this is the first call to malloc, we need to initialize the
     * list holding memory available for the process, and memory
     * allocated by the process.
     */
    if ( init == 0 )
    {
	extern long _end;
	dword_t dummy;
	l4_threadid_t preempter;

	head.next = &tail;
	tail.prev = &head;

	a_head.next = &a_tail;
	a_tail.prev = &a_head;

	heap_start = (((unsigned long) &_end) + PAGE_SIZE-1) & PAGE_MASK;
	heap_end = heap_start;
	memory_in_pool = 0;

	l4_thread_ex_regs(l4_myself(), ~0, ~0,
			  &preempter, &my_pager,
			  &dummy, &dummy, &dummy);

	init = 1;
    }

    /*
     * Traverse the list of available memory, searching for a block of
     * at least size bytes.
     */
    tmp = head.next;
    while( tmp != &tail )
    {
	if ( tmp->size >= size + sizeof(struct node_t) )
	{
	    /*
	     * We have to register all allocations since free() only
	     * give us a pointer to the loaction we should
	     * deallocate.  The allocation is registered in the
	     * a_list.
	     */
	    tmp2 = (struct node_t *) tmp->base;
	    tmp2->base = tmp->base + sizeof(struct node_t);
	    tmp2->size = size;

	    /* Update size and base of the node we took memory from. */
	    tmp->base += ( size + sizeof(struct node_t) );
	    tmp->size -= ( size + sizeof(struct node_t) );
      
	    /* Insert allocation info into head of aloocation list. */
	    tmp2->next = a_head.next;
	    tmp2->prev = &a_head;
	    a_head.next->prev = tmp2;
	    a_head.next = tmp2;

	    memory_in_pool -= ( size + sizeof(struct node_t) );

	    /*
	     * Return the allocated memory.
	     */
	    return (void *) tmp2->base;
	}
	tmp = tmp->next;
    }
  
    /*
     * We have to allocate more memory.
     */
    pages_allocated = ((size + sizeof(struct node_t) + PAGE_SIZE-1)
		       >> PAGE_BITS) + EXTRA_ALLOC;
    tmp = malloc_get_pages(pages_allocated);

    if ( tmp == NULL )
	return NULL;

    memory_in_pool += pages_allocated * PAGE_SIZE;
    
    /*
     * Check if the memory region we allocated should be coalesced
     * with the memory region of the last node in the memory list (we
     * keep entries sorted in ascending order).
     */
    if ( tail.prev->base + tail.prev->size == (char *) tmp )
    {
	/* Yes.  We should coalesce. */
	tail.prev->size += pages_allocated * PAGE_SIZE;
    }
    else
    {
  	/* No.  We can not coalesce.  Create a new node. */
	tmp->size = pages_allocated * PAGE_SIZE - sizeof(struct node_t);
	tmp->base = (char *) tmp + sizeof(struct node_t);
  
	/*
	 * Insert the allocated memory into the list of available
	 * memory for the process.
	 */
	tail.prev->next = tmp;
	tmp->prev = tail.prev;
	tmp->next = &tail;
	tail.prev = tmp;
    }

    /*
     * Do one recursive call.  We know that there is sufficient memory
     * allocated.
     */
    return malloc(size); 
}


/*
 * Function free (ptr)
 *
 *    Free the memory space pointed to by ptr.  
 *
 */
void free(void *ptr)
{
    struct node_t *tmp, *tmp2;


    /*
     * Search the allocated list for entries describing ptr.
     */
    tmp = a_head.next;
    while ( tmp != &a_tail )
    {
	if ( tmp->base == ptr )
	    break;
	tmp = tmp->next;
    }

    /* If ptr was not a previously allocated memory area, return. */
    if ( tmp == &a_tail )
	return;
    
    /* Remove the node from the list of allocated entries. */
    tmp->prev->next = tmp->next;
    tmp->next->prev = tmp->prev;

    /* 
     * Try to put the de-allocated memory back into the list of
     * available memory.
     */
    tmp2 = head.next;
    while ( tmp2 != &tail )
    {
	/* Check if we can coalesced tmp with tmp2 (before tmp2). */
	if ( tmp2->base == (tmp->base + tmp->size) )
	{
	    tmp2->base -= ( tmp->size + sizeof(struct node_t) );
	    tmp2->size += ( tmp->size + sizeof(struct node_t) );
	    return;
	} 

	/* Check if we can coalesced tmp with tmp2 (after tmp2). */
	if ( (tmp2->base + tmp2->size) == (char *) tmp )
	{
	    tmp2->size += tmp->size + sizeof(struct node_t);
	    return;
	}
    
	/* Check if we should insert tmp before tmp2. */
	if ( tmp2->base > tmp->base )
	{
	    tmp2->prev->next = tmp;
	    tmp->prev = tmp2->prev;
	    tmp->next = tmp2;
	    tmp2->prev = tmp;
	    return;
	}

#if 0
	/*
	 * We can not free allocated pages since there is no such
	 * mechanism in the sigma0 RPC protocol.
	 */
#error Unable to free pages using the sigma0 protocol.

	/*
	 * If there are more than 2 pages worth of memory in the pool,
	 * and we get across a node that is aligned on a page
	 * boundary, and is larger than 1 page, we free a part of that
	 * node.
	 */
	if ( memory_in_pool > (PAGE_SIZE * 2) &&
	     !((unsigned long) tmp2->base & ~PAGE_MASK) &&
	     tmp2->size > PAGE_SIZE )
	{
	    /* Make sure that the node still has some memory within it. */
	    int delsize = (tmp2->size - 1) >> PAGE_SHIFT;
	    struct node_t *tmp3;

	    memory_in_pool -= (delsize << PAGE_SHIFT);

	    /*
	     * Create the new shrinked node.
	     */
	    tmp3 = (struct node_t *) ((char *) tmp2 + delsize * PAGE_SIZE);
	    tmp3->base = (char *) tmp3 + sizeof(struct node_t);
	    tmp3->prev = tmp2->prev;
	    tmp3->next = tmp2->next;
	    tmp3->size = tmp2->size - delsize * PAGE_SIZE;

	    tmp2->prev->next = tmp3;
	    tmp2->next->prev = tmp3;

	    /*
	     * Free the pages removed from the pool.
	     */
	    malloc_free_pages(tmp2, delsize);
	    tmp2 = tmp3;
	}
#endif
	
	tmp2 = tmp2->next;
    }
 
    /*
     * Insert memory into end of list, creating a new node.
     */
    tmp2->prev->next = tmp;
    tmp->prev = tmp2->prev;
    tmp->next = tmp2;
    tmp2->prev = tmp;
}
