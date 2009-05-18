/*********************************************************************
 *                
 * Copyright (C) 2000-2003,  Karlsruhe University
 *                
 * File path:     l4/helpers.h
 * Description:   helper functions making live with l4 easier
 *                
 * @LICENSE@
 *                
 * $Id: helpers.h,v 1.10 2003/02/19 15:55:54 sgoetz Exp $
 *                
 ********************************************************************/

#ifndef __L4_HELPERS_H__
#define __L4_HELPERS_H__

#define M_BITS  8 

#define is_good_m(x) ((x) < (1 << M_BITS)) 

#define best_m1(x)      (x) 
#define best_m2(x)      (is_good_m(x) ? x : best_m1(x>>2)) 
#define best_m3(x)      (is_good_m(x) ? x : best_m2(x>>2)) 
#define best_m4(x)      (is_good_m(x) ? x : best_m3(x>>2)) 
#define best_m5(x)      (is_good_m(x) ? x : best_m4(x>>2)) 
#define best_m6(x)      (is_good_m(x) ? x : best_m5(x>>2)) 
#define best_m7(x)      (is_good_m(x) ? x : best_m6(x>>2)) 
#define best_m8(x)      (is_good_m(x) ? x : best_m7(x>>2)) 
#define best_m9(x)      (is_good_m(x) ? x : best_m8(x>>2)) 
#define best_m10(x)     (is_good_m(x) ? x : best_m9(x>>2)) 
#define best_m11(x)     (is_good_m(x) ? x : best_m10(x>>2)) 
#define best_m12(x)     (is_good_m(x) ? x : best_m11(x>>2)) 
#define best_m13(x)     (is_good_m(x) ? x : best_m12(x>>2)) 
#define best_m14(x)     (is_good_m(x) ? x : best_m13(x>>2)) 
#define best_mant(x)    (is_good_m(x) ? x : best_m14(x>>2)) 

#define best_e1(x)      (1) 
#define best_e2(x)      (is_good_m(x) ?  2 : best_e1(x>>2)) 
#define best_e3(x)      (is_good_m(x) ?  3 : best_e2(x>>2)) 
#define best_e4(x)      (is_good_m(x) ?  4 : best_e3(x>>2)) 
#define best_e5(x)      (is_good_m(x) ?  5 : best_e4(x>>2)) 
#define best_e6(x)      (is_good_m(x) ?  6 : best_e5(x>>2)) 
#define best_e7(x)      (is_good_m(x) ?  7 : best_e6(x>>2)) 
#define best_e8(x)      (is_good_m(x) ?  8 : best_e7(x>>2)) 
#define best_e9(x)      (is_good_m(x) ?  9 : best_e8(x>>2)) 
#define best_e10(x)     (is_good_m(x) ? 10 : best_e9(x>>2)) 
#define best_e11(x)     (is_good_m(x) ? 11 : best_e10(x>>2)) 
#define best_e12(x)     (is_good_m(x) ? 12 : best_e11(x>>2)) 
#define best_e13(x)     (is_good_m(x) ? 13 : best_e12(x>>2)) 
#define best_e14(x)     (is_good_m(x) ? 14 : best_e13(x>>2)) 
#define best_exp(x)     (is_good_m(x) ? 15 : best_e14(x>>2)) 
  

#define l4_time(ms) best_mant(ms), best_exp(ms)
  

#define mus(x)          (x) 
#define mills(x)        (1000*mus(x)) 
#define secs(x)         (1000*mills(x)) 
#define mins(x)         (60*secs(x)) 
#define hours(x)        (60*mins(x)) 



L4_INLINE 
void create_thread(int id, void *ip, void *stack, l4_threadid_t pager)
{
    l4_threadid_t tid, preempter;
    dword_t flags;
    tid.id.thread = id;

    l4_thread_ex_regs(tid, (dword_t)ip, (dword_t)stack, 
#if defined(CONFIG_VERSION_X0)
		      &preempter,
#endif
		      &pager, (dword_t*) &flags,
		      (dword_t *) &ip, (dword_t * )&stack);
}

L4_INLINE
l4_threadid_t get_current_pager(l4_threadid_t myself)
{
    l4_threadid_t preempter = L4_INVALID_ID, pager = L4_INVALID_ID;
    dword_t dummy;

    if (l4_is_nil_id(myself))
	myself = l4_myself();
    l4_thread_ex_regs(myself, ~0, ~0,
#if defined(CONFIG_VERSION_X0)
		      &preempter,
#endif
		      &pager, &dummy, &dummy, &dummy);

    return pager;
}

L4_INLINE
int associate_interrupt(int nr)
{
    dword_t dummy;
    l4_msgdope_t result;
    
    return l4_ipc_receive(L4_INTERRUPT(nr), 0, &dummy, &dummy, &dummy,
	L4_IPC_TIMEOUT_NULL, &result);
}

L4_INLINE
void l4_sleep(dword_t us)
{
    dword_t dummy;
    l4_msgdope_t result;

    l4_ipc_receive(L4_NIL_ID, 0, &dummy, &dummy, &dummy,
		   us ? (l4_timeout_t) {timeout : { best_exp(us), best_exp(us), 0, 0, best_mant(us), best_mant(us)}} : L4_IPC_NEVER, &result);
}

/* xxx: hack for L4-KA -- ecx return's the current cpu on smp systems */
#if defined(CONFIG_ARCH_X86)
L4_INLINE dword_t get_current_cpu(void)
{
    dword_t cpu;
    __asm__ __volatile__("pushl	%%ebp		\n"
			 "xorl	%%esi, %%esi	\n"
			 "int	$0x31		\n"
			 "popl	%%ebp		\n"
			 : "=c"(cpu)
			 :
			 : "eax", "ebx", "edx", "esi", "edi");
    return cpu;
}
#else
#define get_current_cpu() (0)
#endif

L4_INLINE void l4_migrate_thread(l4_threadid_t tid, dword_t cpu)
{
    l4_sched_param_t schedparam;
    l4_threadid_t foo_id = L4_INVALID_ID;
    l4_thread_schedule(tid, (l4_sched_param_t) {sched_param:0xFFFFFFFF}, &foo_id, &foo_id, &schedparam);
    schedparam.sp.small = 0;
    schedparam.sp.zero = cpu + 1;
    foo_id = L4_INVALID_ID;
    l4_thread_schedule(tid, schedparam, &foo_id, &foo_id, &schedparam);
}

L4_INLINE void l4_set_prio(l4_threadid_t tid, dword_t prio)
{
    l4_sched_param_t schedparam;
    l4_threadid_t foo_id = L4_INVALID_ID;
    l4_thread_schedule(tid, (l4_sched_param_t) {sched_param:0xFFFFFFFF}, &foo_id, &foo_id, &schedparam);
    schedparam.sp.small = 0;
    schedparam.sp.zero = 0;
    schedparam.sp.prio = prio;
    foo_id = L4_INVALID_ID;
    l4_thread_schedule(tid, schedparam, &foo_id, &foo_id, &schedparam);
}

L4_INLINE dword_t l4_make_small_id(dword_t size, dword_t num)
{
    return ((num << 1) | 1) << (size - 1);
}

L4_INLINE void l4_set_small(l4_threadid_t tid, dword_t small)
{
    l4_sched_param_t schedparam;
    l4_threadid_t foo_id = L4_INVALID_ID;

    l4_thread_schedule(tid, (l4_sched_param_t) {sched_param:0xFFFFFFFF},
		       &foo_id, &foo_id, &schedparam);

    schedparam.sp.small = small;
    schedparam.sp.zero = 0;
    foo_id = L4_INVALID_ID;

    l4_thread_schedule(tid, schedparam, &foo_id, &foo_id, &schedparam);
}


#endif /* __L4_HELPERS_H__ */
