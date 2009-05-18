/*
 * Remote serial-line source-level debugging for the Flux OS Toolkit.
 * Copyright (C) 1996-1994 Sleepless Software
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Author: Bryan Ford
 *	Loosely based on i386-stub.c from GDB 4.14
 */
/*
 * Definitions for remote GDB debugging of x86 programs.
 */
#ifndef _FLUX_X86_GDB_H_
#define _FLUX_X86_GDB_H_

#include <flux/c/common.h>

/* This structure represents the x86 register state frame as GDB wants it.  */
struct gdb_state
{
	unsigned	eax;
	unsigned	ecx;
	unsigned	edx;
	unsigned	ebx;
	unsigned	esp;
	unsigned	ebp;
	unsigned	esi;
	unsigned	edi;
	unsigned	eip;
	unsigned	eflags;
	unsigned	cs;
	unsigned	ss;
	unsigned	ds;
	unsigned	es;
	unsigned	fs;
	unsigned	gs;
};

struct trap_state;
struct termios;

/*
 * While copying data to or from arbitrary locations,
 * the gdb_copyin() and gdb_copyout() routines set this address
 * so they can recover from any memory access traps that may occur.
 * On receiving a trap, if this address is non-null,
 * gdb_trap() simply sets the EIP to it and resumes execution,
 * at which point the gdb_copyin/gdb_copyout routines return an error.
 * If the default gdb_copyin/gdb_copyout routines are overridden,
 * the replacement routines can use this facility as well.
 */
extern unsigned gdb_trap_recover;

/*
 * Use this macro to insert a breakpoint manually anywhere in your code.
 */
#define gdb_breakpoint() asm volatile("int $3");


__FLUX_BEGIN_DECLS
/*
 * Set up serial-line debugging over a COM port
 */
void gdb_pc_com_init(int com_port, struct termios *com_params);
__FLUX_END_DECLS

#endif /* _FLUX_X86_GDB_H_ */
