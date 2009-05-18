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
 * POSIX-defined <sys/wait.h>
 */
#ifndef _FLUX_C_SYS_WAIT_H
#define _FLUX_C_SYS_WAIT_H

#include <flux/c/common.h>
#include <sys/types.h>

/* Upper 8 bits are return code, lowest 7 are status bits */
#define _WSTATMASK            0177
#define _WEXITSTAT(stat_val) ((stat_val) & _WSTATMASK)

/* Evaluates to nonzero if child terminated normally. */
#define WIFEXITED(stat_val)   (_WEXITSTAT(stat_val) == 0)

/* Evaulates to nonzero if child terminated due to unhandled signal */
#define WIFSIGNALED(stat_val) ((_WEXITSTAT(stat_val) != _WSTATMASK) && (_WEXITSTAT(stat_val) != 0))

/* Evaluates to nonzero if child process is currently stopped. */
#define WIFSTOPPED(stat_val)  (_WEXITSTAT(stat_val) == _WSTATMASK)

/*
 * If WIFEXITED(stat_val) is nonzero, this evaulates to the return code of the
 * child process.
 */
#define WEXITSTATUS(stat_val) ((stat_val) >> 8)

/*
 * If WIFSIGNALED(stat_val) is nonzero, this evaluates to the signal number
 * that caused the termination.
 */
#define WTERMSIG(stat_val)    (_WEXITSTAT(stat_val))

/*
 * If WIFSTOPPED(stat_val) is nonzero, this evaluates to the signal number
 * that caused the child to stop.
 */
#define WSTOPSIG(stat_val)    ((stat_val) >> 8)

/*
 * bitwise OR-able option flags to waitpid().  waitpid() will not suspend the
 * caller if WNOHANG is given and there is no process status immediately
 * available.  WUNTRACED causes waitpid() to report stopped processes whose
 * status has not yet been reported since they stopped.
 */
#define WNOHANG    1
#define WUNTRACED  2


__FLUX_BEGIN_DECLS

pid_t wait(int *__stat_loc);
pid_t waitpid(pid_t __pid, int *__stat_loc, int __options);

__FLUX_END_DECLS

#endif
