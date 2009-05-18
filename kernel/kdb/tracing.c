/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     tracing.c
 * Description:   Some sort of tracing facility.
 *                
 * @LICENSE@
 *                
 * $Id: tracing.c,v 1.2 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#if defined(CONFIG_DEBUGGER_NEW_KDB)

#include "kdebug_keys.h"
#include "kdebug.h"

//the length of the buffers for ipc and pf tracing
#define TRACE_BUFF_SIZE 100
//the length of the restriction arrays
#define RESTR_LENGTH 20        
//the length of the trace buffer
#define TRACE_SIZE 718 /* 57 bytes per element, 10 pages used, 718 elementes */


extern char*    get_name(dword_t nr, char* res);
extern int      pf_usermode(dword_t errcode);
extern dword_t* change_idt_entry(int interrupt_no, ptr_t new_handler);
extern dword_t* change_sysenter_eip_msr(ptr_t new_handler);
extern dword_t to_hex(char* name);

//the interrupt assembler stubs
extern "C" void trace_pf();
extern "C" void trace_ipc();
extern "C" void trace_exception();
#if 0
//pmc overflow
extern "C" void apic_int_59();
#endif
//the interrupt assembler stubs for exception monitoring
extern "C" void monitor_int_0();
extern "C" void monitor_int_4();
extern "C" void monitor_int_5();
extern "C" void monitor_int_6();
extern "C" void monitor_int_7();
extern "C" void monitor_int_8();
extern "C" void monitor_int_9();
extern "C" void monitor_int_10();
extern "C" void monitor_int_11();
extern "C" void monitor_int_12();
extern "C" void monitor_int_16();
extern "C" void monitor_int_17();
extern "C" void monitor_int_18();
extern "C" void monitor_int_19();


int is_restricted(dword_t tid, dword_t errcode, char type);


struct {

    struct {
	unsigned use           : 1; //turn on/off ipc tracing
	unsigned to_do         : 1; //enter debugger after every event
	unsigned restr         : 1; //which thread is restricted
	unsigned only_sendpart : 1; //only threads with sendpart
    } ipc;

    struct {
	unsigned use           : 1;
	unsigned to_do         : 1;
	unsigned restr         : 1;
    } pf;

    struct {
	unsigned use           : 1;
	unsigned to_do         : 1;
	unsigned restr         : 1;
	unsigned with_errcode  : 1; //only exceptions with errorcode in intervall are restricted
    } exception;

} trace_controlling = { {NO, DISPLAY, THREADS_IN_LIST, NO}, //ipc
			{NO, DISPLAY, THREADS_IN_LIST},     //pf
			{NO, DISPLAY, THREADS_IN_LIST, NO}  //exception
};


struct trace_element{
    trace_element *prev, *next;
    char type;
    union {
	struct {
	    dword_t errcode; /* to decide if kernel or user pf */
	    dword_t tid;
	    dword_t fault_addr;
	    dword_t ip;
	    dword_t pager;
	    dword_t pg_table;
	    dword_t time;
	    dword_t pmc0;
	    dword_t pmc1;
	    dword_t cpu_id;
	} pf;

	struct {
	    dword_t sender;
	    dword_t receiver;
	    dword_t snd_desc;
	    dword_t rcv_desc;
	    dword_t msg_w0;
	    dword_t msg_w1;
	    dword_t msg_w2;
	    dword_t ip;
	    dword_t timeout;
	    dword_t time;
	    dword_t pmc0;
	    dword_t pmc1;
	} ipc;

	struct {
	    dword_t vector_no;
	    dword_t fault_addr;
	    dword_t errcode;
	    dword_t time;
	    dword_t cpu_id;
	    dword_t pmc0;
	    dword_t pmc1;
	} exception;

	dword_t raw[12];
    };//57 Bytes
}trace_buffer[TRACE_SIZE]; /* 57 bytes per element, 10 pages used, 718 elementes */

static trace_element* current_trace_pos;
static trace_element* start_trace_pos;


//the number of lines on screen (needed for dump mem? /trace ...)
int num_lines;

ptr_t original_int_14; //the original address of the interrupthandler for int 14

ptr_t original_int_48; //the original address of the interrupthandler for int 48


//used for exception monitoring
struct {
    unsigned int_used:1;
} int_list[IDT_SIZE];

extern "C" struct {
    dword_t tid;
    char name[9];
} name_tab[32];   


dword_t* original_interrupt[IDT_SIZE];


void init_tracing() {
  num_lines = 18;


  original_int_14 = 0;
  original_int_48 = 0;
  for (int i=0; i<IDT_SIZE; i++) 
    int_list[i].int_used = 0;

  /* initialisation of the trace_buffer */
  trace_buffer[0].prev = (&trace_buffer[TRACE_SIZE-1]);
  trace_buffer[0].next = (&trace_buffer[1]);
  trace_buffer[0].type = 'u';

  for (int i=1; i<TRACE_SIZE-1; i++) {
    trace_buffer[i].next = (&trace_buffer[i+1]);
    trace_buffer[i].prev = (&trace_buffer[i-1]);
    trace_buffer[i].type = 'u';
  }

  trace_buffer[TRACE_SIZE - 1].prev = (&trace_buffer[TRACE_SIZE - 2]);
  trace_buffer[TRACE_SIZE - 1].next = (&trace_buffer[0]);
  trace_buffer[TRACE_SIZE - 1].type = 'u';

  start_trace_pos = (&trace_buffer[0]);
  current_trace_pos = (&trace_buffer[0]);
}

//holds either a list of threads not to show or a list of threads to show (t/T)
union restriction {
    struct {
	
	//contains the tids of the restricted tids for pf
	dword_t restr[RESTR_LENGTH];
	
	//array for the intervall
	struct {
	    dword_t begin, end;
	} restr_intervall;
    }r;
    dword_t restr_except[RESTR_LENGTH];
}restricted_pf, restricted_ipc, restricted_exceptions;

void dump_restr(char type) {
    
    union restriction* list;
    int mode = 0;
    char tmp[9];
    list = NULL;
    
    switch (type) {

    case K_KERNEL_IPC_TRACE:
	if (trace_controlling.ipc.use == YES) {
	    printf("restricted threads for ipc tracing: ");
	    list = &restricted_ipc;
	    mode = trace_controlling.ipc.restr;
	}
	else {
	    printf("ipc tracing is off\n");
	}
	break;

    case K_MEM_PF_TRACE:
	if (trace_controlling.pf.use == YES) {
	    printf("restricted threads for pf tracing: ");
	    list = &restricted_pf;
	}
	else {
	    printf("pf tracing is off\n");
	}
	break;

    case K_KERNEL_EXCPTN_TRACE:
	if (trace_controlling.exception.use == YES) {
	    printf("restricted threads for exception monitoring: ");
	    list = &restricted_exceptions;
	}
	else {
	    printf("exception tracing is off\n");
	}
	break;
    }

    if (mode == THREADS_IN_LIST) { //the treads in the list are restricted
	putc('\n');

	for (int i=0; i<RESTR_LENGTH; i++) 
	    if (list->r.restr[i]) printf("%s\n", get_name(list->r.restr[i], tmp)); 
	
	if (list->r.restr_intervall.begin || list->r.restr_intervall.end)
	    printf("[%s,%s]\n", get_name(list->r.restr_intervall.begin, tmp),
		   get_name(list->r.restr_intervall.end, tmp));
    }
    else { //all threads except the ones in the list are restricted
	printf("all except: \n");
	for (int i=0; i<RESTR_LENGTH; i++) 
	    if (list->restr_except[i]) printf("%s\n", get_name(list->restr_except[i], tmp));
    }
}


void del_restr(char type) {
    union restriction* list;
    int mode = 0;
    list = NULL;
    
    switch (type) {
    case K_KERNEL_IPC_TRACE:
	list = &restricted_ipc;
	mode = trace_controlling.ipc.restr;
	break;

    case K_MEM_PF_TRACE:
	list = &restricted_pf;
	mode = trace_controlling.pf.restr;

    break;
    case K_KERNEL_EXCPTN_TRACE:
	list = &restricted_exceptions;
	mode = trace_controlling.exception.restr;
    }
    
    for (int i=0; i<RESTR_LENGTH; i++) {
	list->r.restr[i] = 0;
	list->restr_except[i] = 0;
    }
    
    list->r.restr_intervall.begin = 0;
    list->r.restr_intervall.end = 0;
}

void trace(char type, dword_t args[12]) {
    current_trace_pos->type = type;
    for (int i=0; i<11; i++) {
	current_trace_pos->raw[i] = args[i];
    }

    current_trace_pos =  current_trace_pos->next;
    
    if (current_trace_pos == start_trace_pos) {
	start_trace_pos = start_trace_pos->next;
    }
    //current and start are the same after an overflow!
    //the oldest entry is "deleted" in this case.
}


static void dump_trace_element(trace_element *tmp) {

    switch (tmp->type) {
	case 'e':
	    printf("exception %x @ %x errcode %x cpu %d time %d pmc0 %x pmc1 %x\n",
		   tmp->exception.vector_no, tmp->exception.errcode, tmp->exception.errcode,
		   tmp->exception.cpu_id, tmp->exception.time, tmp->exception.pmc0,
		   tmp->exception.pmc1);
	    break;
	    
	case 'i':
	    printf("ipc: %x->%x snd_desc %x rcv_desc %x (%x,%x,%x)"
		   "ip %x timeouts %x time %d pmc0 %x pmc1 %x\n",
		   tmp->ipc.sender, tmp->ipc.receiver, tmp->ipc.snd_desc, tmp->ipc.rcv_desc,
		   tmp->ipc.msg_w0, tmp->ipc.msg_w1, tmp->ipc.msg_w2, tmp->ipc.ip, 
		   tmp->ipc.timeout, tmp->ipc.time, tmp->ipc.pmc0, tmp->ipc.pmc1);
	    break;
	    
	case 'p':
	    printf("%s: thread %s @ %s ip %x pager %x errcode %s cr3 %x"
		   "cpu %d time %d pmc0 %x pmc1 %x\n",
		   pf_usermode(tmp->pf.errcode) ? "UPF" : "KPF" , tmp->pf.tid,
		   tmp->pf.fault_addr, tmp->pf.ip, tmp->pf.pager, tmp->pf.errcode,
		   tmp->pf.pg_table, tmp->pf.cpu_id, tmp->pf.time, tmp->pf.pmc0,
		   tmp->pf.pmc1);
	    break;
    }
}
    
// the index is the point, from which the next num_lines entries are shown
void kdebug_dump_trace() {

    printf("Dump Trace: [Exception/Ipc/Pf]\n");///All] ");
    char type;
    type = getc();

    trace_element *tmp;

#if defined(CONFIG_DEBUGGER_IO_OUTSCRN)

    // the last element in the buffer
    tmp = current_trace_pos->prev;

    while (i<NUM_LINES) {
	if (tmp->type == type) {
	    i++;
	}
	tmp = tmp->prev;
    }

//navigation in the buffer is still mising. (Next/Prev)
    for (int i=0; i<NUM_LINES; i++) {
	if (tmp->type == type) {
	    if ( is_restricted(tmp->raw[0], tmp->raw[3], tmp->type)) {
		dump_trace_element(tmp);
	    }
	    tmp = tmp->next;
	}
    }

#else /* CONFIG_DEBUGGER_OUT_IO_OUTCOM */

    tmp = start_trace_pos;

    if (tmp->type != 'u') {
	// there is somthing in the buffer
	do {
	    if (tmp->type == type) {
		if ( is_restricted(tmp->raw[0], tmp->raw[3], tmp->type)) {
		    dump_trace_element(tmp);
		}
	    }
	    tmp = tmp->next;
	} while (tmp != current_trace_pos);
    }
    else {
	printf("The buffer is empty.\n");
    }
#endif    
}


/**
 * tests, if a thread is restricted or not
 **/
int is_restricted(dword_t tid, dword_t errcode, char type) {

    union restriction* list;
    int restr = 0;
    dword_t search = 0;//to decide if a thread or an errorcode is restricted
    dword_t no_send = 0;//only for ipc tracing: has this ipc a send part?
    no_send = 0;//if not ipc tracing send is always set, so that 
    list = NULL;
    
    switch (type) {
    case K_KERNEL_IPC_TRACE:
	list = &restricted_ipc;
	restr = trace_controlling.ipc.restr;
	search = tid;
	if (trace_controlling.ipc.only_sendpart == YES)//trace ipc is restrictet to ipcs with send part
	    no_send = (errcode == 0xffffffff) ? 1 : 0;
	break;
    case K_MEM_PF_TRACE:
	list = &restricted_pf;
	restr = trace_controlling.pf.restr;
	search = tid;
	break;
    case K_KERNEL_EXCPTN_TRACE:
	list = &restricted_exceptions;
	restr = trace_controlling.exception.restr;
	if (!trace_controlling.exception.with_errcode) {
	    search = tid;
	}
	else {
	    search = errcode;
	}
	break;
    }
    
    if (restr == THREADS_IN_LIST) { //the treads in the list are restricted
	for (int i=0; i<RESTR_LENGTH; i++) {
	    if (search == list->r.restr[i] || no_send) //the thread is restricted
		//send checks if there is a send part in the ipc. if pf or exception
		//send is always clear, so nothing happens to it
		return 1;
	}
    //begin = end is not permitted
	if (list->r.restr_intervall.begin < list->r.restr_intervall.end) 
	    //the thread no. within the intervall are restricted
	    if ((search >= list->r.restr_intervall.begin
		 || search <= list->r.restr_intervall.end)
		&& (list->r.restr_intervall.begin != list->r.restr_intervall.end
		    || no_send))
		//the last part checks if something is in the array or not 
		return 1;
	    else {//the thread no. outside the intervall are restricted
		if ((search <= list->r.restr_intervall.begin 
		     || search >= list->r.restr_intervall.end)
		    && (list->r.restr_intervall.begin != list->r.restr_intervall.end
			|| no_send))
		    //the last part checks if something is in the array or not 
		    return 1;
	    }
	//the tid is not in the list
	return 0;
    }
    else { //all threads except the ones in the list are restricted
	for (int i=0; i<RESTR_LENGTH; i++) 
	    if (search == list->restr_except[i] || no_send) return 0;
	    else return 1;
    }
    return 0; //is never reached
}

/**
 * decides what to do and makes the neccesarry changes in the idt
 * be carefull what you are doing with the idt!
 **/
void kdebug_pagefault_tracing() {
    char c;

    printf("pagefault tracing: ");

    int i = 0;
    c = getc();    
    switch (c) {
    case '+':
	putc(c);
	putc('\n');
	if (! trace_controlling.pf.use) {
	    original_int_14 = change_idt_entry(14, (ptr_t)(*trace_pf));
	    trace_controlling.pf.use = YES;
	}
	trace_controlling.pf.to_do = DISPLAY;
	break;
	
    case '*':
	putc(c);
	putc('\n');
	if (! trace_controlling.pf.use) {
	    original_int_14 = change_idt_entry(14, (ptr_t)(*trace_pf));
	    trace_controlling.pf.use = YES;
	}
	trace_controlling.pf.to_do = TRACING;
	break;
	
    case '-':
	putc(c);
	putc('\n');
	ptr_t dummy;
	if (trace_controlling.pf.use) {
	    dummy = change_idt_entry(14, original_int_14);
	    trace_controlling.pf.use = NO;
	}
	break;
	
    case K_RESTRICT_TRACE:
	printf("restirct: t/T/x/- : ");
	c = getc();
	
	switch (c) {
	case 't'://threads in list
	    printf("thread: ");
	    if (trace_controlling.pf.restr == ALL_OTHER_THREADS) { 
		del_restr(K_MEM_PF_TRACE);//for pf
		trace_controlling.pf.restr = THREADS_IN_LIST;
	    }
	    i = 0;
	    while (restricted_pf.r.restr[i] && i<RESTR_LENGTH) {
		if (i < RESTR_LENGTH)i++;//find next free element
		else {
		    printf("array is full. begining from 0 again\n");
		    i = 0;
		}
	    }
	    restricted_pf.r.restr[i] = kdebug_get_hex(0xffffffff, "invalid");
	    break;
	    
	case 'T':
	    printf("all except: ");
	    if (trace_controlling.pf.restr == THREADS_IN_LIST) {
		del_restr(K_MEM_PF_TRACE);//for pf
		trace_controlling.pf.restr = ALL_OTHER_THREADS;
	    }
	    i = 0;
	    while (restricted_pf.restr_except[i] && i<RESTR_LENGTH) {
		if (i < RESTR_LENGTH)i++;//find next free element
		else {
		    printf("array is full. begining from 0 again\n");
		    i = 0;
		}
	    }
	    restricted_pf.restr_except[i] = kdebug_get_hex(0xffffffff, "invalid");
	    break;
	    
	case 'x':
	    printf("intervall ");
	    //intervall only in THREADS_IN_LIST
	    if (trace_controlling.pf.restr == ALL_OTHER_THREADS) {
		del_restr(K_MEM_PF_TRACE);//for pf
		trace_controlling.pf.restr = THREADS_IN_LIST;
	    }
	    putc('[');
	    restricted_pf.r.restr_intervall.begin = kdebug_get_hex(0xffffffff, "invalid");
	    putc(',');
	    restricted_pf.r.restr_intervall.end = kdebug_get_hex(0xffffffff, "invalid");
	    putc(']');
	    break;
	    
	case '-':
	    putc(c);
	    del_restr(K_MEM_PF_TRACE);//for pf      
	    break;
	}
	break;
    }                       
}


void kdebug_ipc_tracing() {

    char c;
//    ptr_t original_sysenter;

    int i = 0;

    printf("IPC tracing\n");

    c = getc();

    switch (c) {
    case '+':
	putc(c);
	putc('\n');
	
	if (trace_controlling.ipc.use == NO) {
	    original_int_48 = change_idt_entry(48, (ptr_t)(*trace_ipc));
#if 0
//# if defined(CONFIG_ARCH_X86_I686)
	    original_sysenter = change_sysenter_eip_msr((ptr_t)(*trace_ipc));
#endif
	    trace_controlling.ipc.use = YES;
	}
    trace_controlling.ipc.to_do = DISPLAY;
    break;
    
    case '*':
	putc(c);
	putc('\n');
	
	if (trace_controlling.ipc.use == NO) {
	    original_int_48 = change_idt_entry(48, (ptr_t)(*trace_ipc));
#if 0
//# if defined(CONFIG_ARCH_X86_I686)
	    original_sysenter = change_sysenter_eip_msr((ptr_t)(*trace_ipc));
#endif
	    trace_controlling.ipc.use = YES;
	}
	trace_controlling.ipc.to_do = TRACING;
	break;
	
    case '-':
	putc(c);
	putc('\n');
	
	ptr_t dummy;
	if (trace_controlling.ipc.use == YES) {
	    dummy = change_idt_entry(48, original_int_48);
#if 0
//#if defined(CONFIG_ARCH_X86_I686)
	    dummy = change_sysenter_eip_msr(original_sysenter);
#endif
	    trace_controlling.ipc.use = NO;
	}
	break;
	
    case 'r':
	printf("restirct: t/s/T/x/- : ");
	c = getc();
	
	switch (c) {
	case 't':
	    printf("thread: ");
	    if (trace_controlling.ipc.restr == ALL_OTHER_THREADS) {
		del_restr(K_KERNEL_IPC_TRACE);//for ipc
		trace_controlling.ipc.restr = THREADS_IN_LIST;
	    }
	    i = 0;
	    while (restricted_ipc.r.restr[i] && i<RESTR_LENGTH) {
		if (i < RESTR_LENGTH)i++;//find next free element
		else {
		    printf("array is full. begining from 0 again\n");
		    i = 0;
		}
	    }
	    restricted_ipc.r.restr[i] = kdebug_get_hex(0xffffffff, "invalid");
	    break;
	    
	case 's':
	    printf("trace only ipc with send part\n");
	    trace_controlling.ipc.only_sendpart = YES;
	    break;
	    
	case 'T':
	    printf("all except: ");
	    if (trace_controlling.ipc.restr == THREADS_IN_LIST) {
		del_restr(K_KERNEL_IPC_TRACE);//for ipc
		trace_controlling.ipc.restr = ALL_OTHER_THREADS;
	    }
	    i = 0;
	    while (restricted_ipc.restr_except[i] && i<RESTR_LENGTH) {
		if (i < RESTR_LENGTH)i++;//find next free element
		else {
		    printf("array is full. begining from 0 again\n");
		    i = 0;
		}
	    }
	    restricted_ipc.restr_except[i] = kdebug_get_hex(0xffffffff, "invalid");
	    break;
	    
	case 'x':
	    printf("intervall ");
	    if (trace_controlling.ipc.restr == ALL_OTHER_THREADS) {
		del_restr(K_KERNEL_IPC_TRACE);//for ipc
		trace_controlling.ipc.restr = THREADS_IN_LIST;
	    }
	    
	    putc('[');
	    restricted_ipc.r.restr_intervall.begin = kdebug_get_hex(0xffffffff, "invalid");
	    putc(',');
	    restricted_ipc.r.restr_intervall.end = kdebug_get_hex(0xffffffff, "invalid");
	    putc(']');
	    
	    break;
	    
	case '-':
	    putc(c);
	    del_restr(K_KERNEL_IPC_TRACE);//for ipc
	    break;
	}
	break;
    }
}

//checks if this exception is in the list...
static int test_exception(int nr) {
    
    int yes = 0;
    switch (nr) {
    case 0:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_0));
	break;
    case 4:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_4));
    case 5:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_5));
    case 6:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_6));
    case 7:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_7));
    case 8:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_8));
    case 9:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_9));
    case 10:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_10));
    case 11:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_11));
    case 12:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_12));
    case 16:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_16));
    case 17:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_17));
    case 18:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_18));
    case 19:
	yes = 1;
	int_list[nr].int_used = 1;
	original_interrupt[nr] = change_idt_entry(nr, (ptr_t)(*monitor_int_19));
	break;
    }
    return yes;
}

void kdebug_exception_tracing() {
    
    char c;
    int i = 0;
    int do_something = 0;

    printf("Exception tracing:\n");
    c = getc();
    
    switch (c) {
    case '+': {
	putc(c);
	putc('\n');
	
	char tmp[3];
	tmp[0] = getc();
	tmp[1] = getc();
	tmp[2] = '\0';
	
	int nr = (int) to_hex(tmp);
	
	if (!int_list[nr].int_used) {
	    //is this exception in the list of monitored exceptions?
	    do_something = test_exception(nr);    
	}
	if (do_something)
	    trace_controlling.exception.to_do = DISPLAY;
    }
    break;
    
    case '*': {
	putc(c);
	putc('\n');
	for (int i=0; i<IDT_SIZE; i++) {
	    if (!int_list[i].int_used) {
		do_something = test_exception(i);
	    }
	}
	if (do_something)
	    trace_controlling.exception.to_do = TRACING;
    }
    break;
    
    case '-': {//dosn't matter if in the list or not
	putc(c);
	putc('\n');
	
	dword_t* dummy;
	for (int i=0; i<IDT_SIZE; i++) {
	    if (int_list[i].int_used) {
		int_list[i].int_used = 0;
		dummy = change_idt_entry(i, (ptr_t) original_interrupt[i]);
	    }
	}
    }
    break;
    
    case 'r':
	
	printf("restirct: t/T/x/- : ");
	c = getc();
	
	switch (c) {
	case 't': {
	    printf("thread: ");
	    if (trace_controlling.exception.restr == ALL_OTHER_THREADS) {
		del_restr(K_KERNEL_EXCPTN_TRACE);//for exceptions
		trace_controlling.exception.restr = THREADS_IN_LIST;
	    }
	    i = 0;
	    while (restricted_exceptions.r.restr[i] && i<RESTR_LENGTH) {
		if (i < RESTR_LENGTH)i++;//find next free element
		else {
		    printf("array is full. begining from 0 again\n");
		    i = 0;
		}
	    }
	    restricted_exceptions.r.restr[i] = kdebug_get_hex(0xffffffff, "invalid");
	    trace_controlling.exception.use = YES;
	}
	break;
	
	case 'T':
	    printf("all except: ");
	    if (trace_controlling.exception.restr == THREADS_IN_LIST) {
		del_restr(K_KERNEL_EXCPTN_TRACE);//for exceptions
		trace_controlling.exception.restr = ALL_OTHER_THREADS;
	    }
	    i = 0;
	    while (restricted_exceptions.restr_except[i] && i<RESTR_LENGTH) {
		if (i < RESTR_LENGTH)i++;//find next free element
		else {
		    printf("array is full. begining from 0 again\n");
		    i = 0;
		}
	    }
	    restricted_exceptions.restr_except[i] = kdebug_get_hex(0xffffffff, "invalid");
	    trace_controlling.exception.use = YES;
	    break;

	case 'x':
	    printf("intervall ");
	    if (trace_controlling.exception.restr == ALL_OTHER_THREADS) {
		del_restr(K_KERNEL_EXCPTN_TRACE);//for exception
		trace_controlling.exception.restr = THREADS_IN_LIST;
	    }
	    
	    putc('[');
	    restricted_exceptions.r.restr_intervall.begin = kdebug_get_hex(0xffffffff, "invalid");
	    putc(',');
	    restricted_exceptions.r.restr_intervall.end = kdebug_get_hex(0xffffffff, "invalid");
	    putc(']');
	    trace_controlling.exception.use = YES;
	    trace_controlling.exception.with_errcode = YES;
	    break;
	    
	case '-':
	    putc(c);
	    del_restr(K_KERNEL_EXCPTN_TRACE);//for exception
 	    trace_controlling.exception.use = NO;
 	    trace_controlling.exception.with_errcode = NO;
	    break;
	}
	break;
    }
}


#endif /* defined(CONFIG_DEBUGGER_NEW_KDB) */











