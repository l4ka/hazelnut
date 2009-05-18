/* 
 * Copyright (c) 1995-1993 The University of Utah and
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
 *	This file contains definitions for kernel debugging,
 *	which are compiled in on the DEBUG symbol.
 */
#ifndef _FLUX_DEBUG_H_
#define _FLUX_DEBUG_H_

#include <flux/machine/debug.h>

#ifdef DEBUG


/*** Permanent code annotation macros ***/

/* Get the assert() macro from the minimal C library's assert.h.
   Sanity check: make sure NDEBUG isn't also defined... */
#ifdef NDEBUG
#error Inconsistency: DEBUG and NDEBUG both defined!
#endif
#include <flux/c/assert.h>

#define otsan() panic("%s:%d: off the straight and narrow!", __FILE__, __LINE__)

#define do_debug(stmt) stmt

/* XXX not sure how useful this is; just delete? */
#define struct_id_decl		unsigned struct_id;
#define struct_id_init(p,id)	((p)->struct_id = (id))
#define struct_id_denit(p)	((p)->struct_id = 0)
#define struct_id_verify(p,id) \
	({ if ((p)->struct_id != (id)) \
		panic("%s:%d: "#p" (%08x) struct_id should be "#id" (%08x), is %08x\n", \
			__FILE__, __LINE__, (p), (id), (p->struct_id)); \
	})


/*** Macros to provide temporary debugging scaffolding ***/

extern void dump_stack_trace(void); /* defined in libc/<machine>/stack_trace.c */

#include <stdio.h>
#define here() printf("@ %s:%d\n", __FILE__, __LINE__)

#define debugmsg(args) \
	( printf("%s:%d: ", __FILE__, __LINE__), printf args, printf("\n") )

#else /* !DEBUG */

/* Make sure the assert macro compiles to nothing.
   XXX What about if assert.h has already been included?  */
#ifndef NDEBUG
#define NDEBUG
#endif
#include <flux/c/assert.h>

#define otsan()
#define do_debug(stmt)

#define struct_id_decl
#define struct_id_init(p,id)
#define struct_id_denit(p)
#define struct_id_verify(p,id)

#endif /* !DEBUG */

#include <flux/c/common.h>
__FLUX_BEGIN_DECLS
extern void panic(const char *format, ...) __FLUX_NORETURN;
__FLUX_END_DECLS

#endif /* _FLUX_DEBUG_H_ */
