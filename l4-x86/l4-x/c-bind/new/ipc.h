/* 
 * $Id: ipc.h,v 1.1 2001/03/07 11:30:43 voelp Exp $
 */

#ifndef __L4_IPC_H__ 
#define __L4_IPC_H__ 

/*
 * L4 ipc
 */

#include <l4/compiler.h>
#include <l4/types.h>

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
 * Prototypes
 */

L4_INLINE int
l4_i386_ipc_call(l4_threadid_t dest, 
		 const void *snd_msg, dword_t snd_dword0, dword_t snd_dword1, 
		 void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
		 l4_timeout_t timeout, l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_reply_and_wait(l4_threadid_t dest, 
			   const void *snd_msg, 
			   dword_t snd_dword0, dword_t snd_dword1, 
			   l4_threadid_t *src,
			   void *rcv_msg, dword_t *rcv_dword0, 
			   dword_t *rcv_dword1, 
			   l4_timeout_t timeout, l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_reply_deceiting_and_wait(const l4_ipc_deceit_ids_t ids,
				     const void *snd_msg, 
				     dword_t snd_dword0, dword_t snd_dword1,
				     l4_threadid_t *src,
				     void *rcv_msg, 
				     dword_t *rcv_dword0, dword_t *rcv_dword1, 
				     l4_timeout_t timeout, 
				     l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_send(l4_threadid_t dest, 
		 const void *snd_msg, dword_t snd_dword0, dword_t snd_dword1, 
		 l4_timeout_t timeout, l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_send_deceiting(l4_ipc_deceit_ids_t ids,
			   const void *snd_msg, 
			   dword_t snd_dword0, dword_t snd_dword1, 
			   l4_timeout_t timeout, l4_msgdope_t *result);
L4_INLINE int
l4_i386_ipc_wait(l4_threadid_t *src,
		 void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
		 l4_timeout_t timeout, l4_msgdope_t *result);

L4_INLINE int
l4_i386_ipc_receive(l4_threadid_t src,
		    void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
		    l4_timeout_t timeout, l4_msgdope_t *result);


L4_INLINE int l4_ipc_fpage_received(l4_msgdope_t msgdope);
L4_INLINE int l4_ipc_is_fpage_granted(l4_fpage_t fp);
L4_INLINE int l4_ipc_is_fpage_writable(l4_fpage_t fp);

/* IPC bindings for chiefs */

L4_INLINE int
l4_i386_ipc_chief_wait(l4_threadid_t *src, l4_threadid_t *real_dst,
		       void *rcv_msg, dword_t *rcv_dword0, 
		       dword_t *rcv_dword1, l4_timeout_t timeout, 
		       l4_msgdope_t *result);
L4_INLINE int
l4_i386_ipc_chief_receive(l4_threadid_t src, l4_threadid_t *real_dst,
			  void *rcv_msg, dword_t *rcv_dword0, 
			  dword_t *rcv_dword1, l4_timeout_t timeout, 
			  l4_msgdope_t *result);
L4_INLINE int
l4_i386_ipc_chief_call(l4_threadid_t dest, l4_threadid_t fake_src,
		       const void *snd_msg, dword_t snd_dword0, 
		       dword_t snd_dword1, 
		       l4_threadid_t *real_dst,
		       void *rcv_msg, dword_t *rcv_dword0, 
		       dword_t *rcv_dword1, 
		       l4_timeout_t timeout, l4_msgdope_t *result);
L4_INLINE int
l4_i386_ipc_chief_reply_and_wait(l4_threadid_t dest, l4_threadid_t fake_src,
				 const void *snd_msg, 
				 dword_t snd_dword0, dword_t snd_dword1, 
				 l4_threadid_t *src, l4_threadid_t *real_dst,
				 void *rcv_msg, dword_t *rcv_dword0, 
				 dword_t *rcv_dword1, 
				 l4_timeout_t timeout, l4_msgdope_t *result);
L4_INLINE int
l4_i386_ipc_chief_send(l4_threadid_t dest, l4_threadid_t fake_src,
		       const void *snd_msg, dword_t snd_dword0, 
		       dword_t snd_dword1, 
		       l4_timeout_t timeout, l4_msgdope_t *result);





/*
 *
 */

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
 * Implementation
 */

#define SCRATCH 1
#define SCRATCH_MEMORY 1

#ifdef __pic__

L4_INLINE int
l4_i386_ipc_call(l4_threadid_t dest, 
		 const void *snd_msg, dword_t snd_dword0, dword_t snd_dword1, 
		 void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
		 l4_timeout_t timeout, l4_msgdope_t *result)
{
  struct {
    dword_t d1, d2;
  } dwords = {snd_dword0, snd_dword1};

  asm 
    volatile(
	     "pushl	%%ebx		\n\t"  
	     "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	     "movl	4(%%edx), %%ebx	\n\t"
	     "movl	(%%edx), %%edx	\n\t"

	     "movl	%%edi, %%ebp	\n\t"

	     "movl	4(%%esi), %%edi	\n\t"		/* dest.lh.high -> edi */
	     "movl	(%%esi), %%esi	\n\t"		/* dest.lh.low  -> edi */

	     "int	$0x30		\n\t"
	     "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	     "movl	%%ebx, %%ecx	\n\t"
	     "popl	%%ebx		\n\t"
	     : 
	     "=a" (*result),				/* EAX, 0 	*/
	     "=d" (*rcv_dword0),			/* EDX, 1 	*/
	     "=c" (*rcv_dword1)				/* ECX, 2 	*/
	     :
	     "D" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)), /* EDI, 3, rcv msg -> ebp */
	     "S" (&dest),				/* ESI, 4, addr of dest	  */
	     "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),	/* EAX, 0  	*/
	     "1" (&dwords),				/* EDX, 1,	*/
	     "2" (timeout)				/* ECX, 2 	*/
#ifdef SCRATCH
	     :
	     "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
	     , "memory"
#endif /* SCRATCH_MEMORY */
#endif
	     );
  return L4_IPC_ERROR(*result);
}



L4_INLINE int
l4_i386_ipc_reply_and_wait(l4_threadid_t dest, 
			   const void *snd_msg, 
			   dword_t snd_dword0, dword_t snd_dword1, 
			   l4_threadid_t *src,
			   void *rcv_msg, dword_t *rcv_dword0, 
			   dword_t *rcv_dword1, 
			   l4_timeout_t timeout, l4_msgdope_t *result)
{
  struct {
    l4_threadid_t *dest;
    l4_threadid_t *src;
  } addresses = { &dest, src };

  struct {
    dword_t d1, d2;
  } dwords = {snd_dword0, snd_dword1};

  asm 
    volatile(
	     /* eax, edx, ebx loaded, 
	      * edi contains rcv buffer address, must be moved to ebp,
	      * esi contains address of destination id,
	      * $5  address of src id
	      */
	     "pushl	%%ebx		\n\t"  
	     "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	     "movl	4(%%edx), %%ebx	\n\t"
	     "movl	(%%edx), %%edx	\n\t"

	     "pushl	%%esi		\n\t"
	     "movl	(%%esi), %%esi	\n\t"		/* load address of dest */
	     
	     "movl	%%edi, %%ebp	\n\t"
	     "movl	4(%%esi), %%edi	\n\t"		/* dest.lh.high -> edi */
	     "movl	(%%esi), %%esi	\n\t"		/* dest.lh.low  -> esi */
	     "int	$0x30		\n\t"
	     "popl	%%ebp		\n\t"
	     "movl	4(%%ebp), %%ebp	\n\t"		/* load address of src */
	     "movl	%%esi, (%%ebp)  \n\t"		/* esi -> src.lh.low  */
	     "movl	%%edi, 4(%%ebp)\n\t"		/* edi -> src.lh.high */

	     "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	     "movl	%%ebx, %%ecx	\n\t"
	     "popl	%%ebx		\n\t"
	     
      : 
      "=a" (*result),				/* EAX, 0 	*/
      "=d" (*rcv_dword0),			/* EDX, 1 	*/
      "=c" (*rcv_dword1)			/* ECX, 2 	*/
      :
      "2" (timeout),				/* ECX, 3	*/
      "D" (((int)rcv_msg) | L4_IPC_OPEN_IPC),	/* edi, 4  -> ebp rcv_msg */
      "S" (&addresses),				/* ESI ,5	*/
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),	/* EAX, 0 	*/
      "1" (&dwords)				/* EDX, 1 	*/
#ifdef SCRATCH
      :
      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
}


L4_INLINE int
l4_i386_ipc_reply_deceiting_and_wait(l4_ipc_deceit_ids_t ids,
				     const void *snd_msg, 
				     dword_t snd_dword0, dword_t snd_dword1,
				     l4_threadid_t *src,
				     void *rcv_msg, 
				     dword_t *rcv_dword0, dword_t *rcv_dword1, 
				     l4_timeout_t timeout, 
				     l4_msgdope_t *result)
{
  struct {
    l4_ipc_deceit_ids_t *ids;
    l4_threadid_t *src;
  } addresses = { &ids, src };

  struct {
    dword_t d1, d2;
  } dwords = {snd_dword0, snd_dword1};

  asm 
    volatile(
	     /* eax, edx, ebx loaded, 
	      * edi contains rcv buffer address, must be moved to ebp,
	      * esi contains address of destination id,
	      * esi+8 is the address of true src
	      * $5  address of src id
	      */
	     "pushl	%%ebx		\n\t"	
	     "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	     "pushl	%%esi		\n\t"
	     "movl	(%%esi), %%esi	\n\t"		/* load address of ids */

	     "movl	%%edi, %%ebp	\n\t"
	     "pushl	12(%%esi)	\n\t"		/* ids.true_src.lh.high */
	     /*	->(esp+4) */
	     "pushl	8(%%esi)	\n\t"		/* ids.true_src.lh.low  */ 
	     /*   	->(esp)   */
	     "movl	4(%%esi), %%edi	\n\t"		/* ids.dest.lh.high -> edi */
	     "movl	(%%esi), %%esi	\n\t"		/* ids.dest.lh.low  -> edi */
	     
	     "movl	4(%%edx), %%ebx	\n\t"
	     "movl	(%%edx), %%edx	\n\t"
	     

	     "int	$0x30		\n\t"
	     "addl	$8, %%esp	\n\t"		/* remove true_src from stack*/

	     "popl	%%ebp		\n\t"
	     "movl	4(%%ebp), %%ebp	\n\t"		/* load address of src */
	     "movl	%%esi, (%%ebp)  \n\t"		/* esi -> src.lh.low  */
	     "movl	%%edi, 4(%%ebp)\n\t"		/* edi -> src.lh.high */
	     
	     "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	     "movl	%%ebx, %%ecx	\n\t"
	     "popl	%%ebx		\n\t"
      : 
      "=a" (*result),				/* EAX, 0 	*/
      "=d" (*rcv_dword0),			/* EDX, 1 	*/
      "=c" (*rcv_dword1)			/* ECX, 2 	*/
      :
      "S" (&addresses),				/* addresses, 3	*/
      "2" (timeout),				/* ECX, 2	*/
      "D" (((int)rcv_msg) | L4_IPC_OPEN_IPC),	/* EDI, 4 -> ebp rcv_msg */
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),	/* EAX, 0 	*/
      "1" (dwords)				/* EDX, 1 	*/
#ifdef SCRATCH
      :
      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
}


L4_INLINE int 
l4_i386_ipc_send(l4_threadid_t dest, 
		 const void *snd_msg, dword_t snd_dword0, dword_t snd_dword1, 
		 l4_timeout_t timeout, l4_msgdope_t *result)
{
  asm 
    volatile(
	     "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	     "pushl    %%ebx		\n\t"
	     "movl	%5 ,%%ebp	\n\t"
	     "movl	%%esi, %%ebx	\n\t"
	     "movl	(%%edi),%%esi	\n\t"
	     "movl	4(%%edi),%%edi	\n\t"
	     "int	$0x30		\n\t"
	     "popl	%%ebx		\n\t"
	     "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	     : 
	     "=a" (*result)				/* EAX,0 	*/
	     :
	     "d" (snd_dword0),				/* EDX, 1	*/
	     "c" (timeout),				/* ECX, 2	*/
	     "S" (snd_dword1),				/* EBX, 3	*/
	     "D" (&dest),				/* EDI, 4	*/
	     "i" (L4_IPC_NIL_DESCRIPTOR),		/* Int, 5 	*/
	     "0" (((int)snd_msg) & (~L4_IPC_DECEIT))	/* EAX, 0 	*/
#ifdef SCRATCH
	     :
	     "esi", "edi", "ecx", "edx"
#ifdef SCRATCH_MEMORY
	     , "memory"
#endif /* SCRATCH_MEMORY */
#endif
	     );
  return L4_IPC_ERROR(*result);
};

L4_INLINE int
l4_i386_ipc_send_deceiting(l4_ipc_deceit_ids_t ids,
			   const void *snd_msg, 
			   dword_t snd_dword0, dword_t snd_dword1, 
			   l4_timeout_t timeout, l4_msgdope_t *result)
{
  struct {
    dword_t d1, d2;
  } dwords = {snd_dword0, snd_dword1};

  asm 
    volatile(
	     "pushl	%%ebx		\n\t"  

	     "movl	4(%%edx), %%ebx	\n\t"
	     "movl	(%%edx), %%edx	\n\t"
		     
	     "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	     "pushl	4(%%esi)	\n\t"		
	     "pushl	(%%esi)		\n\t"		
	     "movl	(%%edi), %%esi	\n\t"
	     "movl	4(%%edi), %%edi	\n\t"
	     
	     "movl	%6 ,%%ebp	\n\t" 
	     
	     "int	$0x30		\n\t"

	     /* remove true_src from stack */
	     "addl	$8, %%esp	\n\t"
	     "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	     "popl	%%ebx		\n\t"
      : 
      "=a" (*result)				/* EAX,0 	*/
      :
      "d" (&dwords),				/* EDX, 1	*/
      "c" (timeout),				/* ECX, 2	*/
      "D" (&ids.dest),				/* EDI, 4	*/
      "S" (&ids.true_src),			/* ESI, 5	*/
      "i" (L4_IPC_NIL_DESCRIPTOR),		/* Int, 6 	*/
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT))	/* EAX, 0 	*/
#ifdef SCRATCH
      :
      "esi", "edi", "ebx", "ecx", "edx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
};

L4_INLINE int 
l4_i386_ipc_receive(l4_threadid_t src,
		    void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
		    l4_timeout_t timeout, l4_msgdope_t *result)
{
  asm
    volatile (
	      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	      "pushl	%%ebx		\n\t"
	      "movl	%2,%%ebp	\n\t"
	      "movl	(%%edi),%%esi	\n\t"
	      "movl	4(%%edi),%%edi	\n\t" 
	      "int	$0x30		\n\t"
	      "movl	%%ebx, %2	\n\t"
	      "popl	%%ebx		\n\t"
	      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	      : 
	      "=a" (*result),				/* EAX,0 */
	      "=d" (*rcv_dword0),			/* EDX,1 */
	      "=S" (*rcv_dword1)		       	/* ESI,2 */
	      :
	      "c" (timeout),				/* ECX, 3 	*/
	      "D" (&src),		       		/* EDI, 4 	*/
	      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
	      "2" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)) /* ESI, 2, rcv_msg -> EBP */
#ifdef SCRATCH
	      :
	      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
	      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
	      );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int 
l4_i386_ipc_wait(l4_threadid_t *src,
		    void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
		    l4_timeout_t timeout, l4_msgdope_t *result)
{
  asm
    volatile (
	      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	      "pushl	%%ebx		\n\t"
	      "movl	%%esi,%%ebp	\n\t"
	      "pushl	%%edi		\n\t"
	      "int	$0x30		\n\t"
	      "popl	%%ebp		\n\t"
	      "movl	%%esi,(%%ebp)	\n\t"
	      "movl	%%edi,4(%%ebp)	\n\t" 
	      "movl	%%ebx,%%esi	\n\t"
	      "popl	%%ebx		\n\t"
	      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	      : 
	      "=a" (*result),				/* EAX,0 */
	      "=d" (*rcv_dword0),			/* EDX,1 */
	      "=S" (*rcv_dword1)		       	/* ESI,2 */
	      :
	      "c" (timeout),				/* ECX, 3 	*/
	      "D" (src),		       		/* EDI, 4 	*/
	      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
	      "2" (((int)rcv_msg) | L4_IPC_OPEN_IPC)	/* ESI, 2, rcv_msg -> EBP */
#ifdef SCRATCH
	      :
	      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
	      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
	      );
  return L4_IPC_ERROR(*result);
}


/****************************************************************
*****************************************************************
****************************************************************/

#else /* __pic__ */

L4_INLINE int
l4_i386_ipc_call(l4_threadid_t dest, 
		 const void *snd_msg, dword_t snd_dword0, dword_t snd_dword1, 
		 void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
		 l4_timeout_t timeout, l4_msgdope_t *result)
{
  asm volatile(
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "movl	%%edi, %%ebp	\n\t"
      "movl	4(%%esi), %%edi	\n\t"		/* dest.lh.high -> edi */
      "movl	(%%esi), %%esi	\n\t"		/* dest.lh.low  -> edi */
      "int	$0x30		\n\t"
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result),				/* EAX, 0 	*/
      "=d" (*rcv_dword0),			/* EDX, 1 	*/
      "=b" (*rcv_dword1)			/* EBX, 2 	*/
      :
      "c" (timeout),				/* ECX, 3 	*/
      "D" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)),/* EDI, 4, rcv msg -> ebp */
      "S" (&dest),				/* ESI, 5, addr of dest	  */
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),	/* EAX, 0  	*/
      "1" (snd_dword0),				/* EDX, 1,	*/
      "2" (snd_dword1)				/* EBX, 2 	*/
#ifdef SCRATCH
      :
      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int
l4_i386_ipc_reply_and_wait(l4_threadid_t dest, 
			   const void *snd_msg, 
			   dword_t snd_dword0, dword_t snd_dword1, 
			   l4_threadid_t *src,
			   void *rcv_msg, dword_t *rcv_dword0, 
			   dword_t *rcv_dword1, 
			   l4_timeout_t timeout, l4_msgdope_t *result)
{
  struct {
    l4_threadid_t *dest;
    l4_threadid_t *src;
  } addresses = { &dest, src };

  asm volatile(
      /* eax, edx, ebx loaded, 
       * edi contains rcv buffer address, must be moved to ebp,
       * esi contains address of destination id,
       * $5  address of src id
       */
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "pushl	%%esi		\n\t"
      "movl	(%%esi), %%esi	\n\t"		/* load address of dest */

      "movl	%%edi, %%ebp	\n\t"
      "movl	4(%%esi), %%edi	\n\t"		/* dest.lh.high -> edi */
      "movl	(%%esi), %%esi	\n\t"		/* dest.lh.low  -> esi */
      "int	$0x30		\n\t"
      "popl	%%ebp		\n\t"
      "movl	4(%%ebp), %%ebp	\n\t"		/* load address of src */
      "movl	%%esi, (%%ebp)  \n\t"		/* esi -> src.lh.low  */
      "movl	%%edi, 4(%%ebp)\n\t"		/* edi -> src.lh.high */

      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result),				/* EAX, 0 	*/
      "=d" (*rcv_dword0),			/* EDX, 1 	*/
      "=b" (*rcv_dword1)			/* EBX, 2 	*/
      :
      "c" (timeout),				/* ECX, 3	*/
      "D" (((int)rcv_msg) | L4_IPC_OPEN_IPC),	/* edi, 4  -> ebp rcv_msg */
      "S" (&addresses),				/* ESI ,5	*/
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),	/* EAX, 0 	*/
      "1" (snd_dword0),				/* EDX, 1 	*/
      "2" (snd_dword1)				/* EBX, 2 	*/
#ifdef SCRATCH
      :
      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int
l4_i386_ipc_reply_deceiting_and_wait(l4_ipc_deceit_ids_t ids,
				     const void *snd_msg, 
				     dword_t snd_dword0, dword_t snd_dword1,
				     l4_threadid_t *src,
				     void *rcv_msg, 
				     dword_t *rcv_dword0, dword_t *rcv_dword1, 
				     l4_timeout_t timeout, 
				     l4_msgdope_t *result)
{
  struct {
    l4_ipc_deceit_ids_t *ids;
    l4_threadid_t *src;
  } addresses = { &ids, src };

  asm volatile(
      /* eax, edx, ebx loaded, 
       * edi contains rcv buffer address, must be moved to ebp,
       * esi contains address of destination id,
       * esi+8 is the address of true src
       * $5  address of src id
       */
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "pushl	%%esi		\n\t"
      "movl	(%%esi), %%esi	\n\t"		/* load address of ids */

      "movl	%%edi, %%ebp	\n\t"
      "pushl	12(%%esi)	\n\t"		/* ids.true_src.lh.high */
      /*	->(esp+4) */
      "pushl	8(%%esi)	\n\t"		/* ids.true_src.lh.low  */ 
      /*   	->(esp)   */
      "movl	4(%%esi), %%edi	\n\t"		/* ids.dest.lh.high -> edi */
      "movl	(%%esi), %%esi	\n\t"		/* ids.dest.lh.low  -> edi */
      "int	$0x30		\n\t"
      "addl	$8, %%esp	\n\t"		/* remove true_src from stack*/

      "popl	%%ebp		\n\t"
      "movl	4(%%ebp), %%ebp	\n\t"		/* load address of src */
      "movl	%%esi, (%%ebp)  \n\t"		/* esi -> src.lh.low  */
      "movl	%%edi, 4(%%ebp)\n\t"		/* edi -> src.lh.high */

      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result),				/* EAX, 0 	*/
      "=d" (*rcv_dword0),			/* EDX, 1 	*/
      "=b" (*rcv_dword1)			/* EBX, 2 	*/
      :
      "S" (&addresses),				/* addresses, 3	*/
      "c" (timeout),				/* ECX, 4	*/
      "D" (((int)rcv_msg) | L4_IPC_OPEN_IPC),	/* EDI, 5 -> ebp rcv_msg */
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT)),	/* EAX, 0 	*/
      "1" (snd_dword0),				/* EDX, 1 	*/
      "2" (snd_dword1)				/* EBX, 2 	*/
#ifdef SCRATCH
      :
      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
}



L4_INLINE int
l4_i386_ipc_send(l4_threadid_t dest, 
		 const void *snd_msg, dword_t snd_dword0, dword_t snd_dword1, 
		 l4_timeout_t timeout, l4_msgdope_t *result)
{
  asm volatile(
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "movl	%6 ,%%ebp	\n\t" 
      "int	$0x30		\n\t"
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result)				/* EAX,0 	*/
      :
      "d" (snd_dword0),				/* EDX, 1	*/
      "c" (timeout),				/* ECX, 2	*/
      "b" (snd_dword1),				/* EBX, 3	*/
      "D" (dest.lh.high),			/* EDI, 4	*/
      "S" (dest.lh.low),			/* ESI, 5	*/
      "i" (L4_IPC_NIL_DESCRIPTOR),		/* Int, 6 	*/
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT))	/* EAX, 0 	*/
#ifdef SCRATCH
      :
      "esi", "edi", "ebx", "ecx", "edx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
};


L4_INLINE int
l4_i386_ipc_send_deceiting(l4_ipc_deceit_ids_t ids,
			   const void *snd_msg, 
			   dword_t snd_dword0, dword_t snd_dword1, 
			   l4_timeout_t timeout, l4_msgdope_t *result)
{
  asm volatile(
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "pushl	4(%%esi)	\n\t"		
      "pushl	(%%esi)		\n\t"		
      "movl	(%%edi), %%esi	\n\t"
      "movl	4(%%edi), %%edi	\n\t"

      "movl	%6 ,%%ebp	\n\t" 
      
      "int	$0x30		\n\t"

      /* remove true_src from stack */
      "addl	$8, %%esp	\n\t"
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result)				/* EAX,0 	*/
      :
      "d" (snd_dword0),				/* EDX, 1	*/
      "c" (timeout),				/* ECX, 2	*/
      "b" (snd_dword1),				/* EBX, 3	*/
      "D" (&ids.dest),				/* EDI, 4	*/
      "S" (&ids.true_src),			/* ESI, 5	*/
      "i" (L4_IPC_NIL_DESCRIPTOR),		/* Int, 6 	*/
      "0" (((int)snd_msg) & (~L4_IPC_DECEIT))	/* EAX, 0 	*/
#ifdef SCRATCH
      :
      "esi", "edi", "ebx", "ecx", "edx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
};

L4_INLINE int
l4_i386_ipc_wait(l4_threadid_t *src,
		 void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
		 l4_timeout_t timeout, l4_msgdope_t *result)
{
  asm volatile(
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "movl	%%ebx,%%ebp	\n\t" 
      "int	$0x30		\n\t"
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result),				/* EAX,0 */
      "=d" (*rcv_dword0),			/* EDX,1 */
      "=b" (*rcv_dword1),			/* EBX,2 */
      "=D" (src->lh.high),			/* EDI,3 */
      "=S" (src->lh.low)			/* ESI,4 */
      :
      "c" (timeout),				/* ECX, 5 	*/
      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
      "2" (((int)rcv_msg) | L4_IPC_OPEN_IPC)	/* EBX, 2, rcv_msg -> EBP */
#ifdef SCRATCH
      :
      "ecx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int
l4_i386_ipc_receive(l4_threadid_t src,
		    void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
		    l4_timeout_t timeout, l4_msgdope_t *result)
{
  asm volatile(
      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
						   references ("m") after 
						   this point */
      "movl	%%ebx,%%ebp	\n\t" 
      "int	$0x30		\n\t"
      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
						   references ("m") before 
						   this point */
      : 
      "=a" (*result),				/* EAX,0 */
      "=d" (*rcv_dword0),			/* EDX,1 */
      "=b" (*rcv_dword1)			/* EBX,2 */
      :
      "c" (timeout),				/* ECX, 3 	*/
      "D" (src.lh.high),			/* EDI, 4 	*/
      "S" (src.lh.low),				/* ESI, 5 	*/
      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
      "2" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)) /* EBX, 2, rcv_msg -> EBP */
#ifdef SCRATCH
      :
      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  return L4_IPC_ERROR(*result);
}

#endif /* __pic__ */


/* IPC bindings for chiefs -- they're pic by default for now, no
   optimized version for non-pic */

L4_INLINE int
l4_i386_ipc_chief_wait(l4_threadid_t *src, l4_threadid_t *real_dst,
		       void *rcv_msg, dword_t *rcv_dword0, 
		       dword_t *rcv_dword1, l4_timeout_t timeout, 
		       l4_msgdope_t *result)
{
  asm
    volatile (
	      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	      "pushl	%%ebx		\n\t"
	      "movl	%%esi,%%ebp	\n\t"
	      "pushl    %%edx           \n\t"
	      "pushl	%%edi		\n\t"
	      "int	$0x30		\n\t"
	      "xchgl    %%ebp,(%%esp)   \n\t"
	      "movl	%%esi,(%%ebp)	\n\t" /* store src id */
	      "movl	%%edi,4(%%ebp)	\n\t" 
	      "popl     %%esi           \n\t" /* was %ebp after int */
	      "popl     %%ebp           \n\t" /* was %edx before int */
	      "movl     %%ecx,(%%ebp)  \n\t"
	      "movl     %%esi,4(%%ebp)   \n\t" /* store real dest id */
	      "movl	%%ebx,%%esi	\n\t"
	      "popl	%%ebx		\n\t"
	      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	      : 
	      "=a" (*result),				/* EAX,0 */
	      "=d" (*rcv_dword0),			/* EDX,1 */
	      "=S" (*rcv_dword1)		       	/* ESI,2 */
	      :
	      "c" (timeout),				/* ECX, 3 	*/
	      "D" (src),		       		/* EDI, 4 	*/
	      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
	      "1" (real_dst),	                        /* EDX, 1       */
	      "2" (((int)rcv_msg) | L4_IPC_OPEN_IPC)	/* ESI, 2, rcv_msg -> EBP */
#ifdef SCRATCH
	      :
	      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
	      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
	      );
  return L4_IPC_ERROR(*result);
}

L4_INLINE int
l4_i386_ipc_chief_receive(l4_threadid_t src, l4_threadid_t *real_dst,
			  void *rcv_msg, dword_t *rcv_dword0, 
			  dword_t *rcv_dword1, l4_timeout_t timeout, 
			  l4_msgdope_t *result)
{
  asm
    volatile (
	      "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	      "pushl	%%ebx		\n\t"
	      "movl	%2,%%ebp	\n\t"
	      "movl	(%%edi),%%esi	\n\t"
	      "movl	4(%%edi),%%edi	\n\t" 
	      "pushl    %%edx           \n\t"
	      "int	$0x30		\n\t"
	      "popl     %%edi           \n\t"
	      "movl     %%ecx, 4(%%edi) \n\t"
	      "movl     %%ebp, 4(%%edi) \n\t"
	      "movl	%%ebx, %2	\n\t"
	      "popl	%%ebx		\n\t"
	      "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	      : 
	      "=a" (*result),				/* EAX,0 */
	      "=d" (*rcv_dword0),			/* EDX,1 */
	      "=S" (*rcv_dword1)		       	/* ESI,2 */
	      :
	      "c" (timeout),				/* ECX, 3 	*/
	      "D" (&src),		       		/* EDI, 4 	*/
	      "0" (L4_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
	      "1" (real_dst),                           /* EDX, 1       */
	      "2" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)) /* ESI, 2, rcv_msg -> EBP */
#ifdef SCRATCH
	      :
	      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
	      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
	      );
  return L4_IPC_ERROR(*result);
}


L4_INLINE int
l4_i386_ipc_chief_call(l4_threadid_t dest, l4_threadid_t fake_src,
		       const void *snd_msg, dword_t snd_dword0, 
		       dword_t snd_dword1, 
		       l4_threadid_t *real_dst,
		       void *rcv_msg, dword_t *rcv_dword0, 
		       dword_t *rcv_dword1, 
		       l4_timeout_t timeout, l4_msgdope_t *result)
{
  struct {
    dword_t d1, d2;
    l4_threadid_t fs;
  } dwords = {snd_dword0, snd_dword1, fake_src};

  asm 
    volatile(
	     "pushl	%%ebx		\n\t"  
	     "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	     "movl	4(%%edx), %%ebx	\n\t"
	     "movl	(%%edx), %%edx	\n\t"

	     "movl	%%edi, %%ebp	\n\t"

	     "pushl     %%esi           \n\t"
	     "pushl     12(%%esi)       \n\t"
	     "pushl     8(%%esi)        \n\t"
	     "movl	4(%%esi), %%edi	\n\t"		/* dest.lh.high -> edi */
	     "movl	(%%esi), %%esi	\n\t"		/* dest.lh.low  -> edi */

	     "int	$0x30		\n\t"
	     "lea       8(%%esp), %%esp \n\t"
	     "popl      %%esi           \n\t"
	     "movl      %%ecx, 8(%%esi) \n\t"
	     "movl      %%ebp, 12(%%esi) \n\t"
	     "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	     "movl	%%ebx, %%ecx	\n\t"
	     "popl	%%ebx		\n\t"
	     : 
	     "=a" (*result),				/* EAX, 0 	*/
	     "=d" (*rcv_dword0),			/* EDX, 1 	*/
	     "=c" (*rcv_dword1)				/* ECX, 2 	*/
	     :
	     "D" (((int)rcv_msg) & (~L4_IPC_OPEN_IPC)), /* EDI, 3, rcv msg -> ebp */
	     "S" (&dest),				/* ESI, 4, addr of dest	  */
	     "0" ((int)snd_msg),			/* EAX, 0  	*/
	     "1" (&dwords),				/* EDX, 1,	*/
	     "2" (timeout)				/* ECX, 2 	*/
#ifdef SCRATCH
	     :
	     "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
	     , "memory"
#endif /* SCRATCH_MEMORY */
#endif
	     );
  *real_dst = dwords.fs;
  return L4_IPC_ERROR(*result);  
}

L4_INLINE int
l4_i386_ipc_chief_reply_and_wait(l4_threadid_t dest, l4_threadid_t fake_src,
				 const void *snd_msg, 
				 dword_t snd_dword0, dword_t snd_dword1, 
				 l4_threadid_t *src, l4_threadid_t *real_dst,
				 void *rcv_msg, dword_t *rcv_dword0, 
				 dword_t *rcv_dword1, 
				 l4_timeout_t timeout, l4_msgdope_t *result)
{
  struct {
    dword_t d1, d2;
    l4_threadid_t *dest;
    l4_threadid_t *src;
    l4_threadid_t *real;
  } dwords = { snd_dword0, snd_dword1, &dest, src, &fake_src };

  asm 
    volatile(
	     /* eax, edx, ebx loaded, 
	      * edi contains rcv buffer address, must be moved to ebp,
	      * esi contains address of destination id,
	      * $5  address of src id
	      */
	     "pushl	%%ebx		\n\t"  
	     "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	     "movl	%%edi, %%ebp	\n\t" /* rcv desc */
	     "pushl     %%edx           \n\t" /* & dwords */

	     "movl      16(%%edx), %%esi \n\t" /* & real_id */
	     "pushl     %%esi           \n\t"
	     "pushl     4(%%esi)        \n\t" /* real_id.high */
	     "pushl     (%%esi)         \n\t" /* real_id.low */

	     "movl      8(%%edx), %%esi \n\t" /* & dest */
	     "movl      4(%%esi), %%edi \n\t" /* dest.high */
	     "movl      (%%esi), %%esi  \n\t" /* dest.low */
	     
	     "movl	4(%%edx), %%ebx	\n\t" /* dword 2 */
	     "movl	(%%edx), %%edx	\n\t" /* dword 1 */

	     "int	$0x30		\n\t"
	     "leal      8(%%esp), %%esp \n\t"

	     "xchgl	%%ebp, (%%esp)	\n\t" /* & real_id */
	     "movl      %%ecx, (%%ebp)  \n\t" /* real_id.low */
	     "popl      %%ecx           \n\t"
	     "movl      %%ecx, 4(%%ebp) \n\t" /* real_id.high */

	     "popl      %%ebp           \n\t" /* & dwords */
	     "movl      12(%%ebp), %%ebp \n\t" /* & src */
	     
	     "movl	%%esi, (%%ebp)  \n\t" /* esi -> src.lh.low  */
	     "movl	%%edi, 4(%%ebp) \n\t" /* edi -> src.lh.high */

	     "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	     "movl	%%ebx, %%ecx	\n\t"
	     "popl	%%ebx		\n\t"
	     
      : 
      "=a" (*result),				/* EAX, 0 	*/
      "=d" (*rcv_dword0),			/* EDX, 1 	*/
      "=c" (*rcv_dword1)			/* ECX, 2 	*/
      :
      "D" (((int)rcv_msg) | L4_IPC_OPEN_IPC),	/* edi, 3  -> ebp rcv_msg */
      "1" (&dwords),				/* ESI ,4	*/
      "2" (timeout),				/* ECX, 2	*/
      "0" ((int)snd_msg)			/* EAX, 0 	*/
#ifdef SCRATCH
      :
      "esi", "edi", "ecx"
#ifdef SCRATCH_MEMORY
      , "memory"
#endif /* SCRATCH_MEMORY */
#endif
      );
  *real_dst = *dwords.real;
  return L4_IPC_ERROR(*result);
}


L4_INLINE int
l4_i386_ipc_chief_send(l4_threadid_t dest, l4_threadid_t fake_src,
		       const void *snd_msg, dword_t snd_dword0, 
		       dword_t snd_dword1, 
		       l4_timeout_t timeout, l4_msgdope_t *result)
{
  struct {
    l4_threadid_t *d, *fake;
  } addresses = { &dest, &fake_src };

  asm 
    volatile(
	     "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
	     "pushl     %%ebx		\n\t"
	     "movl	%%esi, %%ebx	\n\t"
	     "movl      4(%%edi), %%ebp \n\t"
	     "pushl     4(%%ebp)        \n\t"
	     "pushl     (%%ebp)         \n\t"
	     "movl      (%%edi), %%ebp  \n\t"
	     "movl	(%%ebp),%%esi	\n\t"
	     "movl	4(%%ebp),%%edi	\n\t"
	     "movl	%5 ,%%ebp	\n\t"
	     "int	$0x30		\n\t"
	     "leal      8(%%esp), %%esp \n\t"
	     "popl	%%ebx		\n\t"
	     "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
	     : 
	     "=a" (*result)				/* EAX,0 	*/
	     :
	     "d" (snd_dword0),				/* EDX, 1	*/
	     "c" (timeout),				/* ECX, 2	*/
	     "S" (snd_dword1),				/* EBX, 3	*/
	     "D" (&addresses),				/* EDI, 4	*/
	     "i" (L4_IPC_NIL_DESCRIPTOR),		/* Int, 5 	*/
	     "0" ((int)snd_msg)				/* EAX, 0 	*/
#ifdef SCRATCH
	     :
	     "esi", "edi", "ecx", "edx"
#ifdef SCRATCH_MEMORY
	     , "memory"
#endif /* SCRATCH_MEMORY */
#endif
	     );
  return L4_IPC_ERROR(*result);
}




#endif /* __L4_IPC__ */
