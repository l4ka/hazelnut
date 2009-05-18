/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001,  Karlsruhe University
 *               and Dresden University
 *                
 * File path:     l4ms/x86/ipc.h
 * Description:   ipc syscalls
 *                
 * @LICENSE@
 *                
 * $Id: ipc.h,v 1.2 2001/12/13 08:47:30 uhlig Exp $
 *                
 ********************************************************************/

#ifndef __L4_X86_IPC_H__ 
#define __L4_X86_IPC_H__ 

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


L4_INLINE int
l4_ipc_call(l4_threadid_t dest, 
	    const void *snd_msg, dword_t snd_dword0, dword_t snd_dword1, 
	    dword_t snd_dword2, void *rcv_msg, dword_t *rcv_dword0, 
	    dword_t *rcv_dword1, dword_t *rcv_dword2,
	    l4_timeout_t timeout, l4_msgdope_t *result)
{
    struct {
	dword_t send_msg;
	dword_t rcv_msg;
    } tmp = {
	((dword_t)snd_msg) & (~L4_IPC_DECEIT),
	((dword_t)rcv_msg) & (~L4_IPC_OPEN_IPC)
    };
    __asm {
	lea	eax, tmp;
	mov	ecx, timeout;
	mov	edx, snd_dword0;
	mov	ebx, snd_dword1;
	mov	edi, snd_dword2;
	mov	esi, dest;
	push	ebp;
	mov	ebp, [eax + 4];
	mov	eax, [eax];
	int	0x30;
	pop	ebp;
	mov	ecx, rcv_dword0;
	mov	[ecx], edx;
	mov	ecx, rcv_dword1;
	mov	[ecx], ebx;
	mov	ecx, rcv_dword2;
	mov	[ecx], edi;
	mov	ecx, result;
	mov	[ecx], eax;
    }
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
    struct {
	dword_t send_msg;
	dword_t rcv_msg;
    } tmp = {
	((dword_t)snd_msg) & (~L4_IPC_DECEIT),
	((dword_t)rcv_msg) | L4_IPC_OPEN_IPC
    };
    __asm {
	lea	eax, tmp;
	mov	ecx, timeout;
	mov	edx, snd_dword0;
	mov	ebx, snd_dword1;
	mov	edi, snd_dword2;
	mov	esi, dest;
	push	ebp;
	mov	ebp, [eax + 4];
	mov	eax, [eax];
	int	0x30;
	pop	ebp;
	mov	ecx, rcv_dword0;
	mov	[ecx], edx;
	mov	ecx, rcv_dword1;
	mov	[ecx], ebx;
	mov	ecx, rcv_dword2;
	mov	[ecx], edi;
	mov	ecx, result;
	mov	[ecx], eax;    
    }
    return L4_IPC_ERROR(*result);
}



L4_INLINE int
l4_ipc_send(l4_threadid_t dest, 
	    const void *snd_msg, dword_t snd_dword0, dword_t snd_dword1, 
	    dword_t snd_dword2, 
	    l4_timeout_t timeout, l4_msgdope_t *result)
{
    dword_t _snd_msg = ((dword_t)snd_msg) & (~L4_IPC_DECEIT);
    __asm {
	mov	eax, _snd_msg;
	mov	edx, snd_dword0;
	mov	ebx, snd_dword1;
	mov	edx, snd_dword2;
	mov	esi, dest;
	mov	ecx, timeout;
	push	ebp;
	mov	ebp, L4_IPC_NIL_DESCRIPTOR;
	int	0x30;
	pop	ebp;
	mov	ecx, result;
	mov	[ecx], eax;
    }
    return L4_IPC_ERROR(*result);
};


L4_INLINE int
l4_ipc_wait(l4_threadid_t *src,
	    void *rcv_msg, 
	    dword_t *rcv_dword0, dword_t *rcv_dword1, dword_t *rcv_dword2,
	    l4_timeout_t timeout, l4_msgdope_t *result)
{
    dword_t _rcv_msg = ((int)rcv_msg) | L4_IPC_OPEN_IPC;
    __asm {
	mov	ecx, timeout;
	mov	eax, L4_IPC_NIL_DESCRIPTOR;
	push	ebp;
	mov	ebp, _rcv_msg;
	int	0x30;
	pop	ebp;
	mov	ecx, result;
	mov	[ecx], eax;
	mov	ecx, rcv_dword0;
	mov	[ecx], edx;
	mov	ecx, rcv_dword1;
	mov	[ecx], ebx;
	mov	ecx, rcv_dword2;
	mov	[ecx], edi;
	mov	ecx, src;
	mov	[ecx], esi;
    }
    return L4_IPC_ERROR(*result);
}

L4_INLINE int
l4_ipc_receive(l4_threadid_t src,
	       void *rcv_msg, dword_t *rcv_dword0, dword_t *rcv_dword1, 
	       dword_t *rcv_dword2,
	       l4_timeout_t timeout, l4_msgdope_t *result)
{
    dword_t _rcv_msg = ((int)rcv_msg) & (~L4_IPC_OPEN_IPC);
    __asm {
	mov	ecx, timeout;
	mov	eax, L4_IPC_NIL_DESCRIPTOR;
	push	ebp;
	mov	ebp, _rcv_msg;
	int	0x30;
	pop	ebp;
	mov	ecx, result;
	mov	[ecx], eax;
	mov	ecx, rcv_dword0;
	mov	[ecx], edx;
	mov	ecx, rcv_dword1;
	mov	[ecx], ebx;
	mov	ecx, rcv_dword2;
	mov	[ecx], edi;
	mov	ecx, src;
	mov	[ecx], esi;
    }
    return L4_IPC_ERROR(*result);
}

#if 0
L4_INLINE int l4_ipc_sleep(int man, int exp)
{
    l4_msgdope_t result;
    l4_timeout_t timeout = ((l4_timeout_t.timeout) {exp, 0, 0, 0, 0, man});

    __asm {
	mov	ecx, timeout;
	mov	eax, L4_IPC_NIL_DESCRIPTOR;
	sub	esi, esi;   /* no send */
	push	ebp;
	mov	ebp, L4_NIL_ID;
	int	0x30;
	popl	ebp;
	mov	ecx, result;
	mov	[ecx], eax;
    }
    return 0;
}
#endif

#endif /* __L4_X86_IPC_H__ */

