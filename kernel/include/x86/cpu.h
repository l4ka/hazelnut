/*********************************************************************
 *                
 * Copyright (C) 1999-2002,  Karlsruhe University
 *                
 * File path:     x86/cpu.h
 * Description:   x86 specific definitions and inlines.
 *                
 * @LICENSE@
 *                 * $Id: cpu.h,v 1.58 2002/05/31 11:49:56 stoess Exp $
 *                
 ********************************************************************/
#ifndef __X86_CPU_H__
#define __X86_CPU_H__

/* CPU Features (CPUID) */
#define X86_FEAT_FPU	(1 << 0)
#define X86_FEAT_VME	(1 << 1)
#define X86_FEAT_DE	(1 << 2)
#define X86_FEAT_PSE	(1 << 3)
#define X86_FEAT_TSC	(1 << 4)
#define X86_FEAT_MSR	(1 << 5)
#define X86_FEAT_PAE	(1 << 6)
#define X86_FEAT_MCE	(1 << 7)
#define X86_FEAT_CXS	(1 << 8)
#define X86_FEAT_APIC	(1 << 9)
#define X86_FEAT_SEP	(1 << 11)
#define X86_FEAT_MTRR	(1 << 12)
#define X86_FEAT_PGE	(1 << 13)
#define X86_FEAT_MCA	(1 << 14)
#define X86_FEAT_CMOV	(1 << 15)
#define X86_FEAT_FGPAT	(1 << 16)
#define X86_FEAT_PSE36	(1 << 17)
#define X86_FEAT_PSN	(1 << 18)
#define X86_FEAT_CLFLSH	(1 << 19)
#define X86_FEAT_DS	(1 << 21)
#define X86_FEAT_ACPI	(1 << 22)
#define X86_FEAT_MMX	(1 << 23)
#define X86_FEAT_FXSR	(1 << 24)
#define X86_FEAT_SSE	(1 << 25)
#define X86_FEAT_SSE2	(1 << 26)
#define X86_FEAT_SS	(1 << 27)
#define X86_FEAT_HT	(1 << 28)
#define X86_FEAT_TM	(1 << 29)
#define X86_FEAT_IA64	(1 << 30)


/* Segment register values */
#define X86_KCS		0x08
#define X86_KDS		0x10
#define X86_UCS		0x1b
#define X86_UDS		0x23
#define X86_TSS		0x28
#define X86_KDB		0x30
#define X86_TBS         0x38

/* some bits for the EFLAGS register */
#define X86_EFL_TF	(1 <<  8)	/* trap flag			*/
#define X86_EFL_IF	(1 <<  9)	/* interrupt enable flag	*/
#define X86_EFL_RF	(1 << 16)	/* resume flag			*/
#define X86_EFL_VIF     (1 << 19)       /* virtual interrupt flag       */
#define X86_EFL_VIP     (1 << 20)       /* virtual interrupt pending flag */
#define X86_EFL_IOPL(x)	((x & 3) << 12)	/* the IO privilege level field	*/

#if defined(CONFIG_IO_FLEXPAGES)
#define X86_DEFAULT_IOPL 0
#else
#define X86_DEFAULT_IOPL 3
#endif

#if defined(CONFIG_ENABLE_PVI)
#define X86_USER_FLAGS	(X86_EFL_IOPL(X86_DEFAULT_IOPL) + X86_EFL_IF + X86_EFL_VIF+ 2)
#else
#if defined(CONFIG_USERMODE_NOIRQ)
#define X86_USER_FLAGS	(X86_EFL_IOPL(X86_DEFAULT_IOPL) + 2)
#else
#define X86_USER_FLAGS	(X86_EFL_IOPL(X86_DEFAULT_IOPL) + X86_EFL_IF + 2)
#endif

#endif /* CONFIG_ENABLE_PVI */

/* floating point unit state size 
 * with sse, sse2 ext. we use fxsave and the state is 512 bytes
 * otherwise we use fsave and the size is 108 bytes
 */
#if defined(CONFIG_IA32_FEATURE_FXSR)
# define X86_FLOATING_POINT_SIZE	512
# define X86_FLOATING_POINT_ALLOC_SIZE	0x400
#else
# define X86_FLOATING_POINT_SIZE	108
#endif


/* some bits for the control registers */
#define X86_CR0_PE	(1 <<  0)	/* enable protected mode	*/
#define X86_CR0_EM	(1 <<  2)	/* disable fpu			*/
#define X86_CR0_TS	(1 <<  3)	/* task switched		*/
#define X86_CR0_WP	(1 << 16)	/* force write protection on user
					   read only pages for kernel	*/
#define X86_CR0_NW	(1 << 29)	/*				*/
#define X86_CR0_CD	(1 << 30)	/*				*/
#define X86_CR0_PG	(1 << 31)	/* enable paging		*/
#define X86_CR4_PVI	(1 <<  1)	/* enable protected mode
					   virtual interrupts		*/
#define X86_CR4_PSE	(1 <<  4)	/* enable super-pages (4M)	*/
#define X86_CR4_PGE	(1 <<  7)	/* enable global pages		*/
#define X86_CR4_PCE	(1 <<  8)	/* allow user to use rdpmc	*/
#define X86_CR4_OSFXSR	(1 <<  9)	/* enable fxsave/fxrstor + sse	*/
#define X86_CR4_OSXMMEXCPT (1 << 10)	/* support for unmsk. SIMD exc. */

/*
 * Model specific register locations.
 */

#define IA32_SYSENTER_CS_MSR		0x174
#define IA32_SYSENTER_EIP_MSR		0x176
#define IA32_SYSENTER_ESP_MSR		0x175

#define IA32_DEBUGCTL			0x1d9

#if defined(CONFIG_ARCH_X86_I586)
# define IA32_TSC			0x010
# define IA32_CESR			0x011
# define IA32_CTR0			0x012
# define IA32_CTR1			0x013
#endif /* CONFIG_ARCH_X86_I586 */

#if defined(CONFIG_ARCH_X86_I686)
# define IA32_PERFCTR0			0x0c1
# define IA32_PERFCTR1			0x0c2
# define IA32_EVENTSEL0			0x186
# define IA32_EVENTSEL1			0x187
# define IA32_LASTBRANCHFROMIP		0x1db
# define IA32_LASTBRANCHTOIP		0x1dc
# define IA32_LASTINTFROMIP		0x1dd
# define IA32_LASTINTTOIP		0x1de
#endif /* CONFIG_ARCH_X86_I868 */

#if defined(CONFIG_ARCH_X86_P4)
# define IA32_MISC_ENABLE		0x1a0
# define IA32_COUNTER_BASE		0x300
# define IA32_CCCR_BASE			0x360
# define IA32_TC_PRECISE_EVENT		0x3f0
# define IA32_PEBS_ENABLE		0x3f1
# define IA32_PEBS_MATRIX_VERT		0x3f2
# define IA32_DS_AREA			0x600
# define IA32_LER_FROM_LIP		0x1d7
# define IA32_LER_TO_LIP		0x1d8
# define IA32_LASTBRANCH_TOS		0x1da
# define IA32_LASTBRANCH_0		0x1db
# define IA32_LASTBRANCH_1		0x1dc
# define IA32_LASTBRANCH_2		0x1dd
# define IA32_LASTBRANCH_3		0x1de

/* Processor features in the MISC_ENABLE MSR. */
# define IA32_ENABLE_FAST_STRINGS	(1 << 0)
# define IA32_ENABLE_X87_FPU		(1 << 2)
# define IA32_ENABLE_THERMAL_MONITOR	(1 << 3)
# define IA32_ENABLE_SPLIT_LOCK_DISABLE	(1 << 4)
# define IA32_ENABLE_PERFMON		(1 << 7)
# define IA32_ENABLE_BRANCH_TRACE	(1 << 11)
# define IA32_ENABLE_PEBS		(1 << 12)

/* Preceise Event-Based Sampling (PEBS) support. */
# define IA32_PEBS_REPLAY_TAG_MASK	((1 << 12)-1)
# define IA32_PEBS_UOP_TAG		(1 << 24)
# define IA32_PEBS_ENABLE_PEBS		(1 << 25)
#endif /* CONFIG_ARCH_X86_P4 */



/* PIC */
#define X86_PIC1_CMD	(0x20)
#define X86_PIC1_DATA	(0x21)
#define X86_PIC2_CMD	(0xA0)
#define X86_PIC2_DATA	(0xA1)

/* If PIC handling is done exclusively by the kernel, we can save
   a lot of time by shadowing the mask register. */

#ifdef CONFIG_X86_INKERNEL_PIC
#define PIC1_MASK(irq) \
    do { pic1_mask |= 1 << (irq); \
    outb(X86_PIC1_DATA, pic1_mask); } while (0)
#define PIC1_UNMASK(irq) \
    do { pic1_mask &= ~(1 << (irq)); \
    outb(X86_PIC1_DATA, pic1_mask); } while (0)
#define PIC2_MASK(irq) \
    do { pic2_mask |= 1 << (irq); \
    outb(X86_PIC2_DATA, pic2_mask); } while (0)
#define PIC2_UNMASK(irq) \
    do { pic2_mask &= ~(1 << (irq)); \
    outb(X86_PIC2_DATA, pic2_mask); } while (0)
#define PIC1_SET_MASK(mask) \
    do { pic1_mask = (mask); outb(X86_PIC1_DATA, pic1_mask); } while (0)
#define PIC2_SET_MASK(mask) \
    do { pic2_mask = (mask); outb(X86_PIC2_DATA, pic2_mask); } while (0)
#else
#define PIC1_MASK(irq) \
   outb(X86_PIC1_DATA, inb(X86_PIC1_DATA) | (1 << (irq)))
#define PIC2_MASK(irq) \
   outb(X86_PIC2_DATA, inb(X86_PIC2_DATA) | (1 << (irq)))
#define PIC1_UNMASK(irq) \
   outb(X86_PIC1_DATA, inb(X86_PIC1_DATA) & ~(1 << (irq)))
#define PIC2_UNMASK(irq) \
   outb(X86_PIC2_DATA, inb(X86_PIC2_DATA) & ~(1 << (irq)))
#define PIC1_SET_MASK(mask) \
   outb(X86_PIC1_DATA, (mask))
#define PIC2_SET_MASK(mask) \
   outb(X86_PIC2_DATA, (mask))
#endif

#define X86_RTC_CMD	(0x70)
#define X86_RTC_DATA	(0x71)

#define KERNEL_VERSION_VER	KERNEL_VERSION_CPU_X86

#ifndef ASSEMBLY

#if defined(CONFIG_IO_FLEXPAGES)
#define X86_IOPERMBITMAP_SIZE	0x10000
#else
#define X86_IOPERMBITMAP_SIZE 0
#endif
typedef struct {
    dword_t	link;
    dword_t	esp0, ss0;
    dword_t	esp1, ss1;
    dword_t	esp2, ss2;
    dword_t	cr3;
    dword_t	eip, eflags;
    dword_t	eax, ecx, edx, ebx, esp, ebp, esi, edi;
    dword_t	es, cs, ss, ds, fs, gs;
    dword_t	ldt;
    word_t	trace;
    word_t	iopbm_offset;
#if defined(CONFIG_IO_FLEXPAGES)
    byte_t	io_bitmap[X86_IOPERMBITMAP_SIZE/8+1] __attribute__((aligned(4096)));
#else
    byte_t	io_bitmap[(X86_IOPERMBITMAP_SIZE+7)/8];
#endif    
    byte_t	stopper;
} x86_tss_t;

typedef struct {
    word_t	size;
    ptr_t	ptr __attribute__((packed));
    word_t	zero;
} sys_desc_t;

/* the IDT entry data structure
   ... and a method to set one in a human-readible fashion */
class idt_desc_t {
 public:
    union {
	dword_t raw[2];
	
	struct {
	    dword_t offset_low	: 16;
	    dword_t sel		: 16;
	    dword_t res0	:  8;
	    dword_t type	:  3;
	    dword_t d		:  1;
	    dword_t res1	:  1;
	    dword_t dpl		:  2;
	    dword_t p		:  1;
	    dword_t offset_high	: 16;
	} d;
    } x;
#define IDT_DESC_TYPE_INT	6
#define IDT_DESC_TYPE_TRAP	7
    /* set an entry
       - address is the offset of the handler in X86_KCS
       - type selects Interrupt Gate or Trap Gate respectively
       - dpl sets the numerical maximum CPL of allowed calling code
     */
    inline void set(void (*address)(), int type, int dpl)
	{
	    x.d.offset_low  = ((dword_t) address      ) & 0xFFFF;
	    x.d.offset_high = ((dword_t) address >> 16) & 0xFFFF;
	    x.d.dpl = dpl;
	    x.d.type = type;
	    
	    /* set constant values */
	    x.d.sel = X86_KCS;
	    x.d.p = 1;	/* present	*/
	    x.d.d = 1;	/* size is 32	*/

	    /* clear reserved fields */
	    x.d.res0 = x.d.res1 = 0;
	};
};

class seg_desc_t {
 public:
    union {
	dword_t raw[2];
	struct {
	    dword_t limit_low	: 16;
	    dword_t base_low	: 24 __attribute__((packed));
	    dword_t type	:  4;
	    dword_t s		:  1;
	    dword_t dpl		:  2;
	    dword_t p		:  1;
	    dword_t limit_high	:  4;
	    dword_t avl		:  2;
	    dword_t d		:  1;
	    dword_t g		:  1;
	    dword_t base_high	:  8;
	} d __attribute__((packed));
    } x;

#define GDT_DESC_TYPE_CODE	0xb
#define GDT_DESC_TYPE_DATA	0x3
#define GDT_DESC_TYPE_TSS	0x9
    inline void set_seg(void* base, dword_t limit, int dpl, int type)
	{
	    x.d.limit_low  = (limit >> 12) & 0xFFFF;
	    x.d.limit_high = (limit >> 28) & 0xF;
	    x.d.base_low   =  (dword_t) base        & 0xFFFFFF;
	    x.d.base_high  = ((dword_t) base >> 24) &     0xFF;
	    x.d.type = type;
	    x.d.dpl = dpl;

	    /* default fields */
	    x.d.p = 1;	/* present		*/
	    x.d.g = 1;	/* 4k granularity	*/
	    x.d.d = 1;	/* 32-bit segment	*/
	    x.d.s = 1;	/* non-system segment	*/

	    /* unused fields */
	    x.d.avl = 0;
	}
    inline void set_sys(void* base, dword_t limit, int dpl, int type)
	{
	    x.d.limit_low  =  limit        & 0xFFFF;
	    x.d.limit_high = (limit >> 16) & 0xFF;
	    x.d.base_low   =  (dword_t) base        & 0xFFFFFF;
	    x.d.base_high  = ((dword_t) base >> 24) &     0xFF;
	    x.d.type = type;
	    x.d.dpl = dpl;

	    /* default fields */
	    x.d.p = 1;	/* present		*/
	    x.d.g = 0;	/* byte granularity	*/
	    x.d.d = 0;	/* 32-bit segment	*/
	    x.d.s = 0;	/* non-system segment	*/

	    /* unused fields */
	    x.d.avl = 0;
	}
};


typedef struct {
    dword_t	fault_code;
    dword_t	es;
    dword_t	ds;
    /* pusha */
    dword_t	edi;
    dword_t	esi;
    dword_t	ebp;
    dword_t	esp;
    dword_t	ebx;
    dword_t	edx;
    dword_t	ecx;
    dword_t	eax;
    /* default exception frame */
    dword_t	error_code;
    dword_t	fault_address;
    dword_t	cs;
    dword_t	eflags;
    dword_t	*user_stack;
    dword_t	ss;
} exception_frame_t;    



INLINE void outb(const dword_t port, const byte_t val)
{
    /* GCC can optimize here if constant */
    if (__builtin_constant_p(port) && (port < 0x100))
	__asm__ __volatile__("outb	%1, %0\n"
			     :
			     : "dN"(port), "al"(val));
    else
	__asm__ __volatile__("outb	%1, %%dx\n"
			     :
			     : "d"(port), "al"(val));
}

INLINE byte_t inb(const dword_t port)
{
    byte_t tmp;
    /* GCC can optimize here if constant */
    if (__builtin_constant_p(port) && (port < 0x100))
	__asm__ __volatile__("inb %1, %0\n"
			     : "=al"(tmp)
			     : "dN"(port));
    else
	__asm__ __volatile__("inb %%dx, %0\n"
			     : "=al"(tmp)
			     : "d"(port));
    return tmp;
}

INLINE void outw(const dword_t port, const word_t val)
{
    /* GCC can optimize here if constant */
    if (__builtin_constant_p(port) && (port < 0x100))
	__asm__ __volatile__("outw	%1, %0\n"
			     :
			     : "dN"(port), "ax"(val));
    else
	__asm__ __volatile__("outw	%1, %%dx\n"
			     :
			     : "d"(port), "ax"(val));
}

INLINE word_t inw(const dword_t port)
{
    word_t tmp;
    /* GCC can optimize here if constant */
    if (__builtin_constant_p(port) && (port < 0x100))
	__asm__ __volatile__("inw %1, %0\n"
			     : "=ax"(tmp)
			     : "dN"(port));
    else
	__asm__ __volatile__("inw %%dx, %0\n"
			     : "=ax"(tmp)
			     : "d"(port));
    return tmp;
};

INLINE void outl(const dword_t port, const dword_t val)
{
    /* GCC can optimize here if constant */
    if (__builtin_constant_p(port) && (port < 0x100))
	__asm__ __volatile__("outl	%1, %0\n"
			     :
			     : "dN"(port), "a"(val));
    else
	__asm__ __volatile__("outl	%1, %%dx\n"
			     :
			     : "d"(port), "a"(val));
}

INLINE dword_t inl(const dword_t port)
{
    dword_t tmp;
    /* GCC can optimize here if constant */
    if (__builtin_constant_p(port) && (port < 0x100))
	__asm__ __volatile__("inl %1, %0\n"
			     : "=a"(tmp)
			     : "dN"(port));
    else
	__asm__ __volatile__("inl %%dx, %0\n"
			     : "=a"(tmp)
			     : "d"(port));
    return tmp;
};

INLINE void out_rtc(const dword_t rtc_port, const byte_t val)
{
    outb(X86_RTC_CMD, rtc_port);
    outb(X86_RTC_DATA, val);
}

INLINE byte_t in_rtc(const dword_t rtc_port)
{
    outb(X86_RTC_CMD, rtc_port);
    // wait a little - necessary for older clocks
    __asm__ __volatile__("jmp 1f;1:jmp 1f;1:jmp 1f;1:jmp 1f;1:\n");
    return inb(X86_RTC_DATA);
}

INLINE void enable_interrupts()
{
    __asm__ __volatile__ ("sti\n":);
}

INLINE void disable_interrupts()
{
    __asm__ __volatile__ ("cli\n":);
}

#if !defined(CONFIG_SMP)
INLINE void system_sleep()
{
/* 
 * If we're running in PVI mode, we set the idler's IOPL
 * to 3.
 * If we run in an interrupt, we can check, if we're sigma0 or idle
 * simply by checking the IOPL in the exception frame. We have to do this, as
 * the VIF flags does not help, if we're sigma0 or idle.  
 * This is faster than checking the tcb...
 */ 
    __asm__ __volatile__(
#if defined(CONFIG_ENABLE_PVI)
	"pushf \n"
	"orl $(3 << 12), (%esp)\n"
	"popf \n"
#endif
	"sti	\n"
	"hlt	\n"
	"cli	\n"
	:);
}
#else
/* idling is polling for external events on SMP systems */
void system_sleep();
#endif

INLINE void flush_tlb()
{
    dword_t dummy;
    __asm__ __volatile__("movl %%cr3, %0\n"
			 "movl %0, %%cr3\n"
			 : "=r" (dummy));
}


INLINE void flush_tlbent(ptr_t addr)
{
    __asm__ __volatile__ ("invlpg (%0)	\n"
			  :
			  :"r" (addr));
}

INLINE dword_t get_cpu_features()
{
    dword_t features, dummy;
    __asm__ ("cpuid\n"
	     : "=d"(features), "=a"(dummy)
	     : "a"(1)
	     : "ebx", "ecx");
    return features;
}

INLINE void enable_super_pages()
{
    dword_t dummy;
    __asm__ __volatile__ ("mov %%cr4, %0\n"
			  "orl %1, %0\n"
			  "mov %0, %%cr4\n"
			  : "=r" (dummy)
			  : "i" (X86_CR4_PSE));
}

INLINE void enable_global_pages()
{
    dword_t dummy;
    __asm__ __volatile__ ("mov %%cr4, %0\n"
			  "orl %1, %0\n"
			  "mov %0, %%cr4\n"
			  : "=r" (dummy)
			  : "i" (X86_CR4_PGE));
}

INLINE void enable_paged_mode()
{
    asm("mov %0, %%cr0\n"
	"nop;nop;nop\n"
	:
	: "r"(X86_CR0_PG | X86_CR0_WP | X86_CR0_PE)
	);
}

INLINE void enable_fpu()
{
    dword_t dummy;
    __asm__ __volatile__ ("mov %%cr0, %0\n"
			  "andl %1, %0\n"
			  "mov %0, %%cr0\n"
			  : "=r" (dummy)
			  : "i" (~X86_CR0_TS));
}

INLINE void enable_osfxsr()
{
    dword_t dummy;
    __asm__ __volatile__ ("mov %%cr4, %0	\n"
			  "orl	%1, %0		\n"
			  "mov	%0, %%cr4	\n"
			  : "=r"(dummy)
			  : "i"(X86_CR4_OSFXSR));
}

INLINE void disable_fpu()
{
    dword_t dummy;
    __asm__ __volatile__ ("mov %%cr0, %0\n"
			  "orl %1, %0\n"
			  "mov %0, %%cr0\n"
			  : "=r" (dummy)
			  : "i" (X86_CR0_TS));

}

INLINE void save_fpu_state(byte_t *fpu_state)
{
    __asm__ __volatile__ (
#if !defined(CONFIG_IA32_FEATURE_FXSR)
	"fnsave %0"
#else
	"fxsave %0"
#endif
	: 
	: "m" (*fpu_state));
}

INLINE void load_fpu_state(byte_t *fpu_state)
{
    __asm__ __volatile__ (
#if !defined(CONFIG_IA32_FEATURE_FXSR)
	"frstor %0"
#else
	"fxrstor %0"
#endif
	: 
	: "m" (*fpu_state));
}

INLINE void init_fpu()
{
    __asm__ __volatile__ ("finit");
}


INLINE void load_debug_regs(ptr_t dbg)
{
    __asm__ __volatile__ (
	"	mov 0(%0), %%eax	\n"
	"	mov %%eax, %%dr0	\n"
	"	mov 4(%0), %%eax	\n"
	"	mov %%eax, %%dr1	\n"
	"	mov 8(%0), %%eax	\n"
	"	mov %%eax, %%dr2	\n"
	"	mov 12(%0), %%eax	\n"
	"	mov %%eax, %%dr3	\n"
	"	mov 24(%0), %%eax	\n"
	"	mov %%eax, %%dr6	\n"
	"	mov 28(%0), %%eax	\n"
	"	mov %%eax, %%dr7	\n"
	:
	:"r"(dbg)
	:"eax");
}

INLINE void save_debug_regs(ptr_t dbg)
{
    __asm__ __volatile__ (
	"	mov %%dr0, %%eax	\n"
	"	mov %%eax, 0(%0)	\n"
	"	mov %%dr1, %%eax	\n"
	"	mov %%eax, 4(%0)	\n"
	"	mov %%dr2, %%eax	\n"
	"	mov %%eax, 8(%0)	\n"
	"	mov %%dr3, %%eax	\n"
	"	mov %%eax, 12(%0)	\n"
	"	mov %%dr6, %%eax	\n"
	"	mov %%eax, 24(%0)	\n"
	"	mov %%dr7, %%eax	\n"
	"	mov %%eax, 28(%0)	\n"
	:
	:"r"(dbg)
	:"eax");
}

INLINE void reload_user_segregs (void)
{
    asm volatile (
	"	movl %0, %%es	\n"
#if !defined(CONFIG_TRACEBUFFER)
	"	movl %0, %%fs	\n"
#endif
	"	movl %0, %%gs	\n"
	:
	: "r" (X86_UDS));
}

INLINE qword_t rdpmc(const int ctrsel)
{
    qword_t __return;
    
    __asm__ __volatile__ (
	"rdpmc"
	: "=A"(__return)
	: "c"(ctrsel));
    
    return __return;
}

INLINE qword_t rdtsc(void)
{
    qword_t __return;

    __asm__ __volatile__ (
	"rdtsc"
	: "=A"(__return));

    return __return;
}

INLINE qword_t rdmsr(const dword_t reg)
{
    qword_t __return;

    __asm__ __volatile__ (
	"rdmsr"
	: "=A"(__return)
	: "c"(reg)
	);

    return __return;
}

INLINE void wrmsr(const dword_t reg, const qword_t val)
{
    __asm__ __volatile__ (
	"wrmsr"
	:
	: "A"(val), "c"(reg));
}

INLINE void wbinvd()
{
    __asm__ ("wbinvd\n" : : : "memory");
}

INLINE int lsb (dword_t w) __attribute__ ((const));
INLINE int lsb (dword_t w)
{
    int bitnum;
    __asm__ ("bsf %1, %0" : "=r" (bitnum) : "rm" (w));
    return bitnum;
}

#if defined(CONFIG_ENABLE_PVI)
INLINE void enable_pvi(void){
    dword_t dummy;
    __asm__ __volatile__ ("mov %%cr4, %0\n"
			  "orl %1, %0\n"
			  "mov %0, %%cr4\n"
			  : "=r" (dummy)
			  : "i" (X86_CR4_PVI));
    
}

INLINE void disable_pvi(void){
    dword_t dummy;
    __asm__ __volatile__ ("mov %%cr4, %0\n"
			  "andl %1, %0\n"
			  "mov %0, %%cr4\n"
			  : "=r" (dummy)
			  : "i" (~X86_CR4_PVI));

}    

INLINE dword_t get_cr4(void){
    dword_t ret;
    __asm__ __volatile__ ("mov %%cr4, %0\n"
			  : "=r" (ret));
    return ret;
}
    

#endif /* ENABLE_PVI */

#endif /* ASSEMBLY */

#endif /* __X86_CPU_H__ */
