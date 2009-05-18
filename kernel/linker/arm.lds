/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm.lds
 * Description:   Common ARM linker script.
 *                
 * @LICENSE@
 *                
 * $Id: arm.lds,v 1.18 2001/12/13 12:27:26 ud3 Exp $
 *                
 ********************************************************************/
ENTRY(_start)

excp_paddr = KERNEL_PHYS;
text_paddr = excp_paddr;	/* We count .excp as text */

KERNEL_VIRT = 0xFFFF0000;

SECTIONS
{	
	/*
	 * Exception vector + fast kernel code (i.e. short ipc).
	 */
	.excp EXCP_VADDR : AT (KERNEL_PHYS) {
		text_vaddr = .;
		*(.vect .fiqv .excp)
		. = ALIGN(4K);
	  	_end_excp = .;
  	}


	/*
	 * Kernel code and static data.
	 */
  	.text KERNEL_VIRT + SIZEOF(.excp) : AT (KERNEL_PHYS + SIZEOF(.excp)) {
		.text = .;
    		*(.text)
 		*(.data)
		*(.bss)
  	}
  	.kdebug . : AT (KERNEL_PHYS + SIZEOF(.excp) + SIZEOF(.text)) {
		*(.kdebug)
		.rodata = .;
		/* all the text strings should go here */
		*(.rodata)
		. = ALIGN(4K);
  	}

	_end_text = .;
	KERNEL_SIZE = _end_text - KERNEL_VIRT;
	. = KERNEL_PHYS + KERNEL_SIZE;

	. = ALIGN(16K);
	kernel_ptdir_p = .;
	. = . + 16K;
	kernel_pgtable_p = .;
	. = . + 1K;

	. = ALIGN(4K);
	_end_text_p = .;


	/*
	 * Initalization code.  Freed after initialization.
	 * (must reside in a single 1M section)
	 */
  	.init (INIT_PHYS) : AT (INIT_PHYS) { 
  		_start_init = .;
    		*(.init)
		*(.roinit)
		. = ALIGN(4K);
	  	_end_init = .;
	}

	/DISCARD/ :
	{
		*(.note)
		*(.comment)
		*(.glue*)
		*(COMMON)
	}
}

