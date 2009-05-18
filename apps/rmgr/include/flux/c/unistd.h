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
#ifndef _FLUX_MC_UNISTD_H_
#define _FLUX_MC_UNISTD_H_

#include <flux/c/common.h>

#ifndef _SIZE_T
#define _SIZE_T
typedef __flux_size_t size_t;
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef __flux_ssize_t ssize_t;
#endif

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

/* access function */
#define F_OK            0       /* test for existence of file */
#define X_OK            0x01    /* test for execute or search permission */
#define W_OK            0x02    /* test for write permission */
#define R_OK            0x04    /* test for read permission */

__FLUX_BEGIN_DECLS

void _exit(int __fd) __FLUX_NORETURN;
int close(int __fd);
ssize_t read(int __fd, void *__buf, size_t __n);
ssize_t write(int __fd, const void *__buf, size_t __n);
__flux_off_t lseek(int __fd, __flux_off_t __offset, int __whence);
int rename(const char *__oldpath, const char *__newpath);
void *sbrk(int __size);
int unlink(const char *__path);

__FLUX_END_DECLS

#endif /* _FLUX_MC_UNISTD_H_ */
