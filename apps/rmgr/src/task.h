#ifndef TASK_H
#define TASK_H

#include <string.h>

#include <l4/compiler.h>

#include "globals.h"
#include "quota.h"

L4_INLINE void task_init(void);
L4_INLINE int task_alloc(unsigned taskno, owner_t owner);
L4_INLINE int task_free(unsigned taskno, owner_t owner);
L4_INLINE owner_t task_owner(unsigned taskno);

extern owner_t __task[TASK_MAX];

L4_INLINE void task_init(void)
{
  memset(__task, O_RESERVED, TASK_MAX);
}

L4_INLINE int task_alloc(unsigned taskno, owner_t owner)
{
  if (__task[taskno] == owner)
    return 1;
  if (__task[taskno] != O_FREE)
    return 0;

  if (! quota_alloc_task(owner, taskno))
    return 0;

  __task[taskno] = owner;
  return 1;
}

L4_INLINE int task_free(unsigned taskno, owner_t owner)
{
  if (__task[taskno] != owner && __task[taskno] != O_FREE)
    return 0;

  if (__task[taskno] != O_FREE)
    {
      quota_free_task(owner, taskno);
      __task[taskno] = O_FREE;
    }
  return 1;  
}

L4_INLINE owner_t task_owner(unsigned taskno)
{
  return __task[taskno];
}

#endif
