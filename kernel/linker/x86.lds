/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86.lds
 * Description:   Generic linkser script for x86.
 *                
 * @LICENSE@
 *                
 * $Id: x86.lds,v 1.18 2001/11/22 15:51:36 skoglund Exp $
 *                
 ********************************************************************/
ENTRY(_start)

/* see also include/x86/config.h */
KERNEL_OFFSET = 0xF0000000;

text_paddr	= 0x00100000 + SIZEOF_HEADERS;
text_vaddr 	= text_paddr + KERNEL_OFFSET;

SECTIONS
{
	.text text_vaddr : AT (ADDR(.text) - KERNEL_OFFSET)
	{
		*(.mb_header)
		*(.ipc-s)
		*(.ipc-c)
		*(.text)
		*(.spinlock)

		/* The user-level accessible sysexit trampoline code */
		. = ALIGN(4K); _start_utramp = .;
		*(.utramp)
		. = ALIGN(4K); _end_utramp = .;
	}
	.data . : AT (ADDR(.data) - KERNEL_OFFSET)
	{
		*(.data)
	}
	.kdebug . : AT (ADDR(.kdebug) - KERNEL_OFFSET)
	{
		*(.rodata .kdebug)
	}

	.bss . : AT (ADDR(.bss) - KERNEL_OFFSET)
	{
		*(.bss)
	}

	/* the .cpulocal section is a per CPU .bss -> NOLOAD */
	.cpulocal ALIGN(4K) (NOLOAD): AT (ADDR(.cpulocal) - KERNEL_OFFSET)
	{
		_start_cpu_local = .;
		*(.cpulocal)
		*(.kdebug-bss)
		_end_cpu_local = ALIGN(4K);
	}

	_end_text = ALIGN(4K);
	_end_text_p = (_end_text - KERNEL_OFFSET);
	_start_utramp_p = (_start_utramp - KERNEL_OFFSET);

	.init _end_text_p :
	{
		*(.init .roinit)
	}

	/* throw away all other sections */

	/*
	This one also throws away debugging info
	/DISCARD/ : { *(*) }
	*/

	/DISCARD/ : {
		*(.eh_frame)
		*(.note)
		*(.comment)
	}
}
