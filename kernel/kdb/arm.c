/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     arm.c
 * Description:   ARM specific kdebug stuff.
 *                
 * @LICENSE@
 *                
 * $Id: arm.c,v 1.25 2001/12/13 03:06:05 ud3 Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include INC_ARCH(cpu.h)
#include INC_PLATFORM(platform.h)
#include "kdebug.h"





extern void kdebug_init_serial();
void kdebug_init_arch()
{
#if defined(CONFIG_DEBUGGER_IO_INCOM) || defined(CONFIG_DEBUGGER_IO_OUTCOM)
    kdebug_init_serial();
#endif
}

#if defined(CONFIG_DEBUGGER_KDB)
int kdebug_arch_entry(exception_frame_t* frame)
{
    if ((frame->spsr & 0x1F) == 0x10)	/* called from user mode */
    {
	if (((*(ptr_t)frame->r14) >> 24) == 0xEA)
	{
	    printf("%s", (char*) (frame->r14 + 4));
	    return 0;
	};
	if (((*(ptr_t)frame->r14) >> 16) == 0xE35E)
	    switch ((*(ptr_t)frame->r14) & 0xFFFF)
	    {
	    case 0:			/* outchar */
		putc(frame->r0 & 0xff);
		return 1;
	    case 2:			/* outstring */
		printf((char*)frame->r0);
		return 1;
	    case 5:			/* outhex32 */
		printf("%x", frame->r0);
		return 1;
	    case 11:                    /* outdec */
		printf("%d", frame->r0);
		return 1;
	    case 13:
		frame->r0 = getc();
		return 1;
	    default:
		printf ("Unknown kdebug command: %x\n",
			(*(ptr_t)frame->r14) & 0xFFFF);
		return 0;
	    };
    }
    return 0;
}

void kdebug_arch_exit(exception_frame_t* frame)
{
    return;
};

void kdebug_perfmon()
{
    return;
}

void kdebug_dump_pgtable(ptr_t pgtable)
{

    int start=0; int entries=FLD_MAX;
    fld_t* page_dir = (fld_t*) phys_to_virt(pgtable);
    int i, j;
    sld_t *page_table;

    printf("\nLevel 1 Page Table Dump at %x\n", (int)page_dir);
    for( i = start; (i < FLD_MAX) && (entries > 0); i++, entries-- ) 
	switch (page_dir[i].invalid.type) {
	case FLD_SECTION:
	    printf("%x -> %x (1M), Dom %x, ", fld_to_vaddr(i), page_dir[i].init,
		   page_dir[i].section.domain);
	    switch (page_dir[i].section.ap) {
	    case AP_RESTRICTED:
		printf("AP_RESTRICTED ");
		break;
	    case AP_NOACCESS:
		printf("AP_NOACCESS ");
		break;
	    case AP_READACCESS:
		printf("AP_READACCESS ");
		break;
	    case AP_ALLACCESS:
		printf("AP_ALLACCESS ");
		break;
	    }
	    printf("\n");
	    break;
	case FLD_PAGE:
	    printf("%x -> Page, Dom %x, entry %x\n", fld_to_vaddr(i), 
		   page_dir[i].page.domain, page_dir[i].init);
	    page_table = 
		(sld_t *)phys_to_virt(ptba_to_paddr(page_dir[i].page.ptba));
	    printf("\tLevel 2 Page Table Dump at %x\n", (int)page_table);
	    for(j = 0; j < SLD_MAX; j++)
		switch (page_table[j].invalid.type) {
		case SLD_SMALL:
		    printf("\t%x -> %x (4K), ",
			   (fld_to_vaddr(i)|sld_to_vaddr(j)), 
			   page_table[j].init);
		    switch (page_table[j].small.ap0) {
		    case AP_RESTRICTED:
			printf("AP_RESTRICTED, ");
			break;
		    case AP_NOACCESS:
			printf("AP_NOACCESS, ");
			break;
		    case AP_READACCESS:
			printf("AP_READACCESS, ");
			break;
		    case AP_ALLACCESS:
			printf("AP_ALLACCESS, ");
			break;
		    }
		    printf("\n");
		    break;
		case SLD_LARGE:
		    printf("\t%x -> %x (64K), ", 
			   (fld_to_vaddr(i) | sld_to_vaddr(j)), 
			   page_table[j].init);
		    switch (page_table[j].large.ap0) {
		    case AP_RESTRICTED:
			printf("AP_RESTRICTED, ");
			break;
		    case AP_NOACCESS:
			printf("AP_NOACCESS, ");
			break;
		    case AP_READACCESS:
			printf("AP_READACCESS, ");
			break;
		    case AP_ALLACCESS:
			printf("AP_ALLACCESS, ");
			break;
		    }
		    printf("\n");
		    j += LRG_ENTS;
		    break;
		}
	    break;
	}
    printf("\n");
}

void kdebug_dump_frame(exception_frame_t *frame)
{
    dword_t fsr, far;
    __asm__ __volatile__
        ("mrc   p15, 0, %0, c5, c0      \n\t"
         "mrc   p15, 0, %1, c6, c0      \n\t"
         : "=r" (fsr), "=r" (far));       
    
    printf("frame: %p excp no: %x\n", frame, frame->excp_no);
    printf(" r0  %x  %x  %x  %x\n",
	   frame->r0, frame->r1, frame->r2, frame->r3);
    printf(" r4  %x  %x  %x  %x\n",
	   frame->r4, frame->r5, frame->r6, frame->r7);
    printf(" r8  %x  %x  %x  %x\n",
	   frame->r8, frame->r9, frame->r10, frame->r11);
    printf(" r12 %x  %x  %x\n",
	   frame->r12, frame->r13, frame->r14);
    printf("spsr %x  lr %x  far %x\n", frame->spsr, frame->lr, far);
    printf("cpsr %x  sp %x  fsr %x\n", frame->cpsr, frame->sp, fsr);

}


void kdebug_pf_tracing(int state)
{
    extern int __kdebug_pf_tracing;
    __kdebug_pf_tracing = state;
}

void kdebug_single_stepping(exception_frame_t *frame, dword_t onoff)
{
}

void kdebug_breakpoint()
{
}

void kdebug_arch_help()
{
}

void kdebug_cpustate(tcb_t *current)
{
}

#endif /* CONFIG_DEBUGGER_KDB */

int kdebug_step_instruction(exception_frame_t * frame)
{
    printf("s - Unsupported\n");
    return 0;
}

int kdebug_step_block(exception_frame_t * frame)
{
    printf("S - Unsupported\n");
    return 0;
}


void kdebug_disassemble(exception_frame_t * frame)
{
#if defined(CONFIG_DEBUG_DISAS)
    extern int arm_disas(dword_t pc);
    char c;
    dword_t pc;
 restart:
    printf("ip [current]: ");
    pc = kdebug_get_hex(frame->r14, "current");
    do {
	printf("%x:\t", pc);
	pc += arm_disas(pc);
	printf("\n");
	c = getc();
    } while ((c != 'q') && (c != 'u'));
    if (c == 'u')
	goto restart;
#endif
};




