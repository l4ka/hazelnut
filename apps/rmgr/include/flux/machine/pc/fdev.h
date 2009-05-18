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
 * PC-specific definitions for device interface.
 */

#ifndef _FLUX_X86_PC_FDEV_H_
#define _FLUX_X86_PC_FDEV_H_

#include <flux/c/common.h>

__FLUX_BEGIN_DECLS
/*
 * DRQ manipulation.
 */
int fdev_dma_alloc(int channel);
void fdev_dma_free(int channel);

/*
 * I/O ports (modeled after the Linux interface).
 */
int fdev_port_avail(unsigned port, unsigned size);
void fdev_port_alloc(unsigned port, unsigned size);
void fdev_port_free(unsigned port, unsigned size);
__FLUX_END_DECLS

#endif /* _FLUX_X86_PC_FDEV_H_ */
