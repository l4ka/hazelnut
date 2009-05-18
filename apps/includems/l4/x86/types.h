/**********************************************************************
 * (c) 1999, 2000 by University of Karlsruhe and Dresden University
 *
 * filename: l4/x86/types.h
 * 
 */

#ifndef __L4_X86_TYPES_H__ 
#define __L4_X86_TYPES_H__ 

/* 
 * L4 unique identifiers 
 */
typedef l4_threadid_t l4_taskid_t;
typedef qword_t cpu_time_t;


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


#if 0
L4_INLINE l4_fpage_t l4_fpage(unsigned long address, unsigned int size, 
				  unsigned char write, unsigned char grant)
{
  return ((l4_fpage_t){fp:{grant, write, size, 0, 
			     (address & L4_PAGEMASK) >> 12 }});
}
#endif

#endif /* __L4_TYPESL4X_H__ */ 


