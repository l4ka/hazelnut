/*
 * Copyright (c) 1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 */
#ifndef _FLUX_MC_MALLOC_H_
#define _FLUX_MC_MALLOC_H_

#include <flux/c/common.h>

#ifndef _SIZE_T
#define _SIZE_T
typedef __flux_size_t size_t;
#endif

/* The malloc package in the base C library
   is implemented on top of the List Memory Manager,
   and the underlying memory pool can be manipulated
   directly with the LMM primitives using this lmm structure.  */
extern struct lmm malloc_lmm;

__FLUX_BEGIN_DECLS

/*
 * Don't macro expand protos please.
 */
#ifndef MALLOC_IS_MACRO

void *malloc(size_t size);
void *mustmalloc(size_t size);
void *memalign(size_t alignment, size_t size);
void *calloc(size_t nelt, size_t eltsize);
void *mustcalloc(size_t nelt, size_t eltsize);
void *realloc(void *buf, size_t new_size);
void free(void *buf);

/* Alternate version of the standard malloc functions that expect the
   caller to keep track of the size of allocated chunks.  These
   versions are _much_ more memory-efficient when allocating many
   chunks naturally aligned to their (natural) size (e.g. allocating
   naturally-aligned pages or superpages), because normal memalign
   requires a prefix between each chunk which will create horrendous
   fragmentation and memory loss.  Chunks allocated with these
   functions must be freed with sfree() rather than the ordinary
   free(). */
void *smalloc(size_t size);
void *smemalign(size_t alignment, size_t size);
void *scalloc(size_t size);
void *srealloc(void *buf, size_t old_size, size_t new_size);
void sfree(void *buf, size_t size);

#endif /* MALLOC_IS_MACRO */

/* malloc() and realloc() call this routine when they're about to fail;
   it should try to scare up more memory and add it to the malloc_lmm.
   Returns nonzero if it succeeds in finding more memory.  */
int morecore(size_t size);

/* in a multithreaded client os, these functions should be overridden
   to protect accesses to the malloc_lmm. */
void mem_lock(void);
void mem_unlock(void);

__FLUX_END_DECLS

#endif _FLUX_MC_MALLOC_H_
