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
#ifndef _FLUX_GDB_SERIAL_H_
#define _FLUX_GDB_SERIAL_H_

#include <flux/c/common.h>
#include <flux/machine/types.h>

/*
 * This is actually defined in <flux/machine/gdb.h>,
 * but we don't actually need the full definition in this header file.
 */
struct gdb_state;

__FLUX_BEGIN_DECLS

/*
 * These function pointers define how we send and receive characters
 * over the serial line.  They must be initialized before gdb_serial_stub()
 * is called for the first time.
 */
extern int (*gdb_serial_recv)(void);
extern void (*gdb_serial_send)(int ch);

/*
 * This is the main part of the GDB stub for serial-line remote debugging.
 * Call it with a signal number indicating the signal
 * that caused the program to stop,
 * and a snapshot of the program's register state.
 * After this routine returns, if *inout_signo has been set to 0,
 * the program's execution should be resumed as if nothing had happened.
 * If *inout_signo is nonzero,
 * then that signal should be delivered to the program
 * as if the program had caused the signal itself.
 * The register state in *inout_state may have been changed;
 * the new register state should be used when resuming execution.
 */
void gdb_serial_signal(int *inout_signo, struct gdb_state *inout_state);

/*
 * Call this routine to indicate that the program is exiting normally.
 * The 'exit_code' specifies the exit code sent back to GDB.
 * This function will cause the GDB session to be broken;
 * the caller can then reboot or exit or whatever locally.
 */
void gdb_serial_exit(int exit_code);

/*
 * Call these routines to output characters or strings
 * to the remote debugger's standard output.
 */
void gdb_serial_putchar(int ch);
void gdb_serial_puts(const char *s);
__FLUX_END_DECLS

#endif /* _FLUX_GDB_SERIAL_H_ */
