/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     interrupt.c
 * Description:   Entry point hooks for new kernel debugger.
 *                
 * @LICENSE@
 *                
 * $Id: interrupt.c,v 1.2 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#if defined(CONFIG_DEBUGGER_NEW_KDB)

#include "kdebug_keys.h"
#include "kdebug.h"

extern dword_t get_pf_address();
extern int     pf_usermode(dword_t errcode);
extern dword_t rd_perfctr(int ctrsel);
extern dword_t rd_tsc();
extern int     is_restricted(dword_t tid, dword_t errcode, char type);
extern void    trace(char type, dword_t args[10]);
extern dword_t get_user_time();
ptr_t return_to;/*the address of the original handler after the
                    exception monitoring*/
extern dword_t* original_interrupt[IDT_SIZE];

extern struct {

    struct {
	unsigned use : 1;
	unsigned to_do : 1;
	unsigned restr : 1;
	unsigned only_sendpart : 1;
    } ipc;

    struct {
	unsigned use : 1;
	unsigned to_do : 1;
	unsigned restr : 1;
    } pf;

    struct {
	unsigned use : 1;
	unsigned to_do : 1;
	unsigned restr : 1;
	unsigned with_errcode : 1;
    } exception;

} trace_controlling;

// the handler for pf_tracing
//this handler is never reached when pf_trace_mode == 1 (no tracing)
void trace_pf_handler(dword_t eax, dword_t ecx, dword_t edx, dword_t errcode, dword_t ip) {
        
    tcb_t * current = get_current_tcb();
    dword_t fault = (get_pf_address()); //the faultaddress
    
    //is the faulting thread restricted? if so, do nothing
    //1 for restricted for pf
    if (is_restricted(current->myself.raw, 0, K_MEM_PF_TRACE)) return;
    
    if (trace_controlling.pf.to_do == DISPLAY) {    
	printf("%s:  thread: %p @ %x ip: %x, pager: %p\n"
	       "          err: %x, cr3: %x\n", 
	       pf_usermode(errcode) ? "UPF" : "KPF", current->myself.raw, fault, ip, current->pager.raw,
	       errcode, get_current_pgtable());
    
	enter_kdebug("PF");
    }
    else {
	dword_t pmc_0, pmc_1, tsc;
	qword_t dummy;

	dummy = rdpmc(0);
	pmc_0 = ((dword_t*)dummy)[0];
	dummy = rdpmc(1);
	pmc_1 = ((dword_t*)dummy)[0];
	dummy = rdtsc();
	tsc = ((dword_t*)dummy)[0];

	dword_t args[10] = {
	    errcode, current->myself.raw, fault, ip,
	    current->pager.raw, (dword_t)&(* get_current_pgtable()),
	    tsc,
#if defined(CONFIG_ARCH_X86_I686)
	    pmc_0, pmc_1,
#else 
	    0, 0,
#endif
#if defined(CONFIG_SMP)
	    cpu_id
#endif
	};
	trace('p', args);
    }  
}


// the handler for ipc_tracing
//this handler is never reached when ipc_trace_mode == 1 (no tracing)
void trace_ipc_handler(dword_t msg_w2, dword_t dest, dword_t rcv_desc, dword_t msg_w1, dword_t msg_w0, dword_t timeouts, dword_t snd_desc) {
    
    tcb_t * current = get_current_tcb();

    //is the faulting thread restricted? if so, do nothing
    //0 : restricted for ipc
    if (is_restricted(current->myself.raw, snd_desc, K_KERNEL_IPC_TRACE)) return;
    
    if (trace_controlling.ipc.to_do == DISPLAY){    
	printf("ipc: %p -> %p, snd_desc: %x rcv_desc: %x \n"
	       "(%x %x %x) - ip = %x\n",
	       current->myself.raw, dest, snd_desc, rcv_desc,
	       msg_w0, msg_w1, msg_w2, get_user_ip(current));
	
	enter_kdebug("IPC");
    }
    else {
	dword_t pmc_0, pmc_1, tsc;
	qword_t dummy;

	dummy = rdpmc(0);
	pmc_0 = ((dword_t*)dummy)[0];
	dummy = rdpmc(1);
	pmc_1 = ((dword_t*)dummy)[0];
	dummy = rdtsc();
	tsc = ((dword_t*)dummy)[0];

	dword_t args[12] = {
	    current->myself.raw, dest, snd_desc, rcv_desc, msg_w0,
	    msg_w1, msg_w2, get_user_ip(current), timeouts, tsc
#if defined(CONFIG_ARCH_X86_I686)
	    , pmc_0, pmc_1,
#endif
	};
	trace('i', args);
    }
}

//the handler for exception monitoring
void trace_exception_handler(exception_frame_t* frame) {

    dword_t vector_no, errcode, fault_address;

    vector_no = frame->fault_code;
    errcode = frame->error_code;
    fault_address = frame->fault_address;
    
    if (is_restricted(vector_no, errcode, K_KERNEL_EXCPTN_TRACE)) 
	return;

    if (trace_controlling.exception.to_do == DISPLAY){
	printf("exception %x @ %x error code %x\n", vector_no, fault_address, errcode);
	
	
	enter_kdebug();
    }
    else {
	dword_t pmc_0, pmc_1, tsc;
	qword_t dummy;

	dummy = rdpmc(0);
	pmc_0 = ((dword_t*)dummy)[0];
	dummy = rdpmc(1);
	pmc_1 = ((dword_t*)dummy)[0];
	dummy = rdtsc();
	tsc = ((dword_t*)dummy)[0];

	dword_t args[11] = { 
	    vector_no, fault_address, errcode, tsc,
#if defined(CONFIG_SMP)
	    cpu_id,
#else
	    0,
#endif
#if defined(CONFIG_ARCH_X86_I686)
	    pmc_0, pmc_1, 
#endif
	    0, 0, 0
	};
	trace('e', args);
    }
    
    //the address of the real handler
    return_to = original_interrupt[vector_no];
}

//a handler for int 59 (the performancecounter overflow)
void pmc_overflow_handler() {

    dword_t pmc_0, pmc_1, tsc;
    qword_t dummy;
    
    dummy = rdpmc(0);
    pmc_0 = ((dword_t*)dummy)[0];
    dummy = rdpmc(1);
    pmc_1 = ((dword_t*)dummy)[0];
    dummy = rdtsc();
    tsc = ((dword_t*)dummy)[0];
    
    printf("pmc0: %x, pmc1: %x tsp: %x\n", pmc_0, pmc_1, tsc);
    enter_kdebug("couter_overflow");
}
 
#endif /*defined(CONFIG_DEBUGGER_NEW_KDB) */
