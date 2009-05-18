/*
 * Simple polling serial console for the Flux OS toolkit
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
#ifndef _FLUX_X86_PC_COM_CONS_H_
#define _FLUX_X86_PC_COM_CONS_H_

#include <flux/c/common.h>

struct termios;

__FLUX_BEGIN_DECLS
/*
 * This routine must be called once to initialize the COM port.
 * The com_port parameter specifies which COM port to use, 1 through 4.
 * The supplied termios structure indicates the baud rate and other settings.
 * If com_params is NULL, a default of 9600,8,N,1 is used.
 */
void com_cons_init(int com_port, struct termios *com_params);

/*
 * Primary serial character I/O routines.
 */
int com_cons_getchar(void);
void com_cons_putchar(int ch);

/*
 * Since the com_console operates by polling,
 * there is no need to handle serial interrupts in order to do basic I/O.
 * However, if you want to be notified up when a character is received,
 * call this function immediately after com_cons_init(),
 * and make sure the appropriate IDT entry is initialized properly.
 * For example, the serial debugging code for the PC COM port
 * uses this so that the program can be woken up
 * when the user presses CTRL-C from the remote debugger.
 */
void com_cons_enable_receive_interrupt();
__FLUX_END_DECLS

#endif /* _FLUX_X86_PC_COM_CONS_H_ */
