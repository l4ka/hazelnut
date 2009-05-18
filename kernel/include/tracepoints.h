/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     tracepoints.h
 * Description:   Kernel Tracepoint definitions.
 *                
 * @LICENSE@
 *                
 * $Id: tracepoints.h,v 1.5 2001/11/22 14:56:35 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __TRACEPOINTS_H__
#define __TRACEPOINTS_H__

#include <tracebuffer.h>


/*
 * Create an enum list of all tracepoints.
 */

#define DEFINE_TP(x, str)	TP_##x,
enum tracepoint_t {
    TP_INVALID = 0,
#   include <tracepoint_list.h>    
    TP_MAX
};
#undef DEFINE_TP


#if defined(CONFIG_ENABLE_TRACEPOINTS)


/*
 * First argument to TRACEPOINT() should be the tracepoint name.
 * Second argument (if present) is code to be executed before kdebug
 * is (possibly) entered.
 */

#define TRACEPOINT(x, code...)				\
do {							\
    TBUF_RECORD_EVENT (TP_##x);                         \
    tracepoint_cnt[TP_##x]++;				\
    if (is_tracepoint_set (TP_##x))			\
    {							\
	{code;}						\
	if (does_tracepoint_enter_kdb (TP_##x))		\
	    enter_kdebug (#x);				\
    }							\
} while (0)

/*
 * The following versions of TRACEPOINT() take additional parameters,
 * which can be recorded into the tracebuffer.
 */
 
#define TRACEPOINT_1PAR(x, par1, code...)		\
do {							\
    TBUF_RECORD_EVENT_AND_PAR (TP_##x, par1);		\
    tracepoint_cnt[TP_##x]++;				\
    if (is_tracepoint_set (TP_##x))			\
    {							\
	{code;}						\
	if (does_tracepoint_enter_kdb (TP_##x)) 	\
	    enter_kdebug (#x);				\
    }							\
} while (0)

#define TRACEPOINT_2PAR(x, par1, par2, code...)		\
do {							\
    TBUF_RECORD_EVENT_AND_TWO_PAR (TP_##x, par1, par2);	\
    tracepoint_cnt[TP_##x]++;				\
    if (is_tracepoint_set (TP_##x))			\
    {							\
	{code;}						\
	if (does_tracepoint_enter_kdb (TP_##x)) 	\
	    enter_kdebug (#x);				\
    }							\
} while (0)


extern char *tracepoint_name[];
extern byte_t tracepoint_is_set[];
extern byte_t tracepoint_enters_kdb[];
extern dword_t tracepoint_cnt[];



/*
 * Helper functions.
 */

INLINE void clr_tracepoint (int num)
{
    tracepoint_is_set[num >> 3] &= ~(1 << (num & 0x7));
    tracepoint_enters_kdb[num >> 3] &= ~(1 << (num & 0x7));
}

INLINE void set_tracepoint (int num, int enter_kdb)
{
    tracepoint_is_set[num >> 3] |= 1 << (num & 0x7);
    if ( enter_kdb )
	tracepoint_enters_kdb[num >> 3] |= 1 << (num & 0x7);
    else
	tracepoint_enters_kdb[num >> 3] &= ~(1 << (num & 0x7));
}

INLINE int is_tracepoint_set (int num)
{
    return tracepoint_is_set[num >> 3] & (1 << (num & 0x7));
}

INLINE int does_tracepoint_enter_kdb (int num)
{
    return tracepoint_enters_kdb[num >> 3] & (1 << (num & 0x7));
}




#else /* !CONFIG_ENABLE_TRACEPOINTS */

#define TRACEPOINT(x, code...)			\
do {						\
    TBUF_RECORD_EVENT (TP_##x);			\
} while (0)  
   
#define TRACEPOINT_1PAR(x, par1, code...)	\
do {						\
    TBUF_RECORD_EVENT_AND_PAR (TP_##x, par1);	\
} while (0)  

#define TRACEPOINT_2PAR(x, par1, par2, code...)		\
do {							\
    TBUF_RECORD_EVENT_AND_TWO_PAR (TP_##x, par1, par2);	\
} while (0)  

#endif

#endif /* !__TRACEPOINTS_H__ */
