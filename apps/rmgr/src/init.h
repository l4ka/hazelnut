#ifndef INIT_H
#define INIT_H

#include <l4/compiler.h>

#include "exec.h"

#define INIT_SECTION __attribute__((section (".init")))

void init(void) L4_NORETURN;
int have_hercules(void);

/* the following variables may only by used during boot -- their
   storage is freed after initialization in init.c */

extern int boot_errors, boot_wait;
void boot_error(void);
void boot_fatal(void);
const char *mb_mod_name(int i);
const char *owner_name(owner_t owner);

#define MODS_MAX 20
extern struct multiboot_module mb_mod[MODS_MAX];
extern struct vbe_controller mb_vbe_cont;
extern struct vbe_mode mb_vbe_mode;

extern vm_offset_t mod_range_start, mod_range_end;

void mod_exec_init(struct multiboot_module *mod, 
		   unsigned mod_first, unsigned mod_count);

exec_read_func_t mod_exec_read;
exec_read_exec_func_t mod_exec_read_exec;

struct exec_task
{
    void *mod_start;
    unsigned mod_no;
    unsigned task_no;
};

extern int boot_errors;
extern int boot_wait;
extern int error_stop;
extern int hercules;
extern int boot_mem_dump;
extern int kernel_running;
extern int last_task_module;

extern int l4_version;
#define VERSION_L4_V2		0
#define VERSION_L4_IBM		1
#define VERSION_FIASCO		2
#define VERSION_L4_KA		3
#define VERSION_PISTACHIO	4

#define KERNEL_NAMES ((char*[5]) { "L4/GMD", "LN/IBM", "FIASCO", "L4/Ka", "L4Ka/Pistachio" })

#endif

