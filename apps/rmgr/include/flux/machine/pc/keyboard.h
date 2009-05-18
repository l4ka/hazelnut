/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University.
 * Copyright (c) 1996,1995 The University of Utah and
 * the Computer Systems Laboratory (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY LIABILITY
 * OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF
 * THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/* **********************************************************************
 File:         kd.h
 Description:  definitions for AT keyboard/display driver
 Authors:       Eugene Kuerner, Adrienne Jardetzky, Mike Kupfer

 $ Header: $

 Copyright Ing. C. Olivetti & C. S.p.A. 1988, 1989.
 All rights reserved.
********************************************************************** */
/*
  Copyright 1988, 1989 by Olivetti Advanced Technology Center, Inc.,
Cupertino, California.

		All Rights Reserved

  Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Olivetti
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

  OLIVETTI DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL OLIVETTI BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUR OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
/*
 * Definitions describing the PC keyboard hardware.
 */
#ifndef _FLUX_X86_PC_KEYBOARD_H_
#define _FLUX_X86_PC_KEYBOARD_H_

#include <flux/c/common.h>

/*
 * Keyboard I/O ports.
 */
#define K_RDWR 		0x60		/* keyboard data & cmds (read/write) */
#define K_STATUS 	0x64		/* keybd status (read-only) */
#define K_CMD	 	0x64		/* keybd ctlr command (write-only) */

/*
 * Bit definitions for K_STATUS port.
 */
#define K_OBUF_FUL 	0x01		/* output (from keybd) buffer full */
#define K_IBUF_FUL 	0x02		/* input (to keybd) buffer full */
#define K_SYSFLAG	0x04		/* "System Flag" */
#define K_CMD_DATA	0x08		/* 1 = input buf has cmd, 0 = data */
#define K_KBD_INHIBIT	0x10		/* 0 if keyboard inhibited */
#define K_AUX_OBUF_FUL	0x20		/* 1 = obuf holds aux device data */
#define K_TIMEOUT	0x40		/* timout error flag */
#define K_PARITY_ERROR	0x80		/* parity error flag */

/* 
 * Keyboard controller commands (sent to K_CMD port).
 */
#define KC_CMD_READ	0x20		/* read controller command byte */
#define KC_CMD_WRITE	0x60		/* write controller command byte */
#define KC_CMD_DIS_AUX	0xa7		/* disable auxiliary device */
#define KC_CMD_ENB_AUX	0xa8		/* enable auxiliary device */
#define KC_CMD_TEST_AUX	0xa9		/* test auxiliary device interface */
#define KC_CMD_SELFTEST	0xaa		/* keyboard controller self-test */
#define KC_CMD_TEST	0xab		/* test keyboard interface */
#define KC_CMD_DUMP	0xac		/* diagnostic dump */
#define KC_CMD_DISABLE	0xad		/* disable keyboard */
#define KC_CMD_ENABLE	0xae		/* enable keyboard */
#define KC_CMD_RDKBD	0xc4		/* read keyboard ID */
#define KC_CMD_WIN	0xd0		/* read  output port */
#define KC_CMD_WOUT	0xd1		/* write output port */
#define KC_CMD_ECHO	0xee		/* used for diagnostic testing */
#define KC_CMD_PULSE	0xff		/* pulse bits 3-0 based on low nybble */

/* 
 * Keyboard commands (send to K_RDWR).
 */
#define K_CMD_LEDS	0xed		/* set status LEDs (caps lock, etc.) */

/* 
 * Bit definitions for controller command byte (sent following 
 * K_CMD_WRITE command).
 */
#define K_CB_ENBLIRQ	0x01		/* enable data-ready intrpt */
#define K_CB_SETSYSF	0x04		/* Set System Flag */
#define K_CB_INHBOVR	0x08		/* Inhibit Override */
#define K_CB_DISBLE	0x10		/* disable keyboard */

/* 
 * Bit definitions for "Indicator Status Byte" (sent after a 
 * K_CMD_LEDS command).  If the bit is on, the LED is on.  Undefined 
 * bit positions must be 0.
 */
#define K_LED_SCRLLK	0x1		/* scroll lock */
#define K_LED_NUMLK	0x2		/* num lock */
#define K_LED_CAPSLK	0x4		/* caps lock */

/* 
 * Bit definitions for "Miscellaneous port B" (K_PORTB).
 */
/* read/write */
#define K_ENABLETMR2	0x01		/* enable output from timer 2 */
#define K_SPKRDATA	0x02		/* direct input to speaker */
#define K_ENABLEPRTB	0x04		/* "enable" port B */
#define K_EIOPRTB	0x08		/* enable NMI on parity error */
/* read-only */
#define K_REFRESHB	0x10		/* refresh flag from INLTCONT PAL */
#define K_OUT2B		0x20		/* timer 2 output */
#define K_ICKB		0x40		/* I/O channel check (parity error) */

/*
 * Bit definitions for the keyboard controller's output port.
 */
#define KO_SYSRESET	0x01		/* processor reset */
#define KO_GATE20	0x02		/* A20 address line enable */
#define KO_AUX_DATA_OUT	0x04		/* output data to auxiliary device */
#define KO_AUX_CLOCK	0x08		/* auxiliary device clock */
#define KO_OBUF_FUL	0x10		/* keyboard output buffer full */
#define KO_AUX_OBUF_FUL	0x20		/* aux device output buffer full */
#define KO_CLOCK	0x40		/* keyboard clock */
#define KO_DATA_OUT	0x80		/* output data to keyboard */

__FLUX_BEGIN_DECLS
extern void kb_command(unsigned char ch);
__FLUX_END_DECLS

#endif _FLUX_X86_PC_KEYBOARD_H_
