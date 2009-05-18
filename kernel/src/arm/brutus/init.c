/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm/brutus/init.c
 * Description:   Brutus specific initalization functions.
 *                
 * @LICENSE@
 *                
 * $Id: init.c,v 1.12 2001/12/13 00:11:00 ud3 Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include INC_ARCH(memory.h)
#include INC_ARCH(farcalls.h)
#include INC_PLATFORM(platform.h)

void setup_kernel_pgtable(void) L4_SECT_INIT;
void init_platform(void) L4_SECT_INIT;
void init_timer(void) L4_SECT_INIT;
void init_irqs(void) L4_SECT_INIT;


extern dword_t _start_init[];
extern dword_t text_paddr[];
extern dword_t text_vaddr[];


/*
 * Function setup_kernel_pgtable ()
 *
 *    Create initial pagetable.  This should contain everything we
 *    need in order to run the kernel.  When the function returns, it
 *    should be safe to enable the MMU.
 *
 */
void setup_kernel_pgtable(void)
{
    dword_t i;
    fld_section_t *segment;
    fld_page_t *page;
    sld_largepage_t *large_page;
    sld_smallpage_t *small_page;

    /* Zero out 1st Level Array */
    for(i = 0; i < FLD_MAX; i++)
	kernel_ptdir_p[i].init = 0;

    /*
     * Map in Init segment
     */
    segment = (fld_section_t *) kernel_ptdir_p +
	vaddr_to_fld((dword_t) _start_init);
    segment->type = FLD_SECTION;	/* 1st level segment entry */
    segment->ap   = AP_NOACCESS;	/* Kernel access only */
    segment->sba  = paddr_to_sba((dword_t) _start_init); /* Address of 1MB Fr */

    /*
     * Map in Text segment
     */

    /* Add 1st level entry */
    page = (fld_page_t *) kernel_ptdir_p + vaddr_to_fld((dword_t) text_vaddr);
    page->type = FLD_PAGE;
    page->ptba = paddr_to_ptba((dword_t) kernel_pgtable_p);

    /* Zero out 2nd level array */
    for(i = 0; i < SLD_MAX; i++)
	kernel_pgtable_p[i].init = 0;

    /* Add 2nd level entry */
    large_page = (sld_largepage_t *) kernel_pgtable_p + 
	vaddr_to_sld(0xFFFF0000 & (dword_t) text_vaddr);
    large_page->type	= SLD_LARGE;
    large_page->buf	= TRUE;
    large_page->cache	= TRUE;
    large_page->ap0	= AP_NOACCESS;
    large_page->ap1	= AP_NOACCESS;
    large_page->ap2	= AP_NOACCESS;
    large_page->ap3	= AP_NOACCESS;
    large_page->lpba	= paddr_to_lpba(0xFFFF0000 & (dword_t) text_paddr);

    /* Copy entry (required for large pages) */
    for(i = 1; i < LRG_ENTS; i++)
	*(large_page + i) = *large_page;

    /*
     * Map in Interrupt Controller registers
     */
    small_page = (sld_smallpage_t *) kernel_pgtable_p + 
	vaddr_to_sld(INTCTRL_VBASE);
    small_page->type	= SLD_SMALL;
    small_page->ap0	= AP_NOACCESS;
    small_page->ap1	= AP_NOACCESS;
    small_page->ap2	= AP_NOACCESS;
    small_page->ap3	= AP_NOACCESS;
    small_page->spba	= paddr_to_spba(INTCTRL_PBASE);

    /*
     * Map in OS Timer registers
     */
    small_page = (sld_smallpage_t *) kernel_pgtable_p + 
	vaddr_to_sld(OSTIMER_VBASE);
    *(small_page) = kernel_pgtable_p[vaddr_to_sld(INTCTRL_VBASE)].small;
    small_page->spba = paddr_to_spba(OSTIMER_PBASE); 
   
    /*
     * Map in UART Serial Port registers
     */
    small_page = (sld_smallpage_t *) kernel_pgtable_p + 
	vaddr_to_sld(UART_VBASE);
    *(small_page) = kernel_pgtable_p[vaddr_to_sld(INTCTRL_VBASE)].small;
    small_page->spba = paddr_to_spba(UART_PBASE);
    small_page->ap0 = AP_ALLACCESS;
    small_page->ap1 = AP_ALLACCESS;
    small_page->ap2 = AP_ALLACCESS;
    small_page->ap3 = AP_ALLACCESS;

    /*
     * Map in GPIO registers
     */
    small_page = (sld_smallpage_t *) kernel_pgtable_p +
	vaddr_to_sld(GPIO_VBASE);
    *(small_page) = kernel_pgtable_p[vaddr_to_sld(INTCTRL_VBASE)].small;
    small_page->spba = paddr_to_spba(GPIO_PBASE);

    /* 
     * Map Reset Controller Registers
     */
    small_page = (sld_smallpage_t *) kernel_pgtable_p +
	vaddr_to_sld(RSTCTL_VBASE);
    *(small_page) = kernel_pgtable_p[vaddr_to_sld(INTCTRL_VBASE)].small;
    small_page->spba = paddr_to_spba(RSTCTL_PBASE);
    
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
    init_irqs();
    init_timer();
}
