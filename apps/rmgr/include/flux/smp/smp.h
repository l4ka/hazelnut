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
 *
 */

#ifndef _FLUX_SMP_SMP_H_
#define _FLUX_SMP_SMP_H_

#include <flux/c/common.h>

#undef SMP_DEBUG

/*
 * Header file for SMP-specific functions
 */

/*
 * General Notes:
 * 1. Processors are not assumed to be numbered sequentially.
 * 2. smp_get_num_cpus() may take a LONG time (seconds?) the
 *    first time it is called.
 * XXX == only if called before smp_initialize!!
 *
 * 3. The division of labor between smg_get_num_cpus and
 *    smp_startup_processor is arbitrary, and transparent to
 *    the programmer (except for elapsed time), and may change.
 */


__FLUX_BEGIN_DECLS
/*
 * This gets everything going...
 * This should be the first SMP routine called.
 * It returns 0 on success (if it found a SMP system).
 * Success doesn't mean more than one processor, although
 * failure indicates only one.
 */
int smp_initialize(void);



/*
 * This routine returns the current processor number (UID)
 * It is only valid after smp_initialize() has bee called.
 */
int smp_find_cur_cpu(void);


/*
 * This routine determines the number of processors available.
 * It is called early enough so that any information it may need is
 * still intact.
 */
int smp_get_num_cpus(void);



/*
 * This is an iterator function to get all the CPU ID's
 * Call it the first time with 0; subsequently with (res+1)
 * Returns (<0) if no such processor
 */
int smp_find_cpu(int first);




/*
 * This function causes the processor numbered to begin
 * executing the function passed, with the stack passed, and
 * the data_arg on the stack (so it can bootstrap itself).
 * Note: on the x86, it will already be in protected mode
 *
 * If the processor doesn't exist, it will return E_SMP_NO_PROC
 */
void smp_start_cpu(int processor_id, void (*func)(void *data),
		void *data, void *stack_ptr);

__FLUX_END_DECLS

#define E_SMP_NO_CONFIG	-135
#define E_SMP_NO_PROC	-579

#endif
