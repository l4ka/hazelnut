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
 * Flux device driver interface definitions.
 */
#ifndef _FLUX_FDEV_BUS_H_
#define _FLUX_FDEV_BUS_H_

struct fdev_bus
{
	fdev_t		fdev;

	unsigned char	type;
};
typedef struct fdev_bus fdev_bus_t;

/* Specific well-known bus device types */
#define FDEV_BUS_ISA		0x01
#define FDEV_BUS_EISA		0x02
#define FDEV_BUS_PCI		0x03
#define FDEV_BUS_SCSI		0x04

#endif /* _FLUX_FDEV_BUS_H_ */
