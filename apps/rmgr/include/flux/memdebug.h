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
#ifndef _FLUX_MEMDEBUG_H_
#define _FLUX_MEMDEBUG_H_

/*
 * We define this so malloc.h/stdlib.h knows not to prototype
 * malloc and friends.
 * Otherwise the prototypes will get macro expanded and bad things will
 * happen.
 */
#define MALLOC_IS_MACRO

#include <flux/c/common.h>

#ifndef _SIZE_T
#define _SIZE_T
typedef __flux_size_t size_t;
#endif

#define malloc(s) memdebug_traced_malloc(s, __FILE__, __LINE__)
#define memalign(a, s) memdebug_traced_memalign(a, s, __FILE__, __LINE__)
#define calloc(n, s) memdebug_traced_calloc(n, s, __FILE__, __LINE__)
#define realloc(m, s) memdebug_traced_realloc(m, s, __FILE__, __LINE__)
#define free(p) memdebug_traced_free(p, __FILE__, __LINE__)

#define smalloc(s) memdebug_traced_smalloc(s, __FILE__, __LINE__)
#define smemalign(a, s) memdebug_traced_smemalign(a, s, __FILE__, __LINE__)
/* XXX these two are yet to be implemented. */
/* #define scalloc(n, s) memdebug_traced_scalloc(n, s, __FILE__, __LINE__) */
/* #define srealloc(m, s) memdebug_traced_srealloc(m, s, __FILE__, __LINE__) */
#define sfree(p, s) memdebug_traced_sfree(p, s, __FILE__, __LINE__)

__FLUX_BEGIN_DECLS

/*
 * Mark all currently allocated blocks, for a future memdebug_check().
 */
void memdebug_mark(void);

/*
 * Look for allocated blocks that are unmarked--ie, alloced and
 * not free'd since the last memdebug_mark().
 */
void memdebug_check(void);

/*
 * Run a set of vailidity checks on the given memory block ptr.
 */
#define memdebug_ptrchk(ptr) memdebug_traced_ptrchk(ptr, __FILE__, __LINE__);
int memdebug_traced_ptrchk(void *__ptr, char *file, int line);

/*
 * Sweep through all allocated memory blocks and do a validity check.
 */
void memdebug_sweep(void);

/*
 * These functions are defined in libmemdebug, and are not meant to be
 * used directly, but via the macros defined above.  But who am I to
 * tell you what to do.
 */
void *memdebug_traced_malloc(size_t bytes, char *file, int line);
void *memdebug_traced_memalign(size_t alignment, size_t size, char *file, int line);
void *memdebug_traced_calloc(size_t n, size_t elem_size, char *file, int line);
void memdebug_traced_free(void* mem, char *file, int line);
void *memdebug_traced_realloc(void* oldmem, size_t bytes, char *file, int line);
void *memdebug_traced_smalloc (size_t size, char *file, int line);
void *memdebug_traced_smemalign(size_t alignment, size_t size, char *file, int line);
void memdebug_traced_sfree(void* mem, size_t chunksize, char *file, int line);

int memdebug_printf(const char *fmt, ...);

__FLUX_END_DECLS

#endif _FLUX_MEMDEBUG_H_
