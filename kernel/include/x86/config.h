/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86/config.h
 * Description:   Various kernel configuration for x86.
 *                
 * @LICENSE@
 *                
 * $Id: config.h,v 1.33 2002/05/13 13:04:30 stoess Exp $
 *                
 ********************************************************************/
#ifndef __X86_CONFIG_H__
#define __X86_CONFIG_H__

#include "cpu.h"

/* thread configuration */
#define USER_AREA_MAX		0xC0000000
#define TCB_AREA		0xE0000000
#define TCB_AREA_SIZE		0x01000000

#define MEM_COPYAREA1		0xE2000000
#define MEM_COPYAREA2		(MEM_COPYAREA1 + (PAGEDIR_SIZE * 2))
#define MEM_COPYAREA_END	(MEM_COPYAREA2 + (PAGEDIR_SIZE * 2))


#define SPACE_ID_IDX		((((MEM_COPYAREA_END >> PAGEDIR_BITS)+7)/8)*8)

#if defined(CONFIG_IO_FLEXPAGES)
#define IO_SPACE_IDX		(SPACE_ID_IDX+3)
#endif

#if defined(CONFIG_ENABLE_SMALL_AS)

#undef USER_AREA_MAX
#if defined(CONFIG_SMALL_AREA_2GB)
# define USER_AREA_MAX		0x60000000
#elif defined(CONFIG_SMALL_AREA_1GB)
# define USER_AREA_MAX		0xA0000000
#else
# define USER_AREA_MAX		0xC0000000
#endif

#define SMALL_SPACE_START	USER_AREA_MAX

/* Next entries are aligned to fit in one cache line. */
#define GDT_ENTRY1_IDX		(SPACE_ID_IDX+1)
#define GDT_ENTRY2_IDX		(SPACE_ID_IDX+2)


#endif

#if defined (CONFIG_LARGE_TCB)
#define L4_NUMBITS_TCBS		12
#else
#define L4_NUMBITS_TCBS		10
#endif
#define L4_NUMBITS_THREADS	22

#define L4_TOTAL_TCB_SIZE       (1 << L4_NUMBITS_TCBS)
#define L4_TCB_MASK		((~0) << L4_NUMBITS_TCBS)

/* see also linker/x86.lds */
#define KERNEL_PHYS		0x00100000
#define KERNEL_OFFSET		0xF0000000
#define KERNEL_VIRT		(KERNEL_PHYS + KERNEL_OFFSET)
#define KERNEL_SIZE		0x00012000
#define KERNEL_INIT_PAGES	2
#define KERNEL_CPU_LOCAL_PAGES	2

#define KERNEL_VIDEO		(0xF00b8000)
#define KERNEL_VIDEO_HERC	(0xF00b0000)
#define KERNEL_IO_APIC		(0xF00E0000)
#define KERNEL_LOCAL_APIC	(0xF00F0000)

/* startup code must be 4k aligned and < 1MB */
#define SMP_STARTUP_PAGE	(0x00004000)



/* spurious int vector MUST be @ 0x...f */
#define APIC_SPURIOUS_INT_VECTOR	31

/* apic interrupt vectors */
#define APIC_LINT0_INT_VECTOR		55
#define APIC_LINT1_INT_VECTOR		56
#define APIC_TIMER_INT_VECTOR		57
#define APIC_ERROR_INT_VECTOR		58

/* performance counter overflow */
#define APIC_PERFCTR_INT_VECTOR		59

/* smp interrupt vectors */
#if defined(CONFIG_ENABLE_PROFILING)
# define APIC_SMP_CONTROL_IPI		60
# define APIC_SMP_COMMAND_IPI		61
#else
# define APIC_SMP_CONTROL_IPI		59
# define APIC_SMP_COMMAND_IPI		60
#endif


/* scheduler configuration */
/* 10 ms = 1 timeslice */
#define DEFAULT_TIMESLICE	(10000)

/* 1.953ms per timer tick
 * VU: the hardware clock can only be configured to tick in 2^n Hz
 * 1000 / 512 Hz = 1.953125 ms/tick */
#define TIME_QUANTUM		(1953)


#define IDTSZ_ADD1	0
#define IDTSZ_ADD2	0
#define IDTSZ_ADD3	0

#if defined(CONFIG_X86_APIC)
# undef  IDTSZ_ADD1
# define IDTSZ_ADD1	4
#endif
#if defined(CONFIG_ENABLE_PROFILING)
# undef  IDTSZ_ADD2
# define IDTSZ_ADD2	1
#endif
#if defined(CONFIG_SMP)
# undef  IDTSZ_ADD3
# define IDTSZ_ADD3	2
#endif

#define IDT_SIZE	(55 + IDTSZ_ADD1 + IDTSZ_ADD2 + IDTSZ_ADD3)

#undef IDT_SZ_ADD1
#undef IDT_SZ_ADD2
#undef IDT_SZ_ADD3


/* number of descriptors in the GDT 
   1 null + 2 kernel + 2 user + tss + kdb + tracebuffer */
#ifdef CONFIG_TRACEBUFFER   
#define GDT_SIZE	8
#else
#define GDT_SIZE	7
#endif
#define TSS_SIZE	(sizeof(x86_tss_t))

#if defined(CONFIG_SMP) 
# if CONFIG_SMP_MAX_CPU > 32
/* currently we only support up to 32 cpu's. */
#  error maximum cpu is 32
# endif
#endif


#if !defined(ASSEMBLY)

static const dword_t CPU_FEATURE_MASK = ( X86_FEAT_TSC | X86_FEAT_DE

#if defined(CONFIG_IA32_FEATURE_PSE)
| X86_FEAT_PSE
#endif
#if defined(CONFIG_IA32_FEATURE_PGE)
| X86_FEAT_PGE
#endif
#if defined(CONFIG_IA32_FEATURE_SEP)
| X86_FEAT_SEP
#endif
#if defined(CONFIG_IA32_FEATURE_FXSR)
| X86_FEAT_FXSR
#endif
#if defined(CONFIG_IA32_FEATURE_MSR)
| X86_FEAT_MSR
#endif
#if defined(CONFIG_IA32_FEATURE_CMOV)
| X86_FEAT_CMOV
#endif
#if defined(CONFIG_X86_APIC)
 | X86_FEAT_APIC
#endif
#if defined(CONFIG_X86_P4_BST) || defined(CONFIG_X86_P4_PEBS)
 | X86_FEAT_DS
#endif
    );

#endif

#endif /* __X86_CONFIG_H__ */
