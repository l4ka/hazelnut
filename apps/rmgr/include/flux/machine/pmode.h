/* 
 * Copyright (c) 1996-1994 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 */
#ifndef _FLUX_X86_PMODE_H_
#define _FLUX_X86_PMODE_H_

#include <flux/inline.h>
#include <flux/x86/proc_reg.h>



/* Enter protected mode on x86 machines.
   Assumes:
	* Running in real mode.
	* Interrupts are turned off.
	* A20 is enabled (if on a PC).
	* A suitable GDT is already loaded.

   You must supply a 16-bit code segment
   equivalent to the real-mode code segment currently in use.

   You must reload all segment registers except CS
   immediately after invoking this macro.
*/
FLUX_INLINE void i16_enter_pmode(int prot_cs)
{
	/* Switch to protected mode.  */
	asm volatile("
		movl	%0,%%cr0
		ljmp	%1,$1f
	1:
	" : : "r" (i16_get_cr0() | CR0_PE), "i" (prot_cs));
}



/* Leave protected mode and return to real mode.
   Assumes:
	* Running in protected mode
	* Interrupts are turned off.
	* Paging is turned off.
	* All currently loaded segment registers
	  contain 16-bit segments with limits of 0xffff.

   You must supply a real-mode code segment
   equivalent to the protected-mode code segment currently in use.

   You must reload all segment registers except CS
   immediately after invoking this function.
*/
FLUX_INLINE void i16_leave_pmode(int real_cs)
{
	/* Switch back to real mode.
	   Note: switching to the real-mode code segment
	   _must_ be done with an _immediate_ far jump,
	   not an indirect far jump.  At least on my Am386DX/40,
	   an indirect far jump leaves the code segment read-only.  */
	{
		extern unsigned short real_jmp[];

		real_jmp[3] = real_cs;
		asm volatile("
			movl	%0,%%cr0
			jmp	1f
		1:
		real_jmp:
		_real_jmp:
			ljmp	$0,$1f
		1:
		" : : "r" (i16_get_cr0() & ~CR0_PE));
	}
}



#endif _FLUX_X86_PMODE_H_
