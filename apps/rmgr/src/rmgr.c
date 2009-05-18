#include <stdio.h>
#include <unistd.h>

#include <l4/l4.h>

#include "config.h"
#include "exec.h"
#include "globals.h"
#include "memmap.h"
#include "memmap_lock.h"

/* the interface we define */
#include <l4/rmgr/rmgr.h>
#include "rmgr.h"

extern int boot_mem_dump;

#define MGR_BUFSIZE 1024

#define MGR_STACKSIZE 1024
static char mgr_stack[MGR_STACKSIZE];
static void mgr(void) L4_NORETURN;

/*
 *  we better make this static because of limited stack size
 */
static char str_buf[MGR_BUFSIZE];

/* started from init() in startup.c */
void
rmgr(void)
{
  l4_threadid_t pa, pr;
  dword_t dummy;
  l4_msgdope_t result;

  /* we (the main thread of this task) will serve as a pager for our
     children. */

  /* before that, we start threads for other services, e.g. resource
     management */
  pa = my_pager; pr = my_preempter;
  l4_thread_ex_regs(rmgr_super_id,
		    (dword_t) mgr, (dword_t)(mgr_stack + MGR_STACKSIZE), 
		    &pr, &pa, &dummy, &dummy, &dummy);

  if (boot_mem_dump)
    {
      int c;
      l4_i386_ipc_call(rmgr_super_id, 
		       L4_IPC_SHORT_MSG, RMGR_MEM_MSG(RMGR_MEM_INFO), 0,
                       L4_IPC_SHORT_MSG, &dummy, &dummy,
		       L4_IPC_NEVER, &result);
      printf("\rRMGR: Return continues, Esc panics");
      c = getchar();
      printf("\r%40s\r","");
      if (c == 27)
	panic("RMGR: memory dumped");
    }
  
  /* now start serving the subtasks */
  pager();
}

/* resource manager */

/* defined here because of the lack of a better place */
quota_t quota[TASK_MAX];
owner_t __task[TASK_MAX];
owner_t __small[SMALL_MAX];

struct grub_multiboot_info mb_info;
char mb_cmdline[0x1000];

unsigned first_task_module = 1;
unsigned first_not_bmod_task;
unsigned first_not_bmod_free_task;
unsigned first_not_bmod_free_modnr;
unsigned fiasco_symbols = 0;
unsigned fiasco_symbols_end;
__mb_mod_name_str mb_mod_names[MODS_MAX];
l4_threadid_t mb_mod_ids[MODS_MAX] = { L4_INVALID_ID, };

static void mgr(void)
{
  int task;
  l4_proto_t p;
  dword_t arg, d1, d2, mcp;
  void *desc;
  int err, unhandled, do_log;
  l4_threadid_t sender, partner, n, s;
  l4_msgdope_t result;
  l4_sched_param_t sched;

  struct
  {
	l4_fpage_t fp;
	l4_msgdope_t size_dope;
	l4_msgdope_t send_dope;
#if defined(__X_ADAPTION__) || defined(__VERSION_X__)
	dword_t      dw2[3];
#else /* __X_ADAPTION__ */
	dword_t      dw2[2];
#endif /* __X_ADAPTION__ */
	l4_strdope_t data;
  } msg;

  for (;;)
    {
      msg.size_dope = L4_IPC_DOPE(2, 1);
      msg.send_dope = L4_IPC_DOPE(2, 1);
      msg.data.rcv_size = MGR_BUFSIZE;
      msg.data.rcv_str  = (dword_t)str_buf;
      err = l4_i386_ipc_wait(&sender, &msg, &p.request, &arg, L4_IPC_NEVER, 
			     &result);

      while (!err)
	{
	  /* we received a request here */

	  do_log = ((debug_log_mask & (1L << p.proto.proto))
		    && (! (debug_log_types & 2) 
			|| ((debug_log_types >> 16) == p.proto.action)));

	  if (do_log && (debug_log_types & 1))
	    printf("RMGR: Log: rcv from %x:%x: [0x%x, 0x%x], r = 0x%x\n",
		   sender.lh.high, sender.lh.low, p.request, arg,
		   result.msgdope);
	  
	  /* who's the allocation unit? */
	  if (L4_IPC_MSG_DECEITED(result))
	    check(l4_nchief(sender, &partner) == L4_NC_INNER_CLAN);
	  else
	    partner = sender;

#if 0 /* currently, this assertion doesn't hold when using RMGR_TASK_CREATE */
	  assert(task_owner(partner.id.task) == myself.id.task);
#endif
	  /* handle the supervisor protocol */
	  
	  desc = L4_IPC_SHORT_MSG;
	  d1 = d2 = (dword_t) -1;

	  unhandled = 0;

          if (partner.id.task > O_MAX)
            {
              /* OOPS.. can't help this sender. */
	      unhandled = 1;
	      goto reply;
            }

	  switch (p.proto.proto)
	    {
	    case RMGR_RMGR:
	      switch (p.proto.action)
		{
		case RMGR_RMGR_PING:
		  d1 = 0; d2 = ~(p.proto.param);
		  break;
		default: 
		  unhandled = 1;
		}
	      break;
	    case RMGR_TASK:
	      switch (p.proto.action)
		{
		case RMGR_TASK_DELETE:
		  /* delete a task we've created on behalf of this user */

		case RMGR_TASK_GET:
		  /* transfer task create right for task no
		     p.proto.param to sender */
		  if (p.proto.param >= TASK_MAX)
		    break;
		  
		  if (p.proto.action == RMGR_TASK_GET)
		    {
		      if (!task_alloc(p.proto.param, partner.id.task))
			break;
		    }
		  else
		    {
		      if (task_owner(p.proto.param) != partner.id.task)
			break;
		    }
			  
		  last_pf[p.proto.param].pfa = -1;
		  last_pf[p.proto.param].eip = -1;

		  n = sender;
		  n.id.task = p.proto.param;
		  n.id.chief = myself.id.task; /* XXX I hope the args
						  suffice for l4_task_new() */
		  n = l4_task_new(n, 
				  p.proto.action == RMGR_TASK_GET
				  ? sender.lh.low : myself.lh.low, 
				  0, 0, L4_NIL_ID);

		  if (l4_is_nil_id(n) || p.proto.action == RMGR_TASK_DELETE)
		    {
		      task_free(p.proto.param, partner.id.task);

		      if (l4_is_nil_id(n) && p.proto.action == RMGR_TASK_GET)
			break;	/* failed */
		    }
		  
		  d1 = 0;

		  break;

		case RMGR_TASK_SET_SMALL:
		case RMGR_TASK_SET_PRIO:
#ifdef __L4_STANDARD__
		  n.lh.low = arg;

 		  /* try to guess the high word of the thread id */
		  n.lh.high = partner.lh.high;
		  if (! task_equal(n, partner))
		    {
		      /* XXX only works for childs of partner */
		      n.id.chief = partner.id.task;
		      n.id.nest++;
		    }
#else
# ifdef __L4_VERSION_X__
		  n.raw = arg;
# else
#  warning undefined L4 version
# endif
#endif

		  s = L4_INVALID_ID;
		  l4_thread_schedule(n, L4_INVALID_SCHED_PARAM, &s, &s, &sched);
#ifdef __L4_STANDARD__
		  if (l4_is_invalid_sched_param(sched)) /* error? */
		    {
		      /* XXX another try... we talk about a child of sender */
		      if (task_equal(sender, partner))
			break;

		      n.id.chief = sender.id.task;
		      n.id.nest = sender.id.nest + 1;

		      s = L4_INVALID_ID;
		      l4_thread_schedule(n, L4_INVALID_SCHED_PARAM,
					 &s, &s, &sched);

		      if (l4_is_invalid_sched_param(sched)) /* error? */
			break;	/* give up. */
		    }
#endif /* __L4_STANDARD__ */

		  if (p.proto.action == RMGR_TASK_SET_SMALL)
		    {
		      if (! small_space_size || p.proto.param == 0
			  || p.proto.param >= 128/small_space_size
			  || ! small_alloc(p.proto.param, partner.id.task))
			break;

		      sched.sp.small = small_space_size 
			| (p.proto.param * small_space_size * 2);
		      s = L4_INVALID_ID;
		      l4_thread_schedule(n, sched, &s, &s, &sched);
		    }
		  else		/* p.proto.action == RMGR_TASK_SET_PRIO */
		    {
		      if (quota[partner.id.task].log_mcp < p.proto.param)
			break;

		      sched.sp.prio = p.proto.param;
		      s = L4_INVALID_ID;
		      l4_thread_schedule(n, sched, &s, &s, &sched);
		    }

		  d1 = 0;
		  break;

		case RMGR_TASK_CREATE:
		  /* create a task; arg = task number, p.proto.param = mcp,
		     *(dword_t*)str_buf = esp, 
		     *(dword_t*)(str_buf + 4) = eip,
		     *(l4_threadid_t*)(str_buf + 8) = pager */

		  if (result.msgdope != L4_IPC_DOPE(2, 1).msgdope)
		    break;	/* must have 8 bytes indirect string data */

#ifdef __L4_STANDARD__
		  n = myself;
		  n.id.chief = myself.id.task;
		  /* we use task id + version number to create the task */
		  n.lh.low = arg; 
		  /* n.id.nest++; */
#else
# ifdef __L4_VERSION_X__
		  n.raw = arg;
# else
#  warning undefined L4 version
# endif
#endif
		  task = n.id.task;

		  if (task >= TASK_MAX 
		      || l4_is_nil_id(* (l4_threadid_t*)(str_buf + 8))
		      || !task_alloc(task, partner.id.task))
		    break;

		  mcp = p.proto.param > quota[partner.id.task].log_mcp
		    ? quota[partner.id.task].log_mcp : p.proto.param;

		  if (do_log)
		    printf("RMGR: Log: creating task=%x:%x, esp=%x, "
			   "eip=%x, pager=%x:%x, mcp=%d\n",
			   n.lh.high, n.lh.low,
			   * (dword_t*)str_buf, * (dword_t*)(str_buf + 4),
			   ((l4_threadid_t*)(str_buf + 8))->lh.high,
			   ((l4_threadid_t*)(str_buf + 8))->lh.low,
			   mcp);
		  
		  n = l4_task_new(n, mcp, * (dword_t*)str_buf, 
				  * (dword_t*)(str_buf + 4),
				  * (l4_threadid_t*)(str_buf + 8));

		  if (l4_is_nil_id(n))
		    {
		      task_free(task, partner.id.task);
		      break;
		    }
		  
		  /* XXX for now, just reset all quotas to their
                     minimal value */
		  quota[task].mem.max = 0;
		  quota[task].himem.max = 0;
		  quota[task].task.max = 0;
		  quota[task].small.max = 0;
		  quota[task].irq.max = -1; //XXX: hack, I NEED a irq
		  quota[task].log_mcp = 
		    p.proto.param > quota[partner.id.task].log_mcp 
		    ? quota[partner.id.task].log_mcp : p.proto.param;

		  d1 = n.lh.low;
		  d2 = n.lh.high;

		  break;

		case RMGR_TASK_CREATE_WITH_PRIO:
		  /* create a task; 
		     arg = task number, 
		     p.proto.param = mcp,
		     *(dword_t*)str_buf = esp, 
		     *(dword_t*)(str_buf + 4) = eip,
		     *(l4_threadid_t*)(str_buf + 8) = pager,
		     *(dword_t*)(str_buf + 16) = sched_param */

		  if (result.msgdope != L4_IPC_DOPE(2, 1).msgdope)
		    break;	/* must have 8 bytes indirect string data */

#ifdef __L4_STANDARD__
		  n = myself;
		  n.id.chief = myself.id.task;
		  /* we use task id + version number to create the task */
		  n.lh.low = arg; 
		  /* n.id.nest++; */
#else
# ifdef __L4_VERSION_X__
		  n.raw = arg;
# else
#  warning undefined L4 version
# endif
#endif
		  task = n.id.task;

		  /* extract schedule parameters */
		  sched.sched_param = *(dword_t*)(str_buf + 16);
		  sched.sp.small = 0;
		  if (quota[partner.id.task].log_mcp < sched.sp.prio)
		    sched.sp.prio = quota[partner.id.task].log_mcp;
		  
		  if (task >= TASK_MAX 
		      || l4_is_nil_id(* (l4_threadid_t*)(str_buf + 8))
		      || !task_alloc(task, partner.id.task))
		    break;

 		  if (do_log)
		    printf("RMGR: Log: creating task=%x:%x, esp=%x, "
			   "eip=%x, pager=%x:%x, sched_param: %lx, mcp: %x\n",
			   n.lh.high, n.lh.low,
			   * (dword_t*)str_buf, * (dword_t*)(str_buf + 4),
			   ((l4_threadid_t*)(str_buf + 8))->lh.high,
			   ((l4_threadid_t*)(str_buf + 8))->lh.low,
			   (unsigned long)sched.sched_param, p.proto.param);
		  /* XXX for now, use an L4-mcp of 0 */
		  n = l4_task_new(n, 0, * (dword_t*)str_buf, 
				  * (dword_t*)(str_buf + 4),
				  * (l4_threadid_t*)(str_buf + 8));

		  if (l4_is_nil_id(n))
		    {
		      task_free(task, partner.id.task);
		      break;
		    }
		  
		  l4_thread_schedule(n, sched, &s, &s, &sched);

		  /* XXX for now, just reset all quotas to their
                     minimal value */
		  quota[task].mem.max = 0;
		  quota[task].himem.max = 0;
		  quota[task].task.max = 0;
		  quota[task].small.max = 0;
		  quota[task].irq.max = 0;
		  quota[task].log_mcp = 
		    p.proto.param > quota[partner.id.task].log_mcp 
		    ? quota[partner.id.task].log_mcp : p.proto.param;

		  d1 = n.lh.low;
		  d2 = n.lh.high;
		  break;

		case RMGR_TASK_GET_ID:
		  /* default is error */
		  d1 = L4_INVALID_ID.lh.low;
		  d2 = L4_INVALID_ID.lh.high;
		  if (result.msgdope == L4_IPC_DOPE(2, 1).msgdope)
		    {
		      int a;
		      /* fixme: configfile name should be ignored */
		      for (a = first_task_module; a < mb_info.mods_count; a++)
			{
			char *ptr, *str;
			int len;

			str = mb_mod_names[a];
			if ((ptr = strrchr(str, '/')))
			    ptr++;
			else
			    ptr = mb_mod_names[a];
			
			if (   (len = strcspn(ptr, " "))>0
			       && strncmp(ptr, str_buf, len)==0)
			  {
			  d1 = mb_mod_ids[a].lh.low;
			  d2 = mb_mod_ids[a].lh.high;
			  break;
			}
		      }
		  }
		  break;

		case RMGR_TASK_SET_ID:
		  /* default is error */
		  d1 = L4_INVALID_ID.lh.low;
		  d2 = L4_INVALID_ID.lh.high;
		  if (result.msgdope == L4_IPC_DOPE(2, 1).msgdope)
		    {
		      int a, b;
		    /* fixme: configfile name should be ignored */
		    for (a = first_task_module; a < mb_info.mods_count; a++)
		      {
			/* found in boot_modules */
			if (strcmp(mb_mod_names[a], str_buf) == 0)
			  {
			    mb_mod_ids[a].lh.low = sender.lh.low;
			    mb_mod_ids[a].lh.high = sender.lh.high;
			    mb_mod_ids[a].id.task = p.proto.param;
			    mb_mod_ids[a].id.lthread = 0;
			    mb_mod_ids[a].id.chief = arg;
			    d1 = d2 = 0;
			  break;
			}
		      }
                     /* search in modules we only have the quotas of */
		    for (b = first_not_bmod_task;
			 a < first_not_bmod_free_modnr; a++, b++)
		      {
			if (strcmp(mb_mod_names[a], str_buf) == 0)
			  {
			    if (task_owner(p.proto.param)==partner.id.task)
			      {
				memcpy(&quota[p.proto.param], &quota[b],
				       sizeof(quota[0]));
				d1 = d2 = 0;
				break;
			      }
			  }
		      }
		    }
		  break;

		default:
		  unhandled = 1;
		}
	      break;

	    case RMGR_IRQ:
	      switch (p.proto.action)
		{
		case RMGR_IRQ_GET:
		  if (p.proto.param < IRQ_MAX)
		    {
		      if (irq_alloc(p.proto.param, partner.id.task))
			{
			  /* the irq was free -- detach from it */
			  dword_t e1, e2;
			  n = myself;
			  n.id.lthread = LTHREAD_NO_IRQ(p.proto.param);
			  err = l4_i386_ipc_call(n, L4_IPC_SHORT_MSG, 1, 0,
						 L4_IPC_SHORT_MSG, &e1, &e2,
						 L4_IPC_NEVER, &result);
			  check(err == 0);
			  
			  if (e1 == 0)
			    d1 = 0; /* success! */
			  else
			    /* fucked! (couldn't attach to irq number) */
			    irq_free(p.proto.param, partner.id.task);
			}
		    }
		  break;

		case RMGR_IRQ_FREE:
		  if (p.proto.param < IRQ_MAX
		      && irq_free(p.proto.param, partner.id.task))
		    {
		      dword_t e1, e2;
		      n = myself;
		      n.id.lthread = LTHREAD_NO_IRQ(p.proto.param);
		      err = l4_i386_ipc_call(n, L4_IPC_SHORT_MSG, 0, 0,
					     L4_IPC_SHORT_MSG, &e1, &e2,
					     L4_IPC_NEVER, &result);
		      check(err == 0);

		      if (e1 == 0)
			d1 = 0;	/* success! */
		    }
		  break;

		default:
		  unhandled = 1;
		}
	      break;

	    case RMGR_MEM:
	      /* we must synchronise the access to memmap function */
	      enter_memmap_functions(RMGR_LTHREAD_SUPER, rmgr_pager_id);
	      switch (p.proto.action)
		{
		case RMGR_MEM_FREE:
		  /* must contain physical address and owner (== task_no) */
		  /* a bit of paranoia */
		  if (arg >= 0x40000000 && arg < 0xC0000000) 
		    {
		      arg += 0x40000000;
		      if((sender.id.task != memmap_owner_page(arg))
			  || !memmap_free_superpage(arg, sender.id.task))
			{
			  d1 = -1; /* failure */
			}
		      else 
			{
			  /* we can't unmap page here because the superpage was
			   * granted so sender has to unmap page itself */
			  d1 = 0;
			}
		    }
		  else
		    {
		      if((sender.id.task != memmap_owner_page(arg))
		          ||  !memmap_free_page(arg,sender.id.task))
			  d1 = -1; /* failure */
		      else {
		        /* unmap fpage */
		        l4_fpage_unmap(l4_fpage(arg, L4_LOG2_PAGESIZE,
						L4_FPAGE_RW, L4_FPAGE_MAP),
				       L4_FP_FLUSH_PAGE|L4_FP_OTHER_SPACES);
		        d1 = 0;
		      }
		    }
		  break;
		case RMGR_MEM_FREE_FP:
		  printf("RMGR: unable to handle free fpage request.\n");
		  /* I don't know how to resolve the user's fpage 
		     into a physical address 
		     */
		  d1 = -1; /* failure */
		  break;
		case RMGR_MEM_RESERVE:
		  /* reserve memory; arg = size_and_align, 
		   *(dword_t*)str_buf = range_low,
		   *(dword_t*)(str_buf + 4) = range_high */
		  if (result.msgdope != L4_IPC_DOPE(2, 1).msgdope)
		    break;      /* must have 8 bytes indirect string data */
		  
		  d1 = reserve_range(arg, partner.id.task,
				     *(dword_t*)str_buf, 
				     *(dword_t*)(str_buf+4));
		  break;
		case RMGR_MEM_INFO:
		  show_mem_info();
		  d1 = 0;
		  break;
		case RMGR_MEM_GET_PAGE0:
		  d1 = 0;
		  d2 = l4_fpage(0, L4_LOG2_PAGESIZE,
				L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
		  desc = L4_IPC_SHORT_FPAGE;
		  break;
		default:
		  unhandled = 1;
		}
	      leave_memmap_functions(RMGR_LTHREAD_SUPER, rmgr_pager_id);
	      break;
	    default:
	      unhandled = 1;
	    }

	reply:
	  if (verbose && unhandled)
	    {
	      printf("RMGR: can't handle request=0x%x, data=0x%x "
		     "from thread=%x\n", p.request, arg, sender.lh.low);
	    }

	  
	  if ((do_log && (debug_log_types & 1))
	      || ((verbose || debug) && d1 == (dword_t)-1))
	    {
	      if (d1 != 0 && !do_log) /* error, and request not yet logged */
		printf("RMGR: Log: rcv from %x:%x: [0x%x, 0x%x], r = 0x%x\n",
		       sender.lh.high, sender.lh.low, p.request, arg,
		       result.msgdope);
		
	      printf("RMGR: Log: snd to   %x:%x: [0x%x, 0x%x]\n",
		     sender.lh.high, sender.lh.low, d1, d2);
	    }
	  
	  if ((do_log && (debug_log_types & 4))
	      || (debug && d1 == (dword_t)-1))
	    enter_kdebug("rmgr>");
	  
	  /* send reply and wait for next message */
#if defined(__X_ADAPTION__) || defined(__VERSION_X__)
	  msg.size_dope = L4_IPC_DOPE(3, 1);
#else /* __X_ADAPTION__ */
	  msg.size_dope = L4_IPC_DOPE(2, 1);
#endif /* __X_ADAPTION__ */
	  msg.send_dope = L4_IPC_DOPE(2, 0);
	  msg.data.rcv_size = MGR_BUFSIZE;
	  msg.data.rcv_str  = (dword_t)str_buf;
	  err = l4_i386_ipc_reply_and_wait(sender, desc, d1, d2,
					   &sender, &msg, &p.request, &arg,
					   L4_IPC_TIMEOUT(0,1,0,0,0,0), 
					   /* snd timeout = 0 */
					   &result);
	}
    }
}

