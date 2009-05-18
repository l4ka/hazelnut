/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/dnard/init.c
 * Description:   Dnard specific initialization code.
 *                
 * @LICENSE@
 *                
 * $Id: init.c,v 1.17 2001/12/11 23:38:28 ud3 Exp $
 *                
 ********************************************************************/
#include <universe.h>

#include INC_ARCH(cpu.h)
#include INC_ARCH(memory.h)
#include INC_ARCH(farcalls.h)

#include INC_PLATFORM(platform.h)

void setup_kernel_pgtable(void) L4_SECT_INIT;

void init_platform() L4_SECT_INIT;
void init_timer() L4_SECT_INIT;
/* from arm/dnard/interrupt.c */
void init_irqs() L4_SECT_INIT;

extern word_t _start_init[];

extern word_t text_vaddr[];
extern word_t text_paddr[];
extern word_t excp_vaddr[];
extern word_t excp_paddr[];


void setup_kernel_pgtable()
{
    Flush_ID_Cache();

    dword_t i;
    fld_section_t *segment;
    fld_page_t *page;
    sld_largepage_t *large_page;
    sld_smallpage_t *small_page;

    /* Zero out 1st Level Array */
    for(i = 0; i < FLD_MAX; i++)
	kernel_ptdir_p[i].init = 0;

    /* Map in Init Segment */
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

    /* Map in I/O */
    small_page = (sld_smallpage_t *)kernel_pgtable_p + 
	vaddr_to_sld(IO_VBASE);
    small_page->type = SLD_SMALL;
    small_page->ap0 = AP_ALLACCESS; /* THIS IS ACCESSIBLE FROM USER MODE TOO */
    small_page->ap1 = AP_NOACCESS;
    small_page->ap2 = AP_NOACCESS;
    small_page->ap3 = AP_NOACCESS;
    small_page->spba = paddr_to_spba(IO_PBASE);

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
    Flush_ID_Cache();
}


#warning REVIEWME


void init_timer()
{
    /* wait while update in progress */
    while(in_rtc(0x0a) & 0x80);
    
    out_rtc(0x0a, (in_rtc(0x0a) & 0xF0) | 0x07);
    out_rtc(0x0b, in_rtc(0x0b) | 0x48);
    
    /* reset timer intr line */
    in_rtc(0x0c);
    
    printf("%s\n", __FUNCTION__);
}




void init_platform()
{
    printf("%s\n", __FUNCTION__);
    init_irqs();
    init_timer();
    printf("%s done\n", __FUNCTION__);
};




#if 0

/* Map in Sigma0's one-one Address Space */
void
setup_sigma0_mappings()
{
    dword_t i;
    fld_section_t *segment;
    
    /* Map in DRAM Banks (less kernel regions, First 2MB),
       0x08200000 - 0x0E7FFFFF */
    segment = (fld_section_t *)kernel_ptdir + vaddr_to_fld(0x08200000);
    for(i = 0x08200000;
	i < 0x0E800000;
	i += 0x00100000, segment++) {
	segment->type = FLD_SECTION;
	segment->domain = USER_DOMAIN;
	segment->ap = AP_ALLACCESS;
	segment->sba = vaddr_to_fld(i);
    }
    /* Map in I/O Registers at 0x40000000 - 0x40FFFFFF */
    segment = (fld_section_t *)kernel_ptdir + vaddr_to_fld(0x40000000);
    segment->type = FLD_SECTION;
    segment->domain = USER_DOMAIN;
    segment->ap = AP_ALLACCESS;
    segment->sba = vaddr_to_fld(0x40000000);
}

#endif
