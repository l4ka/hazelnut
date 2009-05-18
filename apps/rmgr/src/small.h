#ifndef SMALL_H
#define SMALL_H

#include <string.h>

#include <l4/compiler.h>

#include "globals.h"
#include "quota.h"

L4_INLINE void small_init(void);
L4_INLINE int small_alloc(unsigned smallno, owner_t owner);
L4_INLINE int small_free(unsigned smallno, owner_t owner);
L4_INLINE owner_t small_owner(unsigned smallno);

#define SMALL_MAX 128

extern owner_t __small[SMALL_MAX];

L4_INLINE void small_init(void)
{
  memset(__small, O_RESERVED, SMALL_MAX);
}

L4_INLINE int small_alloc(unsigned smallno, owner_t owner)
{
  if (__small[smallno] == owner)
    return 1;
  if (__small[smallno] != O_FREE)
    return 0;

  if (! quota_alloc_small(owner, smallno))
    return 0;

  __small[smallno] = owner;
  return 1;
}

L4_INLINE int small_free(unsigned smallno, owner_t owner)
{
  if (__small[smallno] != owner && __small[smallno] != O_FREE)
    return 0;

  if (__small[smallno] != O_FREE)
    {
      quota_free_small(owner, smallno);
      __small[smallno] = O_FREE;
    }
  return 1;  
}

L4_INLINE owner_t small_owner(unsigned smallno)
{
  return __small[smallno];
}

#endif
