/* 
 * Copyright (c) 1996-1994 The University of Utah and
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
#ifndef _FLUX_MC_SYS_MMAN_H_
#define _FLUX_MC_SYS_MMAN_H_

/*
 * Protections are chosen from these bits, or-ed together.
 * NB: These are the same values as the VM_PROT_xxx definitions,
 * and they can be used interchangeably.
 * They are also the same values used in traditional Unix, BSD, and Linux
 * though READ/EXEC are reversed from the traditional Unix filesystem values.
 */
#define	PROT_READ	0x01	/* pages can be read */
#define	PROT_WRITE	0x02	/* pages can be written */
#define	PROT_EXEC	0x04	/* pages can be executed */
#define PROT_NONE	0x00	/* no access permitted */

/*
 * Flags for mmap().
 * These are the same values as in BSD.
 */
#define MAP_SHARED	0x0001	/* writes change the underlying file */
#define MAP_PRIVATE	0x0002	/* writes only change our mapping */
#define MAP_FIXED	0x0010	/* must use specified address exactly */

/*
 * Failure return value for mmmap
 */
#define MAP_FAILED	((void *)-1)

/*
 * Flags for the msync() call.
 */
#define	MS_INVALIDATE	0x0004	/* invalidate cached data */

/*
 * Flags for the mlockall() call.
 */
#define MCL_CURRENT	0x0001	/* lock all currently mapped memory */
#define MCL_FUTURE	0x0002	/* lock all memory mapped in the future */

#endif _FLUX_MC_SYS_MMAN_H_
