/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     brutus.lds
 * Description:   Brutus specific includes for linker script.
 *                
 * @LICENSE@
 *                
 * $Id: brutus.lds,v 1.4 2001/11/22 15:51:36 skoglund Exp $
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
KERNEL_PHYS	= 0xC0000000;

/*
 * Physical address of the init section.
 * Should be in a separate 1M section.
 */
INIT_PHYS	= 0xC0100000;
