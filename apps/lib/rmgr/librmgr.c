/* communication with the L4 RMGR */

#include <l4/l4.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>

#define USE_PRIOS

/* the interface we implement */
#include <l4/rmgr/rmgr.h>
#include <l4/rmgr/librmgr.h>

l4_threadid_t rmgr_id;
l4_threadid_t rmgr_pager_id;

extern inline int rmgr_call(dword_t request, dword_t arg, dword_t *out);
extern int rmgr_call_string(char *string, dword_t request,
			    dword_t arg,  dword_t *out1, dword_t *out2);
			   
			 
extern inline int rmgr_call_string(char *string,
				   dword_t request,
				   dword_t arg,
				   dword_t *out1,
				   dword_t *out2)
{
  l4_msgdope_t result;
  dword_t r1, r2;
  int err;
  struct {
    l4_fpage_t fp;
    l4_msgdope_t size_dope;
    l4_msgdope_t send_dope;
    dword_t dw2[2];
    l4_strdope_t data;
  } msg;

  msg.size_dope = L4_IPC_DOPE(2, 1);
  msg.send_dope = L4_IPC_DOPE(2, 1);
  msg.data.snd_size = strlen(string) + 1;
  msg.data.snd_str = (dword_t)string;
  err = l4_i386_ipc_call(rmgr_id,
			 &msg, request, arg,
			 L4_IPC_SHORT_MSG, &r1, &r2,
			 L4_IPC_NEVER, &result);
  if (err) return err;

  if (out1) *out1 = r1;
  if (out2) *out2 = r2;
  return 0;
}


extern inline int rmgr_call(dword_t request,
			    dword_t arg,
                            dword_t *out)
{
  l4_msgdope_t result;
  dword_t r1, r2;
  int err;

  err = l4_i386_ipc_call(rmgr_id, 
			 L4_IPC_SHORT_MSG, request, arg,
			 L4_IPC_SHORT_MSG, &r1, &r2,
			 L4_IPC_NEVER, &result);

  if (err) return err;

  if (out) *out = r2;
  return r1;
}

int rmgr_init(void)
{
  l4_msgdope_t result;
  l4_threadid_t my_preempter, my_pager;
  
  my_preempter = my_pager = L4_INVALID_ID;

  /*
   *  get preempter and pager ids
   */
#if 0
  dword_t dummy;
  l4_thread_ex_regs(l4_myself(), (dword_t)-1, (dword_t)-1,
			&my_preempter, &my_pager, &dummy, &dummy, &dummy);
#else
  l4_nchief(L4_INVALID_ID,&my_pager);
#endif

  rmgr_id = rmgr_pager_id = my_pager;
  rmgr_id.id.lthread = RMGR_LTHREAD_SUPER;
  rmgr_pager_id.id.lthread = RMGR_LTHREAD_PAGER;

  return (rmgr_call(RMGR_RMGR_MSG(RMGR_RMGR_PING, 0xbeef), 0, &result.msgdope) == 0
	       && result.msgdope == ~0xbeef);
}

int rmgr_set_small_space(l4_threadid_t dest, int num)
{
#ifdef USE_SMALL_SPACES
  l4_sched_param_t schedparam;
  l4_threadid_t foo_id;

  return rmgr_call(RMGR_TASK_MSG(RMGR_TASK_SET_SMALL, num), dest.lh.low, 0);

  foo_id = L4_INVALID_ID;
  l4_thread_schedule(dest, L4_INVALID_SCHED_PARAM, 
		     &foo_id, &foo_id, &schedparam);
  foo_id = L4_INVALID_ID;
  schedparam.sp.small = L4_SMALL_SPACE(USE_SMALL_SPACES, num);
  l4_thread_schedule(dest, schedparam, &foo_id, &foo_id, &schedparam);
#endif

  return 0;
}

int rmgr_set_prio(l4_threadid_t dest, int num)
{
#ifdef USE_PRIOS
  l4_sched_param_t schedparam;
  l4_threadid_t foo_id;

  static short mcp_below = 0xff;

  /* try to set directly first */
  if (num <= mcp_below)
    {
      foo_id = L4_INVALID_ID;
      l4_thread_schedule(dest, L4_INVALID_SCHED_PARAM, 
			 &foo_id, &foo_id, &schedparam);
      if (!l4_is_invalid_sched_param(schedparam))
	{
	  schedparam.sp.prio = num;
	  foo_id = L4_INVALID_ID;
	  l4_thread_schedule(dest, schedparam, &foo_id, &foo_id, &schedparam);
	  if (!l4_is_invalid_sched_param(schedparam))
	    return 0;
 
	  mcp_below = num - 1;
	  if (mcp_below < 0) mcp_below = 0;
	}
      else
	{
	  /* even failed to query the schedparam word... */
	  mcp_below = 0;
	}
    }

  /* failed to set directly -- use the RMGR */
  return rmgr_call(RMGR_TASK_MSG(RMGR_TASK_SET_PRIO, num), dest.lh.low, 0);

#else
  return 0;
#endif
}

/* cheat - query prio without rmgr involvement */

int 
rmgr_get_prio(l4_threadid_t dest, int *num)
{
#ifdef USE_PRIOS
  l4_sched_param_t p;
  l4_threadid_t foo_id = L4_INVALID_ID;

  l4_thread_schedule(dest, L4_INVALID_SCHED_PARAM, 
                     &foo_id, &foo_id, &p);

  return *num = l4_is_invalid_sched_param(p) ? 0 : p.sp.prio;
#else
  return 0;
#endif
}


int rmgr_get_task(int num)
{
  return rmgr_call(RMGR_TASK_MSG(RMGR_TASK_GET, num), 0, 0);
}

int rmgr_get_irq(int num)
{
  return rmgr_call(RMGR_IRQ_MSG(RMGR_IRQ_GET, num), 0, 0);
}

int rmgr_free_fpage(l4_fpage_t fp)
{
  return rmgr_call(RMGR_MEM_MSG(RMGR_MEM_FREE_FP), fp.fpage, 0);
}

int rmgr_free_page(dword_t address)
{
  return rmgr_call(RMGR_MEM_MSG(RMGR_MEM_FREE),address, 0);
}


int rmgr_get_task_id(char *module_name, l4_threadid_t *thread_id)
{
  int err;
  dword_t lo, hi;

  err = rmgr_call_string(module_name, RMGR_TASK_MSG(RMGR_TASK_GET_ID, 0), 0,
								&lo, &hi);
  if (err) {
	*thread_id = L4_INVALID_ID;
	return err;
  }
  thread_id->lh.low  = lo;
  thread_id->lh.high = hi;
  return 0;
}

l4_taskid_t rmgr_task_new(l4_taskid_t dest, dword_t mcp_or_new_chief,
			  dword_t esp, dword_t eip, l4_threadid_t pager)
{
  l4_msgdope_t result;
  dword_t r1, r2;
  int err;

  static struct {
	dword_t fp;
	l4_msgdope_t size_dope;
	l4_msgdope_t send_dope;
	dword_t      d1, d2;
	l4_strdope_t data;
  } msg = {0, L4_IPC_DOPE(2, 1), L4_IPC_DOPE(2, 1), 0, 0, 
	   {16, 0, 0, 0}};

  int deleting = l4_is_nil_id(pager);

  msg.data.snd_str = (dword_t) &esp;

  if (deleting)
    err = l4_i386_ipc_call(rmgr_id, 
			   L4_IPC_SHORT_MSG, 
			   RMGR_TASK_MSG(RMGR_TASK_DELETE, dest.id.task),
			   0,
			   L4_IPC_SHORT_MSG, &r1, &r2,
			   L4_IPC_NEVER, &result);
  else
    err = l4_i386_ipc_call(rmgr_id, 
			   &msg, 
			   RMGR_TASK_MSG(RMGR_TASK_CREATE, mcp_or_new_chief),
			   dest.lh.low,
			   L4_IPC_SHORT_MSG, &r1, &r2,
			   L4_IPC_NEVER, &result);

  if (err)
    return L4_NIL_ID;

  return ((l4_threadid_t){lh: {r1, r2}});
}

l4_taskid_t rmgr_task_new_with_prio(l4_taskid_t dest, dword_t mcp_or_new_chief,
				    dword_t esp, dword_t eip, 
				    l4_threadid_t pager, 
				    l4_sched_param_t sched_param)
{
  l4_msgdope_t result;
  dword_t r1, r2;
  int err;

  static struct {
	dword_t fp;
	l4_msgdope_t size_dope;
	l4_msgdope_t send_dope;
	dword_t      d1, d2;
	l4_strdope_t data;
  } msg = {0, L4_IPC_DOPE(2, 1), L4_IPC_DOPE(2, 1), 0, 0, 
	   {20, 0, 0, 0}};

  int deleting = l4_is_nil_id(pager);

  msg.data.snd_str = (dword_t) &esp;

  if (deleting)
    err = l4_i386_ipc_call(rmgr_id, 
			   L4_IPC_SHORT_MSG, 
			   RMGR_TASK_MSG(RMGR_TASK_DELETE, dest.id.task),
			   0,
			   L4_IPC_SHORT_MSG, &r1, &r2,
			   L4_IPC_NEVER, &result);
  else {
    err = l4_i386_ipc_call(rmgr_id, 
			   &msg, 
			   RMGR_TASK_MSG(RMGR_TASK_CREATE_WITH_PRIO,
					 mcp_or_new_chief),
			   dest.lh.low,
			   L4_IPC_SHORT_MSG, &r1, &r2,
			   L4_IPC_NEVER, &result);
  }

  if (err)
    return L4_NIL_ID;

  return ((l4_threadid_t){lh: {r1, r2}});
}

int rmgr_set_task_id(char *module_name, l4_threadid_t thread_id)
{
  int err;
  dword_t lo, hi;

  err = rmgr_call_string(module_name,
			 RMGR_TASK_MSG(RMGR_TASK_SET_ID,
				       thread_id.id.task),
			 thread_id.id.chief,
			 &lo,
			 &hi);
  return err;
}
