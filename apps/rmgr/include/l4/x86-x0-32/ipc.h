/* 
 * $Id: ipc.h,v 1.5 2000/04/29 12:38:10 ud3 Exp $
 */

#ifndef __L4_IPC_H__ 
#define __L4_IPC_H__ 

#include "../../../../include/config.h"

#if !defined(CONFIG_L4_SYSENTEREXIT)
#define IPC_SYSENTER	"int	$0x30		\n\t"
#else
#define IPC_SYSENTER	\
"push	%%ecx						\n\t" \
"push	%%ebp						\n\t" \
"push	$0x23		/* linear_space_exec	*/	\n\t" \
"push	$0f		/* offset ret_addr	*/	\n\t" \
"mov	%%esp,%%ecx						\n\t" \
"sysenter		/* = db 0x0F,0x34	*/	\n\t" \
"mov	%%ebp,%%edx					\n\t" \
"0:							\n\t"
#endif

/*
 * IPC parameters
 */


/* 
 * Structure used to describe destination and true source if a chief
 * wants to deceit 
 */

typedef struct {
  l4_threadid_t dest, true_src;
} l4_ipc_deceit_ids_t;



/* 
 * Defines used for Parameters 
 */

#define L4_IPC_SHORT_MSG 	0

/*
 * Defines used to build Parameters
 */

#define L4_IPC_STRING_SHIFT 8
#define L4_IPC_DWORD_SHIFT 13
#define L4_IPC_SHORT_FPAGE ((void *)2)

#define L4_IPC_DOPE(dwords, strings) \
( (l4_msgdope_t) {md: {0, 0, 0, 0, 0, 0, strings, dwords }})


#define L4_IPC_TIMEOUT(snd_man, snd_exp, rcv_man, rcv_exp, snd_pflt, rcv_pflt)\
     ( (l4_timeout_t) \
       {to: { rcv_exp, snd_exp, rcv_pflt, snd_pflt, snd_man, rcv_man } } )

#define L4_IPC_NEVER ((l4_timeout_t) {timeout: 0})
#define L4_IPC_MAPMSG(address, size)  \
     ((void *)(dword_t)( ((address) & L4_PAGEMASK) | ((size) << 2) \
			 | (unsigned long)L4_IPC_SHORT_FPAGE)) 

/* 
 * Some macros to make result checking easier
 */

#define L4_IPC_ERROR_MASK 	0xF0
#define L4_IPC_DECEIT_MASK	0x01
#define L4_IPC_FPAGE_MASK	0x02
#define L4_IPC_REDIRECT_MASK	0x04
#define L4_IPC_SRC_MASK		0x08
#define L4_IPC_SND_ERR_MASK	0x10

#define L4_IPC_IS_ERROR(x)		(((x).msgdope) & L4_IPC_ERROR_MASK)
#define L4_IPC_MSG_DECEITED(x) 		(((x).msgdope) & L4_IPC_DECEIT_MASK)
#define L4_IPC_MSG_REDIRECTED(x)	(((x).msgdope) & L4_IPC_REDIRECT_MASK)
#define L4_IPC_SRC_INSIDE(x)		(((x).msgdope) & L4_IPC_SRC_MASK)
#define L4_IPC_SND_ERROR(x)		(((x).msgdope) & L4_IPC_SND_ERR_MASK)
#define L4_IPC_MSG_TRANSFER_STARTED \
				((((x).msgdope) & L4_IPC_ERROR_MASK) < 5)


/* 
 * prototypes 
 */

L4_INLINE int l4_ipc_fpage_received(l4_msgdope_t msgdope);
L4_INLINE int l4_ipc_is_fpage_granted(l4_fpage_t fp);
L4_INLINE int l4_ipc_is_fpage_writable(l4_fpage_t fp);


L4_INLINE int l4_ipc_fpage_received(l4_msgdope_t msgdope)
{
  return msgdope.md.fpage_received != 0;
}
L4_INLINE int l4_ipc_is_fpage_granted(l4_fpage_t fp)
{
  return fp.fp.grant != 0;
}
L4_INLINE int l4_ipc_is_fpage_writable(l4_fpage_t fp)
{
  return fp.fp.write != 0;
}

/*
 * IPC results
 */

#define L4_IPC_ERROR(x)			(((x).msgdope) & L4_IPC_ERROR_MASK)
#define L4_IPC_ENOT_EXISTENT		0x10
#define L4_IPC_RETIMEOUT		0x20
#define L4_IPC_SETIMEOUT		0x30
#define L4_IPC_RECANCELED		0x40
#define L4_IPC_SECANCELED		0x50
#define L4_IPC_REMAPFAILED		0x60
#define L4_IPC_SEMAPFAILED		0x70
#define L4_IPC_RESNDPFTO		0x80
#define L4_IPC_SERCVPFTO		0x90
#define L4_IPC_REABORTED		0xA0
#define L4_IPC_SEABORTED		0xB0
#define L4_IPC_REMSGCUT			0xE0
#define L4_IPC_SEMSGCUT			0xF0


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
l4_i386_ipc_call(l4_threadid_t dest, 
		 const void *snd_msg, 
		 dword_t snd_dword0, dword_t snd_dword1,
		 void *rcv_msg, 
		 dword_t *rcv_dword0, dword_t *rcv_dword1,
		 l4_timeout_t timeout, l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_reply_and_wait(l4_threadid_t dest, 
			   const void *snd_msg, 
			   dword_t snd_dword0, dword_t snd_dword1, 
			   l4_threadid_t *src,
			   void *rcv_msg, 
			   dword_t *rcv_dword0, dword_t *rcv_dword1,
			   l4_timeout_t timeout, l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_send(l4_threadid_t dest, 
		 const void *snd_msg, 
		 dword_t snd_dword0, dword_t snd_dword1,
		 l4_timeout_t timeout, l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_wait(l4_threadid_t *src,
		 void *rcv_msg, 
		 dword_t *rcv_dword0, dword_t *rcv_dword1, 
		 l4_timeout_t timeout, l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_receive(l4_threadid_t src,
		    void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
		    l4_timeout_t timeout, l4_msgdope_t *result);



/*
 * Implementation
 */

#define SCRATCH 1
#define SCRATCH_MEMORY 1
#ifdef __pic__

#error no version X bindings with __pic__ enabled

#else /* __pic__ */

// ok
L4_INLINE int
l4_i386_ipc_call(l4_threadid_t dest, 
		 const void *snd_msg, dword_t snd_dword0, dword_t snd_dword1, 
		 void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
		 l4_timeout_t timeout, l4_msgdope_t *result)
{
    dword_t dummy;
  asm volatile(
      "pushl	%%esi		\n\t"
      "pushl	%%edi		\n\t"
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "movl	%%edi, %%ebp	\n\t"
      IPC_SYSENTER
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      "popl	%%edi		\n\t"
      "popl	%%esi		\n\t"
      : 
      "=a" (*result),				/* EAX, 0 	*/
      "=d" (*rcv_dword0),			/* EDX, 1 	*/
      "=b" (*rcv_dword1),			/* EBX, 2 	*/
      "=c" (dummy)
      :
      "c" (timeout),				/* ECX, 4 	*/
      "D" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)),/* EDI, 5, rcv msg -> ebp */
      "S" (dest),				/* ESI, 6, dest	  */
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),	/* EAX, 0  	*/
      "1" (snd_dword0),				/* EDX, 1,	*/
      "2" (snd_dword1)
      : "memory"
      );
  return L4_IPC_ERROR(*result);
}

// ok
L4_INLINE int
l4_i386_ipc_reply_and_wait(l4_threadid_t dest, 
			   const void *snd_msg, 
			   dword_t snd_dword0, dword_t snd_dword1, 
			   l4_threadid_t *src, void *rcv_msg, 
			   dword_t *rcv_dword0, dword_t *rcv_dword1, 
			   l4_timeout_t timeout, l4_msgdope_t *result)
{
  asm volatile(
      /* eax, edx, ebx loaded, 
       * edi contains rcv buffer address, must be moved to ebp,
       * esi contains address of destination id,
       * $5  address of src id
       */
      "pushl	%%ecx		\n\t"
      "pushl	%%edi		\n\t"
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "movl	%%edi, %%ebp	\n\t"
      IPC_SYSENTER
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      "popl	%%edi		\n\t"
      "popl	%%ecx		\n\t"
      : 
      "=a" (*result),				/* EAX, 0 	*/
      "=d" (*rcv_dword0),			/* EDX, 1 	*/
      "=b" (*rcv_dword1),			/* EBX, 2 	*/
      "=S" (*src)                               /* ESI, 4       */
      :
      "c" (timeout),				/* ECX, 5	*/
      "D" (((int)rcv_msg) | L4_IPC_OPEN_IPC),	/* edi, 6  -> ebp rcv_msg */
      "S" (dest.dw),				/* ESI ,4	*/
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),	/* EAX, 0 	*/
      "1" (snd_dword0),			        /* EDX, 1 	*/
      "2" (snd_dword1)
      :"memory"
      );
  return L4_IPC_ERROR(*result);
}


// ok 

L4_INLINE int
l4_i386_ipc_send(l4_threadid_t dest, 
		 const void *snd_msg, dword_t snd_dword0, dword_t snd_dword1, 
		 l4_timeout_t timeout, l4_msgdope_t *result)
{
    int dummy;
  asm volatile(
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "movl	$-1 ,%%ebp	\n\t" 
      IPC_SYSENTER
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result)				/* EAX,0 	*/
      , "=d"(dummy), "=c"(dummy), "=b"(dummy), "=S"(dummy)
      :
      "d" (snd_dword0),				/* EDX, 1	*/
      "c" (timeout),				/* ECX, 2	*/
      "b" (snd_dword1),				/* EBX, 3	*/
      "S" (dest.dw),			        /* ESI, 4	*/
      "a" (((int)snd_msg) & (~L4_IPC_DECEIT))	/* EAX, 0 	*/
#ifdef SCRATCH
      :
      "edi"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
};


// ok
L4_INLINE int
l4_i386_ipc_wait(l4_threadid_t *src,
		 void *rcv_msg, 
		 dword_t *rcv_dword0, dword_t *rcv_dword1,
		 l4_timeout_t timeout, l4_msgdope_t *result)
{
    dword_t dummy;
  asm volatile(
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "movl	%%ebx,%%ebp	\n\t" 
      IPC_SYSENTER
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result),				/* EAX,0 */
      "=d" (*rcv_dword0),			/* EDX,1 */
      "=b" (*rcv_dword1),			/* EBX,2 */
      "=S" (src->dw),			        /* ESI,4 */
      "=c" (dummy)
      :
      "c" (timeout),				/* ECX, 5 	*/
      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
      "2" (((int)rcv_msg) | L4_IPC_OPEN_IPC)	/* EBX, 2, rcv_msg -> EBP */
#ifdef SCRATCH
      :
      "edi"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
}

// ok

L4_INLINE int
l4_i386_ipc_receive(l4_threadid_t src,
		    void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
		    l4_timeout_t timeout, l4_msgdope_t *result)
{
    dword_t dummy;
  asm volatile(
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "movl	%%ebx,%%ebp	\n\t" 
      IPC_SYSENTER
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result),				/* EAX,0 */
      "=d" (*rcv_dword0),			/* EDX,1 */
      "=b" (*rcv_dword1),			/* EBX,2 */
      "=c" (dummy),
      "=S" (dummy),
      "=D" (dummy)
      :
      "c" (timeout),				/* ECX, 4 	*/
      "S" (src.dw),				/* ESI, 6 	*/
      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
      "2" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)) /* EBX, 2, rcv_msg -> EBP */
      : "memory"
      );
  return L4_IPC_ERROR(*result);
}

#endif /* __pic__ */



#endif /* __L4_IPC__ */
