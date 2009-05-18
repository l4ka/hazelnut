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
 * Standard error codes returned by device drivers
 * in the Flux device driver framework.
 */
#ifndef _FLUX_FDEV_ERROR_H_
#define _FLUX_FDEV_ERROR_H_

typedef int fdev_error_t;

/*
 * Error codes.
 */
#define FDEV_NO_SUCH_DEVICE	1
#define FDEV_BAD_OPERATION	2
#define FDEV_INVALID_PARAMETER	3
#define FDEV_IO_ERROR		4
#define FDEV_MEMORY_SHORTAGE	5

#endif /* _FLUX_FDEV_ERROR_H_ */
