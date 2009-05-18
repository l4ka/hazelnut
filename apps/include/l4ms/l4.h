/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     l4ms/l4.h
 * Description:   standard l4 include file
 *                
 * @LICENSE@
 *                
 * $Id: l4.h,v 1.2 2001/12/13 08:47:07 uhlig Exp $
 *                
 ********************************************************************/

#ifndef __L4__L4_H__
#define __L4__L4_H__




/**********************************************************************
 *
 * base types
 *
 **********************************************************************/

/* architecture dependent types ???
#include <l4/arch/types.h>
*/

typedef unsigned __int64	qword_t;
typedef unsigned __int32	dword_t;
typedef unsigned __int16	word_t;
typedef unsigned __int8		byte_t;

typedef signed __int64		sqword_t;
typedef signed __int32		sdword_t;
typedef signed __int16		sword_t;
typedef signed __int8		sbyte_t;

typedef dword_t*                ptr_t;

#ifndef NULL
#define NULL			(0)
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifndef TRUE
#define TRUE	(!FALSE)
#endif



#if defined(L4_NO_SYSCALL_INLINES) || defined(__GENERATE_L4LIB__)
# define L4_INLINE
#else
# define L4_INLINE extern __inline
#endif




/**********************************************************************
 *
 * configuration ???
 *
 **********************************************************************/


#define L4_NUMBITS_THREADS	22

/**********************************************************************
 *
 * thread ids
 *
 **********************************************************************/


#if defined(CONFIG_VERSION_X0)
typedef union {
    struct {
	unsigned	version		: 10;
	unsigned	thread		: 6;
	unsigned	task		: 8;
	unsigned	chief		: 8;
    } x0id;
    struct {
	unsigned	version		: 10;
	unsigned	thread		: 6+8;
	unsigned	chief		: 8;
    } id;
    dword_t raw;
} l4_threadid_t;

/*
 * Some well known thread id's.
 */
#define L4_KERNEL_ID	((l4_threadid_t) { id : {0,1,0,0} })
#define L4_SIGMA0_ID	((l4_threadid_t) { id : {0,0,2,0} })
#define L4_ROOT_TASK_ID	((l4_threadid_t) { id : {0,0,4,0} })
#define L4_INTERRUPT(x)	((l4_threadid_t) { raw : {x + 1 } })

#elif defined(CONFIG_VERSION_X1)
typedef union {
    struct {
	unsigned	thread		:L4_NUMBITS_THREADS;
	unsigned	version		:(32-L4_NUMBITS_THREADS);
    } id;
    dword_t raw;
} l4_threadid_t;

/*
 * Some well known thread id's.
 */
#define L4_KERNEL_ID	((l4_threadid_t) { id : {thread:1, version:0} })
#define L4_SIGMA0_ID	((l4_threadid_t) { id : {thread:2, version:0} })
#define L4_ROOT_TASK_ID	((l4_threadid_t) { id : {thread:3, version:0} })

#else
#error unknown kernel interface specification
#endif


#define L4_NIL_ID	((l4_threadid_t) { raw :  0 })
#define L4_INVALID_ID	((l4_threadid_t) { raw : ~0 })

#define l4_is_nil_id(id)	((id).raw == L4_NIL_ID.raw)
#define l4_is_invalid_id(id)	((id).raw == L4_INVALID_ID.raw)

#ifdef __cplusplus
static inline int operator == (const l4_threadid_t & t1,
			       const l4_threadid_t & t2)
{
    return t1.raw == t2.raw;
}

static inline int operator != (const l4_threadid_t & t1,
			       const l4_threadid_t & t2)
{
    return t1.raw != t2.raw;
}
#endif /* __cplusplus */







/**********************************************************************
 *
 * flexpages
 *
 **********************************************************************/
/**********************************************************************
 *
 * timeouts
 *
 **********************************************************************/

typedef struct {
  unsigned rcv_exp:4;
  unsigned snd_exp:4;
  unsigned rcv_pfault:4;
  unsigned snd_pfault:4;
  unsigned snd_man:8;
  unsigned rcv_man:8;
} l4_timeout_struct_t;

typedef union {
  dword_t raw;
  l4_timeout_struct_t timeout;
} l4_timeout_t;

#define L4_IPC_NEVER	    ((l4_timeout_t) { raw: {0}})
#define L4_IPC_TIMEOUT_NULL ((l4_timeout_t) { timeout: {15, 15, 15, 15, 0, 0}})

/**********************************************************************
 *
 * ipc message dopes
 *
 **********************************************************************/

typedef union
{
    struct {
	dword_t	msg_deceited	:1;
	dword_t	fpage_received	:1;
	dword_t	msg_redirected	:1;
	dword_t	src_inside	:1;
	dword_t	error_code	:4;
	dword_t	strings		:5;
	dword_t	dwords		:19;
    } md;
    dword_t	raw;
} l4_msgdope_t;

/* 
 * Some macros to make result checking easier
 */

#define L4_IPC_ERROR(x)		((x).md.error_code)

/*
 * IPC results

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

 */




/**********************************************************************
 *
 * sched params
 *
 **********************************************************************/

/**********************************************************************
 *
 * system calls
 *
 **********************************************************************/
#include <l4/x86/l4.h>







#endif /* !__L4__L4_H__ */
