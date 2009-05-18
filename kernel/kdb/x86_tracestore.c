/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     x86_tracestore.c
 * Description:   Automatic logging of various traces to memory.
 *                
 * @LICENSE@
 *                
 * $Id: x86_tracestore.c,v 1.2 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

#if defined(CONFIG_X86_P4_BTS) || defined(CONFIG_X86_P4_PEBS)

#include "kdebug.h"


#define BTS_BUFSIZE		(1024*CONFIG_X86_P4_BTS_BUFSIZE)
#define PEBS_BUFSIZE		(1024*CONFIG_X86_P4_PEBS_BUFSIZE)
#define BTS_SIZE		(BTS_BUFSIZE/sizeof (branch_trace_t))
#define PEBS_SIZE		(PEBS_BUFSIZE/sizeof (pebs_record_t))


typedef struct branch_trace_t	branch_trace_t;
typedef struct pebs_record_t	pebs_record_t;

struct branch_trace_t {
    dword_t	from;
    dword_t	to;
    struct {
	unsigned __pad1:4;
	unsigned predicted:1;
	unsigned __pad2:27;
    } flags;
};

struct pebs_record_t {
    dword_t	eflags;
    dword_t	eip;
    dword_t	eax;
    dword_t	ebx;
    dword_t	ecx;
    dword_t	edx;
    dword_t	esi;
    dword_t	edi;
    dword_t	ebp;
    dword_t	esp;
};


/*
 * The DS_AREA msr specifies the location in memory where the
 * following buffer management pointers are located.
 */

struct ds_bufmng_area_t {
    branch_trace_t	*bts_base;
    branch_trace_t	*bts_index;
    branch_trace_t	*bts_maximum;
    branch_trace_t	*bts_intr;

    pebs_record_t	*pebs_base;
    pebs_record_t	*pebs_index;
    pebs_record_t	*pebs_maximum;
    pebs_record_t	*pebs_intr;
    qword_t		pebs_counter;
    dword_t		__reserved;
};

static struct ds_bufmng_area_t bufmng;




#if defined(CONFIG_X86_P4_BTS)

/*
 * We do not set the DEBUGCTL msr directly since we're not interested
 * in keeping track of branches inside the kernel debugger.  The
 * kernel debugger entry and exit stubs will instead respectively
 * disable and enable the tracing feature.  This variable is used to
 * save the current state of the tracing feature.
 */

extern dword_t x86_debugctl_state;

void kdebug_x86_bts (void)
{
    char c;

    c = kdebug_get_choice ("\nBranch Trace Store", "Enable/Disable/Show", 's');
    switch (c)
    {
    case 'e':
	bufmng.bts_base = bufmng.bts_base;
	bufmng.bts_index = bufmng.bts_base;
	bufmng.bts_maximum = bufmng.bts_base + BTS_SIZE;
	zero_memory ((ptr_t) bufmng.bts_base, BTS_BUFSIZE);
	x86_debugctl_state |= (1<<3) + (1<<2); /* BTS + TR */
	break;

    case 'd':
	x86_debugctl_state &= ~((1<<3) + (1<<2));
	break;

    case 's':
	branch_trace_t *bt = bufmng.bts_index;
	for (int i = BTS_SIZE-1; i >= 0; i--)
	{
	    if (bt->from != bt->to)
		printf ("%4d: %c %p -> %p\n", i,
			bt->flags.predicted ? 'P' : ' ', bt->from, bt->to);
	    if (bt++ == bufmng.bts_maximum)
		bt = bufmng.bts_base;
	}
	break;
    }
}

#endif /* CONFIG_X86_P4_BTS */




#if defined(CONFIG_X86_P4_PEBS)

void kdebug_x86_pebs (void)
{
    char c;

    c = kdebug_get_choice ("\nPrecise Event-Based Sampling",
			   "Enable/Disable/Show", 's');
    switch (c)
    {
    case 'e':
//	wrmsr (IA32_PEBS_ENABLE, IA32_PEBS_ENABLE_PEBS);
	break;

    case 'd':
	break;

    case 's':
	break;
    }
}

#endif /* CONFIG_X86_P4_PEBS */




void init_tracestore (void)
{
#if defined(CONFIG_X86_P4_BTS)
    bufmng.bts_base = (branch_trace_t *) kmem_alloc (BTS_BUFSIZE);
    bufmng.bts_index = bufmng.bts_base;
    bufmng.bts_maximum = bufmng.bts_base + BTS_SIZE;
    bufmng.bts_intr = NULL;
#endif

#if defined(CONFIG_X86_P4_PEBS)
    bufmng.pebs_base = (pebs_record_t *) kmem_alloc (PEBS_BUFSIZE);
    bufmng.pebs_index = bufmng.pebs_base;
    bufmng.pebs_maximum = bufmng.pebs_base + PEBS_SIZE;
    bufmng.pebs_intr = NULL;
    bufmng.pebs_counter = 0;
#endif

    wrmsr (IA32_DS_AREA, (dword_t) &bufmng);
}


#endif /* CONFIG_X86_P4_BTS || CONFIG_X86_P4_PEBS */
