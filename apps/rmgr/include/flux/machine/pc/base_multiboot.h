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
 * Functions and data definitions for kernels
 * that want to start up from a MultiBoot boot loader.
 */
#ifndef _FLUX_X86_PC_BASE_MULTIBOOT_H_
#define _FLUX_X86_PC_BASE_MULTIBOOT_H_

#include <flux/c/common.h>
#include <flux/x86/multiboot.h>
#include <flux/x86/types.h>

/*
 * This variable holds a copy of the multiboot_info structure
 * which was passed to us by the boot loader.
 */
extern struct multiboot_info boot_info;

__FLUX_BEGIN_DECLS
/*
 * This function digs through the boot_info structure
 * to find out what physical memory is available to us,
 * and initalizes the malloc_lmm with those memory regions.
 * It is smart enough to skip our own executable, wherever it may be,
 * as well as the important stuff passed by the boot loader
 * such as our command line and boot modules.
 */
void base_multiboot_init_mem(void);

/*
 * This function parses the command-line passed by the boot loader
 * into a nice POSIX-like set of argument and environment strings.
 * It depends on being able to call malloc().
 */
void base_multiboot_init_cmdline();

/*
 * This is the first C routine called by crt0/multiboot.o;
 * its default implementation sets up the base kernel environment
 * and then calls main() and exit().
 */
void multiboot_main(vm_offset_t boot_info_pa);

/*
 * This routine finds the MultiBoot boot module in the boot_info structure
 * whose associated `string' matches the provided `string' parameter.
 * If the strings attached to boot modules are assumed to be filenames,
 * this function can serve as a primitive "open" operation
 * for a simple "boot module file system".
 */
struct multiboot_module *base_multiboot_find(const char *string);

/*
 * Dump out the boot_info struct nicely.
 */
void multiboot_info_dump();
__FLUX_END_DECLS

#endif /* _FLUX_X86_PC_BASE_MULTIBOOT_H_ */
