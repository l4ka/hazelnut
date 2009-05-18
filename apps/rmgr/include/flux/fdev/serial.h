/* 
 * Copyright (c) 1996 The University of Utah and
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
/*
 * Definitions specific to RS-232 serial character devices.
 */
#ifndef _FLUX_FDEV_SERIAL_H_
#define _FLUX_FDEV_SERIAL_H_

#include <flux/fdev/fdev.h>

struct fdev_serial
{
	fdev_char_t	fdev_char;

	fdev_error_t (*set)(fdev_serial_t *dev, unsigned baud, int mode);
	fdev_error_t (*get)(fdev_serial_t *dev, unsigned *out_baud,
				   int *out_mode, int *out_state);
};
typedef struct fdev_serial fdev_serial_t;

/* Serial-specific device driver interface functions */
#define fdev_serial_set(fdev, baud, mode)	\
	((fdev)->set((fdev), (baud), (mode)))
#define fdev_serial_get(fdev, baud, out_mode, out_state)	\
	((fdev)->get((fdev), (baud), (out_mode), (out_state)))

/* Serial mode flags */
#define FDEV_SERIAL_DTR		0x0010	/* Assert data terminal ready signal */
#define FDEV_SERIAL_RTS		0x0020	/* Assert request-to-send signal */
#define FDEV_SERIAL_CSIZE	0x0300	/* Number of bits per byte: */
#define FDEV_SERIAL_CS5		0x0000	/* 5 bits */
#define FDEV_SERIAL_CS5		0x0100	/* 6 bits */
#define FDEV_SERIAL_CS5		0x0200	/* 7 bits */
#define FDEV_SERIAL_CS5		0x0300	/* 8 bits */
#define FDEV_SERIAL_CSTOPB	0x0400	/* Use two stop bits if set */
#define FDEV_SERIAL_PARENB	0x1000	/* Enable parity */
#define FDEV_SERIAL_PARODD	0x2000	/* Odd parity if set, even if not */

/* Serial line state flags */
#define FDEV_SERIAL_DSR		0x0001	/* Data set ready */
#define FDEV_SERIAL_CTS		0x0002	/* Clear to send */
#define FDEV_SERIAL_CD		0x0004	/* Carrier detect */
#define FDEV_SERIAL_RI		0x0008	/* Ring indicator */

/*
 * Serial drivers call this OS-supplied function
 * when a break signal or parity error is received.
 */
void fdev_serial_recv_special(fdev_char_t *fdev, int event);
#define FDEV_SERIAL_BREAK	0x01	/* Break signal received */
#define FDEV_SERIAL_PARITY_ERR	0x02	/* Parity error detected */
#define FDEV_SERIAL_STATE_CHG	0x03	/* State of RS-232 signals changed */

#endif /* _FLUX_FDEV_SERIAL_H_ */
