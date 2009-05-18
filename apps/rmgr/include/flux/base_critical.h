/* 
 * Copyright (c) 1996 The University of Utah and
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
 * This module implements a simple "global critical region"
 * used throughout the OS toolkit to ensure proper serialization
 * for various "touchy" but non-performance critical activities
 * such as panicking, rebooting, debugging, etc.
 * This critical region can safely be entered recursively;
 * the only requirement is that "enter"s match exactly with "leave"s.
 *
 * The implementation of this module is machine-dependent,
 * and generally disables interrupts and, on multiprocessors,
 * grabs a recursive spin lock.
 */
#ifndef _FLUX_BASE_CRITICAL_H_
#define _FLUX_BASE_CRITICAL_H_

#include <flux/c/common.h>

__FLUX_BEGIN_DECLS
void base_critical_enter(void);
void base_critical_leave(void);
__FLUX_END_DECLS

#endif /* _FLUX_BASE_CRITICAL_H_ */
