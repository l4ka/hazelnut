#include <nucleus/types.h>
#include <nucleus/kernel.h>
#include <nucleus/ipc.h>
#include <nucleus/kdebug.h>

/* 

Hairy C bindings 

Problems: Not enough registers 

*/

int 
ln_i386_ipc_wait_redirect(ln_ipc_deceit_ids_t *ids,
		 void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
		 ln_timeout_t timeout, ln_msgdope_t *result)
{
  volatile ln_msgdope_t x;
  volatile unsigned long y;

#ifdef WR_DEBUG
  x.msgdope = 13;
  y = 14;
#endif  

  __asm__
    __volatile__(
		 "pushl	%%ebp		\n\t"		/* save ebp, no memory 
							   references ("m") after 
							   this point */
		 "movl	%%ebx,%%ebp	\n\t"
		 "int	$0x30		\n\t"
		 "pushl    %%ebp           \n\t"           /* save ebp register */
		 "pushl    %%eax           \n\t"           /* push rcv msgdope on stack */
		 "movl     %%esp,%%ebp     \n\t"           /* get current stack pointer address */
#ifdef WR_DEBUG
		 "popl     0x34(%%ebp)       \n\t"           /* pop eax into variable x : 0x34 w/ printk's */
		 "popl     0x30(%%ebp)       \n\t"           /* pop ebp into variable y : 0x30 w/ printk's  */
#else
		 "popl     0x2c(%%ebp)       \n\t"           /* pop eax into variable x : 0x34 w/ printk's */
		 "popl     0x28(%%ebp)       \n\t"           /* pop ebp into variable y : 0x30 w/ printk's  */
#endif
		 "testb	$0b100,%%al	\n\t"
		 "jnz	1f		\n\t"
		 "subl	%%ecx,%%ecx	\n\t"
		 "jmp	2f		\n\t"
		 "1:			\n\t"
		 "movl	%%ecx,%%eax	\n\t"
		 "movl	%%ebp,%%ecx	\n\t"
		 "2:			\n\t" 

		 "popl	%%ebp		\n\t"		/* restore ebp, no memory 
							   references ("m") before 
							   this point */
		 : 
		 "=a" (ids->dest.lh.low),		/* EAX,0 */
		 "=d" (*rcv_dword0),			/* EDX,1 */
		 "=b" (*rcv_dword1),			/* EBX,2 */
		 /* "=c" (ids->dest.lh.high), */             /* ECX,5 */
		 "=D" (ids->true_src.lh.high),		/* EDI,3 */
		 "=S" (ids->true_src.lh.low)		/* ESI,4 */
		 :
		 "c" (timeout),				/* ECX, 5 	*/
		 "0" (LN_IPC_NIL_DESCRIPTOR),		/* EAX, 0 	*/
		 "2" (((int)rcv_msg) | LN_IPC_OPEN_IPC)	/* EBX, 2 rcv_msg -> EBP */
#ifdef SCRATCH
		 :
		 "ecx"
#ifdef SCRATCH_MEMORY
		 , "memory"
#endif /* SCRATCH_MEMORY */
#endif
		 );

  *result = x;  
  ids->dest.lh.high = y;

  return LN_IPC_ERROR(*result);
}




int 
ln_i386_ipc_reply_deceiting_and_wait_redirect(ln_ipc_deceit_ids_t snd_ids,
				     const void *snd_msg, 
				     dword_t snd_dword0, dword_t snd_dword1,
				     ln_ipc_deceit_ids_t *rcv_ids,
				     void *rcv_msg, 
				     dword_t *rcv_dword0, dword_t *rcv_dword1, 
				     ln_timeout_t timeout, 
				     ln_msgdope_t *result)
{
  volatile ln_msgdope_t x;
  struct {
    ln_ipc_deceit_ids_t *snd_ids;
    ln_ipc_deceit_ids_t *rcv_ids;
  } addresses = { &snd_ids, rcv_ids};

#ifdef RDWR_DEBUG
  printk("sender: %x %x; send dest: %x %x\n", (unsigned)snd_ids.true_src.lh.low, (unsigned)snd_ids.true_src.lh.high,
	 (unsigned)snd_ids.dest.lh.low, (unsigned)snd_ids.dest.lh.high);
  printk("rcv_ids @ %x\n", (unsigned) rcv_ids);
  x.msgdope = 15;
#endif

  __asm__
    __volatile__(
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
		 "movl	(%%esi), %%esi  \n\t"		/* ids.dest.lh.low  -> edi */
		 "int	$0x30		\n\t"
		 "addl	$8, %%esp	\n\t"		/* remove true_src from stack*/
		 "pushl    %%ebp           \n\t"        /* save ebp after return */
		 "pushl    %%eax           \n\t"        /* push msgdope on stack */
		 "movl     %%esp,%%ebp  \n\t"           /* save esp in ebp */ 
#ifdef RDWR_DEBUG
		 "popl     92(%%ebp)  \n\t"           /* pop eax into variable result (x): 92 w/ printk's */
#else
		 "popl     0x38(%%ebp)  \n\t"           /* pop eax into variable result (x): 92 w/ printk's */
#endif
		 "popl     %%ebp        \n\t"           /* restore ebp */

		 "testb	$0b100,%%al	\n\t"  
		 "jnz	1f		\n\t"
		 "subl	%%ecx,%%ecx	\n\t"
		 "jmp	2f		\n\t"
		 "1:			\n\t"
		 "movl	%%ecx,%%eax	\n\t"
		 "movl	%%ebp,%%ecx	\n\t"
		 "2:			\n\t"

		 "popl  %%ebp           \n\t"           /* pop addresses off stack (see pushl %%esi) */
		 "movl  0x4(%%ebp),%%ebp  \n\t"
		 "movl	%%esi, 8(%%ebp)  \n\t"		/* esi -> rcv_ids->true_src.lh.low  */
		 "movl	%%edi, 12(%%ebp)\n\t"		/* edi -> rcv_ids->true_src.lh.high */
		 "popl	%%ebp		\n\t"         	/* restore ebp, no memory 
							   references ("m") before 
							   this point */

		 : 
		 "=a" (rcv_ids->dest.lh.low),		/* EAX, 0 	*/
		 "=d" (*rcv_dword0),			/* EDX, 1 	*/
		 "=b" (*rcv_dword1),			/* EBX, 2 	*/
		 "=c" (rcv_ids->dest.lh.high)              /* ECX,5 */
		 :
		 "S" (&addresses),				/* addresses, 3	*/
		 "c" (timeout),				/* ECX, 4	*/
		 "D" (((int)rcv_msg) | LN_IPC_OPEN_IPC),	/* EDI, 5 -> EBP rcv_msg */
		 "0" (((int)snd_msg) | (LN_IPC_DECEIT)),	/* EAX, 0 	*/
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
  *result = x;

#ifdef RDWR_DEBUG
  printk("result: %x; recv: %x %x; recv dest: %x %x\n", (unsigned) (*result).msgdope, 
	 (unsigned)rcv_ids->true_src.lh.low, (unsigned)rcv_ids->true_src.lh.high,
	 (unsigned)rcv_ids->dest.lh.low, (unsigned)rcv_ids->dest.lh.high);
  enter_kdebug("rd/wr");
#endif

  return LN_IPC_ERROR(*result);
}




static inline int 
ln_i386_ipc_send_deceiting_and_receive(ln_ipc_deceit_ids_t ids,
				     const void *snd_msg, 
				     dword_t snd_dword0, dword_t snd_dword1,
				     ln_threadid_t src,
				     void *rcv_msg, 
				     dword_t *rcv_dword0, dword_t *rcv_dword1, 
				     ln_timeout_t timeout, 
				     ln_msgdope_t *result)
{
  struct {
    ln_ipc_deceit_ids_t *ids;
    ln_threadid_t *src;
  } addresses = { &ids, &src };

  asm(
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
      "D" (((int)rcv_msg) & (~LN_IPC_OPEN_IPC)),	/* EDI, 5 -> ebp rcv_msg */
      "0" (((int)snd_msg) | (LN_IPC_DECEIT)),	/* EAX, 0 	*/
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
  return LN_IPC_ERROR(*result);
}



