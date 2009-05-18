/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001, 2002,  Karlsruhe University
 *                
 * File path:     x86/init.c
 * Description:   x86 specific initialization code.
 *                
 * @LICENSE@
 *                
 * $Id: init.c,v 1.82 2002/05/13 13:04:31 stoess Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include <init.h>

#include <x86/cpu.h>
#include <x86/init.h>
#include <x86/memory.h>
#include <x86/apic.h>
#include <x86/tracebuffer.h>
#include <x86/space.h>

/* relocate to init section */
void init_arch_1() L4_SECT_INIT;
void init_paging() L4_SECT_INIT;
void setup_idt() L4_SECT_INIT;
void setup_tss() L4_SECT_INIT;
void setup_excp_vector() L4_SECT_INIT;
void activate_excp_vector() L4_SECT_INIT;
void calc_processor_speed() L4_SECT_INIT;

void init_map_4k(dword_t * pdir, dword_t addr, dword_t val) L4_SECT_INIT;
void init_map_4mb(dword_t * pdir, dword_t addr, dword_t val) L4_SECT_INIT;

void dump_idt() L4_SECT_INIT;
void dump(void *address, int size) L4_SECT_INIT;

/* initial memory we need for pagetable setup */
#define KERNEL_PAGE(x) ((dword_t*)&init_kernel_mem[PAGE_SIZE * x])
dword_t * kernel_ptdir_root = NULL;
unsigned char init_kernel_mem[PAGE_SIZE * (KERNEL_INIT_PAGES)] __attribute__((aligned(4096)));

/* cpu-local pagetables */
#if defined(CONFIG_SMP)
void init_cpu_local_data();
dword_t * boot_ptdir;
#endif

// cpu-local stuff
x86_tss_t __tss L4_SECT_CPULOCAL;
seg_desc_t gdt[GDT_SIZE] L4_SECT_CPULOCAL;
dword_t __is_small L4_SECT_CPULOCAL = 0;
tcb_t* fpu_owner L4_SECT_CPULOCAL = NULL;


/* The interrupt descriptor table */
idt_desc_t idt[IDT_SIZE];


#define BUGSCREEN(x)										\
do {												\
    for (volatile ptr_t p = (ptr_t) 0xb8000; p < (ptr_t) 0xb8000 + 40*25; p++)			\
	*p = (x * 0x00010001) | (0x97 * 0x01000100U);						\
    *((volatile ptr_t) 0xb8000 + 40*12 + 19) = 'L' | (0xF4 << 8) | (('4' | (0xF4 << 8)) << 16);	\
    *((volatile ptr_t) 0xb8000 + 40*12 + 20) = 'K' | (0xF0 << 8) | (('a' | (0xF0 << 8)) << 16);	\
    wbinvd();											\
    while(1);											\
} while (0)

/* architecture specific initialization
 * invoked by src/init.c before anything is done
 * but running on virtual addresses
 */
void init_arch_1()
{
    init_irqs();

    calc_processor_speed();

    /* 
     * init real-time clock 
     */

    /* set rtc to 512 Hz */
    while(in_rtc(0x0a) & 0x80);

    out_rtc(0x0A, (in_rtc(0x0a) & 0xf0) | 0x07);
    out_rtc(0x0b, in_rtc(0x0b) | 0x48);

    /* reset intr line */
    in_rtc(0x0c);


}


void init_arch_2()
{
#if defined(CONFIG_X86_P4_BTS) || defined(CONFIG_X86_P4_PEBS)
    init_tracestore();
#endif
#if defined(CONFIG_SMP)
    boot_cpu = get_apic_cpu_id();

    /* for SMP we have to re-allocate the kernel space
     * since it is not compatible with space_t assumptions 
     */
    printf("copying kernel pagedir\n");
    char * pdir = (char*)kmem_alloc(PAGE_SIZE * CONFIG_SMP_MAX_CPU);
    for (int i = 0; i < CONFIG_SMP_MAX_CPU; i++)
	memcpy((dword_t*)(pdir + i * PAGE_SIZE), 
	       kernel_ptdir_root, PAGE_SIZE);

    /* switch to newly allocated pagetable */
    set_current_pagetable(virt_to_phys((dword_t*)(pdir + boot_cpu * PAGE_SIZE)));

    /* free old one */
    //kmem_free(kernel_ptdir_root, PAGE_SIZE);

    /* store new space */
    kernel_ptdir_root = (dword_t*)pdir;
    
    /* set boot page table for APs */
    boot_ptdir = get_current_pagetable();

#if defined(CONFIG_DEBUG_TRACE_INIT)
    printf("boot cpu is %d\n", boot_cpu);
#endif
#endif /* CONFIG_SMP */
    
#if defined(CONFIG_IO_FLEXPAGES)
    /* Initialize IO mapping database */
    io_mdb_init();
#endif 

}


/* post processing (e.g., flushing page-table entries) */
void init_arch_3()
{

#if defined(CONFIG_DEBUG_TRACE_INIT)
    printf("CPU features: %x\n", get_cpu_features());
#endif

#if defined(CONFIG_SMP)
    /* init_arch_3 is only executed by the boot cpu */
    boot_cpu = get_apic_cpu_id();

    init_cpu_local_data();
    
    smp_startup_processors();
#endif

    /* Kmem is initialized now. */
    __zero_page = kmem_alloc(PAGE_SIZE);
    zero_memory(__zero_page, PAGE_SIZE);

    /* Flush init section. */
    //kernel_ptdir[0] = 0;
    
    flush_tlb();

#if !defined(CONFIG_X86_APIC)
    /* Do not give out the timer interrupt. */
    interrupt_owner[0] = get_idle_tcb();
    interrupt_owner[2] = get_idle_tcb();
    interrupt_owner[8] = get_idle_tcb();
#endif
}


INLINE void init_zero_memory(void * address, int size)
{
    dword_t *tmp = (dword_t*)address;
    size /= 4;
    while(size--)
	*tmp++ = 0;

}

void wait_for_second_tick()
{
    // wait that update bit is off
    while(in_rtc(0x0a) & 0x80);

    // read second value
    byte_t secstart = in_rtc(0);

    // now wait until seconds change
    while (secstart == in_rtc(0));
}

static void setup_apic_timer(dword_t tickrate) L4_SECT_INIT;
static void setup_apic_timer(dword_t tickrate)
{
    /* divide by 1 */
    set_local_apic(X86_APIC_TIMER_DIVIDE, (get_local_apic(X86_APIC_TIMER_DIVIDE) & ~0xf) | 0xb);
    set_local_apic(X86_APIC_TIMER_COUNT, tickrate);

    /* set timer periodic and enable it */
    set_local_apic(X86_APIC_LVT_TIMER, (get_local_apic(X86_APIC_LVT_TIMER) & ~(APIC_IRQ_MASK)) | 0x20000);
}

void calc_processor_speed()
{
    qword_t cpu_cycles;
#if defined(CONFIG_X86_APICTIMER)
    dword_t bus_cycles;
#endif

#if defined(CONFIG_DEBUG_TRACE_INIT)
    printf("calculating processor speed...\n");
#endif

#if defined(CONFIG_X86_APICTIMER)
    /* set timer to divide-1 mode and reload with a large value */
    setup_apic_timer(0xFFFFFFFF);
#endif

    wait_for_second_tick();
#if defined(CONFIG_X86_APICTIMER)
    bus_cycles = get_local_apic(X86_APIC_TIMER_CURRENT);
#endif
    cpu_cycles = rdtsc();
    wait_for_second_tick();
#if defined(CONFIG_X86_APICTIMER)
    bus_cycles -= get_local_apic(X86_APIC_TIMER_CURRENT);
#endif
    cpu_cycles = rdtsc() - cpu_cycles;

    kernel_info_page.processor_frequency = (dword_t) cpu_cycles;
#if defined(CONFIG_X86_APICTIMER)
    kernel_info_page.bus_frequency = bus_cycles;
#endif

#if defined(CONFIG_DEBUG_TRACE_INIT)
    printf("cpu speed: %d Hz\n", kernel_info_page.processor_frequency);
#if defined(CONFIG_X86_APICTIMER)
    printf("bus speed: %d Hz\n", kernel_info_page.bus_frequency);
#endif
#endif
#if defined(CONFIG_X86_APICTIMER)
    setup_apic_timer(kernel_info_page.bus_frequency / (1000000/TIME_QUANTUM));
#endif
}
    

/**********************************************************************
 *
 *        architecture initialization - before anything else 
 *
 **********************************************************************/


void init_map_4k(dword_t * pdir, dword_t addr, dword_t val)
{
    dword_t *ptab = (dword_t *) pdir[pgdir_idx(addr)];

    /* Check if there is a second level pagetable. */
    if  ( !IS_PAGE_VALID((dword_t) ptab) )
    {
#if 0
	/* Allocate second level table. */
	ptab = (dword_t *) virt_to_phys(kmem_alloc(PAGE_SIZE));
	pdir[pgdir_idx(addr)] = ((dword_t) ptab) | PAGE_KERNEL_TBITS;
#else
	BUGSCREEN('I');
#endif
    }
    ptab = (dword_t*)(((dword_t)ptab) & PAGE_MASK);
    ptab[pgtab_idx(addr)] = val;
} 

void init_map_4mb(dword_t * pdir, dword_t addr, dword_t val)
{
#if defined(CONFIG_IA32_FEATURE_PSE)
    pdir[addr >> PAGEDIR_BITS] = val | PAGE_SUPER;
#else
#error superpages not supported on this platform - emulation please...
#endif
}


#if defined(CONFIG_IA32_FEATURE_SEP)
INLINE void setup_sysenter_msrs()
{
    /* uses external symbols __tss and ipc_sysenter */
    __asm__ __volatile__
	(
	    "/* set up the sysenter MSRs */	\n\t"
	    "mov	$0x174,%%ecx		\n\t"
	    "mov	$8,%%eax		\n\t"
	    "sub	%%edx,%%edx		\n\t"
	    "wrmsr	/* SYSENTER_CS_MSR */	\n\t"
	    
	    "mov	$0x176,%%ecx		\n\t"
	    "mov	$ipc_sysenter,%%eax	\n\t"
	    "sub	%%edx,%%edx		\n\t"
	    "wrmsr	/* SYSENTER_EIP_MSR */	\n\t"
	    
	    "mov	$0x175,%%ecx		\n\t"
	    "mov	$__tss + 4,%%eax	\n\t"
	    "sub	%%edx,%%edx		\n\t"
	    "wrmsr	/* SYSENTER_ESP_MSR */	\n\t"
	    : : : "eax", "edx", "ecx");
}
#endif

#define PRINTTEXT(x) init_print_string(7, x)
#define PRINTINFO(x) init_print_string(2, x)
static int pos = 0;
static void init_putc(const char color, char s) L4_SECT_INIT;
INLINE void init_print_string(const char color, const char * s)
{
    char * __s = s <= (char*)KERNEL_VIRT ? (char*)s : virt_to_phys((char*)s);
    while(*__s)
	init_putc(color, *__s++);
}

static void init_putc(const char color, char s)
{
#define INIT_SCREEN(x) (((char*)0xb8000)[x])
#define POS (*virt_to_phys(&pos))
    switch (s)
    {
    case '\n':
	do {
	    INIT_SCREEN(POS++) = ' ';
	    INIT_SCREEN(POS++) = color;
	} while(POS % 160);
	break;
    default:
	INIT_SCREEN(POS++) = s;
	INIT_SCREEN(POS++) = color;
    }
}

INLINE int has_cpuid()
{
    dword_t flags1, flags2;
    asm("pushf			\n"
	"pop	%0		\n"
	"mov	%0, %1		\n"
	"xorl	%2, %0		\n"
	"push	%0		\n"
	"popf			\n"
	"pushf			\n"
	"pop	%0		\n"
	: "=r"(flags1), "=r"(flags2)
	: "i"(1 << 21)
	: "cc" );
    return flags1 != flags2;
}

static dword_t check_cpu_features() L4_SECT_INIT;
static dword_t check_cpu_features()
{
    dword_t cpu_features = 0;
    static const char* features[] L4_SECT_ROINIT = {
	"fpu",  "vme",  "de",   "pse",   "tsc",  "msr", "pae",  "mce",
	"cx8",  "apic", "?",    "sep",   "mtrr", "pge", "mca",  "cmov",
	"pat",  "pse-36",  "psn",  "cflsh", "?",    "ds",  "acpi", "mmx",
	"fxsr", "sse",  "sse2", "ss",    "?",    "tm",  "?",    "?" };

    /* first check if we have the cpuid instruction */
    if (has_cpuid())
	cpu_features = get_cpu_features();

    if ((cpu_features & CPU_FEATURE_MASK) != CPU_FEATURE_MASK)
    {
	if (!cpu_features) {
	    PRINTINFO("The CPUID instruction is not supported.\n");
	    goto abort;
	}

	PRINTINFO("Your processor does not support the following features:\n");
	for (int i = 0; i < 32; i++)
	    if (((1 << i) & CPU_FEATURE_MASK) && !((1 << i) & cpu_features))
	    {
		PRINTTEXT(features[i]);
		PRINTTEXT(" ");
	    }
	PRINTINFO("\n");
	goto abort;
    }

    dword_t version;
    asm("cpuid\n" : "=a"(version) : "0"(1) : "ebx", "ecx", "edx");
#if CONFIG_ARCH_X86_P4
    /* check that we are really running on a P4 */
    if (((version >> 8) & 0xf) != 0xf) {
	PRINTINFO("Your processor is not an Intel Pentium4\n");
	goto abort;
    }
#endif

    /* everything is fine - lets get the baby going */
    return cpu_features;

 abort:
    PRINTINFO("\nThis configuration of L4Ka/Hazelnut cannot boot on your system.\n");
    PRINTTEXT("For more information on supported hardware refer to\n"
	      "  http://l4ka.org/projects/hazelnut\n"
	      "Your system is halted now.\n");

    /* write back screen... but only on 486 and higher! */
    if (cpu_features)
	wbinvd();
    
    while(1) 
    { /* wait forever */ };
}


void init_paging()
{
    /* clear screen */
    for (volatile ptr_t p = (ptr_t) 0xb8000; p < (ptr_t) 0xb8000 + 40*25; p++)
	*p = (' ' | (0x07 << 8)) * 0x00010001;

    check_cpu_features();

    /* write back + invalidate cache */
    wbinvd();

    /* from kmemory.c */
    extern dword_t kmem_start;
    extern dword_t kmem_end;
    
    dword_t _kmem_start, _kmem_end;

#if defined(CONFIG_TRACEBUFFER)
    (*(dword_t*)(virt_to_phys(&kernel_info_page.main_mem_high)))-=4*1024*1024;
#endif

    /* get end of phys mem from kernel_info_page */
    _kmem_end = phys_to_virt(*(dword_t*)(virt_to_phys(&kernel_info_page.main_mem_high)));

#if defined(CONFIG_TRACEBUFFER)
    tracebuffer_t *_tracebuffer = (tracebuffer_t*)((_kmem_end + 0x3fffff) & (~0x3fffff));
    extern tracebuffer_t *tracebuffer;

    (*(dword_t*)(virt_to_phys(&tracebuffer))) = (dword_t)_tracebuffer;
#endif

#define MAXMEM (240*1024*1024)
    /* limit to a maximum of 240MB - we don't map the last 16M */
    if (virt_to_phys(_kmem_end) > MAXMEM-PAGE_SIZE)
	_kmem_end = phys_to_virt(MAXMEM-PAGE_SIZE);
    /* guess the amount of memory needed for startup - currently 8MB hardcoded */
    _kmem_start = _kmem_end - 8*1024*1024;
    /* SMP machines seem to lack some pages at the end of physical memory */
    _kmem_start += 64*1024; _kmem_start &= ~(1*1024*1024 - 1);

    /* check if there's memory */
    (*(volatile ptr_t) virt_to_phys(_kmem_start)) = 0x14101973;
    wbinvd();
    if ((*(volatile ptr_t) virt_to_phys(_kmem_start)) != 0x14101973)
	BUGSCREEN('1');

    /* write values to kmem's variables - we run still without an MMU */
    *virt_to_phys(&kmem_start) = _kmem_start;
    *virt_to_phys(&kmem_end) = _kmem_end;
    
    init_zero_memory(init_kernel_mem, PAGE_SIZE * KERNEL_INIT_PAGES);

    /* We have to operate on physical addresses. */
    dword_t *pdir_root = virt_to_phys(KERNEL_PAGE(0));
    dword_t *ptab_kernel1 = virt_to_phys(KERNEL_PAGE(1));


    /* Map init section. */
    for (int i = 0; i < MAXMEM; i+=0x00400000)
	init_map_4mb(pdir_root, i, i | PAGE_KERNEL_INIT_SECT);
    
    /* install 2nd level pagetable for the first 4M */
    pdir_root[pgdir_idx(KERNEL_VIRT)] =
	(dword_t) ptab_kernel1 | PAGE_KERNEL_TBITS;

    /* map the whole 4M as 4k pages */
    for ( dword_t addr = KERNEL_OFFSET;
	  addr < KERNEL_OFFSET + 0x00400000;
	  addr += PAGE_SIZE )
	init_map_4k(pdir_root, addr, virt_to_phys(addr) | PAGE_KERNEL_BITS);
    
    /* Map kernel dynamic memory. */
    for ( dword_t addr = 0xf0400000;
	  addr < 0xff000000;
	  addr += 0x00400000 )
       init_map_4mb(pdir_root, addr, (virt_to_phys(addr) & PAGEDIR_MASK) | PAGE_KERNEL_BITS);
      
#if defined(CONFIG_TRACEBUFFER)
    init_map_4mb(pdir_root, (dword_t)_tracebuffer, (virt_to_phys((dword_t)_tracebuffer) & PAGEDIR_MASK) | PAGE_KERNEL_BITS | PAGE_USER);
#endif

    /* Map video memory. */
    init_map_4k(pdir_root, KERNEL_VIDEO, virt_to_phys(KERNEL_VIDEO) | PAGE_KERNEL_BITS | PAGE_CACHE_DISABLE);

    /* Map hercules too... */
    init_map_4k(pdir_root, KERNEL_VIDEO_HERC, virt_to_phys(KERNEL_VIDEO_HERC) | PAGE_KERNEL_BITS | PAGE_CACHE_DISABLE);

#if defined(CONFIG_ENABLE_SMALL_AS)
    /* User-level trampoline for ipc_sysexit. */
    extern dword_t _start_utramp[], _start_utramp_p[];
    init_map_4k(pdir_root, (dword_t) _start_utramp,
		(dword_t) _start_utramp_p | PAGE_USER_BITS);

    /* We need user access in the pdir entry. */
    pdir_root[pgdir_idx(KERNEL_VIRT)] |= PAGE_USER;
#endif

#if defined(CONFIG_X86_APIC) || defined(CONFIG_MEASURE_INT_LATENCY)
    ptab_kernel1[pgtab_idx(KERNEL_LOCAL_APIC)] = 
	X86_LOCAL_APIC_BASE | PAGE_APIC_BITS;
#endif
#ifdef CONFIG_X86_APIC
    ptab_kernel1[pgtab_idx(KERNEL_IO_APIC)] =
	X86_IO_APIC_BASE | PAGE_APIC_BITS;
#endif

#if defined(CONFIG_IA32_FEATURE_PSE)
    /* Turn on super pages. */
    enable_super_pages();
#endif
#if defined(CONFIG_IA32_FEATURE_PGE)
    enable_global_pages();
#endif
#if defined(CONFIG_IA32_FEATURE_SEP)
    setup_sysenter_msrs();
#endif
#if defined(CONFIG_IA32_FEATURE_FXSR)
    enable_osfxsr();
#endif

#if defined(CONFIG_PERFMON)
    asm(
 	/* enable rdpmc instr in user mode */
 	"mov	%%cr4,%%eax\n"
 	"orl	$(1 << 8), %%eax\n"
 	"mov	%%eax,%%cr4\n"
 	: : : "eax");
#endif

    /* Load ptroot. */
    set_current_pagetable(pdir_root);

    /* Turn on paging and pray. */
    enable_paged_mode();

    /* store kernel pdir -- we have virtual mem now */
    kernel_ptdir_root = phys_to_virt(pdir_root);

    /* disable the FPU */
    disable_fpu();
    
#if defined(CONFIG_TRACEBUFFER)
    _tracebuffer->current = 0;
    _tracebuffer->magic = TBUF_MAGIC;
    _tracebuffer->counter = 0;
    _tracebuffer->threshold = 1000;

#if defined(CONFIG_PERFMON)    
    asm ( 
          "mov  $390, %%ecx             \n"     // disable PMCs
          "xor  %%edx, %%edx            \n"
          "xor  %%eax, %%eax            \n"
          "wrmsr                        \n"
          "inc  %%ecx                   \n"
          "wrmsr                        \n"
          
          "mov  $193, %%ecx             \n"     // clear PMCs
          "wrmsr                        \n"
          "inc  %%ecx                   \n"
          "wrmsr                        \n"
          
          "mov  $390, %%ecx             \n"     // init PMCs
          "mov  $0x4100C0, %%eax        \n"     // ENABLE + USER + INST_RETIRED
          "wrmsr                        \n"
          "inc  %%ecx                   \n"
          "mov  $0x4200C0, %%eax        \n"     // ENABLE + KRNL + INST_RETIRED
          "wrmsr                        \n"
          
          :
          :
          : "eax", "ecx", "edx" 
        );
#endif
#endif    
}

void dump_idt()
{
    printf("idt-dump: idt at %x\n", idt);
    for (int i = 0; i < (IDT_SIZE) ; i++) {
	if (idt[i].x.d.p)
	    printf("%d -> %4x:%x , dpl=%d, type=%d (%x:%x)\n", i,
		   idt[i].x.d.sel,
		   idt[i].x.d.offset_low | (idt[i].x.d.offset_high << 16),
		   idt[i].x.d.dpl, idt[i].x.d.type,
		   idt[i].x.raw[0], idt[i].x.raw[1]);
    }
}

void dump(void *address, int size)
{
    for (int i = 0; i < (size / 4); i++)
	printf("%x  ", ((dword_t*)address)[i]);
}


INLINE void add_int_gate(int index, void (*address)())
{
    ASSERT(index < IDT_SIZE);
    idt[index].set(address, IDT_DESC_TYPE_INT, 0);
}

INLINE void add_syscall_gate(int index, void (*address)())
{
    ASSERT(index < IDT_SIZE);
    idt[index].set(address, IDT_DESC_TYPE_INT, 3);
}

INLINE void add_trap_gate(int index, void (*address)())
{
    ASSERT(index < IDT_SIZE);
    idt[index].set(address, IDT_DESC_TYPE_TRAP, 0);
}

void setup_idt()
{
    /* clear IDT */
    init_zero_memory(idt, sizeof(idt));

    /* trap gates */
    add_int_gate(0, int_0);
    add_int_gate(1, int_1);
    add_int_gate(2, int_2);
    add_syscall_gate(3, int_3);
    add_int_gate(4, int_4);
    add_int_gate(5, int_5);
    add_int_gate(6, int_6);
    add_int_gate(7, int_7);
    add_int_gate(8, int_8);
    add_int_gate(9, int_9);
    add_int_gate(10, int_10);
    add_int_gate(11, int_11);
    add_int_gate(12, int_12);
    add_int_gate(13, int_13);
    add_int_gate(14, int_14);
    // 15 undefined
    add_int_gate(16, int_16);
    add_int_gate(17, int_17);
    add_int_gate(18, int_18);
    add_int_gate(19, int_19);

    /* hardware interrupts */
    add_int_gate(32, hwintr_0);
    add_int_gate(33, hwintr_1);
    add_int_gate(34, hwintr_2);
    add_int_gate(35, hwintr_3);
    add_int_gate(36, hwintr_4);
    add_int_gate(37, hwintr_5);
    add_int_gate(38, hwintr_6);
    add_int_gate(39, hwintr_7);
#if defined(CONFIG_X86_APICTIMER)
    add_int_gate(40, hwintr_8);
#else
    add_int_gate(40, timer_irq);
#endif
    add_int_gate(41, hwintr_9);
    add_int_gate(42, hwintr_10);
    add_int_gate(43, hwintr_11);
    add_int_gate(44, hwintr_12);
    add_int_gate(45, hwintr_13);
    add_int_gate(46, hwintr_14);
    add_int_gate(47, hwintr_15);

    /* trap gates -- we use automatic int disable mechanism :-) */
    add_syscall_gate(48, int_48);
    add_syscall_gate(49, int_49);
    add_syscall_gate(50, int_50);
    add_syscall_gate(51, int_51);
    add_syscall_gate(52, int_52);
    add_syscall_gate(53, int_53);
    add_syscall_gate(54, int_54);

#ifdef CONFIG_X86_APIC
    add_int_gate(APIC_LINT0_INT_VECTOR, apic_lint0);
    add_int_gate(APIC_LINT1_INT_VECTOR, apic_lint1);
#if defined(CONFIG_X86_APICTIMER)
    add_int_gate(APIC_TIMER_INT_VECTOR, timer_irq);
#endif
    add_int_gate(APIC_ERROR_INT_VECTOR, apic_error);
    add_int_gate(APIC_SPURIOUS_INT_VECTOR, apic_spurious_int);
# if defined(CONFIG_ENABLE_PROFILING)
    add_int_gate(APIC_PERFCTR_INT_VECTOR, apic_perfctr);
# endif
#endif

#ifdef CONFIG_SMP
    //add_int_gate(APIC_SMP_CONTROL_IPI, apic_smp_control_ipi);
    add_int_gate(APIC_SMP_COMMAND_IPI, apic_smp_command_ipi);
#endif

    //dump(idt, IDT_SIZE);
    //spin();

}

void setup_tss()
{
    init_zero_memory(&__tss, TSS_SIZE);
    /* This defines the stack segment to use when we switch TO privilege
       level 0 */
    __tss.ss0 = X86_KDS;
    /* The TSS contains a pointer to the base of the IO permission bitmap.
       This pointer is relative to the start of the TSS */
    __tss.iopbm_offset = ((dword_t) __tss.io_bitmap) - (dword_t) &__tss;
    /* The last byte in x86_tss_t (effectively the first byte behind the
       IO permission bitmap) is a stopper for the IO permission bitmap.
       It must be 0xFF (see IA32-RefMan, Part 1, Chapter Input/Output) */
    __tss.stopper = 0xFF;
}

void setup_gdt()
{
    /* clear GDT */
    init_zero_memory(gdt, sizeof(gdt));

    /*
     * Fill the GDT.  We also set the user segments to 4GB.  They will
     * be set to 3GB when needed (i.e., when small spaces are enabled).
     */
#   define MB (1024*1024)
#   define GB (MB*1024)
#   define gdt_idx(x) ((x) >> 3)
    gdt[gdt_idx(X86_KCS)].set_seg(0, ~0, 0, GDT_DESC_TYPE_CODE);
    gdt[gdt_idx(X86_KDS)].set_seg(0, ~0, 0, GDT_DESC_TYPE_DATA);
    gdt[gdt_idx(X86_UCS)].set_seg(0, ~0, 3, GDT_DESC_TYPE_CODE);
    gdt[gdt_idx(X86_UDS)].set_seg(0, ~0, 3, GDT_DESC_TYPE_DATA);

    /* the TSS
       The last byte in x86_tss_t is a stopper for the IO permission bitmap.
       That's why we set the limit in the GDT to one byte less than the actual
       size of the structure. (IA32-RefMan, Part 1, Chapter Input/Output) */
    gdt[gdt_idx(X86_TSS)].set_sys(&__tss, sizeof(__tss)-1, 0, GDT_DESC_TYPE_TSS);
    /* kdb data - phys_mem */
    gdt[gdt_idx(X86_KDB)].set_seg((ptr_t) KERNEL_OFFSET, GB*1, 0, GDT_DESC_TYPE_DATA);
    
#ifdef CONFIG_TRACEBUFFER
    gdt[gdt_idx(X86_TBS)].set_seg((ptr_t) tracebuffer, 4*MB, 3, GDT_DESC_TYPE_DATA);
#endif    
}

void activate_gdt()
{
    /* create a temporary GDT descriptor to load the GDTR from */
    sys_desc_t gdt_desc = {sizeof(gdt), (ptr_t)gdt, 0} ;

    asm("lgdt %0		\n"     /* load descriptor table       	*/
	"ljmp	%1,$1f		\n"	/* refetch code segment	descr.	*/
	"1:			\n"	/*   by jumping across segments	*/
	:
	: "m"(gdt_desc), "i" (X86_KCS)
	);

    /* set the segment registers from the freshly installed GDT
       and load the Task Register with the TSS via the GDT */
    asm("mov  %0, %%ds		\n"	/* reload data segment		*/
	"mov  %0, %%es		\n"	/* need valid %es for movs/stos	*/
	"mov  %1, %%ss		\n"	/* reload stack segment		*/
#ifdef CONFIG_TRACEBUFFER        
        "mov  %2, %%fs          \n"     /* tracebuffer segment          */
#endif        
	"movl %3, %%eax		\n"
	"ltr  %%ax		\n"	/* load install TSS		*/
	: 
	: "r"(X86_UDS), "r"(X86_KDS), "r"(X86_TBS), "r"(X86_TSS)
	: "eax");
}

void activate_idt()
{

    // and finally the idt
    sys_desc_t idt_desc = {sizeof(idt), (ptr_t) idt, 0};

    asm ("lidt %0"
	 :
	 : "m"(idt_desc)
	);
}

/* map and setup excp. vector */
void setup_excp_vector()
{
    setup_idt();

    setup_tss();

    setup_gdt();

    activate_gdt();
    activate_idt();
}



#if defined(CONFIG_SMP)
void init_cpu_local_data()
{
    get_idle_tcb()->cpu = get_apic_cpu_id();
    current_max_prio = MAX_PRIO;
    fpu_owner = NULL;
}

void init_smp() L4_SECT_INIT;
void init_smp()
{
#if defined(CONFIG_IA32_FEATURE_PSE)
    /* Turn on super pages. */
    enable_super_pages();
#endif
#if defined(CONFIG_IA32_FEATURE_PGE)
    enable_global_pages();
#endif
#if defined(CONFIG_IA32_FEATURE_SEP)
    setup_sysenter_msrs();
#endif
#if defined(CONFIG_IA32_FEATURE_FXSR)
    enable_osfxsr();
#endif

#if defined(CONFIG_PERFMON)
    asm(
 	/* enable rdpmc instr in user mode */
 	"mov	%%cr4,%%eax\n"
 	"orl	$(1 << 8), %%eax\n"
 	"mov	%%eax,%%cr4\n"
 	: : : "eax");

#endif
    set_current_pagetable(*virt_to_phys(&boot_ptdir));
    enable_paged_mode();

    /**********************************************************************
     *
     *      carefull -- no virtual memory accesses before that point
     *
     **********************************************************************/

    int cpu = get_apic_cpu_id();

#if defined(CONFIG_DEBUG_TRACE_SMP)
    printf("moin - processor %d is alive...\n", cpu);
#endif

    extern char _start_cpu_local;
    extern char _end_cpu_local;

    pgent_t * ref_pgent = get_kernel_space()->pgent(pgdir_idx((dword_t)&_start_cpu_local), boot_cpu);
    pgent_t * cpu_pgent = get_kernel_space()->pgent(pgdir_idx((dword_t)&_start_cpu_local), cpu);

//#if defined(CONFIG_DEBUG_SANITY)
    if (! ref_pgent->is_valid(get_kernel_space(), 1))
    {
	printf("init_smp: initial pagetable broken");
	spin(1);
    }
//#endif
    pgent_t * cpu_ptab = (pgent_t*)kmem_alloc(PAGE_SIZE);
    pgent_t * ref_ptab = ref_pgent->subtree(get_kernel_space(), 1);

#if defined(CONFIG_DEBUG_TRACE_SMP)
    printf("smp: create cpu-local ptab ((%p, %p)->(%p, %p))\n",
	   get_kernel_space()->pagedir(boot_cpu), ref_ptab, 
	   get_kernel_space()->pagedir(cpu), cpu_ptab);
#endif
    
    memcpy((dword_t*)cpu_ptab, (dword_t*)ref_ptab, PAGE_SIZE);

    /* adapt pagetable for cpu-local area */
    cpu_pgent->x.raw = ((dword_t)virt_to_phys(cpu_ptab)) | PAGE_KERNEL_TBITS;

    /* now re-map cpu-local area */
    cpu_ptab = cpu_ptab->next(get_kernel_space(), pgtab_idx((dword_t)&_start_cpu_local));
    int numpages = ((dword_t)(&_end_cpu_local)-(dword_t)(&_start_cpu_local))/PAGE_SIZE;

#if defined(CONFIG_DEBUG_TRACE_SMP)
    printf("remapping %d pages for cpu-local data @ %p\n", 
	   numpages, &_start_cpu_local);
#endif

    for (int i = 0; i < numpages; i++, cpu_ptab->next(get_kernel_space(), 1))
    {
	ptr_t page = kmem_alloc(PAGE_SIZE);
	cpu_ptab->x.raw = ((dword_t)virt_to_phys(page)) | PAGE_KERNEL_BITS;
    }
#if defined(CONFIG_DEBUG_TRACE_SMP)
    printf("cpu %d: switching to pagetable %x\n", 
	   cpu, get_kernel_space()->pagedir_phys(cpu));
#endif
    set_current_pagetable(get_kernel_space()->pagedir_phys(cpu));

#if defined(CONFIG_IA32_FEATURE_PGE)
    /* with global bits on we have to explicitely flush the cpu-local
     * pages (we do not know if we already touched them)
     */
    for (dword_t addr = (dword_t)&_start_cpu_local; 
	 addr < (dword_t)&_end_cpu_local; addr += PAGE_SIZE)
	flush_tlbent((ptr_t)addr);
#endif

#if defined(CONFIG_DEBUG_TRACE_SMP)
    printf("cpu %d: switched to cpu-specific pagetable (%p)\n", 
	   cpu, get_current_pagetable());
#endif

    /* now setup exception vector etc. */
    setup_gdt();
    setup_tss();

    activate_gdt();
    activate_idt();

    /* apic initialization */
    setup_local_apic();
    setup_apic_timer(kernel_info_page.bus_frequency / (1000000/TIME_QUANTUM));
#if defined(CONFIG_DEBUG_TRACE_SMP)
    printf("AP apic initialization done...\n");
#endif

    /* cpu-local initialization - afterwards we can use get_cpu_id() */
    init_cpu_local_data();

    /* deactivate fpu */
    disable_fpu();

    /* now switch to idle to start the other guys */
    switch_to_initial_thread(create_idle_thread());
}
#endif
