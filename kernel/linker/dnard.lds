/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     dnard.lds
 * Description:   DNARD specific includes for linker script.
 *                
 * @LICENSE@
 *                
 * $Id: dnard.lds,v 1.5 2001/12/11 20:19:03 ud3 Exp $
 *                
 ********************************************************************/

/*
 * Virtual address of exception vector page. REMEMBER TO CHANGE THIS
 * IF THE EXCEPTION VECTOR IS NO LONGER RELOCATED!
 */
EXCP_VADDR	= 0x00000000;

/*
 * Physical address of all the kernel code+data
 * Effectively: physical address of the exception vector.
 */
KERNEL_PHYS	= 0x08000000;

/*
 * Physical address of the init section.
 * Should be in a separate 1M section.
 */
INIT_PHYS	= 0x0A000000;
