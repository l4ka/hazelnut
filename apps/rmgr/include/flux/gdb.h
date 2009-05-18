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
 */
/*
 * Generic definitions for remote GDB debugging.
 */
#ifndef _FLUX_GDB_H_
#define _FLUX_GDB_H_

#include <flux/c/common.h>
#include <flux/machine/types.h>

/*
 * This structure must be defined in <flux/machine/gdb.h>
 * to represent the processor register state frame as GDB wants it.
 */
struct gdb_state;

/*
 * This structure must be defined in <flux/machine/base_trap.h>
 * to represent the processor register state
 * as saved by the default trap entrypoint code.
 */
struct trap_state;

__FLUX_BEGIN_DECLS

/*
 * This function provides the necessary glue
 * between the base trap handling mechanism provided by the toolkit
 * and a machine-independent GDB stub such as gdb_serial.
 * This function is intended to be plugged into 'trap_handler_func'
 * (see flux/machine/base_trap.h).
 */
int gdb_trap(struct trap_state *inout_trap_state);

/*
 * Before the above function is called for the first time,
 * this variable must be set to point to the GDB stub it should call.
 */
extern void (*gdb_signal)(int *inout_signo, struct gdb_state *inout_gdb_state);

/*
 * These functions are provided to copy data
 * into and out of the address space of the program being debugged.
 * Machine-dependent code typically provides default implementations
 * that simply copy data into and out of the kernel's own address space.
 * These functions return zero if the copy succeeds,
 * or nonzero if the memory region couldn't be accessed for some reason.
 */
int gdb_copyin(vm_offset_t src_va, void *dest_buf, vm_size_t size);
int gdb_copyout(const void *src_buf, vm_offset_t dest_va, vm_size_t size);

/*
 * The GDB stub calls this architecture-specific function
 * to modify the trace flag in the processor state.
 */
void gdb_set_trace_flag(int trace_enable, struct gdb_state *inout_state);

__FLUX_END_DECLS

/* Bring in machine-dependent definitions. */
#include <flux/machine/gdb.h>

#endif /* _FLUX_GDB_H_ */
