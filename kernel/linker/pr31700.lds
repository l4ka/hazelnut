/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     pr31700.lds
 * Description:   MIPS pr31700 linker script.
 *                
 * @LICENSE@
 *                
 * $Id: pr31700.lds,v 1.2 2001/11/22 15:51:36 skoglund Exp $
 *                
 ********************************************************************/

OUTPUT_FORMAT("elf32-littlemips")

excp_vector	= 0x80000000;
kmem_start 	= 0x80010000;
kmem_end	= 0x80020000;

text_vaddr 	= 0x80102000;
text_paddr	= text_vaddr;

init_addr	= 0x80020000;

