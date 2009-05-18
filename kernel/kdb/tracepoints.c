/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     tracepoints.c
 * Description:   KDB Kernel Tracepoint interface.
 *                
 * @LICENSE@
 *                
 * $Id: tracepoints.c,v 1.5 2001/11/22 12:13:54 skoglund Exp $
 *                
 ********************************************************************/
#include <universe.h>

#if defined(CONFIG_ENABLE_TRACEPOINTS)
#include <tracepoints.h>
#include "kdebug.h"


/*
 * Create list of description strings.
 */
#define DEFINE_TP(x, y)	#x,

char *tracepoint_name[] = {
    "Quit (no tracepoint)",
#   include <tracepoint_list.h>
};


byte_t tracepoint_is_set[(TP_MAX+7) >> 3] L4_SECT_KDEBUG;
byte_t tracepoint_enters_kdb[(TP_MAX+7) >> 3] L4_SECT_KDEBUG;
dword_t tracepoint_cnt[TP_MAX] L4_SECT_KDEBUG;


void kdebug_tracepoint(void)
{
    dword_t num = 0, n, i;
    char c1 = 0, c2 = 0;

    c1 = kdebug_get_choice("\nTracepoint",
			   "Set/Clear/List/Reset counters"
			   "/clear All/Enable all", 'l');
    switch (c1)
    {
    case 'a':
	for ( i = 0; i < sizeof(tracepoint_is_set); i++ )
	    tracepoint_is_set[i] = tracepoint_enters_kdb[i] = 0;
	return;
    case 'e':
	for ( i = 0; i < sizeof(tracepoint_is_set); i++ )
	    tracepoint_is_set[i] = ~0, tracepoint_enters_kdb[i] = 0;
	return;
    case 'r':
	for ( i = 0; i < TP_MAX; i++ )
	    tracepoint_cnt[i] = 0;
	return;
    case 's':
    case 'c':
	break;
    case 'l':
	c2 = kdebug_get_choice("List", "All/Enabled", 'a');
	kdebug_list_tracepoints(c2 == 'a');
	return;
    }

    while ( num == 0 )
    {
	printf("Tracepoint [list]: ");
	num = kdebug_get_dec(0xffff, "list");
	if ( num >= TP_MAX )
	{
	    for ( i = 0; i < (TP_MAX+1)/2; i++ )
	    {
		n = printf(" %2d - %s", i, tracepoint_name[i]);
		if ( i + TP_MAX/2 < TP_MAX )
		{
		    for ( n = 38-n; n--; ) putc(' ');
		    printf("%2d - %s\n", i + TP_MAX/2,
			   tracepoint_name[i + TP_MAX/2]);
		}
		else
		    printf("\n");
	    }
	    num = 0;
	}
	else if ( num == 0 )
	    return;
    }

    if ( c1 == 's' )
    {
	c2 = kdebug_get_choice("Enter kdebug", "Yes/No", 'n');
	set_tracepoint(num, c2 == 'y');
    }
    else
	clr_tracepoint(num);
}


void kdebug_list_tracepoints(int list_all)
{
    printf("Num KDB Name                              Count\n");
    for ( int i = 1; i < TP_MAX; i++ )
	if ( list_all || is_tracepoint_set(i) )
	{
	    dword_t n = printf("%3d  %c  %s", i,
			       does_tracepoint_enter_kdb(i) ? 'y' : ' ',
			       tracepoint_name[i]);

	    for ( n = 40-n; n--; ) putc(' ');
	    printf("%7d\n", tracepoint_cnt[i]);
	}
}




#endif /* CONFIG_ENABLE_TRACEPOINTS */
