/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *               and Dresden University
 *                
 * File path:     l4/x86/ipc.h
 * Description:   IPC bindings for x86
 *                
 * @LICENSE@
 *                
 * $Id: ipc.h,v 1.8 2001/12/13 08:36:40 uhlig Exp $
 *                
 ********************************************************************/
#ifndef __L4_X86_IPC_H__ 
#define __L4_X86_IPC_H__ 


#if !defined(CONFIG_L4_SYSENTEREXIT)
#define IPC_SYSENTER	"int	$0x30		\n\t"
#else
#define OLD_IPC_SYSENTER	\
"push	%%ecx						\n\t" \
"push	%%ebp						\n\t" \
"push	$0x23		/* linear_space_exec	*/	\n\t" \
"push	$0f		/* offset ret_addr	*/	\n\t" \
"mov	%%esp,%%ecx					\n\t" \
"sysenter		/* = db 0x0F,0x34	*/	\n\t" \
"mov	%%ebp,%%edx					\n\t" \
"0:							\n\t"
#define IPC_SYSENTER	\
"push	%%ecx						\n\t" \
"push	%%ebp						\n\t" \
"push	$0x1b		/* linear_space_exec	*/	\n\t" \
"push	$0f		/* offset ret_addr	*/	\n\t" \
"mov	%%esp,%%ecx					\n\t" \
"sysenter		/* = db 0x0F,0x34	*/	\n\t" \
"mov	%%ebp,%%edx					\n\t" \
"0:							\n\t"
#endif

 /*
 * Internal defines used to build IPC parameters for the L4 kernel
 */

#define L4_IPC_NIL_DESCRIPTOR 	(-1)
#define L4_IPC_DECEIT 		1
#define L4_IPC_OPEN_IPC 	1


/*
 * Prototypes
 */

L4_INLINE int
l4_ipc_call(l4_threadid_t dest, 
	    const void *snd_msg, 
	    dword_t snd_dword0, dword_t snd_dword1, dword_t snd_dword2,
	    void *rcv_msg, 
	    dword_t *rcv_dword0, dword_t *rcv_dword1, dword_t *rcv_dword2,
	    l4_timeout_t timeout, l4_msgdope_t *result);

L4_INLINE int
l4_ipc_reply_and_wait(l4_threadid_t dest, 
    		      const void *snd_msg, 
		      dword_t snd_dword0, dword_t snd_dword1, 
		      dword_t snd_dword2,
		      l4_threadid_t *src,
		      void *rcv_msg, dword_t *rcv_dword0, 
		      dword_t *rcv_dword1, dword_t *rcv_dword2,
		      l4_timeout_t timeout, l4_msgdope_t *result);


L4_INLINE int
l4_ipc_send(l4_threadid_t dest, 
	    const void *snd_msg, 
	    dword_t snd_dword0, dword_t snd_dword1, dword_t snd_dword2,
	    l4_timeout_t timeout, l4_msgdope_t *result);

L4_INLINE int
l4_ipc_wait(l4_threadid_t *src,
	    void *rcv_msg, 
	    dword_t *rcv_dword0, dword_t *rcv_dword1, dword_t *rcv_dword2,
	    l4_timeout_t timeout, l4_msgdope_t *result);

L4_INLINE int
l4_ipc_receive(l4_threadid_t src,
	       void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
	       dword_t *rcv_dword2,
	       l4_timeout_t timeout, l4_msgdope_t *result);


		    
		    
/*
 * Implementation
 */

#define SCRATCH_MEMORY 1
#ifdef __pic__

#error no version X bindings with __pic__ enabled

#else /* __pic__ */

L4_INLINE int
l4_ipc_call(l4_threadid_t dest, 
	    const void *snd_msg, dword_t snd_dword0, dword_t snd_dword1, 
	    dword_t snd_dword2, void *rcv_msg, dword_t *rcv_dword0, 
	    dword_t *rcv_dword1, dword_t *rcv_dword2,
	    l4_timeout_t timeout, l4_msgdope_t *result)
{
    dword_t dw[3] = {snd_dword0, snd_dword1, snd_dword2};
    asm volatile(
	"pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
	"movl	%%edi, %%ebp	\n\t"
	"movl     4(%%edx), %%ebx \n\t"
	"movl     8(%%edx), %%edi \n\t"
	"movl     (%%edx), %%edx  \n\t"
	IPC_SYSENTER
	"popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
	: 
	"=a" (*result),				/* EAX, 0 	*/
	"=d" (*rcv_dword0),			/* EDX, 1 	*/
	"=b" (*rcv_dword1),			/* EBX, 2 	*/
	"=D" (*rcv_dword2)                        /* EDI, 3       */
	:
	"c" (timeout),				/* ECX, 4 	*/
	"D" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)),/* EDI, 5, rcv msg -> ebp */
	"S" (dest),				/* ESI, 6, dest	  */
	"0" (((int)snd_msg) & (~L4_IPC_DECEIT)),	/* EAX, 0  	*/
	"1" (&dw[0])				/* EDX, 1,	*/
#ifdef SCRATCH_MEMORY
	: "memory"
#endif /* SCRATCH_MEMORY */
	);
    return L4_IPC_ERROR(*result);
}



L4_INLINE int
l4_ipc_reply_and_wait(l4_threadid_t dest, 
		      const void *snd_msg, 
		      dword_t snd_dword0, dword_t snd_dword1, 
		      dword_t snd_dword2,
		      l4_threadid_t *src,
		      void *rcv_msg, dword_t *rcv_dword0, 
		      dword_t *rcv_dword1, dword_t *rcv_dword2,
		      l4_timeout_t timeout, l4_msgdope_t *result)
{
  dword_t dw[3] = {snd_dword0, snd_dword1, snd_dword2};
#if 1
  asm volatile(
      "pushl	%%ecx		\n\t"
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "movl	%%edi, %%ebp	\n\t"
      "movl     4(%%edx), %%ebx \n\t"           /* get send values */
      "movl     8(%%edx), %%edi \n\t"
      "movl     (%%edx), %%edx  \n\t"
      IPC_SYSENTER
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      "popl	%%ecx		\n\t"
      : 
      "=a" (*result),				/* EAX, 0 	*/
      "=d" (*rcv_dword0),			/* EDX, 1 	*/
      "=b" (*rcv_dword1),			/* EBX, 2 	*/
      "=D" (*rcv_dword2),                       /* EDI, 3       */
      "=S" (*src)                               /* ESI, 4       */
      :
      "c" (timeout),				/* ECX, 5	*/
      "3" (((int)rcv_msg) | L4_IPC_OPEN_IPC),	/* edi, 6  -> ebp rcv_msg */
      "4" (dest.raw),				/* ESI ,4	*/
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),	/* EAX, 0 	*/
      "1" (dw)				        /* EDX, 1 	*/
      );
#endif
  return L4_IPC_ERROR(*result);
}



// ok 

L4_INLINE int
l4_ipc_send(l4_threadid_t dest, 
	    const void *snd_msg, dword_t snd_dword0, dword_t snd_dword1, 
	    dword_t snd_dword2, 
	    l4_timeout_t timeout, l4_msgdope_t *result)
{
  dword_t dummy;
  asm volatile(
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "pushl	%%edx		\n\t"
      "pushl	%%esi		\n\t"
      "pushl	%%edi		\n\t"
      "movl	%8 ,%%ebp	\n\t" 
      IPC_SYSENTER
      "popl	%%edi		\n\t"
      "popl	%%esi		\n\t"
      "popl	%%edx		\n\t"
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result),				/* EAX,0 	*/
      "=b" (dummy),
      "=c" (dummy)
      :
      "d" (snd_dword0),				/* EDX, 1	*/
      "c" (timeout),				/* ECX, 2	*/
      "b" (snd_dword1),				/* EBX, 3	*/
      "D" (snd_dword2),			        /* EDI, 4	*/
      "S" (dest.raw),			        /* ESI, 5	*/
      "i" (L4_IPC_NIL_DESCRIPTOR),		/* Int, 6 	*/
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT))	/* EAX, 0 	*/
#ifdef SCRATCH_MEMORY
      : "memory"
#endif /* SCRATCH_MEMORY */
      );
  return L4_IPC_ERROR(*result);
};


// ok
L4_INLINE int
l4_ipc_wait(l4_threadid_t *src,
	    void *rcv_msg, 
	    dword_t *rcv_dword0, dword_t *rcv_dword1, dword_t *rcv_dword2,
	    l4_timeout_t timeout, l4_msgdope_t *result)
{
  asm volatile(
      
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "pushl	%%ecx		\n\t"
      "movl	%%ebx,%%ebp	\n\t" 
      IPC_SYSENTER
      "popl	%%ecx		\n\t"
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result),				/* EAX,0 */
      "=d" (*rcv_dword0),			/* EDX,1 */
      "=b" (*rcv_dword1),			/* EBX,2 */
      "=D" (*rcv_dword2),			/* EDI,3 */
      "=S" (src->raw)			        /* ESI,4 */
      :
      "c" (timeout),				/* ECX, 5 	*/
      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
      "2" (((int)rcv_msg) | L4_IPC_OPEN_IPC)	/* EBX, 2, rcv_msg -> EBP */

#ifdef SCRATCH_MEMORY
      :"memory"
#endif /* SCRATCH_MEMORY */
      );
  return L4_IPC_ERROR(*result);
}

// ok

L4_INLINE int
l4_ipc_receive(l4_threadid_t src,
	       void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
	       dword_t *rcv_dword2,
	       l4_timeout_t timeout, l4_msgdope_t *result)
{
  asm volatile(
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "pushl	%%ecx		\n\t"
      "pushl	%%esi		\n\t"
      "movl	%%ebx,%%ebp	\n\t" 
      IPC_SYSENTER
      "popl	%%esi		\n\t"
      "popl	%%ecx		\n\t"
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result),				/* EAX,0 */
      "=d" (*rcv_dword0),			/* EDX,1 */
      "=b" (*rcv_dword1),			/* EBX,2 */
      "=D" (*rcv_dword2)                        /* EDI,3 */
      :
      "c" (timeout),				/* ECX, 4 	*/
      "S" (src.raw),				/* ESI, 6 	*/
      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
      "2" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)) /* EBX, 2, rcv_msg -> EBP */
      :
#ifdef SCRATCH_MEMORY
       "memory"
#endif /* SCRATCH_MEMORY */
      );
  return L4_IPC_ERROR(*result);
}


L4_INLINE int l4_ipc_sleep(int man, int exp)
{
    l4_msgdope_t result;
    l4_timeout_t timeout = ((l4_timeout_t) { timeout: {exp, 0, 0, 0, 0, man}}); 
    
    asm volatile (
	"pushl	    %%ebp	    \n"
	"pushl	    %%ecx	    \n"
	"subl	    %%ebp, %%ebp    \n"
	"subl	    %%esi, %%esi    \n"
	IPC_SYSENTER
	"popl	    %%ecx	    \n"
	"popl	    %%ebp	    \n"
	: 
	"=a" (result)
	: 
	"c" (timeout.raw),
	"0" (L4_IPC_NIL_DESCRIPTOR)
	:
	"ebx", "esi", "edi", "edx"
    );
    return 0;
}

#endif /* __pic__ */


#endif /* __L4_X86_IPC_H__ */

