/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     x86_profiling.c
 * Description:   Random sampling profiling support for x86.
 *                
 * @LICENSE@
 *                
 * $Id: x86_profiling.c,v 1.16 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

#if defined(CONFIG_ENABLE_PROFILING)

#include <x86/apic.h>
#include <x86/init.h>
#include "kdebug.h"


#define PROF_MIN_SAMPLE_PERIOD	1000
#define PROF_DEF_SAMPLE_PERIOD	4096
#define PROF_LOG_WINDOW_SIZE	(4096*128)
#define PROF_LOG_MAX		(PROF_LOG_WINDOW_SIZE/sizeof(prof_event_t))
#define PROF_DEF_LOW_DUMPLIMIT	10
#define PROF_FREQ_COUNTERS	8
#define PROF_NUM_VALUES		CONFIG_PROFILING_MAX_VAL

#define PROF_LOG_NONE		(0)
#define PROF_LOG_USER		(1)
#define PROF_LOG_KERNEL		(2)
#define PROF_LOG_BOTH		(PROF_LOG_USER | PROF_LOG_KERNEL)



typedef struct prof_event_t	prof_event_t;
typedef struct prof_freq_t	prof_freq_t;

struct prof_event_t {
    l4_threadid_t	tid;
    dword_t		eip;
    dword_t		value[PROF_NUM_VALUES];
};

struct prof_freq_t {
    float		prob;
    dword_t		counter[PROF_FREQ_COUNTERS];
    dword_t		value[PROF_FREQ_COUNTERS];
};



static int		 prof_do_sampling;
static prof_event_t	*prof_log;
static dword_t		 prof_pos;
static dword_t		 prof_sample_period;
static dword_t		 prof_thread;
static dword_t		 prof_task;
static dword_t		 prof_region_beg;
static dword_t		 prof_region_end;
static dword_t		 prof_trigger_event;
static dword_t		 prof_evsel[2];
static dword_t		 prof_valuetype[PROF_NUM_VALUES];
static dword_t		 prof_restore_single_step;

/* From x86/init.c */
extern idt_desc_t	 idt[];


static void reset_perfctr(void);
static void clear(prof_freq_t *fr);
static void add(prof_freq_t *fr, dword_t val);
static void reduce(prof_freq_t *fr);
static void dump(prof_freq_t *fr);
static void srandom(dword_t seed);
static dword_t random(void);


#define TASK_TCB(x) ((dword_t) (x) & (L4_TCB_MASK << L4_X0_THREADID_BITS))

void kdebug_setperfctr(dword_t ctrsel, dword_t value, int counter_value)
{
    /* Validate counter selector. */
    if ( ctrsel > 1 )
 	return;
    
    asm volatile (
 	"	lea	%c3(%1), %%ecx		\n"
 	"	sub	%%edx, %%edx		\n"
 	"	wrmsr				\n"
 	"	lea	%c4(%1), %%ecx		\n"
 	"	movl	%2, %%eax		\n"
 	"	wrmsr				\n"
	
 	: /* Outputs */
 	"+a"(value)
	
 	: /* Inputs */
 	"r"(ctrsel), "r"(counter_value),
	"i"(IA32_EVENTSEL0), "i"(IA32_PERFCTR0)
	
 	: /* Clobbers */
 	"ecx", "edx");
}


/*
**
** The following pseudo random generator should produce numbers which
** are uniformely distributed between 0 and 0xffffffff.
**
*/

static dword_t random_seed = 1;

static void srandom(dword_t seed)
{
    random_seed = seed;
}

static dword_t random(void)
{
    long result, div, mod;
    
    div = random_seed / 127773;
    mod = random_seed % 127773;
    result = 16807 * mod - 2836 * div;
    if ( result <= 0 )
	result += 0x7fffffff;
    random_seed = result;

    return result;
}



/*
**
** Code for sorting the profiling log.  The log is sorted on ascending
** thread id's, and ascending eip's if thread id's are equal.  Sorting
** the log makes it much easier to parse it afterwards.
**
*/

static inline void swap(prof_event_t *a, prof_event_t *b)
{
    prof_event_t c = *a;
    *a = *b;
    *b = c;
}

static inline int operator == (const prof_event_t a, const prof_event_t b)
{
    return (a.tid.raw == b.tid.raw) && (a.eip == b.eip);
}

static inline int operator < (const prof_event_t a, const prof_event_t b)
{
    return (a.tid.raw < b.tid.raw) || 
	((a.tid.raw == b.tid.raw) && (a.eip < b.eip));
}

static inline int operator <= (const prof_event_t a, const prof_event_t b)
{
    return (a < b) || (a == b);
}

static void kdebug_proflog_sort(void)
{
    dword_t child, parse;
    dword_t max_pos = prof_pos;
    prof_event_t tmp;

    /*
     * The following heaposrt alogorithm assumes that the first
     * element in the array has index 1 and the last element has index
     * max_pos.
     */
    prof_log--;

    /* Create initial heap. */
    for ( dword_t i = max_pos/2; i > 0; i-- )
    {
	for ( parse = i; (child = parse * 2) <= max_pos; parse = child)
	{
	    if ( (child < max_pos) && (prof_log[child] < prof_log[child+1]) )
		child++;

	    if ( prof_log[child] <= prof_log[parse] )
		break;

	    swap(&prof_log[parse], &prof_log[child]);
	}
    }

    while ( max_pos > 1 )
    {
	/* Save largest element into its final slot. */
	tmp = prof_log[max_pos];
	prof_log[max_pos] = prof_log[1];
	max_pos--;

	/* Reheapify. */
	for ( parse = 1; (child = parse * 2) <= max_pos; parse = child )
	{
	    if ( (child < max_pos) && (prof_log[child] < prof_log[child+1]) )
		child++;

	    prof_log[parse] = prof_log[child];
	}
	for (;;)
	{
	    child = parse;
	    parse = child / 2;
	    if ( (child == 1) || (tmp < prof_log[parse]) )
	    {
		prof_log[child] = tmp;
		break;
	    }
	    prof_log[child] = prof_log[parse];
	}
    }

    /* Reset first element to have index 0. */
    prof_log++;

    return;
}




/*
**
** Profile dumping code.
**
*/


void kdebug_profile_dump(tcb_t *current)
{
    static prof_freq_t freq[PROF_NUM_VALUES];
    static x86_fpu_state_t fpu_state;
    l4_threadid_t p_tid = L4_NIL_ID;
    dword_t dthread = 0, dtask = 0;
    dword_t p_eip = ~0UL, eip_cnt = 0;
    dword_t low_limit, region_beg, region_end;
    char c;

    region_beg = 0;
    region_end = 0xffffffff;

    printf("\nLog size = %d (max %d),  Sample period = ~%d events\n",
	   prof_pos, PROF_LOG_MAX, prof_sample_period);

    c = kdebug_get_choice("Dump", prof_thread == 0 ? prof_task == 0 ?
			  "thRead/All/reGion/Nothing" :
			  "thRead/taSk/All/reGion/Nothing" :
			  "All/reGion/Nothing", 'a');

    switch (c)
    {
    case 'n': return;
    case 'r': dthread = (dword_t) kdebug_get_thread(current); break;
    case 's': dtask = (dword_t) kdebug_get_task(current); break;
    case 'g':
	printf("Region beg. [00000000]: ");
	region_beg = kdebug_get_hex(0);
	printf("Region end  [ffffffff]: ");
	region_end = kdebug_get_hex(0xffffffff);
	break;
    }

    printf("Lower count limit [%d]: ", PROF_DEF_LOW_DUMPLIMIT);
    low_limit = kdebug_get_dec(PROF_DEF_LOW_DUMPLIMIT);

    kdebug_proflog_sort();

    enable_fpu();
    save_fpu_state((byte_t *) &fpu_state);

    for ( dword_t i = 0; i < prof_pos; i++ )
    {
	/* Skip unwanted threads or tasks. */
	if ( (dthread &&  dthread != (dword_t) tid_to_tcb(prof_log[i].tid)) ||
	     (dtask && dtask != TASK_TCB(tid_to_tcb(prof_log[i].tid))) )
	    continue;

	/* Skip unwanted regions. */
	if ( prof_log[i].eip < region_beg || prof_log[i].eip > region_end )
	    continue;
	
	if ( (prof_log[i].eip != p_eip) || (prof_log[i].tid != p_tid) )
	{
	    /* Dump counterd values. */
	    if ( p_eip != ~0UL && eip_cnt >= low_limit )
	    {
		printf("  %p:%6d", p_eip, eip_cnt);
		for ( dword_t k = 0; k < PROF_NUM_VALUES; k++ )
		{
		    if ( prof_valuetype[k] == 0 )
		    {
			if ( k == 0 ) putc('\n');
			break;
		    }
		    printf("%s  v%d", k>0 ? "                 " : "", k);
		    dump(freq + k);
		}
		if ( PROF_NUM_VALUES == 0 ) putc('\n');
	    }

	    /* Reset counters. */
	    p_eip = prof_log[i].eip;
	    eip_cnt = 0;
	    for ( dword_t k = 0; k < PROF_NUM_VALUES; k++ )
	    {
		if ( prof_valuetype[k] == 0 ) break;
		clear(freq + k);
	    }
	}

	if ( prof_log[i].tid != p_tid )
	{
	    p_tid = prof_log[i].tid;
	    printf("Thread %p (%t):\n", p_tid.raw, p_tid);
	}

	/* Increase counters. */
	eip_cnt++;
	for ( dword_t k = 0; k < PROF_NUM_VALUES; k++ )
	{
	    if ( prof_valuetype[k] == 0 ) break;
	    add(freq + k, prof_log[i].value[k]);
	}
    }

    if ( p_eip != ~0UL && eip_cnt >= low_limit )
    {
	/* Dump counted values. */
	printf("  %p:%6d", p_eip, eip_cnt);
	for ( dword_t k = 0; k < PROF_NUM_VALUES; k++ )
	{
	    if ( prof_valuetype[k] == 0 )
	    {
		if ( k == 0 ) putc('\n');
		break;
	    }
	    printf("%s  v%d", k>0 ? "                 " : "", k);
	    dump(freq + k);
	}
	if ( PROF_NUM_VALUES == 0 ) putc('\n');
    }

    load_fpu_state((byte_t *) &fpu_state);
    disable_fpu();
}



/*
**
** The prof_freq_t structure is a conatiner which implements the
** Gibbons and Mattias' algorithm for summarizing a stream of data.
** The PROF_FREQ_COUNTERS tells how many different values the
** container can hold.
**
*/

void clear(prof_freq_t *fr)
{
    for ( int i = 0; i < PROF_FREQ_COUNTERS; i++ )
	fr->counter[i] = fr->value[i] = 0;
    fr->prob = 1.0;
}

void add(prof_freq_t *fr, dword_t val)
{
    int i;

    /* Search for slot with given value. */
    for ( i = 0; i < PROF_FREQ_COUNTERS; i++ )
	if ( fr->value[i] == val )
	    break;

    while ( i == PROF_FREQ_COUNTERS )
    {
	/* Search for free slot. */
	for ( i = 0; i < PROF_FREQ_COUNTERS; i++ )
	    if ( fr->counter[i] == 0 )
	    {
		fr->value[i] = val;
		break;
	    }
	if ( i == PROF_FREQ_COUNTERS )
	    /* No free slots. */
	    reduce(fr);
    }

    /* Update counter with probability fr->prob. */
    if ( fr->prob == 1.0 || /* Common case */
	 random() <= (fr->prob * 0xffffffff) )
	fr->counter[i]++;
}

void reduce(prof_freq_t *fr)
{
    for ( dword_t i = 0; i < PROF_FREQ_COUNTERS; i++ )
    {
	dword_t cnt = 0;
	while ( fr->counter[i]-- )
	    if ( (random() & (PROF_FREQ_COUNTERS-1)) != 0 )
		cnt++;
	fr->counter[i] = cnt;
    }
    fr->prob *= ((float) (PROF_FREQ_COUNTERS-1)) / PROF_FREQ_COUNTERS;
}

void dump(prof_freq_t *fr)
{
    dword_t i;
    dword_t sum = 0;
    
    for ( i = 0; i < PROF_FREQ_COUNTERS; i++ )
    {
	if ( fr->counter[i] == 0 )
	    continue;
	printf(" [%4d:%p]",
	       (dword_t) (((float) fr->counter[i]) / fr->prob),
	       fr->value[i]);
	sum += fr->counter[i];
    }
    putc('\n');
}




/*
**
** The kdebug interface to the profiling functions.  Everything above
** this is only code for processing and presenting the logged profile.
**
*/

void kdebug_profiling(tcb_t *current)
{
    char c = 0;
    
    c = kdebug_get_choice("\nProfile",
			  "thRead/taSk/reGion/All/Clear log/Nothing", 'a');

    if ( c != 'c' )
    {
	prof_do_sampling = PROF_LOG_NONE;
	prof_sample_period = prof_thread = prof_task = 0;
	prof_region_beg = 0;
	prof_region_end = 0xffffffff;
    }

    switch (c)
    {
    case 'n':
	/* Reset NMI interrupts. */
	idt[2].set((void (*)()) int_2, IDT_DESC_TYPE_INT, 0);
	return;
    case 'r': prof_thread = (dword_t) kdebug_get_thread(current); break;
    case 's': prof_task = (dword_t) kdebug_get_task(current); break;
    case 'g':
	printf("Region beg. [00000000]: ");
	prof_region_beg = kdebug_get_hex(0);
	printf("Region end  [ffffffff]: ");
	prof_region_end = kdebug_get_hex(0xffffffff);
	break;
    case 'c':
	goto Clear_log;
    }

#if defined(CONFIG_PROFILING_WITH_NMI)
    if ( prof_region_beg >= KERNEL_VIRT )
    {
	prof_do_sampling = PROF_LOG_KERNEL;
    }
    else if ( prof_region_end >= KERNEL_VIRT )
    {
	c = kdebug_get_choice("Profile", "Kernel/User/Both", 'u');
	switch (c)
	{
	case 'k': prof_do_sampling = PROF_LOG_KERNEL; break;
	case 'u': prof_do_sampling = PROF_LOG_USER; break;
	case 'b': prof_do_sampling = PROF_LOG_BOTH; break;
	}
    }

    /* Redirect NMI interrupts to the perfctr handler. */
    idt[2].set((void (*)()) apic_perfctr, IDT_DESC_TYPE_INT, 0);
#else
    prof_do_sampling = PROF_LOG_USER;
#endif

    prof_trigger_event = kdebug_get_perfctr("Trigger event", 0x79);

    while ( prof_sample_period < PROF_MIN_SAMPLE_PERIOD )
    {
	printf("Sample period [%d]: ", PROF_DEF_SAMPLE_PERIOD);
	prof_sample_period = kdebug_get_dec(PROF_DEF_SAMPLE_PERIOD);
	if ( prof_sample_period < PROF_MIN_SAMPLE_PERIOD )
	    printf("Min. period is %d\n", PROF_MIN_SAMPLE_PERIOD);
    }

    for ( dword_t vnum = 0; vnum < PROF_NUM_VALUES; vnum++ )
    {
	prof_valuetype[vnum] = 0;
	if ( vnum > 0 && prof_valuetype[vnum-1] == 0 )
	    continue;

	printf("Sample value %d ", vnum);
	c = kdebug_get_choice("", vnum < 2 ? "Perfctr/Register/None" :
			      "Register/None", 'n');
	switch (c)
	{
	case 'p':
	    if ( prof_do_sampling & PROF_LOG_KERNEL )
		printf("Note: Can not enable perfctr sampling "
		       "in kernel.\n");
	    prof_valuetype[vnum] = 0xff;
	    prof_evsel[vnum] = kdebug_get_perfctr("Event selector", 0xff);
	    break;
	case 'r':
	    c = kdebug_get_choice("Register",
				  "eAx/eBx/eCx/eDx/Esi/edI/ebP/eSp/eFlags/"
				  "data seG", 'a');
	    switch (c)
	    {
	    case 'a': prof_valuetype[vnum] = 10; break; /* eax */
	    case 'c': prof_valuetype[vnum] = 9;  break; /* ecx */
	    case 'd': prof_valuetype[vnum] = 8;  break; /* edx */
	    case 'b': prof_valuetype[vnum] = 7;  break; /* ebx */
	    case 'p': prof_valuetype[vnum] = 5;  break; /* ebp */
	    case 'e': prof_valuetype[vnum] = 4;  break; /* esi */
	    case 'i': prof_valuetype[vnum] = 3;  break; /* edi */
	    case 's': prof_valuetype[vnum] = 15; break; /* esp */
	    case 'f': prof_valuetype[vnum] = 14; break; /* efl */
	    case 'g': prof_valuetype[vnum] = 2;  break; /* ds  */
	    }
	    break;
	case 'n':
	    break;
	}
    }

Clear_log:

    /* Clear log. */
    prof_pos = 0;
    for ( dword_t i = 0; i < PROF_LOG_MAX; i++ )
    {
	prof_log[i].tid.raw = prof_log[i].eip = 0;
	for ( dword_t k = 0; k < PROF_NUM_VALUES; k++ )
	    prof_log[i].value[k] = 0;
    }

    if ( prof_do_sampling )
	reset_perfctr();
}


/*
 * Handler invoked when the instruction following the trigger point
 * has been executred.
 */

void handle_perfctr_bounce_back(exception_frame_t *frame)
{
    for ( dword_t i = 0; i < PROF_NUM_VALUES; i++ )
    {
	if ( prof_valuetype[i] == 0x00 )
	    break;
	if ( prof_valuetype[i] == 0xff )
	{
	    if ( prof_log[prof_pos].eip < KERNEL_VIRT )
		prof_log[prof_pos].value[i] = rdpmc(i);
	}
	else
	{
	    prof_log[prof_pos].value[i] =
		((dword_t *) frame)[prof_valuetype[i]];
	    if ( prof_valuetype[i] == 2 )
		prof_log[prof_pos].value[i] &= 0xffff;;
	}
    }

    if ( prof_pos < PROF_LOG_MAX-1 )
	prof_pos++;

    /* Restore single stepping state. */
    idt[1].set((void (*)()) int_1, IDT_DESC_TYPE_INT, 0);
    if ( prof_restore_single_step )
	frame->eflags |= (1 << 16);
    else
	frame->eflags &= ~(1 << 8);

    reset_perfctr();
}


/*
 * Handler invoked on every profiling trigger interrupt.
 */

void apic_handler_perfctr(dword_t eip, dword_t *eflags)
{
    if ( ! prof_do_sampling )
    {
	/* Turn off random sampling. */
	kdebug_setperfctr(0, 0, 0);
	apic_ack_irq();
	return;
    }

    tcb_t *current = get_current_tcb();
    int value;

    /* Skip samples outside our sampling area. */
    if ( ((eip < KERNEL_VIRT) && (~prof_do_sampling & PROF_LOG_USER)) ||
	 ((eip >= KERNEL_VIRT) && (~prof_do_sampling & PROF_LOG_KERNEL)) )
	goto Skip_tick;
    if ( (eip < prof_region_beg) || (eip > prof_region_end) )
	goto Skip_tick;

    /* Skip samples in unwanted threads or tasks. */
    if ( (prof_thread && (prof_thread != (dword_t) current)) ||
	 (prof_task && (prof_task != (dword_t) TASK_TCB(current))) )
	goto Skip_tick;

    /* Skip samples in kdebug, but keep samples in the idler. */
    if ( ((dword_t) current > TCB_AREA+TCB_AREA_SIZE) &&
	 (current != get_idle_tcb()) )
	goto Skip_tick;

    prof_log[prof_pos].tid = current->myself;
    prof_log[prof_pos].eip = eip;

    if ( (PROF_NUM_VALUES > 0) && prof_valuetype[0] )
    {
	/*
	 * Do value sampling.  Single step next instruction.
	 */
	prof_restore_single_step = *eflags & (1<<8);
	*eflags |= (1 << 8) + (1 << 16);
	idt[1].set((void (*)()) perfctr_bounce_back, IDT_DESC_TYPE_INT, 0);

	if ( (eip < KERNEL_VIRT) &&
	     (prof_valuetype[0] == 0xff ||
	      ((PROF_NUM_VALUES > 1) && prof_valuetype[1] == 0xff)) )
	{
	    /* Use performance counters. */
	    value = prof_evsel[0];
	    value |= (1 << 22);		/* Enable counter */
	    value |= (1 << 16);		/* Count user-level events */
	    kdebug_setperfctr(0, value, 0);

	    value = prof_evsel[1];
	    value |= (1 << 16);		/* Count user-level events */
	    kdebug_setperfctr(1, value, 0);
	}

	apic_ack_irq();
	return;
    }

    if ( prof_pos < PROF_LOG_MAX )
	prof_pos++;

Skip_tick:

    reset_perfctr();
}


static void reset_perfctr(void)
{
    dword_t value;

    value = prof_trigger_event;
    value |= (1 << 22);		/* Enable counter */
    value |= (1 << 20);		/* Generate APIC int */
    value |= (1 << 16);		/* Count user-level events */
    if ( prof_do_sampling & PROF_LOG_KERNEL )
	value |= (1 << 17);	/* Count kernel-level events */

    dword_t randvar = (rdtsc() & 0xff);
    kdebug_setperfctr(0, value, -(prof_sample_period ^ randvar));
    kdebug_setperfctr(1, 0, 0);
    apic_ack_irq();
}


void kdebug_init_profiling(void)
{
    prof_log = (prof_event_t *) kmem_alloc(PROF_LOG_WINDOW_SIZE);
    prof_pos = 0;
    prof_do_sampling = 0;
    srandom(rdtsc());

    if ( prof_log == NULL )
	printf("*** Failed allocating profiling log.\n");
}



#endif /* CONFIG_ENABLE_PROFILING */

#if defined(CONFIG_DEBUGGER_NEW_KDB)
/* for the new debugger */

#include "pmc_values.h"

//from x86.c
void enable_pmc_of_int();
void disable_pmc_of_int();
void clear_perfctr(int counter);
void set_event(int counter, dword_t arg);


void init_pmcs() {
    set_event(0, 0);
    clear_perfctr(0);
    set_event(1, 0);
    clear_perfctr(1);
}

void clear_perfctr(int counter) {
    int counter_register = PERFCTR0 + counter;
    wrmsr(counter_register, 0);
}

void set_event(int counter, dword_t arg) {
    int selection_register = PERFEVENTSEL0 + counter;
    clear_perfctr(counter);
    wrmsr(selection_register, (qword_t) arg);
}

/* end of my part */
#endif /* define(CONFIG_DEBUGGER_NEW_KDB) */
