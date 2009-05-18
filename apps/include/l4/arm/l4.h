/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     l4/arm/l4.h
 * Description:    Data types and syscall bindings for ARM.
 *                
 * @LICENSE@
 *                
 * $Id: l4.h,v 1.13 2002/10/01 04:37:27 ud3 Exp $
 *                
 ********************************************************************/
#ifndef __L4__ARM__L4_H__
#define __L4__ARM__L4_H__


#ifdef __cplusplus
extern "C" {
#endif

#if defined(L4_NO_SYSCALL_INLINES) || defined(__GENERATE_L4LIB__)
# define L4_INLINE
#else
# define L4_INLINE extern __inline__
#endif


/*
**
** Data type definitions.
**
*/

typedef qword_t cpu_time_t;

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

typedef struct {
    unsigned grant:1;
    unsigned write:1;
    unsigned size:6;
    unsigned zero:4;
    unsigned page:20;
} l4_fpage_struct_t;

typedef union {
    dword_t raw;
    l4_fpage_struct_t fp;
} l4_fpage_t;

typedef struct {
    dword_t snd_size;
    dword_t snd_str;
    dword_t rcv_size;
    dword_t rcv_str;
} l4_strdope_t;


#define L4_FP_REMAP_PAGE	0x00	/* Page is set to read only */
#define L4_FP_FLUSH_PAGE	0x02	/* Page is flushed completly */
#define L4_FP_OTHER_SPACES	0x00	/* Page is flushed in all other */
					/* address spaces */
#define L4_FP_ALL_SPACES	0x80000000U
#define L4_FP_ADDRMASK		0xfffff000U
#define L4_WHOLE_ADDRESS_SPACE	(32)


#define L4_IPC_OPEN_IPC 	1



/*
**
** Prototypes.
**
*/

L4_INLINE l4_threadid_t l4_myself         (void);
L4_INLINE int           l4_nchief         (l4_threadid_t	destination,
					   l4_threadid_t	*next_chief);
L4_INLINE void          l4_fpage_unmap    (l4_fpage_t		fpage,
					   dword_t		mask);
L4_INLINE l4_fpage_t    l4_fpage          (dword_t		address,
					   dword_t		size, 
					   dword_t		write,
					   dword_t		grant);
L4_INLINE void          l4_thread_switch  (l4_threadid_t	destintaion);
L4_INLINE void          l4_yield          (void);
L4_INLINE void          l4_thread_ex_regs (l4_threadid_t	destination,
					   dword_t		ip,
					   dword_t		sp,
#if defined(CONFIG_VERSION_X0)
					   l4_threadid_t	*preempter, 
#endif
					   l4_threadid_t	*pager, 
					   dword_t		*old_ip,
					   dword_t		*old_sp,
					   dword_t		*old_cpsr);
#if defined(CONFIG_VERSION_X0)
L4_INLINE void		l4_task_new       (l4_threadid_t	dest,
					   dword_t		mcp,
					   dword_t		usp,
					   dword_t		uip,
					   l4_threadid_t	pager);
#elif defined(CONFIG_VERSION_X1)
L4_INLINE void		l4_create_thread  (l4_threadid_t	tid,
					   l4_threadid_t	master,
					   l4_threadid_t	pager,
					   dword_t		uip,
					   dword_t		usp);
#endif
L4_INLINE cpu_time_t    l4_thread_schedule(l4_threadid_t	dest,
					   l4_sched_param_t	param,
					   l4_threadid_t	*ext_preempter,
					   l4_threadid_t	*partner,
					   l4_sched_param_t	*old_param);




/*
**
** System call identification numbers (these must match the ones defined
** within the kernel).
**
*/

#ifndef L4_NO_SYSCALL_INLINES

#ifdef EXCEPTION_VECTOR_RELOCATED

#define L4_SYSCALL_MAGIC_OFFSET 	8
#define L4_SYSCALL_IPC			(-0x00000004-L4_SYSCALL_MAGIC_OFFSET)
#define L4_SYSCALL_ID_NEAREST		(-0x00000008-L4_SYSCALL_MAGIC_OFFSET)
#define L4_SYSCALL_FPAGE_UNMAP		(-0x0000000C-L4_SYSCALL_MAGIC_OFFSET)
#define L4_SYSCALL_THREAD_SWITCH	(-0x00000010-L4_SYSCALL_MAGIC_OFFSET)
#define L4_SYSCALL_THREAD_SCHEDULE	(-0x00000014-L4_SYSCALL_MAGIC_OFFSET)
#define L4_SYSCALL_LTHREAD_EX_REGS	(-0x00000018-L4_SYSCALL_MAGIC_OFFSET)
#define L4_SYSCALL_CREATE_THREAD	(-0x0000001C-L4_SYSCALL_MAGIC_OFFSET)
#define L4_SYSCALL_ENTER_KDEBUG 	(-0x00000020-L4_SYSCALL_MAGIC_OFFSET)

#else

#define L4_SYSCALL_IPC			0x00000004
#define L4_SYSCALL_ID_NEAREST		0x00000008
#define L4_SYSCALL_FPAGE_UNMAP		0x0000000C
#define L4_SYSCALL_THREAD_SWITCH	0x00000010
#define L4_SYSCALL_THREAD_SCHEDULE	0x00000014
#define L4_SYSCALL_LTHREAD_EX_REGS	0x00000018
#define L4_SYSCALL_CREATE_THREAD	0x0000001C
#define L4_SYSCALL_ENTER_KDEBUG		0x00000020

#endif /* !EXCEPTION_VECTOR_RELOCATED */

#define IPC_SYSENTER				\
	"	mov	lr, pc		\n"	\
	"	mov	pc, %0		\n"




/*
**
** Kdebug stuff.
**
*/

#define enter_kdebug(text...)			\
__asm__ __volatile__ (				\
    "	mov	lr, pc		\n"		\
    "	mov	pc, %0		\n"		\
    "	b	1f		\n"		\
    "	.ascii	\"" text "\"	\n"		\
    "	.byte	0		\n"		\
    "	.align	2		\n"		\
    "1:				\n"		\
    :						\
    : "i" (L4_SYSCALL_ENTER_KDEBUG)		\
    : "lr")


L4_INLINE void outstring(const char* x)
{
    __asm__ __volatile__ (
	"	mov	r0, %1		\n"
	"	mov	lr, pc		\n"
	"	mov	pc, %0		\n"
	"	cmp	lr, #2		\n"
	:
	: "i" (L4_SYSCALL_ENTER_KDEBUG), "r"(x)
	: "r0", "lr");
}

L4_INLINE void outdec(const dword_t x)
{
}

L4_INLINE void kd_display(const char* x)
{
}

L4_INLINE char kd_inchar()
{
    char c;
    __asm__ __volatile__ (
	"	mov	lr, pc		\n"
	"	mov	pc, %1		\n"
	"	cmp	lr, #13		\n"
	"	mov	%0, r0		\n"
	: "=r" (c)
	: "i" (L4_SYSCALL_ENTER_KDEBUG)	
	: "r0", "lr");
    return c;
}


/*
**
** Syscall binding implementation.
**
*/

L4_INLINE l4_threadid_t l4_myself(void)
{
    l4_threadid_t id;

    __asm__ __volatile__ (
	"	mov	r0, %2					\n"
	"	mov	lr, pc					\n"
	"	mov	pc, %1					\n"
	"	mov	%0, r0					\n"
	:"=r" (id)
	:"i" (L4_SYSCALL_ID_NEAREST),
	 "i" (L4_NIL_ID.raw)
	:"r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
         "r10", "r11", "r12", "r14", "memory");

    return id;
}



L4_INLINE void l4_fpage_unmap(l4_fpage_t fpage, dword_t mask)
{
    __asm__ __volatile__ (
	"	mov	r0, %1		@ l4_fpage_unmap	\n"
	"	mov	r1, %2					\n"
	"	mov	pc, %0					\n"
	:
	:"i" (L4_SYSCALL_FPAGE_UNMAP),
	 "ri" (fpage.raw),
	 "ri" (mask)
	:"r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
	 "r10", "r11", "r12", "r14", "memory");
}



L4_INLINE l4_fpage_t l4_fpage(dword_t address,
			      dword_t size, 
			      dword_t write,
			      dword_t grant)
{
    return ((l4_fpage_t){fp:{grant, write, size, 0, 
				 (address & L4_FP_ADDRMASK) >> 12 }});
}



L4_INLINE void l4_thread_switch(l4_threadid_t dest)
{
    __asm__ __volatile__ (
	"	mov	r0, %1		@ l4_thread_switch	\n"
	"	mov	lr, pc					\n"
	"	mov	pc, %0					\n"
	: 
	:"i" (L4_SYSCALL_THREAD_SWITCH),
	 "r" (dest)
	:"r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
	 "r10", "r11", "r12", "r14", "memory");
}



L4_INLINE void l4_yield(void)
{
    l4_thread_switch(L4_NIL_ID);
}



L4_INLINE void l4_thread_ex_regs(l4_threadid_t destination,
				 dword_t ip,
				 dword_t sp,	
#if defined(CONFIG_VERSION_X0)
				 l4_threadid_t *preempter,
#endif
				 l4_threadid_t *pager,
				 dword_t *old_ip,
				 dword_t *old_sp, 
				 dword_t *old_cpsr)
{
    struct {
	l4_threadid_t tid;
	dword_t ip;
	dword_t sp;
	l4_threadid_t pager;
	dword_t flags;
    } x = { destination, ip, sp, *pager, 0 };

#if defined(CONFIG_VERSION_X0)
    x.tid.raw = destination.id.thread;
#endif

    __asm__ __volatile__ (
	"	ldr	r0, %1		@ l4_lthread_ex_regs	\n"
	"	ldr	r1, %2					\n"
	"	ldr	r2, %3					\n"
	"	ldr	r3, %4					\n"

	"	mov	lr, pc					\n"
	"	mov	pc, %0					\n"

	"	str	r0, %1					\n"
	"	str	r1, %2					\n"
	"	str	r2, %3					\n"
	"	str	r3, %4					\n"
	"	str	r4, %5					\n"

	:
	:
	"i" (L4_SYSCALL_LTHREAD_EX_REGS),
	"m" (x.tid),
	"m" (x.ip),
	"m" (x.sp),
	"m" (x.pager),
	"m" (x.flags)
	:"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
	"r10", "r11", "r12", "r14", "memory");

    *pager = x.pager;
    *old_ip = x.ip;
    *old_sp = x.sp;
    *old_cpsr = x.flags;
}

#if defined(CONFIG_VERSION_X0)

L4_INLINE void l4_task_new(l4_threadid_t dest,
			   dword_t mcp,
			   dword_t usp,
			   dword_t uip,
			   l4_threadid_t pager)
{
    dword_t x[] = {dest.raw, mcp, pager.raw, uip, usp};

    __asm__ __volatile__ ("			\n"
	"	/* l4_task_new() */		\n"
	"	ldr	r0, %1			\n"
	"	ldr	r1, %2			\n"
	"	ldr	r2, %3			\n"
	"	ldr	r3, %4			\n"
	"	ldr	r4, %5			\n"
	"	mov	lr, pc			\n"
	"	mov	pc, %0			\n"
	:
	:"i" (L4_SYSCALL_CREATE_THREAD),
	 "m" (x[0]), "m" (x[1]), "m" (x[2]), "m" (x[3]), "m" (x[4])
	:"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
         "r10", "r11", "r12", "r14", "memory");
    
}

#elif defined(CONFIG_VERSION_X1)

L4_INLINE void l4_create_thread(l4_threadid_t tid,
				l4_threadid_t master,
				l4_threadid_t pager,
				dword_t uip,
				dword_t usp)
{
    dword_t x[] = {tid.raw, master.raw, pager.raw, uip, usp};

    __asm__ __volatile__ (
	"\n\t/* create_thread(start) */	\n"
	"	ldr	r0, %1		\n"
	"	ldr	r1, %2		\n"
	"	ldr	r2, %3		\n"
	"	ldr	r3, %4		\n"
	"	ldr	r4, %5		\n"
	"	mov	lr, pc		\n"
	"	mov	pc, %0		\n"
	"\t/* create_thread(end) */	\n"
	:
	:"i" (L4_SYSCALL_CREATE_THREAD),
	"m" (x[0]), "m" (x[1]), "m" (x[2]), "m" (x[3]), "m" (x[4])
	: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
	"r10", "r11", "r12", "r14", "memory");
    
}

#endif /* CONFIG_VERSION_X1 */

L4_INLINE cpu_time_t l4_thread_schedule(l4_threadid_t dest,
					l4_sched_param_t param,
					l4_threadid_t *ext_preempter,
					l4_threadid_t *partner,
					l4_sched_param_t *old_param)
{
    /* external preempter and ipc partner are ignored */
    __asm__ __volatile__ (
	"/* l4_thread_schedule */				\n"
	"	mov	r0, %4					\n"
	"	mov	r1, %3					\n"

	"	mov	lr, pc					\n"
	"	mov	pc, %2					\n"

	"	mov	r0, %1					\n"


	:"=r" (partner),
	 "=r" (*old_param)
	:"i" (L4_SYSCALL_THREAD_SCHEDULE),
	 "r" (dest),
	 "r" (param)
	:"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r10", "r11", "r12", "r14");

    return 0;
}



L4_INLINE int l4_ipc_wait(l4_threadid_t *src,
			  void *rcv_msg, 
			  dword_t *rcv_dword0,
			  dword_t *rcv_dword1,
			  dword_t *rcv_dword2,
			  l4_timeout_t timeout,
			  l4_msgdope_t *result)
{
    union {
	struct {
	    dword_t rcv;
	    l4_timeout_t timeout;
	} in;
	struct {
	    dword_t dw0;
	    dword_t dw1;
	    dword_t dw2;
	    l4_threadid_t src;
	    l4_msgdope_t result;
	} out;
    } x = { in: {((dword_t) rcv_msg  | L4_IPC_OPEN_IPC), timeout}};

    /* sys_ipc */
    asm volatile(
	"/* l4_ipc_wait(start) */	\n"
	"	ldr	r2, %1		\n"
	"	ldr	r3, %7		\n"
	"	mov	r1, #0xFFFFFFFF	\n"
	"	mov	lr, pc		\n"
	"	mov	pc, %0		\n"
	"	str	r0, %6		\n"
	"	str	r1, %2		\n"
	"	str	r4, %3		\n"
	"	str	r5, %4		\n"
	"	str	r6, %5		\n"
	"\t/* l4_ipc_wait(end) */	\n"
	:
	: "i" (L4_SYSCALL_IPC),
	"m" (x.in.rcv),
	"m" (x.out.src),
	"m" (x.out.dw0),
	"m" (x.out.dw1),
	"m" (x.out.dw2),
	"m" (x.out.result),
	"m" (x.in.timeout)
	: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
	 "r10", "r11", "r12", "r14", "memory");
    *rcv_dword0 = x.out.dw0;
    *rcv_dword1 = x.out.dw1;
    *rcv_dword2 = x.out.dw2;
    *result	= x.out.result;
    *src	= x.out.src;

    return result->md.error_code;
}



L4_INLINE int l4_ipc_receive(l4_threadid_t src,
			     void *rcv_msg, 
			     dword_t *rcv_dword0,
			     dword_t *rcv_dword1,
			     dword_t *rcv_dword2,
			     l4_timeout_t timeout,
			     l4_msgdope_t *result)
{
    union {
	struct {
	    l4_threadid_t src;
	    dword_t rcv;
	    l4_timeout_t timeout;
	} in;
	struct {
	    dword_t dw0;
	    dword_t dw1;
	    dword_t dw2;
	    l4_msgdope_t result;
	} out;
    } x = { in: {src, (dword_t) rcv_msg, timeout}};

    /* sys_ipc */
    asm volatile(
	"/* l4_ipc_receive(start) */	\n"
	"	ldr	r1, %2		\n"
	"	ldr	r2, %1		\n"
	"	ldr	r3, %7		\n"
	"	mov	r1, #0xFFFFFFFF	\n"
	"	mov	lr, pc		\n"
	"	mov	pc, %0		\n"
	"	str	r0, %6		\n"
	"	str	r1, %2		\n"
	"	str	r4, %3		\n"
	"	str	r5, %4		\n"
	"	str	r6, %5		\n"
	"\t/* l4_ipc_receive(end) */	\n"
	:
	: "i" (L4_SYSCALL_IPC),
	"m" (x.in.rcv),
	"m" (x.in.src),
	"m" (x.out.dw0),
	"m" (x.out.dw1),
	"m" (x.out.dw2),
	"m" (x.out.result),
	"m" (x.in.timeout)
	: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
	 "r10", "r11", "r12", "r14", "memory"
	);
    *rcv_dword0 = x.out.dw0;
    *rcv_dword1 = x.out.dw1;
    *rcv_dword2 = x.out.dw2;
    *result	= x.out.result;
#if defined(CONFIG_VERSION_X1)
    *src	= x.out.src;
#endif

    return result->md.error_code;
}



L4_INLINE int l4_ipc_send(l4_threadid_t dest, 
			  const void *snd_msg, 
			  dword_t snd_dword0,
			  dword_t snd_dword1, 
			  dword_t snd_dword2,
			  l4_timeout_t timeout,
			  l4_msgdope_t *result)
{
    struct
    {
	dword_t		tid;
	dword_t		snd_dsc;
	dword_t		timeout;
	dword_t		dw0;
	dword_t		dw1;
	dword_t		dw2;
	l4_msgdope_t	result;
    } x = {dest.raw, (dword_t) snd_msg, timeout.raw,
	   snd_dword0, snd_dword1, snd_dword2};
    
    __asm__ __volatile__ (
	"/* l4_ipc_send(start) */		\n"
	"	ldr	r0, %1			\n"
	"	ldr	r1, %2			\n"
	"	mov	r2, %3			\n"
	"	ldr	r4, %4			\n"
	"	ldr	r5, %5			\n"
	"	ldr	r6, %6			\n"
	"	mov	lr, pc			\n"
	"	mov	pc, %0			\n"
	"	str	r0, %7			\n"
	"\t/*l4_ipc_send(end) */		\n"
	:
	: "i" (L4_SYSCALL_IPC),
	"m" (x.tid),
	"m" (x.snd_dsc),
	"i" (~0U),
	"m" (x.dw0),
	"m" (x.dw1),
	"m" (x.dw2),
	"m" (x.result)
	:"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
	 "r10", "r11", "r12", "r14", "memory"
	);

    *result = x.result;

    return result->md.error_code;
}



L4_INLINE int l4_ipc_call(l4_threadid_t dest, 
			  const void *snd_msg, 
			  dword_t snd_dword0,
			  dword_t snd_dword1, 
			  dword_t snd_dword2,
			  void *rcv_msg,
			  dword_t *rcv_dword0, 
			  dword_t *rcv_dword1,
			  dword_t *rcv_dword2,
			  l4_timeout_t timeout,
			  l4_msgdope_t *result)
{
    struct
    {
	dword_t		tid;
	dword_t		snd_dsc;
	dword_t		rcv_dsc;
	dword_t		timeout;
	dword_t		dw0;
	dword_t		dw1;
	dword_t		dw2;
	l4_msgdope_t	result;
    } x = {dest.raw, (dword_t) snd_msg, (dword_t) rcv_msg, timeout.raw,
	   snd_dword0, snd_dword1, snd_dword2};
    
    __asm__ __volatile__ (
	"/* l4_ipc_call(start) */		\n"
	"	ldr	r0, %1			\n"
	"	ldr	r1, %2			\n"
	"	ldr	r2, %3			\n"
	"	ldr	r3, %8			\n"
	"	ldr	r4, %4			\n"
	"	ldr	r5, %5			\n"
	"	ldr	r6, %6			\n"
	"	mov	lr, pc			\n"
	"	mov	pc, %0			\n"
	"	str	r0, %7			\n"
	"	str	r4, %4			\n"
	"	str	r5, %5			\n"
	"	str	r6, %6			\n"
	"\t/*l4_ipc_call(end) */		\n"
	:
	: "i" (L4_SYSCALL_IPC),
	"m" (x.tid),
	"m" (x.snd_dsc),
	"m" (x.rcv_dsc),
	"m" (x.dw0),
	"m" (x.dw1),
	"m" (x.dw2),
	"m" (x.result),
	"m" (x.timeout)
	:"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
	 "r10", "r11", "r12", "r14", "memory");
    *rcv_dword0 = x.dw0;
    *rcv_dword1 = x.dw1;
    *rcv_dword2 = x.dw2;
    *result = x.result;

    return result->md.error_code;
}



L4_INLINE int l4_ipc_reply_and_wait(l4_threadid_t dest, 
				    const void *snd_msg, 
				    dword_t snd_dword0,
				    dword_t snd_dword1, 
				    dword_t snd_dword2,
				    l4_threadid_t *src,
				    void *rcv_msg,
				    dword_t *rcv_dword0, 
				    dword_t *rcv_dword1,
				    dword_t *rcv_dword2,
				    l4_timeout_t timeout,
				    l4_msgdope_t *result)
{
    struct
    {
	dword_t tid;
	dword_t snd_dsc;
	dword_t rcv_dsc;
	l4_timeout_t	timeout;
	dword_t dw0;
	dword_t dw1;
	dword_t dw2;
	l4_msgdope_t	result;
    } x = {dest.raw, (dword_t) snd_msg, (dword_t) rcv_msg | L4_IPC_OPEN_IPC,
	   timeout, snd_dword0, snd_dword1, snd_dword2};
    
    __asm__ __volatile__ (
	"/* l4_ipc_reply_and_wait(start) */	\n"
	"	ldr	r0, %1			\n"
	"	ldr	r1, %2			\n"
	"	ldr	r2, %3			\n"
	"	ldr	r3, %8			\n"
	"	ldr	r4, %4			\n"
	"	ldr	r5, %5			\n"
	"	ldr	r6, %6			\n"
	"	mov	lr, pc			\n"
	"	mov	pc, %0			\n"
	"	str	r0, %7			\n"
	"	str	r1, %1			\n"
	"	str	r4, %4			\n"
	"	str	r5, %5			\n"
	"	str	r6, %6			\n"
	"\t/*l4_ipc_reply_and_wait(end) */	\n"
	:
	: "i" (L4_SYSCALL_IPC),
	"m" (x.tid),
	"m" (x.snd_dsc),
	"m" (x.rcv_dsc),
	"m" (x.dw0),
	"m" (x.dw1),
	"m" (x.dw2),
	"m" (x.result),
	"m" (x.timeout)	
	:"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
	 "r10", "r11", "r12", "r14", "memory");
    *rcv_dword0 = x.dw0;
    *rcv_dword1 = x.dw1;
    *rcv_dword2 = x.dw2;
    src->raw = x.tid;
    *result = x.result;

    return result->md.error_code;
    return 0;
}


#endif /* !NO_SYSCALL_INLINES */

#ifdef __cplusplus
}
#endif


#endif /* !__L4__ARM__L4_H__ */
