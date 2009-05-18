/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86/kdebug.h
 * Description:   x86 specific kdebug stuff.
 *                
 * @LICENSE@
 *                
 * $Id: kdebug.h,v 1.18 2001/11/22 14:56:37 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __X86_KDEBUG_H__
#define __X86_KDEBUG_H__

#if defined(CONFIG_X86_P4_BTS) || defined(CONFIG_X86_P4_PEBS)
void init_tracestore(void) L4_SECT_INIT;
#endif


INLINE void spin(int pos = 80) {
    while(1) 
#if !defined(CONFIG_SMP)
        ((word_t*)(KERNEL_VIDEO))[pos] += 1;
#else
        ((word_t*)(KERNEL_VIDEO))[pos + (get_cpu_id() * 80)] += 1;
#endif
}

INLINE void spin1(int pos) {
#if defined(CONFIG_DEBUG_SPIN)
# if !defined(CONFIG_SMP)
    ((char*)(KERNEL_VIDEO))[pos * 2] += 1;
    ((char*)(KERNEL_VIDEO))[pos * 2 + 1] = 7;
# else
    ((char*)(KERNEL_VIDEO))[(get_cpu_id() * 80 + pos) * 2] += 1;
    ((char*)(KERNEL_VIDEO))[(get_cpu_id() * 80 + pos) * 2 + 1] = 7;
# endif    
#endif
}

#define enter_kdebug(arg...)			\
__asm__ __volatile__ (				\
    "int $3;"					\
    "jmp 1f;"					\
    ".ascii	\"KD# " arg "\";"		\
    "1:")

INLINE ptr_t get_current_pgtable()
{
    ptr_t t;
    __asm__ __volatile__ ("mov	%%cr3, %0" : "=r" (t));
    return t;
}

#endif /* __X86_KDEBUG_H__ */
