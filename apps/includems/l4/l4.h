/**********************************************************************
 * (c) 1999, 2000 by University of Karlsruhe
 *
 * filename: l4.h
 * 
 * $Log: l4.h,v $
 * Revision 1.1  2000/05/16 14:41:10  uhlig
 * include files for x86 (mostly tested)
 * with these files it is possible to compile l4 apps with MSDEV-Studio
 * simply add includems into the include search path (before you include
 * the normal include path) and ms-c will function properly
 *
 * Revision 1.1  2000/03/11 10:29:38  uhlig
 * include files for the Microsoft C compiler.
 * This allows to compile L4/x86 files with devstudio.
 * currently untested on a real box - only by looking at the generated asm.
 *
 * Revision 1.6  2000/02/24 21:53:13  ud3
 * updated l4_threadid_t to conform with kernel declaration
 *
 * Revision 1.5  2000/02/17 13:42:39  skoglund
 * Fixed error in the L4_NIL_ID and L4_INVALID_ID definitions.
 *
 * Revision 1.4  2000/02/14 20:46:17  uhlig
 * timeouts added
 *
 * Revision 1.3  2000/02/14 19:57:37  ud3
 * updated version X.0 sigma0 id to 2 and X.0 root task id to 4
 *
 * Revision 1.2  2000/02/08 14:55:41  ud3
 * *** empty log message ***
 *
 * Revision 1.1  2000/02/07 23:27:17  ud3
 * Yet another try to build a plain userland.
 *
 *
 */

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

typedef unsigned __int64	QWORD;
typedef unsigned __int32	DWORD;
typedef unsigned __int16	WORD;
typedef unsigned __int8		BYTE;

typedef const char *		LPCSTR;
typedef char *			LPSTR;
typedef void *			LPVOID;


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
union l4_threadid_t {
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
    inline l4_threadid_t(dword_t _raw) { raw = _raw; }
    inline l4_threadid_t() {}
};
typedef union l4_threadid_t l4_threadid_t;

/*
 * Some well known thread id's.
 */
#define L4_KERNEL_ID	((l4_threadid_t) { id : {0,1,0,0} })
#define L4_SIGMA0_ID	((l4_threadid_t) { id : {0,0,2,0} })
#define L4_ROOT_TASK_ID	((l4_threadid_t) { id : {0,0,4,0} })
#define L4_INTERRUPT(x)	((dword_t)(x + 1))

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


#define L4_NIL_ID	((dword_t) 0)
#define L4_INVALID_ID	((dword_t)~0)

#define l4_is_nil_id(id)	((id).raw == L4_NIL_ID)
#define l4_is_invalid_id(id)	((id).raw == L4_INVALID_ID)

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

union l4_timeout_t {
    dword_t raw;
    l4_timeout_struct_t timeout;
    l4_timeout_t() {}
    l4_timeout_t(dword_t _raw) { raw = _raw; }
};
typedef union l4_timeout_t l4_timeout_t;

#define L4_IPC_NEVER	    (0U)
#define L4_IPC_TIMEOUT_NULL (0xFFFF0000U)

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
