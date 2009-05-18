/* 
 * Copyright (c) 1995 The University of Utah and
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
 * PC-specific flag bits and priority values
 * for the List Memory Manager (LMM)
 * relevant for kernels managing physical memory.
 */
#ifndef _FLUX_X86_PC_PHYS_LMM_H_
#define _FLUX_X86_PC_PHYS_LMM_H_

#include <flux/c/common.h>
#include <flux/x86/types.h>

/*
 * <1MB memory is most precious, then <16MB memory, then high memory.
 * Assign priorities to each region accordingly
 * so that high memory will be used first when possible,
 * then 16MB memory, then 1MB memory.
 */
#define LMM_PRI_1MB	-2
#define LMM_PRI_16MB	-1
#define LMM_PRI_HIGH	0

/*
 * For memory <1MB, both LMMF_1MB and LMMF_16MB will be set.
 * For memory from 1MB to 16MB, only LMMF_16MB will be set.
 * For all memory higher than that, neither will be set.
 */
#define LMMF_1MB	0x01
#define LMMF_16MB	0x02

__FLUX_BEGIN_DECLS
/*
 * This routine sets up the malloc_lmm with three regions,
 * one for each of the memory types above.
 * You can then call phys_lmm_add() to add memory to those regions.
 */
void phys_lmm_init(void);

/*
 * Call one of these routines to add a chunk of physical memory found
 * to the appropriate region(s) on the malloc_lmm.
 * The provided memory block may be arbitrarily aligned
 * and may cross region boundaries (e.g. the 16MB boundary);
 * it will be shrunken and split apart as necessary.
 * Note that these routines take _physical_ addresses,
 * not virtual addresses as the underlying LMM routines do.
 */
void phys_lmm_add(vm_offset_t min_pa, vm_size_t size);
void i16_phys_lmm_add(vm_offset_t min_pa, vm_size_t size);
__FLUX_END_DECLS

/*
 * The above routines keep this variable up-to-date
 * so that it always records the highest physical memory address seen so far;
 * i.e. the end address of the highest free block added to the free list.
 * After all physical memory is found and collected on the free list,
 * this variable will contain the highest physical memory address
 * that the kernel should ever have to deal with.
 */
extern vm_offset_t phys_mem_max;

#endif /* _FLUX_X86_PC_PHYS_LMM_H_ */
