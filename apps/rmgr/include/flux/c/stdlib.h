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
#ifndef _MACH_SA_STDLIB_H_
#define _MACH_SA_STDLIB_H_

#include <flux/c/common.h>

#ifndef _SIZE_T
#define _SIZE_T
typedef __flux_size_t size_t;
#endif

#ifndef NULL
#define NULL 0
#endif

__FLUX_BEGIN_DECLS

/*
 * These function prototypes are roughly in the same order
 * as their descriptions in the ISO/ANSI C standard, section 7.10.
 */

#define atoi(str) ((int)atol(str))
long atol(const char *__str);
long strtol(const char *__p, char **__out_p, int __base);
unsigned long strtoul(const char *__p, char **__out_p, int __base);

int rand(void);
void srand(unsigned __seed);

/* Don't macro expand protos please. */
#ifndef MALLOC_IS_MACRO
void *malloc(size_t __size);
void *mustmalloc(size_t __size);
void *calloc(size_t __nelt, size_t __eltsize);
void *mustcalloc(size_t __nelt, size_t __eltsize);
void *realloc(void *__buf, size_t __new_size);
void free(void *__buf);
#endif /* MALLOC_IS_MACRO */

void abort(void) __FLUX_NORETURN;
void exit(int __status) __FLUX_NORETURN;
void panic(const char *__format, ...) __FLUX_NORETURN;

int atexit(void (*__function)(void));

char *getenv(const char *__name);

void qsort(void *__array, size_t __nelt, size_t __eltsize, 
	   int (*__cmp)(void *, void *));

#define abs(n) __builtin_abs(n)

__FLUX_END_DECLS

#endif _MACH_SA_STDLIB_H_
