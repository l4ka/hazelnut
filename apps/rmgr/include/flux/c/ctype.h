/*
 * Copyright (c) 1996-1994 The University of Utah and
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
#ifndef _FLUX_MC_CTYPE_H_
#define _FLUX_MC_CTYPE_H_

#include <flux/c/common.h>

__FLUX_INLINE int iscntrl(int c)
{
	return ((c) < ' ') || ((c) > 126);
}

__FLUX_INLINE int isdigit(int c)
{
	return ((c) >= '0') && ((c) <= '9');
}

__FLUX_INLINE int isgraph(int c)
{
	return ((c) > ' ') && ((c) <= 126);
}

__FLUX_INLINE int islower(int c)
{
	return (c >= 'a') && (c <= 'z');
}

__FLUX_INLINE int isprint(int c)
{
	return ((c) >= ' ') && ((c) <= 126);
}

__FLUX_INLINE int isspace(int c)
{
	return ((c) == ' ') || ((c) == '\f')
		|| ((c) == '\n') || ((c) == '\r')
		|| ((c) == '\t') || ((c) == '\v');
}

__FLUX_INLINE int isupper(int c)
{
	return (c >= 'A') && (c <= 'Z');
}

__FLUX_INLINE int isxdigit(int c)
{
	return isdigit(c) ||
		((c >= 'A') && (c <= 'F')) ||
		((c >= 'a') && (c <= 'f'));
}

__FLUX_INLINE int isalpha(int c)
{
	return islower(c) || isupper(c);
}

__FLUX_INLINE int isalnum(int c)
{
	return isalpha(c) || isdigit(c);
}

__FLUX_INLINE int ispunct(int c)
{
	return isgraph(c) && !isalnum(c);
}

__FLUX_INLINE int toupper(int c)
{
	return ((c >= 'a') && (c <= 'z')) ? (c - 'a' + 'A') : c;
}

__FLUX_INLINE int tolower(int c)
{
	return ((c >= 'A') && (c <= 'Z')) ? (c - 'A' + 'a') : c;
}


#endif _FLUX_MC_CTYPE_H_
