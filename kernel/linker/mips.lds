/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     mips.lds
 * Description:   General MIPS linker script.
 *                
 * @LICENSE@
 *                
 * $Id: mips.lds,v 1.3 2001/11/22 15:51:36 skoglund Exp $
 *                
 ********************************************************************/
ENTRY(_start)

tcbs 		= 0xE0000000;
tcb_start	= 0xC0000000;


SECTIONS
{
/*
	.init init_addr : AT (init_addr) {
		_start_init = .;
		*(.init)
		*(.roinit)
	} > mem_init
	. = ALIGN(4K);
	_end_init = .;
*/
	.excp excp_vector : AT (excp_vector)
	{
		*(.excp)

	}

	.text text_vaddr : AT (text_paddr)
	{
		*(.init)
		*(.roinit)
		*(.text)
		*(.rel.dyn)
		*(.reginfo)
		*(.mdebug)
	}
	.data . : AT (text_paddr + SIZEOF(.text))
	{
		.data = .;
		*(.data)
	}


	.kdebug . : AT (text_paddr + SIZEOF(.text) + SIZEOF(.data))
	{
		.rodata = .;
		*(.rodata)
		.kdebug = .;
		*(.kdebug)
	}

	.bss . : AT (text_paddr + SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.kdebug))
	{
		.bss = .;
		*(.bss)
	} = 0x00000000

	_end_text = .;
	/* . = ALIGN(0x400); */
	_end_text_p = (. - text_vaddr + text_paddr);

	/DISCARD/ :
	{
		*(.eh_frame)
		*(.note)
		*(.comment)
	}
}
