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
#ifndef _FLUX_X86_PC_SERIAL_GDB_H_
#define _FLUX_X86_PC_SERIAL_GDB_H_

#include <flux/c/common.h>

extern int serial_gdb_enable;

__FLUX_BEGIN_DECLS
extern void serial_gdb_init();
extern int serial_gdb_signal(int *inout_sig, struct trap_state *ts);
extern void serial_gdb_shutdown(int exitcode);
__FLUX_END_DECLS

#endif /* _FLUX_X86_PC_SERIAL_GDB_H_ */
