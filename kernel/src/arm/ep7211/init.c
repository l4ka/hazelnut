/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/ep7211/init.c
 * Description:   EP7211 specific initialization code.
 *                
 * @LICENSE@
 *                
 * $Id: init.c,v 1.10 2001/11/22 13:18:49 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

#include INC_ARCH(cpu.h)
#include INC_ARCH(memory.h)
#include INC_ARCH(farcalls.h)

#include INC_PLATFORM(platform.h)

void setup_kernel_pgtable(void) L4_SECT_INIT;

void init_platform() L4_SECT_INIT;
//void init_timer() L4_SECT_INIT;
void init_serial() L4_SECT_INIT;
/* from arm/dnard/interrupt.c */
void init_irqs() L4_SECT_INIT;

extern word_t _start_init[];

extern word_t excp_vaddr[];
extern word_t excp_paddr[];


void setup_kernel_pgtable()
{
    dword_t i;
    fld_section_t *segment;
    fld_page_t *page;
    sld_largepage_t *large_page;
    sld_smallpage_t *small_page;

    /* Zero out 1st Level Array */
    for(i = 0; i < FLD_MAX; i++)
	kernel_ptdir_p[i].init = 0;

    /* Map in Init Segment - this first entry might
       be replaced when installing the kernel text entry */
    segment = (fld_section_t *)kernel_ptdir_p + vaddr_to_fld((dword_t)_start_init);
    segment->type = FLD_SECTION;          /* 1st Level Segment Entry */
    segment->ap = AP_NOACCESS;            /* Kernel Access only */
    segment->sba = paddr_to_sba((dword_t)_start_init); /* Address of 1MB Frame */

    /* Map in Text Segment */
    /* Add 1st Level Entry */
    page = (fld_page_t *)kernel_ptdir_p + vaddr_to_fld(KERNEL_VIRT);
    page->type = FLD_PAGE;
    page->ptba = paddr_to_ptba((dword_t)kernel_pgtable_p);

    /* Zero out 2nd Level Array */
    for(i = 0; i < SLD_MAX; i++)
	kernel_pgtable_p[i].init = 0;

    /* Add 2nd Level Entry */
    large_page = (sld_largepage_t *)kernel_pgtable_p + 
	vaddr_to_sld(KERNEL_VIRT);
    large_page->type = SLD_LARGE;
    large_page->buf = TRUE;
    large_page->cache = TRUE;
    large_page->ap0 = AP_NOACCESS;
    large_page->ap1 = AP_NOACCESS;
    large_page->ap2 = AP_NOACCESS;
    large_page->ap3 = AP_NOACCESS;
    large_page->lpba = paddr_to_lpba((dword_t) excp_paddr);
    /* Copy Entry (Required for Large Pages) */
    for(i = 1; i < LRG_ENTS; i++)
	*(large_page + i) = *large_page;

    *(volatile dword_t*) 0x800022c0 = ((1 << 6) | (0x7 << 2) | 0);

    /* Map in I/O */
    large_page = (sld_largepage_t *)kernel_pgtable_p + 
	vaddr_to_sld(IO_VBASE);
    large_page->type = SLD_LARGE;
    large_page->ap0 = AP_ALLACCESS; /* THIS IS ACCESSIBLE FROM USER MODE TOO */
    large_page->ap1 = AP_ALLACCESS;
    large_page->ap2 = AP_ALLACCESS;
    large_page->ap3 = AP_ALLACCESS;
    large_page->cache = FALSE;
    large_page->buf = FALSE;
    large_page->lpba = paddr_to_lpba(IO_PBASE);
    /* Copy Entry (Required for Large Pages) */
    for(i = 1; i < LRG_ENTS; i++)
	*(large_page + i) = *large_page;

    /* Map in Contiguous RAM */
    segment = (fld_section_t *)kernel_ptdir_p + vaddr_to_fld(cpaddr);
#if ((cpaddr + PHYS_DRAMSIZE) < cpaddr)
#error: Too much memory
#endif
    for (i = cpaddr; i < cpaddr+DRAM_SIZE; i+=0x100000)	/* in steps of 1M */
	{
	    segment->sba = paddr_to_sba(virt_to_phys(i));
	    segment->type = FLD_SECTION;	/* 1st Level Segment Entry  */
	    segment->ap = AP_NOACCESS;		/* Kernel Access only       */
	    segment++;
	};
}

void init_platform()
{
    printf("%s\n", __FUNCTION__);
    init_irqs();
    printf("%s done\n", __FUNCTION__);
};

