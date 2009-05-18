/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86/init.h
 * Description:   Initialization declarations for x86.
 *                
 * @LICENSE@
 *                
 * $Id: init.h,v 1.13 2002/05/13 13:04:30 stoess Exp $
 *                
 ********************************************************************/
#ifndef __X86_INIT_H__
#define __X86_INIT_H__

extern "C" void int_0();
extern "C" void int_1();
extern "C" void int_2();
extern "C" void int_3();
extern "C" void int_4();
extern "C" void int_5();
extern "C" void int_6();
extern "C" void int_7();
extern "C" void int_8();
extern "C" void int_9();
extern "C" void int_10();
extern "C" void int_11();
extern "C" void int_12();
extern "C" void int_13();
extern "C" void int_14();
extern "C" void int_16();
extern "C" void int_17();
extern "C" void int_18();
extern "C" void int_19();

extern "C" void hwintr_0();
extern "C" void hwintr_1();
extern "C" void hwintr_2();
extern "C" void hwintr_3();
extern "C" void hwintr_4();
extern "C" void hwintr_5();
extern "C" void hwintr_6();
extern "C" void hwintr_7();
extern "C" void hwintr_8();
extern "C" void hwintr_9();
extern "C" void hwintr_10();
extern "C" void hwintr_11();
extern "C" void hwintr_12();
extern "C" void hwintr_13();
extern "C" void hwintr_14();
extern "C" void hwintr_15();

extern "C" void int_48();
extern "C" void int_49();
extern "C" void int_50();
extern "C" void int_51();
extern "C" void int_52();
extern "C" void int_53();
extern "C" void int_54();

extern "C" void timer_irq();

#if defined (CONFIG_X86_APIC)
extern "C" void apic_lint0();
extern "C" void apic_lint1();
extern "C" void apic_error();
extern "C" void apic_spurious_int();
#endif

#if defined (CONFIG_ENABLE_PROFILING)
extern "C" void apic_perfctr();
extern "C" void perfctr_bounce_back();
#endif

#if defined (CONFIG_SMP)
extern "C" void apic_smp_control_ipi();
extern "C" void apic_smp_command_ipi();
#endif /* CONFIG_SMP */

void init_irqs() L4_SECT_INIT;

#if defined(CONFIG_IO_FLEXPAGES)
/* From io_mapping.c */
void io_mdb_init(void) L4_SECT_INIT;
#endif 

#endif




