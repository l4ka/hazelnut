#ifndef CFG_H
#define CFG_H

#include <l4/compiler.h>

#include "globals.h"

void cfg_setup_input(const char *cfg_buffer, const char *cfg_buffer_end);

int __cfg_parse(void);
void __cfg_setup_mem(void);
void __cfg_destroy_mem(void);

extern unsigned __cfg_task;

L4_INLINE void cfg_init(void);
L4_INLINE int cfg_parse(void);

L4_INLINE void cfg_init(void)
{
  /* first task the config file configures */
  /* XXX the task number should be possible to specify in the config file */
  __cfg_task = myself.id.task;
}

L4_INLINE int cfg_parse(void)
{
  int r;
  __cfg_setup_mem();
  r = __cfg_parse();
  __cfg_destroy_mem();
  return r;
}

#endif
