/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     arm-booter/linker.lds
 * Description:   linker script
 *                
 * @LICENSE@
 *                
 * $Id: linker.lds,v 1.5 2001/12/09 04:06:30 ud3 Exp $
 *                
 ********************************************************************/

SECTIONS
{
	/DISCARD/ : { *(.comment) *(.debug*) *(.glue*) }
	.text : { *(.text) *(.data) *(.rodata) *(.bss) }
	. = ALIGN(0x10);
	.modules : { *(*) }
	_end = .;
}
