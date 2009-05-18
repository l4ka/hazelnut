/* 
 * $Id: syscalls.h,v 1.1 2000/07/18 13:13:38 voelp Exp $
 */

#ifndef __L4_SYSCALLS_H__ 
#define __L4_SYSCALLS_H__ 

#include <l4/compiler.h>
#include <l4/types.h>
#include <l4/kdebug.h>

#define L4_FP_REMAP_PAGE	0x00	/* Page is set to read only */
#define L4_FP_FLUSH_PAGE	0x02	/* Page is flushed completly */
#define L4_FP_OTHER_SPACES	0x00	/* Page is flushed in all other */
					/* address spaces */
#define L4_FP_ALL_SPACES	0x80000000U
					/* Page is flushed in own address */ 
					/* space too */

#define L4_NC_SAME_CLAN		0x00	/* destination resides within the */
					/* same clan */
#define L4_NC_INNER_CLAN	0x0C	/* destination is in an inner clan */
#define L4_NC_OUTER_CLAN	0x04	/* destination is outside the */
					/* invoker's clan */

#define L4_CT_LIMITED_IO	0
#define L4_CT_UNLIMITED_IO	1
#define L4_CT_DI_FORBIDDEN	0
#define L4_CT_DI_ALLOWED	1

/*
 * prototypes
 */
L4_INLINE void 
l4_fpage_unmap(l4_fpage_t fpage, dword_t map_mask);

L4_INLINE l4_threadid_t 
l4_myself(void);

L4_INLINE int 
l4_nchief(l4_threadid_t destination, l4_threadid_t *next_chief);

L4_INLINE void
l4_thread_ex_regs(l4_threadid_t destination, dword_t eip, dword_t esp,
		  l4_threadid_t *preempter, l4_threadid_t *pager,
		  dword_t *old_eflags, dword_t *old_eip, dword_t *old_esp);
L4_INLINE void
l4_thread_switch(l4_threadid_t destination);

L4_INLINE cpu_time_t
l4_thread_schedule(l4_threadid_t dest, l4_sched_param_t param,
		   l4_threadid_t *ext_preempter, l4_threadid_t *partner,
		   l4_sched_param_t *old_param);

L4_INLINE l4_taskid_t 
l4_task_new(l4_taskid_t destination, dword_t mcp_or_new_chief, 
	    dword_t esp, dword_t eip, l4_threadid_t pager);


/*
 * Implementation
 */

/*
 * L4 flex page unmap
 */

L4_INLINE void
l4_fpage_unmap(l4_fpage_t fpage, dword_t map_mask)
{
  __asm__
    __volatile__(
#ifdef __pic__
		 "pushl %%ebx   \n\t"
#endif
		 "pushl	%%ebp	\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
		 "int	$0x32	\n\t"
		 "popl	%%ebp	\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
#ifdef __pic__
		 "popl  %%ebx   \n\t"
#endif

		 : 
		 /* No output */
		 : 
		 "a" (fpage),
		 "c" (map_mask)
		 :
#ifndef __pic__
		 "ebx", 
#endif
		 "edx", "edi", "esi", "eax", "ecx"
		 );
};

/*
 * L4 id myself
 */

L4_INLINE l4_threadid_t
l4_myself(void)
{
  l4_threadid_t temp_id;

  __asm__(
#ifdef __pic__
	  "pushl %%ebx   \n\t"
#endif
	  "pushl	%%ebp	\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
	  "int	$0x31		\n\t"
	  "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
#ifdef __pic__
	  "popl  %%ebx   \n\t"
#endif
	  : 
	  "=S" (temp_id.lh.low),	/* ESI, 0 */
	  "=D" (temp_id.lh.high) 	/* EDI, 1 */
	  : 
	  "0" (0)			/* ESI, nil id (id.low = 0) */
	  :
#ifndef __pic__
	  "ebx", 
#endif
	  "eax", "ecx", "edx"
	  );
  return temp_id;
}

/*
 * L4 id next chief
 */


L4_INLINE int 
l4_nchief(l4_threadid_t destination, l4_threadid_t *next_chief)
{
  int type;
  __asm__(
#ifdef __pic__
	  "pushl %%ebx   \n\t"
#endif
	  "pushl	%%ebp	\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
	  "int	$0x31		\n\t"
	  "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
#ifdef __pic__
	  "popl  %%ebx   \n\t"
#endif
	  : 
	  "=S" (next_chief->lh.low),	/* ESI, 0 */
	  "=D" (next_chief->lh.high),	/* EDI, 1 */
	  "=a" (type)			/* EAX, 2 */
	  : 
	  "0" (destination.lh.low),	/* ESI */
	  "1" (destination.lh.high) 	/* EDI */
	  :
#ifndef __pic__
	  "ebx", 
#endif
	  "ecx", "edx"
	  );
  return type;
}

/*
 * L4 lthread_ex_regs
 */
L4_INLINE void
l4_thread_ex_regs(l4_threadid_t destination, dword_t eip, dword_t esp,
		  l4_threadid_t *preempter, l4_threadid_t *pager,
		  dword_t *old_eflags, dword_t *old_eip, dword_t *old_esp)
{
  __asm__ __volatile__(
#ifdef __pic__
	  "pushl        %%ebx   \n\t"
	  "movl	%%edi, %%ebx	\n\t"
#endif
	  "pushl	%%ebp	\n\t"	/* save ebp, no memory 
					   references ("m") after 
					   this point */
	  "pushl	%%ebx	\n\t"	/* save address of preempter */
	  
	  "movl	4(%%esi), %%edi	\n\t"	/* load new pager id */	
	  "movl	(%%esi), %%esi	\n\t"

	  "movl	4(%%ebx), %%ebp	\n\t"	/* load new preempter id */
	  "movl	(%%ebx), %%ebx	\n\t"

	  "int	$0x35		\n\t"	/* execute system call */

	  "xchgl (%%esp), %%ebx	\n\t"	/* save old preempter.lh.low
					   and get address of preempter */
	  "movl	%%ebp, 4(%%ebx)	\n\t"	/* write preempter.lh.high */
	  "popl	%%ebp		\n\t"	/* get preempter.lh.low */
	  "movl	%%ebp, (%%ebx)	\n\t"	/* write preempter.lh.low */
	  "popl	%%ebp		\n\t"	/* restore ebp, no memory 
					   references ("m") before 
					   this point */
#ifdef __pic__
	  "popl %%ebx		\n\t"
#endif
	  :
	  "=a" (*old_eflags),		/* EAX, 0 */
	  "=c" (*old_esp),		/* ECX, 1 */
	  "=d" (*old_eip),		/* EDX, 2 */
	  "=S" (pager->lh.low),		/* ESI, 3 */
	  "=D" (pager->lh.high)		/* EDI, 4 */
	  :
	  "0" (destination.id.lthread),
	  "1" (esp),
	  "2" (eip),
	  "3" (pager),	 		/* ESI */
#ifdef __pic__
	  "4" (preempter)		/* EDI */
#else
	  "b" (preempter)		/* EBX, 5 */
#endif
#ifndef __pic__
	  : 
	  "ebx"
#endif
	  );
}


/*
 * L4 thread switch
 */

L4_INLINE void
l4_thread_switch(l4_threadid_t destination)
{
  __asm__
    __volatile__(
#ifdef __pic__
		 "pushl %%ebp	\n\t"
#endif
		 "pushl	%%ebp	\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
		 "int	$0x33	\n\t"
		 "popl	%%ebp	\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
#ifdef __pic__
		 "popl %%ebp	\n\t"
#endif
		 : 
		 /* No output */
		 : 
		 "S" (destination.lh.low)
		 :  
#ifndef __pic__
		 "ebx",
#endif
		 "eax", "ecx", "edx", "edi", "esi"
		 );
}

/*
 * L4 thread schedule
 */

L4_INLINE cpu_time_t
l4_thread_schedule(l4_threadid_t dest, l4_sched_param_t param,
		   l4_threadid_t *ext_preempter, l4_threadid_t *partner,
		   l4_sched_param_t *old_param)
{
  cpu_time_t temp;

  __asm__
    __volatile__(
#ifdef __pic__
		 "pushl        %%ebx	\n\t"
		 "movl	%%edx, %%ebx	\n\t"
#endif
		 "pushl %%ebp		\n\t"	/* save ebp, no memory 
						   references ("m") after 
						   this point */
		 "pushl	%%ebx		\n\t"	/* save address of preempter */
		 "movl 	4(%%ebx), %%ebp	\n\t"	/* load preempter id */
		 "movl 	(%%ebx), %%ebx	\n\t"
/* 		 asm_enter_kdebug("before calling thread_schedule") */
		 "int	$0x34		\n\t"
/* 		 asm_enter_kdebug("after calling thread_schedule") */
		 "xchgl	(%%esp), %%ebx	\n\t"	/* save old preempter.lh.low
						   and get address of 
						   preempter */
		 "movl	%%ebp, 4(%%ebx)	\n\t"	/* write preempter.lh.high */
		 "popl	%%ebp		\n\t"	/* get preempter.lh.high */
		 "movl	%%ebp, (%%ebx)	\n\t"	/* write preempter.lh.high */
		 "popl	%%ebp		\n\t"	/* restore ebp, no memory 
						   references ("m") before 
						   this point */
#ifdef __pic__
		 "popl        %%ebx	\n\t"
#endif

		 : 
		 "=a" (*old_param), 			/* EAX, 0 */
		 "=c" (((l4_low_high_t *)&temp)->low),	/* ECX, 1 */
		 "=d" (((l4_low_high_t *)&temp)->high),	/* EDX, 2 */
		 "=S" (partner->lh.low),		/* ESI, 3 */
		 "=D" (partner->lh.high)		/* EDI, 4 */
		 : 
#ifdef __pic__
		 "2" (ext_preempter),   /* EDX, 2 */
#else
		 "b" (ext_preempter), 	/* EBX, 5 */
#endif
		 "0" (param),		/* EAX */
		 "3" (dest.lh.low),	/* ESI */
		 "4" (dest.lh.high)	/* EDI */
#ifndef __pic__
		 :
		 "ebx"
#endif
		 );
  return temp;
}

  
  
/*
 * L4 task new
 */
#ifndef __pic__
L4_INLINE l4_taskid_t 
l4_task_new(l4_taskid_t destination, dword_t mcp_or_new_chief, 
	    dword_t esp, dword_t eip, l4_threadid_t pager)
{
  l4_taskid_t temp_id;
  __asm__
    __volatile__(
		 "pushl	%%ebp		\n\t"	/* save ebp, no memory 
						   references ("m") after 
						   this point */
		 "movl  4(%%ebx), %%ebp	\n\t"	/* load pager id */
		 "movl  (%%ebx), %%ebx	\n\t"
		 "int	$0x36		\n\t"
		 "popl	%%ebp		\n\t"	/* restore ebp, no memory 
						   references ("m") before 
						   this point */
		 : 
		 "=S" (temp_id.lh.low),		/* ESI, 0 */
		 "=D" (temp_id.lh.high)		/* EDI, 1 */
		 :
		 "b" (&pager),			/* EBX, 2 */
		 "a" (mcp_or_new_chief),	/* EAX, 3 */
		 "c" (esp),			/* ECX, 4 */
		 "d" (eip),			/* EDX, 5 */
		 "0" (destination.lh.low),	/* ESI */
		 "1" (destination.lh.high)	/* EDI */
		 :
		 "eax", "ebx", "ecx", "edx"
		 );
  return temp_id;
}

#else /* __pic__ */

L4_INLINE l4_taskid_t 
l4_task_new(l4_taskid_t destination, dword_t mcp_or_new_chief, 
	    dword_t esp, dword_t eip, l4_threadid_t pager)
{
  l4_taskid_t temp_id;
  __asm__
    __volatile__(
		 "pushl %%ebx		\n\t"
		 "pushl	%%ebp		\n\t"	/* save ebp, no memory 
						   references ("m") after 
						   this point */
		 "movl  4(%%esi), %%edi \n\t"
		 "movl  (%%esi), %%esi	\n\t"

		 "movl  4(%%edi), %%ebp	\n\t"	/* load pager id */
		 "movl  (%%edi), %%ebx	\n\t"
		 "int	$0x36		\n\t"
		 "popl	%%ebp		\n\t"	/* restore ebp, no memory 
						   references ("m") before 
						   this point */
		 "popl	%%ebx		\n\t"
		 : 
		 "=S" (temp_id.lh.low),		/* ESI, 0 */
		 "=D" (temp_id.lh.high)		/* EDI, 1 */
		 :
		 "a" (mcp_or_new_chief),	/* EAX, 2 */
		 "c" (esp),			/* ECX, 3 */
		 "d" (eip),			/* EDX, 4 */
		 "0" (destination),		/* ESI */
		 "1" (&pager)			/* EDI  */
		 :
		 "eax", "ecx", "edx"
		 );
  return temp_id;
}
#endif /* __pic__ */

#endif






