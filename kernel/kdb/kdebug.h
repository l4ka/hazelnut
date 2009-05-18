/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     kdebug.h
 * Description:   Global kdebug definitions and declarations.
 *                
 * @LICENSE@
 *                
 * $Id: kdebug.h,v 1.26 2001/12/04 21:15:25 uhlig Exp $
 *                
 ********************************************************************/
#ifndef __KDB__KDEBUG_H__
#define __KDB__KDEBUG_H__

#include <universe.h>

void kdebug_init_arch() L4_SECT_KDEBUG;
void kdebug_dump_map(dword_t paddr) L4_SECT_KDEBUG;
int kdebug_arch_entry(exception_frame_t * frame) L4_SECT_KDEBUG;
void kdebug_arch_exit(exception_frame_t * frame) L4_SECT_KDEBUG;
void kdebug_interrupt_association() L4_SECT_KDEBUG;
void kdebug_single_stepping(exception_frame_t *frame, dword_t onoff) L4_SECT_KDEBUG;
void kdebug_perfmon() L4_SECT_KDEBUG;
void kdebug_breakpoint() L4_SECT_KDEBUG;
void kdebug_arch_help() L4_SECT_KDEBUG;
void kdebug_cpustate(tcb_t *current) L4_SECT_KDEBUG;
int kdebug_step_instruction(exception_frame_t * frame) L4_SECT_KDEBUG;
int kdebug_step_block(exception_frame_t * frame) L4_SECT_KDEBUG;
void kdebug_disassemble(exception_frame_t * frame) L4_SECT_KDEBUG;


tcb_t *kdebug_find_task_tcb(dword_t tasknum, tcb_t *current) L4_SECT_KDEBUG;
int lookup_mapping(space_t *, dword_t, ptr_t *) L4_SECT_KDEBUG;
#define kdebug_get_mem(ptab, addr, ret)					\
({									\
    ptr_t __retval_p;							\
    int __ok;								\
    __ok = lookup_mapping((ptab), (dword_t) (addr), &__retval_p);	\
    if (__ok)								\
	*(ret) = *((typeof(ret)) phys_to_virt(__retval_p));		\
    __ok;								\
})

/* From x86_profiling.c */
void kdebug_init_profiling(void) L4_SECT_KDEBUG;
void kdebug_profiling(tcb_t *current) L4_SECT_KDEBUG;
void kdebug_profile_dump(tcb_t *current) L4_SECT_KDEBUG;

/* From x86_tracestore.c */
void kdebug_x86_bts(void) L4_SECT_KDEBUG;
void kdebug_x86_pebs(void) L4_SECT_KDEBUG;

/* From input.c */
dword_t kdebug_get_hex(dword_t def = 0, const char *str = NULL) L4_SECT_KDEBUG;
dword_t kdebug_get_dec(dword_t def = 0, const char *str = NULL) L4_SECT_KDEBUG;
tcb_t *kdebug_get_thread(tcb_t *current) L4_SECT_KDEBUG;
tcb_t *kdebug_get_task(tcb_t *current) L4_SECT_KDEBUG;
ptr_t kdebug_get_pgtable(tcb_t *current, char *str = NULL) L4_SECT_KDEBUG;
char kdebug_get_choice(const char *, const char *, char = 0, int = 0) L4_SECT_KDEBUG;

/* From x86_input.c */
#if defined(CONFIG_ARCH_X86_I686)
dword_t kdebug_get_perfctr(char *str, dword_t def = 0xff) L4_SECT_KDEBUG;
#elif defined(CONFIG_ARCH_X86_P4)
void kdebug_describe_perfctr(dword_t ctr) L4_SECT_KDEBUG;
dword_t kdebug_get_perfctr(dword_t *, qword_t *, qword_t *) L4_SECT_KDEBUG;
#endif

/* From tracepoint.c */
void kdebug_tracepoint(void) L4_SECT_KDEBUG;
void kdebug_list_tracepoints(int) L4_SECT_KDEBUG;

/* From x86_smp.c */
#if defined(CONFIG_SMP)
void kdebug_smp_info();
#endif


/* hacky */
#if defined(CONFIG_DEBUGGER_GDB)
#define CONFIG_DEBUGGER_IO_INCOM 1
#define CONFIG_DEBUGGER_IO_OUTCOM 1
#endif

#endif /* !__KDB__KDEBUG_H__ */
