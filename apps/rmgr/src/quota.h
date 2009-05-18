#ifndef QUOTA_H
#define QUOTA_H

#include <l4/compiler.h>

#include "globals.h"

typedef struct
{
  unsigned low, high;
  unsigned max, current;
} __attribute__((packed)) u_quota_t;

typedef struct
{
  unsigned short low, high;
  unsigned short max, current;
} __attribute__((packed)) us_quota_t;

typedef struct
{
  unsigned char low, high;
  unsigned char max, current;
} __attribute__((packed)) ub_quota_t;

typedef struct
{
  unsigned max, current;
} __attribute__((packed)) mask_quota_t;

typedef struct
{
  u_quota_t mem, himem;
  us_quota_t task;
  ub_quota_t small;
  mask_quota_t irq;  
  unsigned short log_mcp;
} __attribute__((packed)) quota_t;

typedef struct
{
  unsigned char mcp;
  unsigned char prio;
  unsigned char small_space;
  unsigned char mods;
} __attribute__((packed)) bootquota_t;

extern quota_t quota[TASK_MAX];
extern bootquota_t bootquota[TASK_MAX];

L4_INLINE void quota_init(void);

L4_INLINE void quota_init(void)
{
  unsigned i;

  for (i = 0; i < TASK_MAX; i++)
    {
      quota[i].mem.low = quota[i].mem.current = 0;
      quota[i].mem.high = quota[i].mem.max = -1;
      quota[i].himem.low = quota[i].himem.current = 0;
      quota[i].himem.high = quota[i].himem.max = -1;
      quota[i].task.low = quota[i].task.current = 0;
      quota[i].task.high = quota[i].task.max = -1;
      quota[i].small.low = quota[i].small.current = 0;
      quota[i].small.high = quota[i].small.max = -1;
      quota[i].irq.current = 0;
      quota[i].irq.max = -1;
      quota[i].log_mcp = -1;

      bootquota[i].mcp = 255;	/* L4 default */
      bootquota[i].prio = 0x10;	/* L4 default */
      bootquota[i].small_space = 0xff; /* for L4, 0xff means "no change" */
      bootquota[i].mods = 0;	/* number of boot mods assn'd to this task */
    }
}

#define __quota_check_u(q, where, amount)				\
( ((q).max >= (amount) && (q).current <= (q).max - (amount)) ?		\
  (((q).low <= (where) && (q).high >= (where) + (amount) - 1) ? 1 : 0) 	\
  : 0)

#define quota_check_mem(t, where, amount)			\
( __quota_check_u((where) < mem_high ? quota[(t)].mem 		\
		                     : quota[(t)].himem,	\
		  (where), (amount)))

#define __quota_alloc_u(q, where, amount)				\
( ((q).max >= (amount) && (q).current <= (q).max - (amount)) ?		\
  (((q).low <= (where) && (q).high >= (where) + (amount) - 1) ?		\
   (q).current += (amount), 1 : 0)					\
  : 0)
     
#define quota_alloc_mem(t, where, amount)			\
( __quota_alloc_u((where) < mem_high ? quota[(t)].mem 		\
		                     : quota[(t)].himem,	\
		  (where), (amount)))

#define quota_alloc_task(t, where)			\
( __quota_alloc_u(quota[(t)].task, (where), 1))

#define quota_alloc_small(t, where)			\
( __quota_alloc_u(quota[(t)].small, (where), 1))

#define quota_alloc_irq(t, where)			\
( (quota[(t)].irq.max & (1L << (where))) ?		\
  (quota[(t)].irq.current |= (1L << (where))), 1 : 0)


#define __quota_free_u(q, amount) ( (q).current -= (amount) )

#define quota_free_mem(t, where, amount) 				\
__quota_free_u((where) < mem_high ? quota[(t)].mem : quota[(t)].himem,	\
	       (amount))

#define quota_free_task(t, where) __quota_free_u(quota[(t)].task, 1)

#define quota_free_small(t, where) __quota_free_u(quota[(t)].small, 1)

#define quota_free_irq(t, where)  ( quota[(t)].irq.current &= ~(1L << (where)))

#endif
