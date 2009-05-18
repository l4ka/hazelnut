#ifndef __L4_I386_RMGR_H
#define __L4_I386_RMGR_H

#include <l4/arch/types.h>

extern int have_rmgr;
extern l4_threadid_t rmgr_id;

#ifdef __cplusplus
extern "C" {
#endif

int rmgr_init(void);

int rmgr_set_small_space(l4_threadid_t dest, int num);
int rmgr_set_prio(l4_threadid_t dest, int num);
int rmgr_get_prio(l4_threadid_t dest, int *num);

int rmgr_get_task(int num);
int rmgr_delete_task(int num);
int rmgr_get_irq(int num);

int rmgr_get_task_id(char *module_name, l4_threadid_t *thread_id);

l4_taskid_t rmgr_task_new(l4_taskid_t dest, dword_t mcp_or_new_chief,
			  dword_t esp, dword_t eip, l4_threadid_t pager);

l4_taskid_t rmgr_task_new_with_prio(l4_taskid_t dest, dword_t mcp_or_new_chief,
				    dword_t esp, dword_t eip, 
				    l4_threadid_t pager, 
				    l4_sched_param_t sched_param);

int rmgr_free_fpage(l4_fpage_t fp);
int rmgr_free_page(dword_t address);

#ifdef __cplusplus
}
#endif

#endif
