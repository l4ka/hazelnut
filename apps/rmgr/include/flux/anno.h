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
#ifndef _FLUX_ANNO_H_
#define _FLUX_ANNO_H_

#include <flux/c/common.h>
#include <flux/machine/types.h>

/*
 * One global variable of type 'struct anno_table' should be declared
 * for each separate set of annotation entries that may exist.
 */
struct anno_table
{
	struct anno_entry	*start;
	struct anno_entry	*end;
};

/*
 * Declare a variable of this type in the special "anno" section
 * to register an annotation.
 */
struct anno_entry
{
	integer_t		val1;
	integer_t		val2;
	integer_t		val3;
	struct anno_table	*table;
};

/*
 * This routine should be called once at program startup;
 * it sorts all of the annotation entries appropriately
 * and initializes the annotation tables they reside in.
 */
__FLUX_BEGIN_DECLS
void anno_init(void);
__FLUX_END_DECLS

#include <flux/machine/anno.h>

#endif /* _FLUX_ANNO_H_ */
