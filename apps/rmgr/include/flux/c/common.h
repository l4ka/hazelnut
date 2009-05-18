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
/*
 * Common private header file used by the Flux minimal C library headers.
 * This header file does not cause any non-POSIX-reserved symbols to be defined.
 * Also, all the symbols it defines are prefixed with __FLUX or __flux
 * to increase the likelyhood that it can be used cleanly
 * with or by the header files of other C libraries.
 */
#ifndef _FLUX_MC_COMMON_H_
#define _FLUX_MC_COMMON_H_

#ifdef __cplusplus
#define __FLUX_BEGIN_DECLS extern "C" {
#define __FLUX_END_DECLS }
#else
#define __FLUX_BEGIN_DECLS
#define __FLUX_END_DECLS
#endif

#ifndef __FLUX_INLINE
#define __FLUX_INLINE static __inline
#endif

#ifdef __GNUC__
#define __FLUX_NORETURN __attribute((noreturn))
#else
#define __FLUX_NORETURN
#endif

typedef	unsigned short	__flux_dev_t;	/* device id */
typedef	unsigned long	__flux_gid_t;	/* group id */
typedef	unsigned long	__flux_ino_t;	/* inode number */
typedef	unsigned short	__flux_mode_t;	/* permissions */
typedef	unsigned short	__flux_nlink_t;	/* link count */
typedef	unsigned long	__flux_uid_t;	/* user id */
typedef	signed long	__flux_pid_t;	/* process id */


/* Also include machine-dependent common definitions.  */
#include <flux/machine/c/common.h>

#endif /* _FLUX_MC_COMMON_H_ */
