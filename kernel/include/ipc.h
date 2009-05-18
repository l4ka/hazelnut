/*********************************************************************
 *                
 * Copyright (C) 1999, 2000, 2001, 2002,  Karlsruhe University
 *                
 * File path:     ipc.h
 * Description:   IPC message and IPC handling declarations.
 *                
 * @LICENSE@
 *                
 * $Id: ipc.h,v 1.20 2002/06/07 17:01:45 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __IPC_H__
#define __IPC_H__

typedef struct {
    unsigned grant:1;
    unsigned write:1;
    unsigned size:6;
    unsigned uncacheable:1;
    unsigned unbufferable:1;
    unsigned zero:2;
    unsigned page:20;
} fpage_struct_t;

typedef union {
    dword_t raw;
    fpage_struct_t fpage;
} fpage_t;

#define FPAGE_WHOLE_SPACE	((fpage_t) { fpage: { 0, 0, 32, 0, 0 } })
#define FPAGE_OWN_SPACE		(1<<31)
#define FPAGE_FLUSH		(1<<1)


typedef struct {
  unsigned msg_deceited:1;
  unsigned fpage_received:1;
  unsigned msg_redirected:1;
  unsigned src_inside:1;
  unsigned snd_error:1;
  unsigned error_code:3;
  unsigned strings:5;
  unsigned dwords:19;
} msgdope_struct_t;

typedef union {
  dword_t raw;
  msgdope_struct_t msgdope;
} msgdope_t;


typedef struct {
    fpage_t rcv_fpage;
    msgdope_t size_dope;
    msgdope_t send_dope;
    dword_t dwords[0];
} memmsg_t;

typedef struct {
    dword_t	send_size:31;
    dword_t	send_continue:1;
    ptr_t	send_address;

    dword_t	rcv_size:31;
    dword_t	rcv_continue:1;
    ptr_t	rcv_address;
} stringdope_t;



#define IPC_DESC_REGONLY	0x00
#define IPC_DESC_REGMAP		0x02

#define IPC_PAGEFAULT_FPAGE	(0x80)
#define IPC_PAGEFAULT_DOPE	(IPC_PAGEFAULT_FPAGE | IPC_DESC_REGMAP)

/* error codes */
#define IPC_ERR_SEND		0x10
#define IPC_ERR_RECV		0x00
#define IPC_ERR_EXIST		0x10
#define IPC_ERR__TIMEOUT	0x20
#define IPC_ERR_CANCELED	0x40
#define IPC_ERR_ABORTED		0xC0
#define IPC_ERR_CUTMSG		0xe0
#define IPC_ERR_SENDTIMEOUT	(IPC_ERR__TIMEOUT + IPC_ERR_SEND)
#define IPC_ERR_RECVTIMEOUT	(IPC_ERR__TIMEOUT + IPC_ERR_RECV)

#define IS_IPC_ERROR(x)		((x) & 0xf0)

/* what happened...*/
#define IPC_MAP_MESSAGE		0x02

/* Number of dwords before not included in message buffer. */
#define IPC_DWORD_GAP	3

/* slave bit for indirect strings */
#define IPC_DOPE_CONTINUE	0x80000000


/* 
 * IPC timeouts
 */
typedef struct {
	unsigned rcv_exp:4;
	unsigned snd_exp:4;
  	unsigned rcv_pfault:4;
  	unsigned snd_pfault:4;
  	unsigned snd_man:8;
  	unsigned rcv_man:8;
} timeout_struct_t;


typedef union {
	dword_t raw;
	timeout_struct_t timeout;
} timeout_t;

/* 
 * timeout macros 
 */
#define TIMEOUT_NEVER		    ((timeout_t) { 0 })
#define IS_INFINITE_SEND_TIMEOUT(x) (x.timeout.snd_exp == 0)
#define IS_INFINITE_RECV_TIMEOUT(x) (x.timeout.rcv_exp == 0)
#define TIMEOUT(exp,man)	    ((1 << (2 * (15 - exp))) * man)
#define IS_INFINITE_RECV_PFAULT(x)  (~x.timeout.rcv_pfault)
#define IS_INFINITE_SEND_PFAULT(x)  (~x.timeout.snd_pfault)

INLINE timeout_t GET_RECV_PF_TIMEOUT(timeout_t to)
{
    timeout_t pf_to;
    if (!IS_INFINITE_RECV_PFAULT(to))
    {
	pf_to.timeout.snd_man = 1;
	pf_to.timeout.rcv_man = 1;
	pf_to.timeout.snd_exp = pf_to.timeout.rcv_exp = to.timeout.rcv_pfault;
    }
    else 
	pf_to.timeout.snd_exp = pf_to.timeout.rcv_exp = 0;

    return pf_to;
}

INLINE timeout_t GET_SEND_PF_TIMEOUT(timeout_t to)
{
    timeout_t pf_to;
    if (!IS_INFINITE_SEND_PFAULT(to))
    {
	pf_to.timeout.snd_man = 1;
	pf_to.timeout.rcv_man = 1;
	pf_to.timeout.snd_exp = pf_to.timeout.rcv_exp = to.timeout.rcv_pfault;
    }
    else 
	pf_to.timeout.snd_exp = pf_to.timeout.rcv_exp = 0;

    return pf_to;
}

class space_t;
int map_fpage(tcb_t *from, tcb_t *to, dword_t base, fpage_t snd_fp, fpage_t rcv_fp);
void fpage_unmap(space_t *space, fpage_t fpage, dword_t mapmask);


ptr_t get_copy_area(tcb_t * from, tcb_t * to, ptr_t addr);
void free_copy_area(tcb_t * tcb);

/* ipc.c */
notify_procedure(copy_pagefault, dword_t fault, tcb_t * partner);

int ipc_handle_kernel_id(tcb_t* current, l4_threadid_t dest);
void extended_transfer(tcb_t * from, tcb_t * to, dword_t snd_desc);

#endif /* __IPC_H__ */
