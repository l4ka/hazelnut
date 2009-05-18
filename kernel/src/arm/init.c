/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/init.c
 * Description:   ARM specifuc initalization code.
 *                
 * @LICENSE@
 *                
 * $Id: init.c,v 1.26 2001/12/07 19:09:50 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

#include INC_ARCH(cpu.h)
#include INC_ARCH(memory.h)
#include INC_PLATFORM(platform.h)

/* relocate to init section */
void init_arch_1() L4_SECT_INIT;
void init_arch_2() L4_SECT_INIT;
void init_paging() L4_SECT_INIT;
void setup_excp_vector() L4_SECT_INIT;
void zero_memory(void * address, int size);

void init_platform() L4_SECT_INIT;

void init_arch_2()
{
}

/* post processing (e.g., flushing page-table entries) */
void init_arch_3()
{
    extern dword_t _start_init[];

    /* kmem is initialized now */
    __zero_page = kmem_alloc(PAGE_SIZE);
    zero_memory(__zero_page, PAGE_SIZE);

    /* remove init area from pagetable */
    ((fld_t *) kernel_ptdir)[vaddr_to_fld((dword_t) _start_init)].page.type = FLD_INVALID;

    flush_tlb();
    Flush_ID_Cache();
}



void zero_memory(void * address, int size)
{
    dword_t *tmp = (dword_t*)address;
    size /= 4;
    while(size--)
	*tmp++ = 0;
}


#include INC_ARCH(farcalls.h)

/* architecture specific initialization
 * invoked by /init.c before anything is done
 * but running on virtual addresses
 */
void init_arch_1()
{
    extern dword_t kmem_start;
    extern dword_t kmem_end;
    dword_t cpsr;
    dword_t cp15_0;
    
    kernel_ptdir = phys_to_virt((dword_t*) kernel_ptdir_p);

    init_platform();

    /* set the range of memory for the kernel memory allocator */
    kmem_start = phys_to_virt(kernel_info_page.main_mem_high-256*1024);
    kmem_end = phys_to_virt(kernel_info_page.main_mem_high-1);

    __asm__ __volatile__(
	"mrs	%0, cpsr					\n"
	"mrc	p15, 0, %1, c0, c0, 0	@ Load CPUID into %1	\n"
	:"=r"(cpsr), "=r"(cp15_0));	
    printf("Hello, I'm your kernel. Running on %x, CPSR is %x\n", cp15_0, cpsr);

}


#if !defined(HAVE_ARCH_KMEMORY)
#if !defined(EXCEPTION_VECTOR_RELOCATED)
static const ptr_t (*__farcall_kmem_alloc)(dword_t) L4_SECT_INIT = kmem_alloc;
#define kmem_alloc __farcall_kmem_alloc
#endif
#else
static const ptr_t (*__farcall_kmem_alloc_1k)() L4_SECT_INIT = kmem_alloc_1k;
#define kmem_alloc_1k __farcall_kmem_alloc_1k
#endif
    
/* architecture initialization - before everything else */
extern word_t excp_vaddr[];
extern word_t excp_paddr[];

/*
 * Function setup_excp_vector ()
 *
 *    Map the page containing the exception vector.
 *
 */
void setup_excp_vector(void)
{
#if !defined(EXCEPTION_VECTOR_RELOCATED)
    int i;
    sld_t *excp_pgtable;
    fld_page_t *page;
    sld_smallpage_t *small_page;

    /* Allocate 1KB page for holding the page table */
    excp_pgtable = (sld_t *) kmem_alloc(SLD_SIZE);

    /* Add 1st Level Entry */
#warning REVIEWME: ptdir[excp_vaddr] ?
    page = (fld_page_t *) kernel_ptdir;
    page->type          = FLD_PAGE;
    page->domain        = USER_DOMAIN;
    page->ptba          =
        paddr_to_ptba(virt_to_phys((dword_t) excp_pgtable));

    /* Zero out 2nd Level Array */
    for(i = 0; i < SLD_MAX; i++)
        excp_pgtable[i].init = 0;

    /* Add 2nd Level Entry */
    small_page = (sld_smallpage_t *) excp_pgtable;
    small_page->type    = SLD_SMALL;
    small_page->buf     = TRUE;
    small_page->cache   = TRUE;
    small_page->ap0     = AP_NOACCESS;
    small_page->ap1     = AP_NOACCESS;
    small_page->ap2     = AP_NOACCESS;
    small_page->ap3     = AP_NOACCESS;
    small_page->spba    = paddr_to_spba((dword_t) excp_paddr);
#endif
}

