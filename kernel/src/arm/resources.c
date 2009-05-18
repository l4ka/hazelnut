/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     arm/resources.c
 * Description:   Thread resource management for ARM.
 *                
 * @LICENSE@
 *                
 * $Id: resources.c,v 1.1 2001/12/07 19:05:43 skoglund Exp $
 *                
 ********************************************************************/

#include <universe.h>
#include <thread.h>
#include <tracepoints.h>

void save_resources (tcb_t *current, tcb_t *dest)
{
    TRACEPOINT	(SAVE_RESOURCES, printf ("save_resources: tcb=%p  rc=%p\n",
					 current, current->resources));

    if (current->resources | TR_IPC_MEM)
    {
	ptr_t pdir = current->space->pagedir ();
	pdir[MEM_COPYAREA1 >> PAGEDIR_BITS] = 0;
	pdir[(MEM_COPYAREA1 >> PAGEDIR_BITS) + 1] = 0;

	pdir[MEM_COPYAREA2 >> PAGEDIR_BITS] = 0;
	pdir[(MEM_COPYAREA2 >> PAGEDIR_BITS) + 1] = 0;

	if ((dest == get_idle_tcb ()) || same_address_space (current, dest))
	    flush_tlb ();
    }
	
    current->resources = 0;
}

void purge_resources (tcb_t * tcb)
{
    TRACEPOINT (PURGE_RESOURCES, printf("purge_resources: tcb=%p\n", tcb));
}


void load_resources (tcb_t *current)
{
    TRACEPOINT (LOAD_RESOURCES, printf ("load_resources: tcb=%p  rc=%p\n",
					current, current->resources));
}


void init_resources (tcb_t * tcb)
{ 
}

void free_resources (tcb_t * tcb)
{
}

