/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86.c
 * Description:   Various x86 specifuc kdebug stuff.
 *                
 * @LICENSE@
 *                
 * $Id: x86.c,v 1.81 2001/12/09 04:21:02 ud3 Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <tracepoints.h>
#include "kdebug.h"

#ifdef CONFIG_TRACEBUFFER
tracebuffer_t *tracebuffer;
#endif

#if defined(CONFIG_DEBUGGER_NEW_KDB)
static dword_t pmc_overflow_interrupt_mask = 0;
extern "C" void apic_pmc_of_int();
#endif

#define any_to_virt(x) ((x) < KERNEL_OFFSET ? ((x)+KERNEL_OFFSET) : (x))

/* x86-only prototypes */
void kdebug_init_serial(void) L4_SECT_KDEBUG;
void kdebug_setperfctr(dword_t sel, dword_t value) L4_SECT_KDEBUG;


void kdebug_init_arch()
{
#if defined(CONFIG_DEBUGGER_IO_INCOM) || defined(CONFIG_DEBUGGER_IO_OUTCOM)
    kdebug_init_serial();
#endif
}


void kdebug_hwreset()
{
    __asm__ __volatile__ (
	"	movb	$0xFE, %al	\n"
	"	outb	%al, $0x64	\n"
	);
}

void kdebug_single_stepping(exception_frame_t *frame, dword_t onoff)
{
    frame->eflags &= ~(1 << 8);
    frame->eflags |= (onoff << 8);
}




#if defined(CONFIG_DEBUGGER_KDB) || defined(CONFIG_DEBUGGER_GDB)

enum kdebug_steping_t {
    STEPPING_NONE,
    STEPPING_INSTR,
    STEPPING_BLOCK
} kdebug_stepping = STEPPING_NONE;


void dump_count()
{
#define MAX_DUMP_COUNT 10
    static int count = 0;
    count++;
    if (count == MAX_DUMP_COUNT)
    {
	getc();
	count = 0;
    }
}

void kdebug_dump_pgtable(ptr_t pgtable)
{
    int d_beg = 0, d_end = 0;
    char c;

    if ((dword_t) pgtable < TCB_AREA)
	/* seems to be a physical address */
	pgtable = phys_to_virt(pgtable);

    c = kdebug_get_choice("Dump pagetable", "All/User/Kernel/Tcbs"
#if defined(CONFIG_ENABLE_SMALL_AS)
			  "/Small spaces"
#endif
			  , 'a');

    switch (c)
    {
    case 'a': d_beg = 0; d_end = 1024; break;
    case 'u':
	d_beg = 0;
	d_end = (USER_AREA_MAX >> PAGEDIR_BITS);
	break;
    case 'k':
	d_beg = (KERNEL_VIRT >> PAGEDIR_BITS);
	d_end = 1024;
	break;
    case 't':
	d_beg = (TCB_AREA >> PAGEDIR_BITS);
	d_end = ((TCB_AREA + TCB_AREA_SIZE) >> PAGEDIR_BITS);
	break;
#if defined(CONFIG_ENABLE_SMALL_AS)
    case 's':
	d_beg = (SMALL_SPACE_START >> PAGEDIR_BITS);
	d_end = (TCB_AREA >> PAGEDIR_BITS);
	break;
#endif
    }
    

    for (int i = d_beg; i < d_end; i++)
    {
	if (pgtable[i]) {
	    if (0) {}
#if defined(CONFIG_ENABLE_SMALL_AS)
	    else if ( i == SPACE_ID_IDX )
	    {
		space_t * space = (space_t *) virt_to_phys (pgtable);
		if (!space->is_small ())
		    printf("%x -> small_space=%x  [INVALID]\n",
		       0x400000 * i, pgtable[i]);
		else
		    printf("%x -> small_space = %x  [size=%dMB  num=%d]\n",
			   0x400000 * i, pgtable[i],
			   (space->smallid ().bytesize ()) / (1024*1024),
			   space->smallid ().num ());
	    }
	    else if ( i >= (int) GDT_ENTRY1_IDX && i <= (int) GDT_ENTRY2_IDX )
	    {
		printf("%x -> GDT Entry %d = %x\n",
		       0x400000 * i, i - GDT_ENTRY1_IDX, pgtable[i]);
	    }
#endif
	    else if ( (pgtable[i] & PAGE_SUPER) && (pgtable[i] & PAGE_SUPER) )
	    {
		printf("%x -> %x (4MB)  addr=%x  [%c%c%c%c%c%c]\n",
		       0x400000 * i, pgtable[i],
		       pgtable[i] & PAGEDIR_MASK,
		       pgtable[i] & PAGE_WRITABLE	? 'w' : 'r',
		       pgtable[i] & PAGE_USER		? 'u' : 'k',
		       pgtable[i] & PAGE_ACCESSED	? 'a' : '.',
		       pgtable[i] & PAGE_DIRTY		? 'd' : '.',
		       pgtable[i] & PAGE_GLOBAL		? 'g' : '.',
		       pgtable[i] & PAGE_CACHE_DISABLE	? '.' : 'c');
		//dump_count();
	    }
	    else {
		printf("%x -> %x (PT)   addr=%x\n", 0x400000 * i, pgtable[i],
		       pgtable[i] & PAGE_MASK);
		dword_t * ptab = phys_to_virt(ptr_t(pgtable[i] & PAGE_MASK));
		for (int j = 0; j < 1024; j++) {
		    if (ptab[j] & PAGE_VALID) {
			printf("%x -> %x (4K)   addr=%x  [%c%c%c%c%c%c]\n",
			       (0x400000 * i + 0x1000 * j), ptab[j],
			       ptab[j] & PAGE_MASK,
			       ptab[j] & PAGE_WRITABLE		? 'w' : 'r',
			       ptab[j] & PAGE_USER		? 'u' : 'k',
			       ptab[j] & PAGE_ACCESSED		? 'a' : '.',
			       ptab[j] & PAGE_DIRTY		? 'd' : '.',
			       ptab[j] & PAGE_GLOBAL		? 'g' : '.',
			       ptab[j] & PAGE_CACHE_DISABLE	? '.' : 'c');
			//dump_count();
		    }
		}
	    }
	}
    }
}

void dump_eflags(const dword_t eflags)
{
    printf("%c%c%c%c%c%c%c%c%c%c%c",
	   eflags & (1 <<  0) ? 'C' : 'c',
	   eflags & (1 <<  2) ? 'P' : 'p',
	   eflags & (1 <<  4) ? 'A' : 'a',
	   eflags & (1 <<  6) ? 'Z' : 'z',
	   eflags & (1 <<  7) ? 'S' : 's',
	   eflags & (1 << 11) ? 'O' : 'o',
	   eflags & (1 << 10) ? 'D' : 'd',
	   eflags & (1 <<  9) ? 'I' : 'i',
	   eflags & (1 <<  8) ? 'T' : 't',
	   eflags & (1 << 16) ? 'R' : 'r',
	   ((eflags >> 12) & 3) + '0'
	);
}

void kdebug_dump_frame(exception_frame_t *frame)
{
#if !defined(CONFIG_SMP)
    printf("exception %d (current: %p, cr3: %p)\n", frame->fault_code, 
	   frame, get_current_pagetable());
#else
    printf("exception %d (current: %p, cr3: %p, cpu: %d, apic: %d)\n", 
	   frame->fault_code, frame, get_current_pagetable(), get_cpu_id(),
	   get_apic_cpu_id());
#endif
    printf("fault addr: %x\tstack: %x\terror code %x\n", 
	   frame->fault_address, frame->user_stack, frame->error_code);

    printf("eax: %x\tebx: %x\n", frame->eax, frame->ebx);
    printf("ecx: %x\tedx: %x\n", frame->ecx, frame->edx);
    printf("esi: %x\tedi: %x\n", frame->esi, frame->edi);
    printf("ebp: %x\tefl: %x [", frame->ebp, frame->eflags);
    dump_eflags(frame->eflags);printf("]\n");
#if defined(CONFIG_ENABLE_SMALL_AS)
    printf("cs:  %x\tss:  %x\n", (word_t) frame->cs, (word_t) frame->ss);
    printf("ds:  %x\tes:  %x\n", (word_t) frame->ds, (word_t) frame->es);
#endif
}

void kdebug_dump_frame_short(exception_frame_t *frame)
{
    printf("eip=%p              ", frame->fault_address);
#if defined(CONFIG_DEBUG_DISAS)
    extern int x86_disas(dword_t pc);
    x86_disas(frame->fault_address);
#endif
    printf("\n"
	   "             "
	   "eax=%p ebx=%p ecx=%p edx=%p ebp=%p\n"
	   "             "
	   "esi=%p edi=%p esp=%p efl=%p [",
	   frame->eax, frame->ebx, frame->ecx, frame->edx, frame->ebp,
	   frame->esi, frame->edi,frame->user_stack, frame->eflags);
    dump_eflags(frame->eflags);printf("]\n");
}

#if !defined(CONFIG_DEBUGGER_NEW_KDB) /* not necessary in the new debugger */
void kdebug_pf_tracing(int state)
{
    extern int __kdebug_pf_tracing;
    __kdebug_pf_tracing = state;
}
#endif

#if defined(CONFIG_DEBUGGER_NEW_KDB)
static qword_t saved_time;
static int cache_enabled = 0;

void use_cache()
{
    (cache_enabled ^= 1) ? printf("cache enabled\n") : printf("cache disabled\n");
}    

dword_t get_user_time() {
    return ((dword_t*)saved_time)[1];
}

dword_t* change_idt_entry(int interrupt_no, ptr_t new_handler) {
    
    extern idt_desc_t idt[];
    dword_t original_handler_address;
    
    //idt = read_idt_register();    
    
    if ((interrupt_no < IDT_SIZE) && (interrupt_no >= 0) ) {
	
	//storing the old value of the IDT entry 
	original_handler_address =
	    idt[interrupt_no].x.d.offset_high <<16 |
	    idt[interrupt_no].x.d.offset_low;

	//here i register the new handler
	idt[interrupt_no].x.d.offset_high = ((dword_t) (new_handler) >> 16);
	idt[interrupt_no].x.d.offset_low = ((dword_t) (new_handler));

	printf("the address of the new handler is: %x\n", new_handler);
	printf("the address of the old handler is: %x\n", original_handler_address);
    }
    else {
	printf("interrupt no. out of range!\n");
	original_handler_address = 0; //error!
    }
    return (ptr_t) original_handler_address;
}

dword_t* change_sysenter_eip_msr(ptr_t new_handler){
    ptr_t original_sysenter_eip_msr = (ptr_t) rdmsr(0x176);
    wrmsr(0x176, (qword_t)new_handler);
    return original_sysenter_eip_msr;
}

//#if defined(...) x686
static dword_t *original_pmc_of_int = 0;

void enable_pmc_of_int() {
    /* set the mask for enable interrupt
       it is enabled for both counters till it is disabled. */
    pmc_overflow_interrupt_mask = (1<<20);

    //the counters which should generate the interrupts must be reseted.
    kdebug_perfmon();
    
    //enter the new handler to the idt
    original_pmc_of_int = change_idt_entry(APIC_PERFCTR_INT_VECTOR, (ptr_t)(*apic_pmc_of_int));
}

void disable_pmc_of_int() {
    //clear the mask to disable interrupt
    pmc_overflow_interrupt_mask = 0;

// clear bit 20 in the eventselect registers (disable the interrupt). the event dosn't change.
    qword_t dummy;
    
    dummy = rdmsr(390);
    dummy = dummy & (1 << 20);
    wrmsr(390, dummy);

    //bring the old handler back.
    change_idt_entry(APIC_PERFCTR_INT_VECTOR, original_pmc_of_int);
}


/**
 * test if the pagefault was generated in usermode
 * last change: 01.18.2001
 **/

int pf_usermode(dword_t errcode) {
    
#define PF_USERMODE(x)          (x & PAGE_USER)  
#define PAGE_USER          (1<<2)                

  return( PF_USERMODE(errcode) );
}

dword_t get_pf_address() {
    register dword_t tmp;
    
    asm ("movl  %%cr2, %0\n"
         :"=r" (tmp));
    
    return tmp;
}

#endif /*new debugger*/

int kdebug_arch_entry(exception_frame_t * frame)
{
#if defined (CONFIG_DEBUGGER_NEW_KDB)
/* save the time */
    saved_time = rdtsc();

# if defined(CONFIG_ARCH_X86_I686)
//only on Intel 686
/* disable the performancecounters while in debugger */
    qword_t dummy;

    dummy = rdmsr(0x186);
    dummy = dummy & (1 << 22);
    wrmsr(0x186, dummy);
# endif /* config_arch_x86_i686 */

    if(cache_enabled)
    {
/* disable cache while in debugger */
	asm volatile(
	    "mov %%cr0, %%eax\n" /* copy CR0 into eax */
	    "orl $((1<<30)|(1<<29)), %%eax\n" /* set bits CD and NW */
	    "mov %%eax, %%cr0\n" /* write back in CR0 */
	    : /* no outputs */
	    : /* no inputs */
	    : "eax"
	    );
    }
#endif /*new debugger*/
    /*
     * The return value of this function determines whether
     * the interactive kernel debugger is invoked or not.
     * Returning 0 invokes the KDB.
     */
    
    if (frame->fault_code == 3)
    {
	/* breakpoint exception - KDB interface
	 *
	 * The KDB interface is based on the int3 instruction. The
	 * int3-handler checks the instructions that caused the
	 * exception to decode the requested operation.
	 *
	 * The following code sequences are known so far:
	 *
	 * --- enter_kdebug --- print text thru KDIO and enter KDB
	 *
	 *	int3
	 *	jmp	1f
	 *	.ascii	"some text"
	 *  1:
	 *
	 *	If "some text" starts with *, the text is written to
	 *	KDB's trace and the interactive KDB is not invoked.
	 *
	 *
	 * --- kd_display --- print text thru KDIO
	 *	int3
	 *	nop
	 *	jmp	1f
	 *	.ascii	"some text"
	 *  1:
	 *
	 *
	 *
	 * --- other KDB ops ---
	 *	int3
	 *	cmp	x, %al
	 *
	 * with x
	 *	0x00	print character in %al
	 *	0x01	print pascal string at %eax
	 *	0x02	print 0-terminated string at %eax
	 *	0x03	clear page/screen
	 *	0x04	cursor control			???
	 *	0x05	print %eax as 32-bit hex
	 *	0x06	print %eax as 20-bit hex
	 *	0x07	print %eax as 16-bit hex
	 *	0x08	print %eax as 12-bit hex
	 *	0x09	print %eax as  8-bit hex
	 *	0x0a	print %eax as  4-bit hex
	 *	0x0b	print %eax as decimal
	 *	0x0c	read character into %al (non-blocking)
	 *	0x0d	read character into %al (blocking)
	 *	0x0e	read 32-bit hex into %eax
	 *	0x0f	read 16-bit hex into %eax
	 *	0x10	read 8-bit hex into %eax
	 *	0x11	read 4-bit hex into %eax	???
	 *	0x1E	set EvntSel0 MSR to %eax	(proposed)
	 *	0x1F	set EvntSel1 MSR to %eax	(proposed)
	 *
	 */

	unsigned char * c = (unsigned char *) frame->fault_address;

#if defined(CONFIG_ENABLE_SMALL_AS)
	/*
	 * Make sure that we translate user-level addresses into
	 * something that can be accessed through kernel segments.
	 */
	tcb_t *current = ptr_to_tcb((ptr_t) frame);

	if ( (dword_t) current >= TCB_AREA &&
	     (dword_t) current <  TCB_AREA + TCB_AREA_SIZE )
	{
	    smallid_t small = current->space->smallid ();
	    if (small.is_valid () && (dword_t) c < small.bytesize ())
		c += small.offset ();
	}
#endif
#if !defined(CONFIG_DEBUGGER_GDB)
	/* The fault_address points to the instruction following the int3.
	   For readability, we let the fault_address instead point to the
	   int3 instruction itself and restore it in kdebug_arch_exit */
	frame->fault_address--;
#endif

	switch(*c)
	{
	case 0x3c:
	    TRACEPOINT(KDB_OPERATION, printf("KDB operation (0x%02x)\n",c[1]));
	    switch(c[1])
	    {
	    case 0x00:			/* outchar	*/
		putc(frame->eax & 0xff);
		return 1;
	    case 0x01:			/* outstring	*/
	    {
		/* print a string: <length>,<string> */
		char* p = (char*) any_to_virt(frame->eax);
		for (int i = *p++; i--; p++)
		    putc(*p);
		return 1;
	    }
	    case 0x02:			/* kd_display	*/
		printf((char*)frame->eax);
		return 1;
	    case 0x05:			/* outhex32	*/
		printf("%x", frame->eax);
		return 1;
	    case 0x07:			/* outhex16	*/
		printf("%4x", frame->eax & 0xFFFF);
		return 1;
	    case 0x08:			/* outhex8	*/
		printf("%3x", frame->eax & 0xFFF);
		return 1;
	    case 0x09:			/* outhex8	*/
		printf("%2x", frame->eax & 0xFF);
		return 1;
	    case 0x0b:                  /* outdec	*/
		printf("%d", frame->eax);
		return 1;
	    case 0x0d:			/* inchar	*/
		frame->eax = getc();
		return 1;
	    case 0x0e:			/* inhex32	*/
		frame->eax = kdebug_get_hex();
		return 1;
	    case 0x0f:			/* inhex16	*/
		frame->eax &= 0xFFFF0000;
		frame->eax |= (kdebug_get_hex() & 0xFFFF);
		return 1;
	    case 0x11:			/* inhext	*/
		frame->eax = kdebug_get_hex();
		return 1;
	    case 0x1E:			/* set EvntSel0	*/
	    case 0x1F:			/* set EvntSel1	*/
#if defined(CONFIG_PERFMON)
		kdebug_setperfctr(c[1] & 1, frame->eax);
#endif
		return 1;
	    default:
		printf("unknown KDIO call %2x\n", c[1]);
		return 0;
	    }
	    break;
	case 0x90:			/* kd_display	*/
	    c++;
	    if (c[0] == 0xeb)
	    {
		for (int i = 0; i < c[1]; i++)
		    putc(c[i+2]);
		return 1;
	    } break;
	case 0xeb:			/* enter_kdebug	*/
	{
	    if (c[1] > 0 && c[2] == '*')
		/* trace event - not implemented */
		return 1;

	    printf("--- ");
	    for (int i = 0; i < c[1]; i++)
		putc(c[i+2]);
	    printf(" ---\n");
	    return 0;
	}
	}
	return 0;
    }

    if ( frame->fault_code == 1 )
    {
	/* Debug exception */
	switch ( kdebug_stepping )
	{
	case STEPPING_BLOCK:
	    dword_t last_branch_ip;
	    wrmsr(IA32_DEBUGCTL, 0);
#if defined(CONFIG_ARCH_X86_I686)
	    last_branch_ip = rdmsr(IA32_LASTBRANCHFROMIP);
#elif defined(CONFIG_ARCH_X86_P4)
	    last_branch_ip = (rdmsr(IA32_LASTBRANCH_0 +
				    rdmsr(IA32_LASTBRANCH_TOS)) >> 32);
#endif
#if defined(CONFIG_DEBUG_DISAS)
	    int x86_disas(dword_t);
	    printf("eip=%p              ", last_branch_ip);
	    x86_disas(last_branch_ip);
	    putc('\n');
#else
	    printf("eip=%p branch to\n", last_branch_ip);
#endif
	    /* FALLTHROUGH */

	case STEPPING_INSTR:
	    kdebug_dump_frame_short(frame);
	    kdebug_stepping = STEPPING_NONE;
	    kdebug_single_stepping(frame, 0);
	    return 0;

	default:
	{
	    dword_t *dr3;
	    asm ("mov %%dr3, %0" :"=r"(dr3));
	    if (dr3 != 0)
	    {
		if ((*dr3 & 0xff) != 0) {
		    frame->eflags |= (1 << 16);
		    return 1;
		}
	    }
	}
	    frame->eflags |= (1 << 16);
	    kdebug_dump_frame(frame);
	    return 0;
	}
    }

    if ( frame->fault_code == 2 )
    {
	/* NMI */
	printf("\nNMI\n");
	kdebug_dump_frame(frame);
	return 0;
    }

    return 0;
}

void kdebug_arch_exit(exception_frame_t * frame)
{
#if defined(CONFIG_DEBUGGER_NEW_KDB)
/* reset the clock */
    
    wrmsr(0x10, saved_time);

# if defined(CONFIG_ARCH_X86_I686)
//only on Intel 686
//#if defined CONFIG_PERFMON || defined CONFIG_ENABLE_PROFILING
    qword_t dummy;
    dummy = rdmsr(0x186);
    dummy = dummy | (1 << 22);
    wrmsr(0x186, dummy);
# endif

    if(cache_enabled)
    {    
/* enable cache after debugging */
	asm volatile(
	    "mov %%cr0, %%eax\n" /* copy CR0 into eax */
	    "xor $((1<<30)|(1<<29)), %%eax\n" /* clear bits CD and NW */
	    "mov %%eax, %%cr0\n" /* write back in CR0 */
	    : /* no outputs */
	    : /* no inputs */
	    : "eax"
	    );
    }
#endif /*new debugger*/
    if (frame->fault_code == 2)	/* NMI */
    {
	/* re-enable nmi */
	outb(0x61, inb(0x61) | (0xC));
	for (volatile int i = 10000000; i--; );
	/* re-enable nmi */
	outb(0x61, inb(0x61) & ~(0xC));

	in_rtc(0x8f);
	in_rtc(0x0f);
    };
#if !defined(CONFIG_DEBUGGER_GDB)
    if (frame->fault_code == 3)
    {
	/* now readjust the fault_adress we faked in
	   kdebug_arch_entry to point to the int3 itself */
	frame->fault_address++;
    };
#endif
};

#if defined(CONFIG_PERFMON) || defined(CONFIG_DEBUGGER_NEW_KDB)
// this is needed for the new debugger
void kdebug_setperfctr(dword_t sel, dword_t value)
{
    /* validate counter number */
    switch (sel)
    {
    case 0: break;
    case 1: break;
    default: return;
    };
    
    /* set the EvntSelX and clear the associated PerfCtrX */
    __asm__ __volatile__ (
 	"/* write EvntSel */\n"
 	"lea	0x186(%1),%%ecx\n"
 	"sub	%%edx,%%edx\n"
 	"wrmsr\n"
 	"/* clear the counter */\n"
 	"lea	0xc1(%1),%%ecx\n"
 	"sub	%%edx,%%edx\n"
 	"sub	%%eax,%%eax\n"
 	"wrmsr\n"
 	: "+a"(value) : "r"(sel) :"ecx", "edx");
}
#endif

void kdebug_perfmon()
{
#if defined(CONFIG_PERFMON)
# if defined(CONFIG_ARCH_X86_I686)
    switch(kdebug_get_choice("What", "Set/Print/Clear"
#if defined(CONFIG_DEBUGGER_NEW_KDB)
			     "/Write/Interrupt enabled/interrupt Disabled\n"
#endif
			     , 'p'))
    {
    case 's': 
    {
	dword_t num;
	dword_t val;
	char c;

	printf("num(0,1) ");
	do {
	    c = getc();
	    if ((c == '0') || (c == '1'))
		break;
	} while (1);
	putc(c);
	num = c - '0';
    
	printf(" (u,k,a) ");
	val = 0;
	do {
	    c = getc();
	    switch (c)
	    {
	    case 'u': val = 0x10000; break;
	    case 'k': val = 0x20000; break;
	    case 'a': val = 0x30000; break;
	    }
	} while (!val);
	printf("%c  ", c);

	/* read event number to count */
	val |= kdebug_get_perfctr("event", 0xff);
    
	/* enable counter */
	val |= (1 << 22);
#if defined(CONFIG_DEBUGGER_NEW_KDB)
// to enable the pmc overflow interrupt 
	val |= pmc_overflow_interrupt_mask;
#endif
	kdebug_setperfctr(num, val);
    } break;
    
    case 'p':
	printf("rdpmc: 0=%x, 1=%x\n", rdpmc(0), rdpmc(1));
	break;

    case 'c':
	__asm__ (
	    "/* clear the counter */\n"
	    "mov	$0xc1,%%ecx\n"
	    "sub	%%edx,%%edx\n"
	    "sub	%%eax,%%eax\n"
	    "wrmsr		   \n"
	    "mov	$0xc2,%%ecx\n"
	    "wrmsr		   \n"
	    :
	    :
	    : "eax", "ecx", "edx");
	break;
    }
#if defined (CONFIG_DEBUGGER_NEW_KDB)
   case 'w': /* write the counter */
        
        //select counter
        printf("set counter: num(0,1) ");
        do {
            c = getc();
            if ((c == '0') || (c =='1')) {
                break;
            }
        } while (1);
        putc(c);

        //get the value
        dword_t value_low;
        printf(" low order 32 bits [0]: ");
        value_low = kdebug_get_hex(0x0, "0");

        //set the counter
        int counter_address;
        counter_address = PERFCTR0 + c - '0';
	wrmsr(counter_address, (qword_t) value_low);

        putc('\n');
        break;

    case 'i':    
        printf("pmc overflow interrupt enabled. you have to reset the counter(s)\n");
        enable_pmc_of_int();
        break;
    case 'd':
        printf("pmc overflow interrupt disabled\n");
        disable_pmc_of_int();
        break;

    }
#endif /*new debugger*/

# elif defined(CONFIG_ARCH_X86_P4)
    dword_t ctr;

    switch ( kdebug_get_choice("Performance counter",
			       "Enable/Disable/Clear/clear All/View", 'v') )
    {
    case 'e':
    {
	qword_t escr, cccr;
	dword_t escrmsr;

	ctr = kdebug_get_perfctr(&escrmsr, &escr, &cccr);
	if ( ctr != ~0UL )
	{
	    wrmsr(escrmsr, escr);
	    wrmsr(IA32_CCCR_BASE + ctr, cccr);
	    wrmsr(IA32_COUNTER_BASE + ctr, 0);
	}
	break;
    }

    case 'd':
	printf("Counter (0-17): ");
	ctr = kdebug_get_dec(99, "");
	if ( ctr >= 0 && ctr <= 17 )
	{
	    wrmsr(IA32_CCCR_BASE + ctr, 0);
	    wrmsr(IA32_COUNTER_BASE + ctr, 0);
	}
	break;

    case 'c':
	printf("Counter (0-17): ");
	ctr = kdebug_get_dec(99, "");
	if ( ctr >= 0 && ctr <= 17 )
	    wrmsr(IA32_COUNTER_BASE + ctr, 0);
	break;

    case 'a':
	for ( int i = 0; i < 18; i++ )
	    wrmsr(IA32_COUNTER_BASE + i, 0);
	break;

    case 'v':
	for ( int i = 0; i < 18; i++ )
	{
	    qword_t count = rdmsr(IA32_COUNTER_BASE + i);
	    printf("%2d: %02x%08x  ", i,
		   (dword_t) ((count >> 32) & 0xff), (dword_t) count);
	    kdebug_describe_perfctr(i);
	    putc('\n');
	}
	break;
    }


# endif /* CONFIG_ARCH_X86_P4 */
#endif /* CONFIG_PERFMON */
}

void kdebug_breakpoint()
{
    printf("breakpoint [-/?/0..3]: ");
    
    /* the breakpoint register to be used */
    dword_t db7;
    /* breakpoint address */
    dword_t addr = 0;
    
    int num = getc();putc(num); 
    switch (num) {
	/* set debug register 0..3 manually */
    case '0'...'3':
	num -= '0';
	break;
	/* reset all debug registers */
    case '-':
	__asm__ __volatile__ ("mov %%db7,%0": "=r" (db7));
	db7 &= ~(0x00000FF);
	__asm__ __volatile__ ("mov %0, %%db7": :"r" (db7));
	return;break;
	
	/* any key dumps debug registers */
    case '?':
    default:
        __asm__ ("mov %%db7,%0": "=r"(db7));
	printf("\nDR7: %x\n", db7);
	__asm__ ("mov %%db6,%0": "=r"(db7));
	printf("DR6: %x\n", db7); addr=db7;
	__asm__ ("mov %%db3,%0": "=r"(db7));
	printf("DR3: %x %c\n", db7, addr & 8 ? '*' : ' ');
	__asm__ ("mov %%db2,%0": "=r"(db7));
	printf("DR2: %x %c\n", db7, addr & 4 ? '*' : ' ');
	__asm__ ("mov %%db1,%0": "=r"(db7));
	printf("DR1: %x %c\n", db7, addr & 2 ? '*' : ' ');
	__asm__ ("mov %%db0,%0": "=r"(db7));
	printf("DR0: %x %c\n", db7, addr & 1 ? '*' : ' ');
	return;
    }
    /* read debug control register */
    __asm__ __volatile__ ("mov %%db7,%0" : "=r" (db7));
    
    
    printf(" [Instr/Access/pOrt/Write/-/+] :");
    /* breakpoint type */
    char t;
    while (42)
    {
	t = getc();
	if ((t=='i') || (t=='a') || (t=='o') ||
	    (t=='w') || (t=='-') || (t=='+'))
	    break;
    }
    
    putc(t);
    switch (t)
    {
    case '-':
	db7 &= ~(2 << (num * 2)); /* disable */
	num = -1;
	break;
    case '+':
	db7 |=  (2 << (num * 2)); /* enable */
	num = -1;
	break;
    case 'i': /* instruction execution */
	addr = kdebug_get_hex();
	db7 &= ~(0x000F0000 << (num * 4));
	db7 |= (0x00000000 << (num * 4));
	db7 |= (2 << (num * 2)); /* enable */
	break;
    case 'w': /* data write */
	addr = kdebug_get_hex();
	db7 &= ~(0x000F0000 << (num * 4));
	db7 |= (0x00010000 << (num * 4));
	db7 |= (2 << (num * 2)); /* enable */
	break;
    case 'o': /* I/O */
	addr = kdebug_get_hex();
	db7 &= ~(0x000F0000 << (num * 4));
	db7 |= (0x00020000 << (num * 4));
	db7 |= (2 << (num * 2)); /* enable */
	break;
    case 'a': /* read/write */
	addr = kdebug_get_hex();
	db7 &= ~(0x000F0000 << (num * 4));
	db7 |= (0x00030000 << (num * 4));
	db7 |= (2 << (num * 2)); /* enable */
	break;
    };
    if (num==0) __asm__ __volatile__ ("mov %0, %%db0" : : "r" (addr));
    if (num==1) __asm__ __volatile__ ("mov %0, %%db1" : : "r" (addr));
    if (num==2) __asm__ __volatile__ ("mov %0, %%db2" : : "r" (addr));
    if (num==3) __asm__ __volatile__ ("mov %0, %%db3" : : "r" (addr));
    __asm__ __volatile__ ("mov %0, %%db7" : : "r" (db7));
    
}

void kdebug_arch_help()
{
    printf(
	"c   profile          C dump profile\n"
	"T   single stepping\n"
	"e   perfmon\n"
	"b   breakpoint\n"
        "      b?         list\n"
        "      b-         disable all\n"
        "      b#i<addr>  instruction\n"
        "      b#w<addr>  write\n"
        "      b#o<addr>  I/O\n"
        "      b#a<addr>  access\n"
	"      b#-        disable this dr\n"	  
	);
}

INLINE void cpuid(dword_t index,
		  dword_t* eax, dword_t* ebx, dword_t* ecx, dword_t* edx)
{
    __asm__ (
	"cpuid"
	: "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
	: "a" (index)
	);
}

#if defined(CONFIG_X86_IOAPIC)
#include INC_ARCH(apic.h)
#endif

#if defined(CONFIG_ENABLE_SMALL_AS)
void kdebug_small_spaces(tcb_t *current)
{
    switch ( kdebug_get_choice("\nSmall spaces",
			       "View/make Small/make Large", 'v') )
    {
    case 's':
    {
	dword_t size, s, idx;
	tcb_t * tcb = kdebug_get_task(current);
	tcb = kdebug_find_task_tcb(tasknum(tcb), current);
	if ( tcb == NULL )
	    break;

	printf("Size (MB) [64]: ");
	size = kdebug_get_dec(64) / SMALL_SPACE_MIN;
	printf("Index [0]: ");
	idx = kdebug_get_dec(0);
	for ( s = 0; ((1 << s) & size) == 0; s++ ) ;
	if ( size )
	    make_small_space (tcb->space, ((idx << 1) | 1) << s);
	break;
    }

    case 'l':
    {
	tcb_t * tcb = kdebug_get_task(current);
	tcb = kdebug_find_task_tcb(tasknum(tcb), current);
	if (tcb && tcb->space->is_small ())
	    small_space_to_large (tcb->space);
	break;
    }

    case 'v':
    {
	extern space_t *small_space_owner[];
	space_t *owner;
	dword_t i, k, n;
	tcb_t *tcb;

	putc('\n');
	for ( i = 0; i < MAX_SMALL_SPACES; i++ )
	{
	    if ( small_space_owner[i] == NULL )
		continue;

	    owner = small_space_owner[i];
	    for ( n = 0; i+n < MAX_SMALL_SPACES; n++ )
		if ( small_space_owner[i+n] != owner )
		    break;

	    for ( tcb = get_idle_tcb()->present_next;
		  tcb->space != owner && tcb != get_idle_tcb();
		  tcb = tcb->present_next )
		;

	    k = printf("%3d-%d", i, i+n-1);
	    for ( k = 9-k; k--; ) putc(' ');
	    printf("<%p,%p>  size=%3dMB  space=%p  task=",
		   SMALL_SPACE_START + (i << SMALL_SPACE_SHIFT),
		   SMALL_SPACE_START + ((i+n) << SMALL_SPACE_SHIFT),
		   n * SMALL_SPACE_MIN, owner);
	    printf(tcb->space == owner ? "%2x\n" : "?\n",
		   tcb->myself.x0id.task);

	    i += n-1;
	}
	break;
    }
    }
}
#endif /* CONFIG_SMALL_SPACES */

static void kdebug_msrs()
{

    dword_t msr;
    qword_t val;

    switch ( kdebug_get_choice("\nModel Specific Registers",
			       "Show/Read/Write", 's') )
    {
    case 'r':
	printf("msr=");
	msr = kdebug_get_hex();
	val = rdmsr(msr);
	printf("rdmsr(%x)=%x\n", msr, val);
	break;

    case 'w':
	printf("msr=");
	msr = kdebug_get_hex();
	printf("val=");
	val = kdebug_get_hex();
	wrmsr(msr, val);
	break;

    default:
	printf("SYSENTER_CS_MSR:    %x\n", rdmsr(IA32_SYSENTER_CS_MSR));
	printf("SYSENTER_EIP_MSR:   %x\n", rdmsr(IA32_SYSENTER_EIP_MSR));
	printf("SYSENTER_ESP_MSR:   %x\n", rdmsr(IA32_SYSENTER_ESP_MSR));

#if defined(CONFIG_ARCH_X86_I686)
	printf("LASTBRANCH_FROM_IP: %x\n", rdmsr(IA32_LASTBRANCHFROMIP));
	printf("LASTBRANCH_TO_IP:   %x\n", rdmsr(IA32_LASTBRANCHTOIP));
	printf("LASTINT_FROM_IP:    %x\n", rdmsr(IA32_LASTINTFROMIP));
	printf("LASTINT_TO_IP:      %x\n", rdmsr(IA32_LASTINTTOIP));
#endif

#if defined(CONFIG_ARCH_X86_P4)
	printf("LER_FROM_IP:        %x\n", rdmsr(IA32_LER_FROM_LIP));
	printf("LER_TO_IP:          %x\n", rdmsr(IA32_LER_TO_LIP));
	dword_t tos = rdmsr(IA32_LASTBRANCH_TOS);
	for ( int i = 0; i < 4; i++ )
	{
	    qword_t br = rdmsr(IA32_LASTBRANCH_0 + ((i + tos) & 0x03));
	    printf("LASTBRANCH_%d:       %x -> %x\n", (i + tos) & 0x03,
		   (dword_t) (br >> 32), (dword_t) br);
	}
#endif
	break;
    };

}


void kdebug_cpustate(tcb_t *current)
{
    int l;
    char c;
    dword_t foo;

    printf("\nx86 - ");
    l = printf("[Gdt/Crs/Idt/iOapic/Ports/Msr/cpUid"
#if defined(CONFIG_ENABLE_SMALL_AS)
	       "/Smallspaces"
#endif
#if defined(CONFIG_X86_P4_BTS)
	       "/Bts"
#endif
#if defined(CONFIG_X86_P4_PEBS)
	       "/pEbs"
#endif
	       "]: ");
    switch (getc())
    {
    case 'c':
	
	__asm__ ("mov %%cr4,%0": "=r"(foo));
	printf("\nCR4: %x\n", foo);
	__asm__ ("mov %%cr3,%0": "=r"(foo));
	printf("CR3: %x\n", foo);
	__asm__ ("mov %%cr2,%0": "=r"(foo));
	printf("CR2: %x\n", foo);
	__asm__ ("mov %%cr0,%0": "=r"(foo));
	printf("CR0: %x\n", foo);

	break;

    case 'C':

	printf("\nset CR");
	while (1) { c = getc(); if ((c >= '0') && (c <= '4')) break; };
	printf("%d=", c-'0');
	foo = kdebug_get_hex();
	
	switch (c)
	{
	case '0': __asm__ __volatile__ ("mov %0,%%cr0": : "r"(foo)); break;
	case '2': __asm__ __volatile__ ("mov %0,%%cr2": : "r"(foo)); break;
	case '3': __asm__ __volatile__ ("mov %0,%%cr3": : "r"(foo)); break;
	case '4': __asm__ __volatile__ ("mov %0,%%cr4": : "r"(foo)); break;
	};

	break;

    case 'm':

	kdebug_msrs();
	break;

    case 'g':

	extern seg_desc_t gdt[];
	printf("\nGDT-dump: gdt at %x\n", gdt);
	for (int i = 0; i < GDT_SIZE; i++)
	{
	    seg_desc_t *ent = gdt+i;
	    printf("GDT[%d] = %x:%x", i, ent->x.raw[0], ent->x.raw[1]);
	    if ( (ent->x.raw[0] == 0 && ent->x.raw[1] == 0) ||
		 (! ent->x.d.s) )
	    {
		printf("\n");
		continue;
	    }
	    printf(" <%p,%p> ",
		   ent->x.d.base_low + (ent->x.d.base_high << 24),
		   ent->x.d.base_low + (ent->x.d.base_high << 24) +
		   (ent->x.d.g ? 0xfff |
		    (ent->x.d.limit_low + (ent->x.d.limit_high << 16)) << 12 :
		    (ent->x.d.limit_low + (ent->x.d.limit_high << 16))));
	    printf("dpl=%d %d-bit ", ent->x.d.dpl, ent->x.d.d ? 32 : 16);
	    if ( ent->x.d.type & 0x8 )
		printf("code %cC %cR ",
		       ent->x.d.type & 0x4 ? ' ' : '!',
		       ent->x.d.type & 0x2 ? ' ' : '!');
	    else
		printf("data E%c R%c ",
		       ent->x.d.type & 0x4 ? 'D' : 'U',
		       ent->x.d.type & 0x2 ? 'W' : 'O');
	    printf("%cP %cA\n",
		   ent->x.d.p ? ' ' : '!',
		   ent->x.d.type & 0x1 ? ' ' : '!');
	}
	break;
    
    case 'i':
	
	extern idt_desc_t idt[];
	printf("\nIDT-dump: idt at %x\n", idt);
	for (int i = 0; i < (IDT_SIZE) ; i++)
	    if (idt[i].x.d.p)
		printf("%2x -> %4x:%x , dpl=%d, type=%d (%x:%x)\n", i,
		       idt[i].x.d.sel,
		       idt[i].x.d.offset_low | (idt[i].x.d.offset_high << 16),
		       idt[i].x.d.dpl, idt[i].x.d.type,
		       idt[i].x.raw[0], idt[i].x.raw[1]);
	break;

    case 'o':
	
#if defined(CONFIG_X86_IOAPIC)
	printf("\nIO-APIC redirection table\n");
	for (int i = 0; i < 24; i++)
	    printf("ioredir(%d): %x, %x\n", i, get_io_apic(0x10 + (i*2)), get_io_apic(0x11 + (i*2)));
#endif
	break;
	

    case 'p':

#define B(x) do { for (int __i =0; __i < (x); __i++) { putc(8); putc(' '); putc(8); } } while (0)

	word_t port;
	B(l);
	l = printf("[In/Out]: ");
	c = getc();
	switch(c)
	{
	case 'i':
	    
	    B(l);
	    printf("in "); port = kdebug_get_hex();
	    printf(" -> %2x\n", inb(port));
		   
	    break;

	case 'o':
	    
	    B(l);
	    printf("out "); port = kdebug_get_hex();
	    printf(", "); outb(port, kdebug_get_hex());
		   
	    break;
	};
	break;

#if defined(CONFIG_ENABLE_SMALL_AS)
    case 's':
	kdebug_small_spaces(current);
	break;
#endif

#if defined(CONFIG_X86_P4_BTS)
    case 'b':
	kdebug_x86_bts();
	break;
#endif

#if defined(CONFIG_X86_P4_PEBS)
    case 'e':
	kdebug_x86_pebs();
	break;
#endif

    case 'u':

	/* Reference:
	   
	   AP-485
	   APPLICATION NOTE
	   Intel Processor Identification and
	   the CPUID Instruction
	   
	   June 2001
	   Order Number: 241618-018
	   
	   http://developer.intel.com/design/pentiumII/applnots/241618.htm
	*/
	
	/* from above document, table 5, pages 14-15 */
	const char* features[] = {
	    "fpu",  "vme",    "de",   "pse",   "tsc",  "msr", "pae",  "mce",
	    "cx8",  "apic",   "?",    "sep",   "mtrr", "pge", "mca",  "cmov",
	    "pat",  "pse-36", "psn",  "cflsh", "?",    "ds",  "acpi", "mmx",
	    "fxsr", "sse",    "sse2", "ss",    "ht",   "tm",  "ia64", "?" };
	/* from above document, table 7, page 17 */
	const char* cachecfg[16][16] =
	{
	    { /* 0x00 */ 
		"",
		"ITLB: 32*4K, 4w", "ITLB: 2*4M",
		"DTLB: 64*4K, 4w", "DTLB: 8*4M, 4w", 0,
		"ICache: 8K, 4w, 32", 0, "ICache: 16K, 4w, 32", 0,
		"DCache: 8K, 2w, 32", 0, "DCache: 16K, 4w, 32" },
	    { /* 0x10 */ }, { /* 0x20 */ }, { /* 0x30 */ }, 
	    { /* 0x40 */
		"no L2 or L3",
		"Cache: 128K, 4w, 32", "Cache: 256K, 4w, 32",
		"Cache: 512K, 4w, 32", "Cache: 1M, 4w, 32",
		"Cache: 2M, 4w, 32",
	    },
	    { /* 0x50 */
		"ITLB: 64*{4K,2M/4M}", "ITLB: 128*{4K,2M/4M}",
		"ITLB: 256*{4K,2M/4M}", 0, 0, 0, 0, 0, 0, 0, 0,
		"DTLB: 64*{4K,4M}", "DTLB: 128*{4K,4M}",
		"DTLB: 256*{4K,4M}"
	    },
	    { /* 0x60 */
		0, 0, 0, 0, 0, 0,
		"DCache: 8K, 4w, 64", "DCache: 16K, 4w, 64",
		"DCache: 32K, 4w, 64"
	    },
	    { /* 0x70 */
		"TC: 12Kuop, 8w", "TC: 16Kuop, 8w", "TC: 32Kuop, 8w", 0,
		0, 0, 0, 0, 0,
		"Cache: 128K, 8w, 64", "Cache: 256K, 8w, 64",
		"Cache: 512K, 8w, 64", "Cache: 1M, 8w, 64"
	    },
	    { /* 0x80 */
		0, 0,
		"Cache: 256K, 8w, 32", "Cache: 512K, 8w, 32",
		"Cache: 1M, 8w, 32", "Cache: 2M, 8w, 32"
	    }


	};

	dword_t id[4][4];
	dword_t i;
	printf("\n");
	for (i = 0; i < 4; i++)
	    cpuid(i, &id[i][0], &id[i][1], &id[i][2], &id[i][3]);
	for (i = 0; i <= id[0][0]; i++)
	    printf("cpuid(%d):%x:%x:%x:%x\n", i,
		   id[i][0], id[i][1], id[i][2], id[i][3]);
	printf("0: max=%d \"", id[0][0]);
	for (i = 0; i < 12; i++) putc(((char*) &id[0][1])[i]);
	printf("\"\n1: fam=%d, mod=%d, step=%d\n1: ",
	       (id[1][0] >> 8) & 0xF,
	       (id[1][0] >> 4) & 0xF,
	       (id[1][0] >> 0) & 0xF);
	for (i = 0; i < 32; i++)
	    if ((id[1][3] >> i) & 1) printf("%s ", features[i]);
	printf("\n");
	/* 2: eax[7:0] determines, how often 2 must be called - noimp */
	for (i = 1; i < 16; i++)
	{
	    int j = ((char*)id[2])[i];
	    if (((id[2][i/4] & 0x80000000) == 0) && (j != 0))
		printf("%s\n", cachecfg[0][j]);
	};
	break;
    }
}

int kdebug_step_instruction(exception_frame_t * frame)
{
    kdebug_single_stepping(frame, 1);
    kdebug_stepping = STEPPING_INSTR;
    frame->eflags |= (1 << 16);
    putc('s'); putc('\n');
    return 1;
}

int kdebug_step_block(exception_frame_t * frame)
{
#if defined(CONFIG_ARCH_X86_I686) || defined(CONFIG_ARCH_X86_P4)
    kdebug_single_stepping(frame, 1);
    kdebug_stepping = STEPPING_BLOCK;
    frame->eflags |= (1 << 16);
    wrmsr(IA32_DEBUGCTL, ((1<<0) + (1<<1))); /* LBR + BTF */
    putc('S'); putc('\n');
    return 1;
#else
    /* Single stepping on branches is not supported on older CPUs. */
    printf("S - Unsupported\n");
    return 0;
#endif
}


void kdebug_disassemble(exception_frame_t * frame)
{
#if defined(CONFIG_DEBUG_DISAS)
    extern int x86_disas(dword_t pc);
    char c;
    dword_t pc;
 restart:
    printf("ip [current]: ");
    pc = kdebug_get_hex(frame->fault_address, "current");
    do {
	printf("%x: ", pc);
	pc += x86_disas(pc);
	printf("\n");
	c = getc();
    } while ((c != 'q') && (c != 'u'));
    if (c == 'u')
	goto restart;
#endif
};

#if defined(CONFIG_TRACEBUFFER)
void kdebug_dump_tracebuffer()
{
  dword_t count = tracebuffer->current / sizeof(trace_t);
  dword_t index, top = 0, bottom = count, chunk = (count<32) ? count : 32;
  tracestatus_t old = { 0,0,0 }, sum = { 0,0,0 };
  trace_t *current;
  char c;

  if (tracebuffer->magic != TBUF_MAGIC)
    {
      printf("Bad tracebuffer signature at %x\n",(dword_t)(&tracebuffer->magic));
      return;
    }  
    
  c = kdebug_get_choice("Dump tracebuffer", "All/Region/Top/Bottom", 'b');
  switch (c)
    {
      case 'a' : break;
      case 'r' : printf("From record [0]: ");
                 top = kdebug_get_dec();
                 printf("To record [%d]: ", bottom);
                 bottom = kdebug_get_dec(bottom);
                 break;
      case 't' : printf("Record count [%d]: ", chunk);
                 bottom = kdebug_get_dec(chunk);
                 break;
      default  : printf("Record count [%d]: ", chunk);
                 top = bottom - kdebug_get_dec(chunk);
                 break;
    }
    
  if (bottom > count)
    bottom = count;
  if (top > bottom) 
    top = bottom;

  printf("\nRecord  Cycles    UL Ins    Event1    Identifier\n");
  for (index = top; index<bottom; index++)
    {
      current = &(tracebuffer->trace[index]);
      if (!old.cycles) 
        old = current->status;
          
      printf("%6d%10d%10d%10d  ", index,
             (current->status.cycles - old.cycles), 
             (current->status.pmc0 - old.pmc0), 
             (current->status.pmc1 - old.pmc1));

      sum.cycles += (current->status.cycles - old.cycles);
      sum.pmc0 += (current->status.pmc0 - old.pmc0);
      sum.pmc1 += (current->status.pmc1 - old.pmc1);
      old = current->status;

      switch (current->identifier)
        {
#define DEFINE_TP(x, str) 					\
          case TP_##x:						\
             printf("*KRNL* ");					\
             printf(str, current->data[0], current->data[1],	\
                         current->data[2], current->data[3]);	\
             break;
#include <tracepoint_list.h>
#undef DEFINE_TP

          /* Entries from user level are displayed as plain text */

          default: 
             printf("[%c%c%c%c]", (char)((current->identifier>>24)&0xFF), 
                                  (char)((current->identifier>>16)&0xFF), 
                                  (char)((current->identifier>>8)&0xFF),
                                  (char)(current->identifier&0xFF));
                                  
             c = (current->identifier>>24)&0xFF;
             if ((c>='1') && (c<='4'))
               {
                 int i=0;
                 printf(", par=(%xh", current->data[i++]);
                 while ((--c)>='1')
                   printf(",%xh", current->data[i++]);
                 putc(')');
	       }  
             break;
        }
        
      putc('\n');  
    }
    
  printf("----------------------------------------------------\n");  
  printf("      %10d%10d%10d  %6d entries\n", sum.cycles, sum.pmc0, sum.pmc1, bottom-top);
}

void kdebug_flush_tracebuffer()
{
  printf("Tracebuffer flushed\n");
  tracebuffer->current = 0;
}
#endif /* defined(CONFIG_TRACEBUFFER) */

#endif /* defined(CONFIG_DEBUGGER_KDB) */
