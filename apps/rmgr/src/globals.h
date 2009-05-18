#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdlib.h>
#include <flux/machine/types.h>
#include <l4/l4.h>

/* globals defined here (when included from globals.c) */

#ifdef DEFINE_GLOBALS
# define __EXTERN
#else
# define __EXTERN extern
#endif

/* thread ids of global importance */
__EXTERN l4_threadid_t my_pager, my_preempter, myself;
__EXTERN l4_threadid_t rmgr_pager_id, rmgr_super_id;

/* RAM memory size */
__EXTERN vm_offset_t mem_lower, mem_upper;

/* number of small address spaces configured at boot */
__EXTERN unsigned small_space_size;

__EXTERN int debug, debug_log_mask, debug_log_types, verbose, no_pentium;
__EXTERN int no_checkdpf;

#undef __EXTERN

/* more global stuff */

typedef byte_t owner_t;		/* an owner is a task number < 256 */
#define O_MAX (255)
#define O_FREE (0)
#define O_RESERVED (1)

#ifndef __VERSION_X__
#define TASK_MAX (1L << 11)
#else /* __X_ADAPTION__ */
#define TASK_MAX (1L << 8)
#endif /* __X_ADAPTION__ */

extern int _start;		/* begin of RMGR memory -- defined in crt0.S */
extern int _stext;		/* begin of RGMR text -- defined by rmgr.ld */
extern int _stack;		/* end of RMGR stack -- defined in crt0.S */
extern int _end;		/* end of RGMR memory -- defined by rmgr.ld */
extern int _etext;		/* begin of data memory -- defined by crt0.S */

/* the check macro is like assert(), but also evaluates its argument
   if NCHECK is defined */
#ifdef NCHECK
# define check(expression) ((void)(expression))
#else /* ! NCHECK */
# define check(expression) \
         ((void)((expression) ? 0 : \
                (panic("RMGR: "__FILE__":%u: failed check `"#expression"'", \
                        __LINE__), 0)))
#endif /* ! NCHECK */

#endif
