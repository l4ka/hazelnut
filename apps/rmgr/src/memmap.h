#ifndef MEMMAP_H
#define MEMMAP_H

#include <string.h>
#include <assert.h>
#include <flux/machine/types.h>

#include <l4/l4.h>

#include "globals.h"
#include "quota.h"

#define MEM_MAX (256L*1024L*1024L) /* max RAM we handle */
#define SUPERPAGE_MAX (1024)	/* no of superpages in the system */

typedef struct {
  word_t free_pages;
  owner_t owner;
  byte_t padding;
} __attribute__((packed)) __superpage_t;

typedef struct {
  dword_t pfa;
  dword_t eip;
} __attribute__((packed)) last_pf_t;

extern owner_t __memmap[MEM_MAX/L4_PAGESIZE];
extern __superpage_t __memmap4mb[SUPERPAGE_MAX];
extern last_pf_t last_pf[];
extern vm_offset_t mem_high;
extern vm_offset_t mem_phys;

extern l4_kernel_info_t *l4_info;

void pager(void) L4_NORETURN;
vm_offset_t reserve_range(unsigned int size_and_align, owner_t owner, 
                          vm_offset_t range_low, vm_offset_t range_high);
void show_mem_info(void);

L4_INLINE void memmap_init(void);
L4_INLINE int memmap_free_page(vm_offset_t address, owner_t owner);
L4_INLINE int memmap_alloc_page(vm_offset_t address, owner_t owner);
L4_INLINE owner_t memmap_owner_page(vm_offset_t address);
L4_INLINE int memmap_free_superpage(vm_offset_t address, owner_t owner);
L4_INLINE int memmap_alloc_superpage(vm_offset_t address, owner_t owner);
L4_INLINE owner_t memmap_owner_superpage(vm_offset_t address);
L4_INLINE unsigned memmap_nrfree_superpage(vm_offset_t address);

L4_INLINE void memmap_init(void)
{
  unsigned i;
  __superpage_t *s;

  memset(__memmap, O_RESERVED, MEM_MAX/L4_PAGESIZE);
  for (s = __memmap4mb, i = 0; i < SUPERPAGE_MAX; i++, s++)
    *s = (__superpage_t) { 0, O_RESERVED, 0 };
}

L4_INLINE int memmap_free_page(vm_offset_t address, owner_t owner)
{
  owner_t *m = __memmap + address/L4_PAGESIZE;
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  if (s->owner == O_FREE)
    return 1;			/* already free */
  if (s->owner != O_RESERVED)
    return 0;
  if (*m == O_FREE)
    return 1;			/* already free */
  if (*m != owner)
    return 0;

  quota_free_mem(owner, address, L4_PAGESIZE);

  *m = O_FREE;
  s->free_pages++;

  if (s->free_pages == L4_SUPERPAGESIZE/L4_PAGESIZE)
    s->owner = O_FREE;
  return 1;
}

L4_INLINE int memmap_alloc_page(vm_offset_t address, owner_t owner)
{
  owner_t *m = __memmap + address/L4_PAGESIZE;
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  if (*m == owner)
    return 1;			/* already allocated */
  if (*m != O_FREE)
    return 0;

  if (s->owner == O_FREE)
    s->owner = O_RESERVED;
  else if (s->owner != O_RESERVED)
    return 0;

  if (! quota_alloc_mem(owner, address, L4_PAGESIZE))
    return 0;

  s->free_pages--;

  *m = owner;
  return 1;
}

L4_INLINE owner_t memmap_owner_page(vm_offset_t address)
{
  owner_t *m = __memmap + address/L4_PAGESIZE;
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  return s->owner == O_RESERVED ? *m : s->owner;
}

L4_INLINE int memmap_free_superpage(vm_offset_t address, owner_t owner)
{
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  if (s->owner == O_FREE)
    return 1;			/* already free */
  if (s->owner != owner)
    return 0;

  quota_free_mem(owner, address, L4_SUPERPAGESIZE);

  s->owner = O_FREE;
  s->free_pages = L4_SUPERPAGESIZE/L4_PAGESIZE;

  /* assume __memmap[] is correctly set up here (all single pages
     marked free) */
  return 1;
}

L4_INLINE int memmap_alloc_superpage(vm_offset_t address, owner_t owner)
{
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  if (s->owner == owner)
    return 1;			/* already allocated */
  if (s->owner != O_FREE)
    {
      /* check if all pages inside belong to the new owner already */

      owner_t *m, *sm;
      vm_size_t already_have = 0;

      if (s->owner != O_RESERVED)
	return 0;

      address &= L4_SUPERPAGEMASK;
      if (address >= MEM_MAX)
	return 0;

      for (sm = m = __memmap + address/L4_PAGESIZE; 
	   m < sm + L4_SUPERPAGESIZE/L4_PAGESIZE;
	   m++)
	{
	  if (! (*m == O_FREE || *m == owner))
	    return 0;		/* don't own superpage exclusively */

	  if (*m == owner)
	    already_have += L4_PAGESIZE;
	}

      if (already_have)
	{
	  /* XXX we're sloppy with the exact addresses we free here,
	     knowing that the address doesn't really matter to
	     quota_free_mem() */
	  quota_free_mem(owner, address, already_have);
	}
	 
      if (! quota_alloc_mem(owner, address, L4_SUPERPAGESIZE))
	{
	  if (already_have)
	    /* XXX we deal with the quota's private values here... :-( */
	    quota[owner].mem.current += already_have; /* reset */

	  return 0;
	}

      /* ok, we're the exclusive owner of this superpage */
      memset(sm, O_FREE, L4_SUPERPAGESIZE/L4_PAGESIZE);      

    }
  else  
    {
      if (! quota_alloc_mem(owner, address & L4_SUPERPAGEMASK, L4_SUPERPAGESIZE))
	return 0;
    }
  
  s->owner = owner;
  return 1;
}

L4_INLINE owner_t memmap_owner_superpage(vm_offset_t address)
{
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  return s->owner;
}

L4_INLINE unsigned memmap_nrfree_superpage(vm_offset_t address)
{
  __superpage_t *s = __memmap4mb + address/L4_SUPERPAGESIZE;

  if (s->owner == O_FREE || s->owner == O_RESERVED)
    return s->free_pages;

  return 0;
}


#endif
