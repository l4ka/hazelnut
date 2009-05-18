 /* 
 * $Id: types.h,v 1.2 2000/04/18 21:35:53 ud3 Exp $
 */

#ifndef __L4_TYPES_H__ 
#define __L4_TYPES_H__ 

#if defined(__L4_STANDARD__)
# error L4 version clash
#endif


typedef unsigned char       byte_t;
typedef unsigned short int  word_t;
typedef unsigned int        dword_t;
typedef unsigned long long  qword_t;


typedef signed char         sbyte_t;
typedef signed short int    sword_t;
typedef signed int          sdword_t;
typedef signed long long    sqword_t;

typedef long long	    cpu_time_t;

typedef union {
  dword_t low, high;
} l4_low_high_t;

/* 
 * L4 unique identifiers 
 */

typedef struct {
  unsigned version_low:10;
  unsigned lthread:6;
  unsigned task:8;
  unsigned chief:8;
} l4_threadid_struct_t;

typedef union {
  l4_low_high_t lh;
  dword_t dw;
  dword_t raw;
  l4_threadid_struct_t id;
} l4_threadid_t;

typedef l4_threadid_t l4_taskid_t;

typedef struct {
  unsigned intr:8;
  unsigned char zero[3];
} l4_intrid_struct_t;

typedef union {
  dword_t dw;
  l4_intrid_struct_t id;
} l4_intrid_t;

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

#define L4_NIL_ID 		((l4_threadid_t){dw:0})
#define L4_INVALID_ID		((l4_threadid_t){dw:0xffffffff})

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

L4_INLINE int l4_is_invalid_sched_param(l4_sched_param_t sp)
{
  return sp.sched_param == 0xffffffff;
}

L4_INLINE int l4_is_nil_id(l4_threadid_t id)
{
  return id.raw == 0;
}

L4_INLINE int l4_is_invalid_id(l4_threadid_t id)
{
  return id.raw == 0xffffffff;
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
  return (t1.raw == t2.raw);
}

#define TASK_MASK 0xffff03ff
L4_INLINE int
task_equal(l4_threadid_t t1,l4_threadid_t t2)
{
    return (t1.raw & TASK_MASK) == (t2.raw & TASK_MASK); 
}

#endif /* __L4TYPES_H__ */ 
