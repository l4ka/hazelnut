#ifndef RMGR_H
#define RMGR_H

#include <string.h>
#include <l4/compiler.h>

#include "irq.h"
#include "task.h"
#include "small.h"
#include "quota.h"
#include "init.h"

extern struct grub_multiboot_info mb_info;

extern unsigned first_task_module;
extern unsigned first_not_bmod_task;
extern unsigned first_not_bmod_free_task;
extern unsigned first_not_bmod_free_modnr;
extern unsigned fiasco_symbols;
extern unsigned fiasco_symbols_end;

#define MOD_NAME_MAX 1000
typedef char __mb_mod_name_str[MOD_NAME_MAX];
extern __mb_mod_name_str mb_mod_names[MODS_MAX];
extern l4_threadid_t mb_mod_ids[MODS_MAX];
extern char mb_cmdline[0x1000];

void rmgr(void) L4_NORETURN;

#endif
