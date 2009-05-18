 /* 
 * $Id: x-types.h,v 1.1 2000/07/18 13:13:38 voelp Exp $
 */

#ifndef __LN_TYPES_H__ 
#define __LN_TYPES_H__ 

#include <l4/compiler.h>

typedef unsigned char       byte_t;
typedef unsigned short int  word_t;
typedef unsigned int        dword_t;
typedef unsigned long long  qword_t;


typedef signed char         sbyte_t;
typedef signed short int    sword_t;
typedef signed int          sdword_t;
typedef signed long long    sqword_t;

typedef long long	    cpu_time_t;

/* 
 * LN unique identifiers 
 */

typedef struct {
  unsigned version____:10;
  unsigned lthread____:6;
  unsigned task____:8;
  unsigned chief____:8;
} Struct_LnThreadid;

typedef union {
  dword_t lh;
  Struct_LnThreadid;
} LnThread;

typedef LnThread LnTask;

typedef struct {
  unsigned intr:8;
  unsigned char zero[3];
} Struct_LnIntrid;

typedef union {
  dword_t lh;
  Struct_LnIntrid id;
} LnIntr;

#define Nil_LnThread 		((LnThread){lh:{0}})
#define Invalid_LnThread		((LnThread){lh:{0xffffffff}})

/*
 * L4 flex pages
 */

typedef struct {
  unsigned grant:1;
  unsigned write:1;
  unsigned size:6;
  unsigned zero:4;
  unsigned page:20;
} l4_fpage_struct_t;

typedef union {
  dword_t fpage;
  l4_fpage_struct_t fp;
} l4_fpage_t;

#define L4_PAGESIZE 	(0x1000)
#define L4_PAGEMASK	(~(L4_PAGESIZE - 1))
#define L4_LOG2_PAGESIZE (12)
#define L4_SUPERPAGESIZE (0x400000)
#define L4_SUPERPAGEMASK (~(L4_SUPERPAGESIZE - 1))
#define L4_LOG2_SUPERPAGESIZE (22)
#define L4_WHOLE_ADDRESS_SPACE (32)
#define L4_FPAGE_RO	0
#define L4_FPAGE_RW	1
#define L4_FPAGE_MAP	0
#define L4_FPAGE_GRANT	1

typedef struct {
  dword_t snd_base;
  l4_fpage_t fpage;
} l4_snd_fpage_t;

/*
 * L4 message dopes
 */

typedef struct {
  unsigned msg_deceited:1;
  unsigned fpage_received:1;
  unsigned msg_redirected:1;
  unsigned src_inside:1;
  unsigned snd_error:1;
  unsigned error_code:3;
  unsigned strings:5;
  unsigned dwords:19;
} l4_msgdope_struct_t;

typedef union {
  dword_t msgdope;
  l4_msgdope_struct_t md;
} l4_msgdope_t;

/*
 * L4 string dopes
 */

typedef struct {
  dword_t snd_size;
  dword_t snd_str;
  dword_t rcv_size;
  dword_t rcv_str;
} l4_strdope_t;

/*
 * L4 timeouts
 */

typedef struct {
  unsigned rcv_exp:4;
  unsigned snd_exp:4;
  unsigned rcv_pfault:4;
  unsigned snd_pfault:4;
  unsigned snd_man:8;
  unsigned rcv_man:8;
} l4_timeout_struct_t;

typedef union {
  dword_t timeout;
  l4_timeout_struct_t to;
} l4_timeout_t;

/*
 * l4_schedule param word
 */

typedef struct {
  unsigned prio:8;
  unsigned small:8;
  unsigned zero:4;
  unsigned time_exp:4;
  unsigned time_man:8;
} l4_sched_param_struct_t;

typedef union {
  dword_t sched_param;
  l4_sched_param_struct_t sp;
} l4_sched_param_t;

#define L4_INVALID_SCHED_PARAM ((l4_sched_param_t){sched_param:-1})
#define L4_SMALL_SPACE(size_mb, nr) ((size_mb >> 2) + nr * (size_mb >> 1))

/*
 * Some useful operations and test functions for id's and types
 */

L4_INLINE int l4_is_invalid_sched_param(l4_sched_param_t sp);
L4_INLINE int l4_is_nil_id(l4_threadid_t id);
L4_INLINE int l4_is_invalid_id(l4_threadid_t id);
L4_INLINE l4_fpage_t l4_fpage(unsigned long address, unsigned int size, 
				  unsigned char write, unsigned char grant);
L4_INLINE l4_threadid_t get_taskid(l4_threadid_t t);
L4_INLINE int thread_equal(l4_threadid_t t1,l4_threadid_t t2);
L4_INLINE int task_equal(l4_threadid_t t1,l4_threadid_t t2);

L4_INLINE bool Equal_LnThread (LnThread l,r);
L4_INLINE bool IsNil_LnThread (LnThread t);
L4_INLINE bool IsInvalid_LnThread (LnThread t);
L4_INLINE int TaskNo_LnThread (LnThread t);
L4_INLINE int ThreadNo_LnThread (LnThread t);
L4_INLINE int LThreadNo_LnThread (LnThread t);
L4_INLINE int ChiefNo_LnThread (LnThread t);
L4_INLINE int Version_LnThread (LnThread t);
L4_INLINE SetTaskNo_LnThread (LnThread t, int i);
L4_INLINE SetThreadNo_LnThread (LnThread t, int i);
L4_INLINE SetLThreadNo_LnThread (LnThread t, int i);
L4_INLINE SetChiefNo_LnThread (LnThread t, int i);
L4_INLINE SetVersion_LnThread (LnThread t, int i);


L4_INLINE bool Equal_LnThread (LnThread l,r)
{
  l==r
}

L4_INLINE bool IsNil_LnThread (LnThread t)
{
  return Equal_LnThread (t, Nil_LnThread)
}

L4_INLINE bool IsInvalid_LnThread (LnThread t)
{
  return Equal_LnThread (t, Invalid_LnThread)
}

L4_INLINE int TaskNo_LnThread (LnThread t)
{
  return t.task____
}

L4_INLINE int ThreadNo_LnThread (LnThread t)
{
  return t.lthread____
}

L4_INLINE int LThreadNo_LnThread (LnThread t);
L4_INLINE int ChiefNo_LnThread (LnThread t);
L4_INLINE int Version_LnThread (LnThread t);
L4_INLINE SetTaskNo_LnThread (LnThread t, int i);
L4_INLINE SetThreadNo_LnThread (LnThread t, int i);
L4_INLINE SetLThreadNo_LnThread (LnThread t, int i);
L4_INLINE SetChiefNo_LnThread (LnThread t, int i);
L4_INLINE SetVersion_LnThread (LnThread t, int i);






L4_INLINE int l4_is_invalid_sched_param(l4_sched_param_t sp)
{
  return sp.sched_param == 0xffffffff;
}
L4_INLINE int l4_is_nil_id(l4_threadid_t id)
{
  return id.lh.low == 0;
}
L4_INLINE int l4_is_invalid_id(l4_threadid_t id)
{
  return id.lh.low == 0xffffffff;
}













L4_INLINE l4_fpage_t l4_fpage(unsigned long address, unsigned int size, 
				  unsigned char write, unsigned char grant)
{
  return ((l4_fpage_t){fp:{grant, write, size, 0, 
			     (address & L4_PAGEMASK) >> 12 }});
}

L4_INLINE l4_threadid_t 
get_taskid(l4_threadid_t t)
{
  t.id.lthread = 0;
  return t; 
}

L4_INLINE int
thread_equal(l4_threadid_t t1,l4_threadid_t t2)
{
  if((t1.lh.low != t2.lh.low) || (t1.lh.high != t2.lh.high))
    return 0;
  return 1;
}

#define TASK_MASK 0xfffe03ff
L4_INLINE int
task_equal(l4_threadid_t t1,l4_threadid_t t2)
{
  if ( ((t1.lh.low & TASK_MASK) == (t2.lh.low & TASK_MASK)) && 
       (t1.lh.high == t2.lh.high) )
    return 1;
  else
    return 0;
  
/*   t1.id.lthread = 0; */
/*   t2.id.lthread = 0; */

/*   if((t1.lh.low != t2.lh.low) || (t1.lh.high != t2.lh.high)) */
/*     return 0; */
/*   return 1; */

}

#endif /* __L4TYPES_H__ */ 


