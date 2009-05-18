/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     r4000.lds
 * Description:   MIPS R4000 linker script.
 *                
 * @LICENSE@
 *                
 * $Id: r4000.lds,v 1.2 2001/11/22 15:51:36 skoglund Exp $
 *                
 ********************************************************************/

excp_vector	= 0x88000000;
kmem_start 	= 0x88010000;
kmem_end	= 0x88020000;

text_vaddr 	= 0x88002000;
text_paddr	= text_vaddr;

init_addr	= 0x88020000;


MEMORY
{
	mem_init	: ORIGIN = 0x88020000, LENGTH = 64K
	mem_excp	: ORIGIN = 0x88000000, LENGTH = 4K
	mem_kernel 	: ORIGIN = 0x88002000, LENGTH = 64K
	mem_tcbs	: ORIGIN = 0xC0000000, LENGTH = 256M
}
