/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     x86_input.c
 * Description:   x86 specific kdebug input functions.
 *                
 * @LICENSE@
 *                
 * $Id: x86_input.c,v 1.4 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>
#include "kdebug.h"

#if defined(CONFIG_ARCH_X86_I686)

struct perfctr_t {
    word_t	num;
    word_t	unit_mask;
    char 	*name;
};

static struct perfctr_t perf_counters[] = {
    { 0x43, 0,	"DCU mem refs" },
    { 0x45, 0,	"DCU lines allocated" },
    { 0x46, 0,	"DCU M lines allocated" },
    { 0x47, 0,	"DCU M lines evicted" },
    { 0x48, 0,	"DCU outstanding miss cycles" },
    { 0x80, 0,	"IFU inst. fetches" },
    { 0x81, 0,	"IFU ITLB misses" },
    { 0x86, 0,	"IFU stalls" },
    { 0x28, 1,	"L2 instr. fetches" },
    { 0x29, 1,	"L2 data loads" },
    { 0x2A, 1,	"L2 data stores" },
    { 0x24, 0,	"L2 lines allocated" },
    { 0x26, 0,	"L2 lines evicted" },
    { 0x25, 0,	"L2 M lines allocated" },
    { 0x27, 0,	"L2 M lines evicted" },
    { 0x62, 2,	"BUS DRDY# clocks" },
    { 0x63, 2,	"BUS LOCK# clocks" },
    { 0xC1, 0,	"FP floating point ops." },
    { 0xC0, 0,	"INST instr. retired" },
    { 0xC2, 0,	"INST uops. retired" },
    { 0xD0, 0,	"INST instr. decoded" },
    { 0xC8, 0,	"INT HW intr. received" },
    { 0xC6, 0,	"INT intr. disabled cycles" },
    { 0xC7, 0,	"INT intr. pendeing cycles" },
    { 0xC4, 0,	"BR branches retired" },
    { 0xC5, 0,	"BR mispred. branches" },
    { 0xE2, 0,	"BR BTB misses" },
    { 0xA2, 0,	"STA Resource stalls" },
    { 0xD2, 0,	"STA Partial resource stalls" },
    { 0x06, 0,	"SEG Segment reg loads" },
    { 0x79, 0,	"CLK CPU clock cycles" },
    { 0xFF, 0,	"This help message" }
};


static void print_perfctrs(void)
{
    dword_t tbl_max = sizeof(perf_counters)/sizeof(struct perfctr_t);
    dword_t i, n;

    for ( i = 0; i < (tbl_max+1) / 2; i++ )
    {
	/* Left column */
	n = printf(" %02x - %s", perf_counters[i].num, perf_counters[i].name);
	if ( i + (tbl_max+1)/2 < tbl_max )
	{
	    /* Right column */
	    for ( n = 38-n; n--; ) putc(' ');
	    printf("%02x - %s\n", perf_counters[i + (tbl_max+1)/2].num,
		   perf_counters[i + (tbl_max+1)/2].name);
	}
	else
	    printf("\n");
    }
}


dword_t kdebug_get_perfctr(char *str, dword_t def)
{
    dword_t tbl_max = sizeof(perf_counters)/sizeof(struct perfctr_t);
    dword_t ctr, i;
    char c;

    do {
	if ( def == 0xff ) printf("%s [help]: ", str);
	else printf("%s [%02x]: ", str, def);
	ctr = kdebug_get_hex(def, def == 0xff ? "help" : NULL);
	if ( ctr == 0xff )
	    print_perfctrs();
    } while ( ctr == 0xff );

    /*
     * Check if we should specify a unit mask.
     */
    for ( i = 0; i < tbl_max; i++ )
	if ( perf_counters[i].num == ctr )
	{
	    switch (perf_counters[i].unit_mask)
	    {
	    case 2:
		/* External bus event. */
		if ( kdebug_get_choice("Bus user", "Self/Any", 's') == 'a' )
		    ctr |= 0x2000;
		return ctr;
	    case 1:
		/* L2 cache event. */
		printf("MESI mask (M/E/S/I) [mesi]: ");
		while ( (c = getc()) != '\r' )
		    switch (c)
		    {
		    case 'm': putc(c); ctr |= 0x800; break;
		    case 'e': putc(c); ctr |= 0x400; break;
		    case 's': putc(c); ctr |= 0x200; break;
		    case 'i': putc(c); ctr |= 0x100; break;
		    }
		if ( (ctr & 0xF00) == 0 )
		{
		    printf("mesi");
		    ctr |= 0xF00;
		}
		putc('\n');

	    case 0:
		/* No mask. */
		return ctr;
	    }
	}

    /* Some other event.  Maybe unit mask. */
    printf("Unit mask [00]: ");
    ctr |= (kdebug_get_hex(0) << 8);

    return ctr;
}

#elif defined(CONFIG_ARCH_X86_P4)

/*
 * Specification of the Pentium 4 events, their allowed counters, and
 * the valid event masks.
 */

struct perfctr_t {
    byte_t	event_select;
    byte_t	escr_select;
    byte_t	ctr_low;
    byte_t	ctr_high;
    byte_t	special;
    char	*name;
    struct {
	byte_t	bitnum;
	char	*desc;
    } mask[9];
} __attribute__ ((packed));

static struct perfctr_t perf_counters[] = {
    /*
     * Non-Retirement events
     */

    { 0x06, 0x05, 12, 17, 0, "Branches", {
	{ 0, "Predicted NT" },
	{ 1, "Mispredicted NT" },
	{ 2, "Predicted taken" },
	{ 3, "Mispredicted taken" },
	{ 0, NULL }} },
    { 0x03, 0x04, 12, 17, 0, "Mispredicted branches", {
	{ 0, "Not bogus" },
	{ 0, NULL }} },
    { 0x01, 0x01, 4, 7, 0, "Trace cache deliver", {
	{ 2, "Delivering" },
	{ 5, "Building" },
	{ 0, NULL }} },
    { 0x03, 0x00, 0, 3, 0, "BPU fetch request", {
	{ 0, "TC lookup miss" },
	{ 0, NULL }} },
    { 0x18, 0x03, 0, 3, 0, "ITLB reference", {
	{ 0, "Hit" },
	{ 1, "Miss" },
	{ 2, "Uncachable hit" },
	{ 0, NULL }} },
    { 0x02, 0x05, 8, 11, 0, "Memory cancel", {
	{ 2, "Req buffer full" },
	{ 7, "Cache miss" },
	{ 0, NULL }} },
    { 0x08, 0x02, 8, 11, 0, "Memory complete", {
	{ 0, "Load split completed" },
	{ 1, "Split store completed" },
	{ 0, NULL }} },
    { 0x04, 0x02, 8, 11, 0, "Load port replay", {
	{ 1, "Split load" },
	{ 0, NULL }} },
    { 0x05, 0x02, 8, 11, 0, "Store port replay", {
	{ 1, "Split store" },
	{ 0, NULL }} },
    { 0x03, 0x02, 0, 3, 0, "MOB load replay", {
	{ 1, "Unknown store address" },
	{ 3, "Unknown store data" },
	{ 4, "Partial overlap" },
	{ 5, "Unaligned address" },
	{ 0, NULL }} },
    { 0x01, 0x04, 0, 3, 0, "Page walk type", {
	{ 0, "DTLB miss" },
	{ 1, "ITLB miss" },
	{ 0, NULL }} },
    { 0x0c, 0x07, 0, 3, 1, "L2 cache reference", {
	{ 0, "Read hit shared" },
	{ 1, "Read hit exclusive" },
	{ 2, "Read hit modified" },
	{ 8, "Read miss" },
	{ 10, "Write miss" },
	{ 0, NULL }} },
    { 0x03, 0x06, 0, 1, 1, "Bus transaction", {
	{ 0, "Req type (SET THIS)" },
	{ 5, "Read" },
	{ 6, "Write" },
	{ 7, "UC access" },
	{ 8, "WC access" },
	{ 13, "Own CPU stores" },
	{ 14, "Non-local access" },
	{ 15, "Prefetch" },
	{ 0, NULL }} },
    { 0x17, 0x06, 0, 3, 1, "FSB activity", {
	{ 0, "Ready drive" },
	{ 1, "Ready own" },
	{ 2, "Ready other" },
	{ 3, "Busy drive" },
	{ 4, "Busy own" },
	{ 5, "Busy other" },
	{ 0, NULL }} },
    { 0x05, 0x07, 0, 1, 2, "Bus operation", {
	/* Special mask handling. */
	{ 0, NULL }} },
    { 0x02, 0x05, 12, 17, 0, "Machine clear", {
	{ 0, "Any reason" },
	{ 2, "Memory ordering" },
	{ 0, NULL }} },

    /*
     * At-Retirement events.
     */

    { 0x08, 0x05, 12, 17, 3, "Front end event", {
	{ 0, "Not bogus" },
	{ 1, "Bogus" },
	{ 0, NULL }} },
    { 0x0c, 0x05, 12, 17, 4, "Execution event", {
	{ 0, "Not bogus 0" },
	{ 1, "Not bogus 1" },
	{ 2, "Not bogus 2" },
	{ 3, "Not bogus 3" },
	{ 4, "Bogus 0" },
	{ 5, "Bogus 1" },
	{ 6, "Bogus 2" },
	{ 7, "Bogus 3" },
	{ 0, NULL }} },
    { 0x09, 0x05, 12, 17, 5, "Replay event", {
	{ 0, "Not bogus" },
	{ 1, "Bogus" },
	{ 0, NULL }} },
    { 0x02, 0x04, 12, 17, 6, "Instr retired", {
	{ 0, "Non bogus untagged" },
	{ 1, "Non bogus tagged" },
	{ 2, "Bogus untagged" },	
	{ 3, "Bogus tagged" },
	{ 0, NULL }} },
    { 0x01, 0x04, 12, 17, 7, "Uops retired", {
	{ 0, "Not bogus" },
	{ 1, "Bogus" },
	{ 0, NULL }} }
};

struct replay_tag_t {
    uint16	pebs_enable;
    uint16	pebs_matrix;
    char 	*desc;
} __attribute__ ((packed));

static struct replay_tag_t replay_tags[] = {
    { 0x0001, 0x01, "L1 cache load miss" },
    { 0x0002, 0x01, "L2 cache load miss" },
    { 0x0004, 0x01, "DTLB load miss" },
    { 0x0004, 0x02, "DTLB store miss" },
    { 0x0004, 0x03, "DTLB all miss" },
    { 0x0100, 0x01, "MOB load" },
    { 0x0200, 0x01, "Split load" },
    { 0x0200, 0x02, "Split store" }
};


/*
 * Helper array for easily keeping track of which ESCR MSRs are in use.
 */

static word_t escr_msr[18];


/*
 * Translation table for ESCR numbers (as specified by cccr_select) to
 * ESCR addresses (add 0x300 to get actual address).  A zero value
 * indicates an invalid ESCR selector.
 */

static byte_t escr_to_addr[18][8] = {
    { 0xa0, 0xb4, 0xaa, 0xb6, 0xac, 0xc8, 0xa2, 0xa0 }, // Ctr 0
    { 0xb2, 0xb4, 0xaa, 0xb6, 0xac, 0xc8, 0xa2, 0xa0 }, // Ctr 1
    { 0xb3, 0xb5, 0xab, 0xb7, 0xad, 0xc9, 0xa3, 0xa1 }, // Ctr 2
    { 0xb3, 0xb5, 0xab, 0xb7, 0xad, 0xc9, 0xa3, 0xa1 }, // Ctr 3
    { 0xc0, 0xc4, 0xc2, 0x00, 0x00, 0x00, 0x00, 0x00 }, // Ctr 4
    { 0xc0, 0xc4, 0xc2, 0x00, 0x00, 0x00, 0x00, 0x00 }, // Ctr 5
    { 0xc1, 0xc5, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00 }, // Ctr 6
    { 0xc1, 0xc5, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00 }, // Ctr 7
    { 0xa6, 0xa4, 0xae, 0xb0, 0x00, 0xa8, 0x00, 0x00 }, // Ctr 8
    { 0xa6, 0xa4, 0xae, 0xb0, 0x00, 0xa8, 0x00, 0x00 }, // Ctr 9
    { 0xa7, 0xa5, 0xaf, 0xb1, 0x00, 0xa9, 0x00, 0x00 }, // Ctr 10
    { 0xa7, 0xa5, 0xaf, 0xb1, 0x00, 0xa9, 0x00, 0x00 }, // Ctr 11
    { 0xba, 0xca, 0xbe, 0xbc, 0xb8, 0xcc, 0xe0, 0x00 }, // Ctr 12
    { 0xba, 0xca, 0xbe, 0xbc, 0xb8, 0xcc, 0xe0, 0x00 }, // Ctr 13
    { 0xbb, 0xcb, 0xbd, 0x00, 0xb9, 0xcd, 0xe1, 0x00 }, // Ctr 14
    { 0xbb, 0xcb, 0xbd, 0x00, 0xb9, 0xcd, 0xe1, 0x00 }, // Ctr 15
    { 0xba, 0xca, 0xbe, 0xbc, 0xb8, 0xcc, 0xe0, 0x00 }, // Ctr 16
    { 0xbb, 0xcb, 0xbd, 0x00, 0xb9, 0xcd, 0xe1, 0x00 }  // Ctr 17
};

static inline dword_t escr_addr(dword_t ctr, dword_t num)
{
    return escr_to_addr[ctr][num] == 0 ? 0 : escr_to_addr[ctr][num] + 0x300;
}

static void print_perfctrs(void)
{
    dword_t tbl_max = sizeof(perf_counters)/sizeof(struct perfctr_t) + 2;
    dword_t i, n;

    for ( i = 0; i < (tbl_max+1)/2; i++ )
    {
	/* Left column */
	if ( i == 0 ) n = printf("  0 - This help message");
	else n = printf(" %2d - %s", i, perf_counters[i-1].name);

	/* Right column */
	if ( i + (tbl_max+1)/2 < tbl_max )
	{
	    for ( n = 38-n; n--; ) putc(' ');
	    if ( i + (tbl_max+1)/2 == tbl_max - 1 )
		printf(" 99 - Other performance counter\n");
	    else
		printf(" %2d - %s\n", i + (tbl_max+1)/2,
		       perf_counters[i-1 + (tbl_max+1)/2].name);
	}
	else
	    printf("\n");
    }
}

void kdebug_describe_perfctr(dword_t ctr)
{
    dword_t tbl_max = sizeof(perf_counters)/sizeof(struct perfctr_t);
    qword_t cccr = rdmsr(IA32_CCCR_BASE + ctr);
    struct perfctr_t *ev = NULL;

    if ( ! (cccr & (1<<12)) )
	/* Counter not enabled. */
	return;

    qword_t escr = 0;
    dword_t escr_select = (cccr >> 13) & 0x7;
    dword_t escr_msr = escr_addr(ctr, escr_select);
    if ( escr_msr != 0 )
	escr = rdmsr(escr_msr);

    dword_t event = (escr >> 25) & 0x3f;
    dword_t mask = (escr >> 9) & 0xffff;

    /* Find event description. */
    for ( dword_t i = 0; i < tbl_max; i++ )
	if ( perf_counters[i].event_select == event &&
	     perf_counters[i].escr_select == escr_select &&
	     perf_counters[i].ctr_low <= ctr &&
	     perf_counters[i].ctr_high >= ctr )
	{
	    ev = perf_counters + i;
	    break;
	}

    /* User/Kernel level state. */
    printf("%c%c  ", escr & (1<<2) ? 'U' : ' ', escr & (1<<3) ? 'K' : ' ');
    
    if ( ev )
    {
	dword_t i, bit = 0, first_p = 1;
	printf("%s (", ev->name);

	switch ( ev->special )
	{
	case 2:
	{
	    /* Special bus operation mask. */
	    static char * req_type[] = {
		"read", "read invalidate", "write", "writeback"};
	    static char * mem_type[] = {
		"uncachable", "write combined",	NULL, NULL, "write through",
		"write p", "write back", NULL };
	    static byte_t chunks[] = { 0, 1, 0, 8 };
	    printf("%s, %d chunks, %sdemand, %s)", req_type[mask & 0x3],
		   chunks[(mask >> 2) & 0x3], (mask & (1<<9)) ? "" : "non-",
		   mem_type[(mask >> 11) & 0x07]);
	    break;
	}
	case 5:
	{
	    /* Replay events. */
	    dword_t pebs_enable = rdmsr(IA32_PEBS_ENABLE) & ~(3<<24);
	    dword_t pebs_matrix = rdmsr(IA32_PEBS_MATRIX_VERT);
	    for ( i = 0; i < sizeof(replay_tags)/sizeof(replay_tags[0]); i++ )
		if ( replay_tags[i].pebs_enable == pebs_enable &&
		     replay_tags[i].pebs_matrix == pebs_matrix )
		{
		    printf("%s, ", replay_tags[i].desc);
		    break;
		}
	}

	default:
	    /* Normal event mask. */
	    while ( mask != 0 )
	    {
		if ( mask & 0x1 )
		{
		    char *desc = NULL;
		    for ( int i = 0; desc == NULL && ev->mask[i].desc; i++ )
			if ( ev->mask[i].bitnum == bit )
			    desc = ev->mask[i].desc;

		    if ( ! first_p ) printf(", ");
		    if ( desc ) printf("%s", desc);
		    else printf("bit%d", bit);
		    first_p = 0;
		}

		mask >>= 1;
		bit++;
	    }
	    putc(')');
	}
    }
    else
	printf("event=%02x  mask=%x", event, mask);
}


/*
 * Function kdebug_get_perfctr (escrmsr, escr, cccr)
 *
 *    Prompt user for a Pentium 4 performance event, including event
 *    mask, counter number, and privilege level.  Results are not
 *    written to MSRs, but are returned via output arguments.
 *    Function returns selected counter number, or -1 upon failure.
 *
 */
dword_t kdebug_get_perfctr(dword_t *escrmsr, qword_t *escr, qword_t *cccr)
{
    dword_t tbl_max = sizeof(perf_counters)/sizeof(struct perfctr_t);
    dword_t ctr, i, j, k, msr, c_low = 0, c_high = 17;
    char c, t;

    do {
	printf("Event [help]: ");
	ctr = kdebug_get_dec(0, "help");
	if ( ctr == 0 )
	    print_perfctrs();
    } while ( (ctr == 0) || (ctr > tbl_max && ctr != 99) );
    ctr--;

    *escr = (1 << 4);
    *cccr = (1 << 12) + (3 << 16);

    if ( ctr < tbl_max )
    {
	/* Predefined event */
	struct perfctr_t *ev = perf_counters + ctr;
	char defmask_str[17];
	dword_t defmask = 0, mask = 0;

	*escr |= ev->event_select << 25;
	*cccr |= ev->escr_select << 13;
	c_low = ev->ctr_low;
	c_high = ev->ctr_high;

	/* Find default mask. */
	for ( j = 0; ev->mask[j].desc; j++ )
	{
	    defmask_str[j] = ev->mask[j].bitnum < 10 ?
		ev->mask[j].bitnum + '0' : ev->mask[j].bitnum-10 + 'a';
	    defmask |= (1 << ev->mask[j].bitnum);
	}
	defmask_str[j] = 0;

	/* Special handling. */
	switch ( ev->special )
	{
	case 1:
	    /* Edge triggered event. */
	    *cccr |= (1 << 24);
	    break;

	case 2:
	    /* Special handling for bus operations. */
	    *cccr |= (1 << 24);
	    c= kdebug_get_choice("Request type",
				 "Read/read Invalidate/Write/writeBack", 'r');
	    switch (c)
	    {
	    case 'r': mask = 0; break;
	    case 'i': mask = 1; break;
	    case 'w': mask = 2; break;
	    case 'b': mask = 3; break;
	    }
	    c= kdebug_get_choice("Requested chunks", "0/1/8", '0');
	    switch (c)
	    {
	    case '0': mask |= (0 << 2); break;
	    case '1': mask |= (1 << 2); break;
	    case '8': mask |= (3 << 2); break;
	    }
	    c = kdebug_get_choice("Request is a demand", "Yes/No", 'y');
	    if ( c == 'y' ) mask |= (1 << 9);
	    c = kdebug_get_choice("Memory type", "Uncachable/writeCombined/"
				  "writeThrough/writeP/writeBack", 'b');
	    switch (c)
	    {
	    case 'u': mask |= (0 << 11); break;
	    case 'c': mask |= (1 << 11); break;
	    case 't': mask |= (4 << 11); break;
	    case 'p': mask |= (5 << 11); break;
	    case 'b': mask |= (6 << 11); break;
	    }
	    break;

	case 5:
	    /* Replay events. */	
	    for ( i = 0; i < sizeof(replay_tags)/sizeof(replay_tags[0]); i++ )
		printf("%3d - %s\n", i, replay_tags[i].desc);
	    do {
		printf("Select replay event [0]: ");
		k = kdebug_get_dec();
	    } while ( k >= sizeof(replay_tags)/sizeof(replay_tags[0]) );

	    wrmsr(IA32_PEBS_ENABLE, replay_tags[k].pebs_enable + (3<<24));
	    wrmsr(IA32_PEBS_MATRIX_VERT, replay_tags[k].pebs_matrix);
	    break;
	}

	/* Get event mask. */
	if ( j > 1 )
	{
	    printf("Valid event masks:\n");
	    for ( j = 0; ev->mask[j].desc; j++ )
		printf("  %c - %s\n",
		       ev->mask[j].bitnum < 10 ?
		       ev->mask[j].bitnum + '0' : ev->mask[j].bitnum-10 + 'a',
		       ev->mask[j].desc);
	    printf("Event mask [%s]: ", defmask_str);
	    while ( (t = c = getc()) != '\r' )
	    {
		switch (c)
		{
		case '0' ... '9': c -= '0'; break;
		case 'a' ... 'f': c -= 'a' - 'A';
		case 'A' ... 'F': c = c - 'A' + 10; break;
		default: continue;
		}
		putc(t);
		mask |= (1 << c);
	    }
	    if ( mask == 0 )
		printf("%s", defmask_str);
	    putc('\n');
	}
	if ( mask == 0 )
	    mask = defmask;

	*escr |= (mask & 0xffff) << 9;
    }
    else
    {
	/* No predefined event, set manually. */
	printf("Event select [0]: ");
	*escr |= (kdebug_get_hex() & 0x3f) << 25;
	printf("ESCR select [0]: ");
	*cccr |= (kdebug_get_hex() & 0x07) << 13;
	printf("Event mask [0]: ");
	*escr |= (kdebug_get_hex() & 0xffff) << 9;
    }

    /* Get counter number. */
    for (;;)
    {
	printf("Counter (%d-%d) [%d]: ", c_low, c_high, c_low);
	ctr = kdebug_get_dec(c_low);
	msr = escr_addr(ctr, (*cccr >> 13) & 0x7);

	if ( ctr >= c_low && ctr <= c_high )
	{
	    /* Check if ESCR is already in use. */
	    for ( i = 0; i < 18; i++ )
	    {
    		if ( escr_msr[i] == 0 || ctr == i )
		    continue;
		if ( rdmsr(IA32_CCCR_BASE + i) & (1 << 12) )
		{
		    if ( escr_msr[i] == msr )
			break;
		}
		else
		    escr_msr[i] = 0;
	    }

	    if ( i < 18 )
	    {
		c = kdebug_get_choice("ESCR already in use",
				      "Overwrite/Try other counter/Cancel",
				      't');
		if ( c == 'o' )  break;
		else if ( c == 'c' ) return ~0;
	    }
	    else
		/* ESCR is currently not in use. */
		break;
	}
	
    }

    /* Should only occur if ESCR and event mask is selected manually. */
    if ( msr == 0 )
    {
	printf("Invalid perf counter/ESCR selector.\n");
	return ~0;
    }
    *escrmsr = escr_msr[ctr] = msr;

    /* Select privilege level where counter should be active. */
    c = kdebug_get_choice("Privilege level", "User/Kernel/Both", 'b');
    switch (c)
    {
    case 'u': *escr |= (1 << 2); break;
    case 'b': *escr |= (1 << 2);
    case 'k': *escr |= (1 << 3); break;
    }

    return ctr;
}


#endif /* CONFIG_ARCH_X86_P4 */
