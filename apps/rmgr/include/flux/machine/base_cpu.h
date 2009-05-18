/* 
 * Copyright (c) 1995-1994 The University of Utah and
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
 * Functions for setting up and activating
 * a basic x86 protected-mode kernel execution environment.
 */
#ifndef _FLUX_X86_BASE_CPU_H_
#define _FLUX_X86_BASE_CPU_H_

#include <flux/c/common.h>
#include <flux/x86/cpuid.h>

__FLUX_BEGIN_DECLS
/*
 * CPU information for the processor base_cpu_init() was called on.
 */
extern struct cpu_info base_cpuid;

/*
 * Initialize all the processor data structures defining the base environment:
 * the base_cpuid, the base_idt, the base_gdt, and the base_tss.
 */
extern void base_cpu_init(void);

/*
 * Load all of the above into the processor,
 * properly setting up the protected-mode environment.
 */
extern void base_cpu_load(void);
__FLUX_END_DECLS

/*
 * Do both at once.
 */
#define base_cpu_setup() (base_cpu_init(), base_cpu_load())

#endif /* _FLUX_X86_BASE_CPU_H_ */
