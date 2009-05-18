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
 *	Basic page directory and page table management routines for the x86.
 */
#ifndef	_FLUX_X86_BASE_PAGING_H_
#define _FLUX_X86_BASE_PAGING_H_

#include <flux/c/common.h>
#include <flux/inline.h>
#include <flux/x86/proc_reg.h>
#include <flux/x86/paging.h>
#include <flux/x86/base_vm.h>

/*
 * Find the entry in a page directory or page table
 * for a particular linear address.
 */
#define pdir_find_pde(pdir_pa, la)					\
	(&((pd_entry_t*)phystokv(pdir_pa))[lin2pdenum(la)])
#define ptab_find_pte(ptab_pa, la)					\
	(&((pt_entry_t*)phystokv(ptab_pa))[lin2ptenum(la)])

/*
 * Find a page table entry given a page directory and a linear address.
 * Returns NULL if there is no page table covering that address.
 * Assumes that if there is a valid PDE, it's a page table, _not_ a 4MB page.
 */
FLUX_INLINE pt_entry_t *pdir_find_pte(vm_offset_t pdir_pa, vm_offset_t la)
{
	pd_entry_t *pde = pdir_find_pde(pdir_pa, la);
	if (!(*pde & INTEL_PDE_VALID))
		return 0;
	return ptab_find_pte(pde_to_pa(*pde), la);
}

/*
 * Get the value of a page table entry,
 * or return zero if no such entry exists.
 * As above, doesn't check for 4MB pages.
 */
FLUX_INLINE pt_entry_t pdir_get_pte(vm_offset_t pdir_pa, vm_offset_t la)
{
	pt_entry_t *pte = pdir_find_pte(pdir_pa, la);
	return pte ? *pte : 0;
}

__FLUX_BEGIN_DECLS
/*
 * Functions called by the following routines to allocate a page table.
 * Supplies a single page of CLEARED memory, by its physical address.
 * Returns zero if successful, nonzero on failure.
 * The default implementation simply allocates a page from the malloc_lmm,
 * returning -1 if the LMM runs out of memory.
 * This implementation can be overridden, of course.
 */
int ptab_alloc(vm_offset_t *out_ptab_pa);

/*
 * Free a page table allocated using ptab_alloc().
 * The caller must ensure that it is not still in use in any page directories.
 */
void ptab_free(vm_offset_t ptab_pa);

/*
 * Map a 4KB page into a page directory.
 * Calls page_alloc_func if a new page table needs to be allocated.
 * If page_alloc_func returns nonzero, pdir_map_page aborts with that value.
 * Otherwise, inserts the mapping and returns zero.
 * Doesn't check for 4MB pages.
 */
int pdir_map_page(vm_offset_t pdir_pa, vm_offset_t la, pt_entry_t mapping);


/*
 * Map a continuous range of virtual addresses
 * to a continuous range of physical addresses.
 * If the processor supports 4MB pages, uses them if possible.
 * Assumes that there are no valid mappings already in the specified range.
 * The 'mapping_bits' parameter must include INTEL_PTE_VALID,
 * and may include other permissions as desired.
 */
int pdir_map_range(vm_offset_t pdir_pa, vm_offset_t la, vm_offset_t pa,
		   vm_size_t size, pt_entry_t mapping_bits);

/*
 * Change the permissions on an existing mapping range.
 * The 'new_mapping_bits' parameter must include INTEL_PTE_VALID,
 * and may include other permissions as desired.
 * Assumes that the mappings being changed were produced
 * by a previous call to pdir_map_range()
 * with identical linear address and size parameters.
 */
void pdir_prot_range(vm_offset_t pdir_pa, vm_offset_t la, vm_size_t size,
		     pt_entry_t new_mapping_bits);

/*
 * Unmap a continuous range of virtual addresses.
 * Assumes that the mappings being unmapped were produced
 * by a previous call to pdir_map_range()
 * with identical linear address and size parameters.
 */
void pdir_unmap_range(vm_offset_t pdir_pa, vm_offset_t la, vm_size_t size);

/*
 * These functions are primarily for debugging VM-related code:
 * they pretty-print a dump of a page directory or page table
 * and all their mappings.
 */
void pdir_dump(vm_offset_t pdir_pa);
void ptab_dump(vm_offset_t ptab_pa, vm_offset_t base_la);

/*
 * Initialize a basic paged environment.
 * Sets up the base_pdir (see below) which direct maps
 * all physical memory from 0 to (at least) phys_mem_max,
 * and enables paging on the current processor using that page directory.
 * The client can then modify this page directory or create others as needed.
 * The base physical memory mappings are accessible from user mode by default.
 */
void base_paging_init(void);
__FLUX_END_DECLS

/*
 * Physical address of the base page directory,
 * set up by base_paging_init().
 */
extern vm_offset_t base_pdir_pa;

#endif	_FLUX_X86_BASE_PAGING_H_
