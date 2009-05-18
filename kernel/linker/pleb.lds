/*********************************************************************
 *                
 * Copyright (C) 2000,  University of Karlsruhe
 *                
 * Filename:      pleb.lds
 * Description:   PLEB specific includes for linker script.
 *                
 ********************************************************************/

/*
 * Virtual address of exception vector page. REMEMBER TO CHANGE THIS
 * IF THE EXCEPTION VECTOR IS NO LONGER RELOCATED!
 */
EXCP_VADDR	= 0xFFFF0000;

/*
 * Physical address of all the kernel code+data
 * Effectively: physical address of the exception vector.
 */
KERNEL_PHYS	= 0xC8000000;

/*
 * Physical address of the init section.
 * Should be in a separate 1M section.
 */
INIT_PHYS	= 0xC8100000;
