#ifndef IRQ_H
#define IRQ_H

#include <string.h>

#include <l4/compiler.h>

#include "globals.h"
#include "quota.h"

L4_INLINE void irq_init(void);
L4_INLINE int irq_alloc(unsigned irqno, owner_t owner);
L4_INLINE int irq_free(unsigned irqno, owner_t owner);

#define IRQ_MAX (16)

extern owner_t __irq[IRQ_MAX];

#define __IRQ_STACKSIZE (256)
extern char __irq_stacks[IRQ_MAX * __IRQ_STACKSIZE];
void __irq_thread(unsigned irq) L4_NORETURN;


L4_INLINE void irq_init(void)
{
  memset(__irq, O_RESERVED, IRQ_MAX);
}

L4_INLINE int irq_alloc(unsigned irqno, owner_t owner)
{
  if (__irq[irqno] == owner)
    return 1;
  if (__irq[irqno] != O_FREE)
    return 0;

  if (! quota_alloc_irq(owner, irqno))
    return 0;

  __irq[irqno] = owner;
  return 1;
}

L4_INLINE int irq_free(unsigned irqno, owner_t owner)
{
  if (__irq[irqno] != owner && __irq[irqno] != O_FREE)
    return 0;

  if (__irq[irqno] != O_FREE)
    {
      quota_free_irq(owner, irqno);
      __irq[irqno] = O_FREE;
    }
  
  return 1;  
}

#endif
