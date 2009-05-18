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
 * Definitions specific to character devices.
 */
#ifndef _FLUX_FDEV_CHAR_H_
#define _FLUX_FDEV_CHAR_H_

#include <flux/c/common.h>
#include <flux/fdev/fdev.h>

struct fdev_char
{
	fdev_t		fdev;

	unsigned char	type;

	fdev_error_t (*write)(fdev_t *dev, void *buf, fdev_buf_vect_t *bufvec);
};
typedef struct fdev_char fdev_char_t;

__FLUX_BEGIN_DECLS
/*
 * Character drivers call this OS-supplied function
 * when one or more characters have been received.
 */
void fdev_char_recv(fdev_char_t *fdev, char *data, unsigned size);
__FLUX_END_DECLS

/* Specific character device types */
#define FDEV_CHAR_SERIAL	0x01	/* RS-232 serial port */
#define FDEV_CHAR_PARALLEL	0x02	/* Centronics parallel port */
#define FDEV_CHAR_KEYBOARD	0x03
#define FDEV_CHAR_MOUSE		0x04
#define FDEV_CHAR_DISPLAY	0x05

#endif /* _FLUX_FDEV_CHAR_H_ */
