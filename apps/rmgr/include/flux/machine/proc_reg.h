/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University.
 * Copyright (c) 1996,1995 The University of Utah and
 * the Computer Systems Laboratory (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY LIABILITY
 * OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF
 * THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * Processor registers for i386 and i486.
 */
#ifndef	_FLUX_X86_PROC_REG_H_
#define	_FLUX_X86_PROC_REG_H_

#include <flux/config.h>

/*
 * CR0
 */
#define	CR0_PG	0x80000000		/*	 enable paging */
#define	CR0_CD	0x40000000		/* i486: cache disable */
#define	CR0_NW	0x20000000		/* i486: no write-through */
#define	CR0_AM	0x00040000		/* i486: alignment check mask */
#define	CR0_WP	0x00010000		/* i486: write-protect kernel access */
#define	CR0_NE	0x00000020		/* i486: handle numeric exceptions */
#define	CR0_ET	0x00000010		/*	 extension type is 80387 */
					/*	 (not official) */
#define	CR0_TS	0x00000008		/*	 task switch */
#define	CR0_EM	0x00000004		/*	 emulate coprocessor */
#define	CR0_MP	0x00000002		/*	 monitor coprocessor */
#define	CR0_PE	0x00000001		/*	 enable protected mode */

/*
 * CR4
 */
#define CR4_VME	0x00000001		/* enable virtual intrs in v86 mode */
#define CR4_PVI	0x00000002		/* enable virtual intrs in pmode */
#define CR4_TSD	0x00000004		/* disable RDTSC in user mode */
#define CR4_DE	0x00000008		/* debug extensions (I/O breakpoints) */
#define CR4_PSE	0x00000010		/* page size extensions */
#define CR4_PGE	0x00000020		/* page global extensions */
#define CR4_MCE	0x00000040		/* machine check exception */
#define CR4_PCE	0x00000100		/* enable read perf counter instr */

#ifndef	ASSEMBLER
#ifdef	__GNUC__

#include <flux/inline.h>


/* Some processors, notably my Am386DX/40,
   seem to have some rather subtle pipeline- or timing-related bugs
   which case really weird things to happen with pushf's and popf's
   that come too close together...  */

FLUX_INLINE unsigned get_eflags()
{
	unsigned eflags;
	asm volatile("
		jmp	1f
	1:	jmp	1f
	1:	jmp	1f
	1:	pushf
		jmp	1f
	1:	jmp	1f
	1:	jmp	1f
	1:	popl %0" : "=r" (eflags));
	return eflags;
}

FLUX_INLINE void set_eflags(unsigned eflags)
{
	asm volatile("
		pushl %0
		jmp	1f
	1:	jmp	1f
	1:	jmp	1f
	1:	popf
		jmp	1f
	1:	jmp	1f
	1:	jmp	1f
	1:	" : : "r" (eflags));
}

FLUX_INLINE void cli() { asm volatile("cli"); }
FLUX_INLINE void sti() { asm volatile("sti"); }
FLUX_INLINE void cld() { asm volatile("cld"); }
#ifdef __cplusplus
/* in C++, std is a reserved keyword -- so use a function macro instead */
#define std() asm volatile("std")
#else
FLUX_INLINE void std() { asm volatile("std"); }
#endif
FLUX_INLINE void clts() { asm volatile("clts"); }

FLUX_INLINE unsigned short get_cs()
{
	unsigned short cs;
	asm volatile("movw %%cs,%w0" : "=r" (cs));
	return cs;
}

FLUX_INLINE unsigned short get_ds()
{
	unsigned short ds;
	asm volatile("movw %%ds,%w0" : "=r" (ds));
	return ds;
}
FLUX_INLINE void set_ds(unsigned short ds)
{
	asm volatile("movw %w0,%%ds" : : "r" (ds));
}

FLUX_INLINE unsigned short get_es()
{
	unsigned short es;
	asm volatile("movw %%es,%w0" : "=r" (es));
	return es;
}
FLUX_INLINE void set_es(unsigned short es)
{
	asm volatile("movw %w0,%%es" : : "r" (es));
}

FLUX_INLINE unsigned short get_fs()
{
	unsigned short fs;
	asm volatile("movw %%fs,%w0" : "=r" (fs));
	return fs;
}
FLUX_INLINE void set_fs(unsigned short fs)
{
	asm volatile("movw %w0,%%fs" : : "r" (fs));
}

FLUX_INLINE unsigned short get_gs()
{
	unsigned short gs;
	asm volatile("movw %%gs,%w0" : "=r" (gs));
	return gs;
}
FLUX_INLINE void set_gs(unsigned short gs)
{
	asm volatile("movw %w0,%%gs" : : "r" (gs));
}

FLUX_INLINE unsigned short get_ss()
{
	unsigned short ss;
	asm volatile("movw %%ss,%w0" : "=r" (ss));
	return ss;
}
FLUX_INLINE void set_ss(unsigned short ss)
{
	asm volatile("movw %w0,%%ss" : : "r" (ss));
}

#define get_eax() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%eax, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_ebx() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%ebx, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_ecx() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%ecx, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_edx() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%edx, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_esi() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%esi, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_edi() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%edi, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_ebp() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%ebp, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_esp() \
    ({ \
	register unsigned int _temp__; \
	asm("movl %%esp, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define get_eflags() \
    ({ \
	register unsigned int _temp__; \
	asm volatile("pushf; popl %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define	get_cr0() \
    ({ \
	register unsigned int _temp__; \
	asm volatile("mov %%cr0, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define	set_cr0(value) \
    ({ \
	register unsigned int _temp__ = (value); \
	asm volatile("mov %0, %%cr0" : : "r" (_temp__)); \
     })

#ifdef HAVE_CR4
#define get_cr4() \
    ({ \
	register unsigned int _temp__; \
	asm volatile("mov %%cr4, %0" : "=r" (_temp__)); \
	_temp__; \
    })
#define set_cr4(value) \
    ({ \
	register unsigned int _temp__ = (value); \
	asm volatile("mov %0, %%cr4" : : "r" (_temp__)); \
     })
#else /* not HAVE_CR4 */
#define get_cr4() \
    ({ \
	register unsigned int _temp__; \
	asm volatile(".byte 0x0f,0x20,0xe0" : "=a" (_temp__)); \
	_temp__; \
    })
#define set_cr4(value) \
	asm volatile(".byte 0x0f,0x22,0xe0" : : "a" (value));
#endif /* HAVE_CR4 */

#define get_msw() \
    ({ \
	unsigned short _msw__; \
	asm volatile("smsw %0" : "=r" (_msw__)); \
	msw; \
    })

#define	get_cr2() \
    ({ \
	register unsigned int _temp__; \
	asm("mov %%cr2, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define	get_cr3() \
    ({ \
	register unsigned int _temp__; \
	asm("mov %%cr3, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define	set_cr3(value) \
    ({ \
	register unsigned int _temp__ = (value); \
	asm volatile("mov %0, %%cr3" : : "r" (_temp__)); \
     })

#define	set_ts() \
	set_cr0(get_cr0() | CR0_TS)

#define	clear_ts() \
	asm volatile("clts")

#define	get_tr() \
    ({ \
	unsigned short _seg__; \
	asm volatile("str %0" : "=rm" (_seg__) ); \
	_seg__; \
    })

#define	set_tr(seg) \
	asm volatile("ltr %0" : : "rm" ((unsigned short)(seg)) )

#define set_gdt(pseudo_desc) \
    ({ \
	asm volatile("lgdt %0" : : "m" ((pseudo_desc)->limit)); \
    })

#define set_idt(pseudo_desc) \
    ({ \
	asm volatile("lidt %0" : : "m" ((pseudo_desc)->limit)); \
    })

#define	get_ldt() \
    ({ \
	unsigned short _seg__; \
	asm volatile("sldt %0" : "=rm" (_seg__) ); \
	_seg__; \
    })

#define	set_ldt(seg) \
	asm volatile("lldt %0" : : "rm" ((unsigned short)(seg)) )

/*
 * Read the 64-bit timestamp counter (TSC) register.
 * Works only on Pentium and higher processors,
 * and in user mode only if the TSD bit in CR4 is not set.
 */
#if HAVE_RDTSC
#define get_tsc() \
    ({ \
	unsigned long low, high; \
	asm volatile("rdtsc" : "=d" (high), "=a" (low)); \
	((unsigned long long)high << 32) | low; \
    })
#else
#define get_tsc() \
    ({ \
	unsigned long low, high; \
	asm volatile( \
	".byte 0x0f; .byte 0x31" \
	: "=d" (high), "=a" (low)); \
	((unsigned long long)high << 32) | low; \
    })
#endif

/*
 * This doesn't set a processor register,
 * but it's often used immediately after setting one,
 * to flush the instruction queue.
 */
#define flush_instr_queue() \
	asm volatile("
		jmp	0f
		0:
	")

/* Inline functions work fine for 16-bit code as well.  */
#ifdef CODE16
#define i16_get_eflags		get_eflags
#define i16_set_eflags		set_eflags
#define i16_cli			cli
#define i16_sti			sti
#define i16_cld			cld
#define i16_std			std
#define i16_clts		clts
#define i16_get_cs		get_cs
#define i16_set_cs		set_cs
#define i16_get_ds		get_ds
#define i16_set_ds		set_ds
#define i16_get_es		get_es
#define i16_set_es		set_es
#define i16_get_fs		get_fs
#define i16_set_fs		set_fs
#define i16_get_gs		get_gs
#define i16_set_gs		set_gs
#define i16_get_ss		get_ss
#define i16_set_ss		set_ss
#define i16_get_cr0		get_cr0
#define i16_set_cr0		set_cr0
#define i16_get_cr3		get_cr3
#define i16_set_cr3		set_cr3
#define i16_get_msw		get_msw
#define i16_set_gdt		set_gdt
#define i16_set_idt		set_idt
#define i16_set_ldt		set_ldt
#define i16_set_tr		set_tr
#define i16_flush_instr_queue	flush_instr_queue
#endif

#endif	/* __GNUC__ */
#endif	/* ASSEMBLER */

#endif	/* _FLUX_X86_PROC_REG_H_ */
