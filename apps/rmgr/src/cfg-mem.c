#include <malloc.h>
#include <flux/lmm.h>
#include "cfg.h"

/* this file implements a static malloc pool for the flex-generated
   config file scanner */

#define POOL_SIZE 0x10000
static char pool[POOL_SIZE];

static lmm_region_t region;

void __cfg_setup_mem(void)
{
  lmm_init(&malloc_lmm);
  lmm_add_region(&malloc_lmm, &region, (void*)0, (vm_size_t)-1, 0, 0);
  lmm_add_free(&malloc_lmm, pool, POOL_SIZE);
}

void __cfg_destroy_mem(void)
{
  lmm_init(&malloc_lmm);	/* reset the malloc_lmm */
}
