#include <l4/l4.h>

#include "globals.h"

#include "irq.h"

owner_t __irq[IRQ_MAX];
char __irq_stacks[IRQ_MAX * __IRQ_STACKSIZE];

static int irq_attach(unsigned irq);
static int irq_detach(unsigned irq);

void __irq_thread(unsigned irq)
{
  dword_t d1, d2;
  l4_msgdope_t result;
  int err;
  l4_threadid_t t;

  int is_failure = 1, is_attached = 0;

  /* first, register to the irq */
  if (irq_attach(irq))
    {
      /* success! */
      is_failure = 0;
      is_attached = 1;
    }

  /* shake hands with the main thread who started and initialized us */
  err = l4_i386_ipc_reply_and_wait(myself, L4_IPC_SHORT_MSG, is_failure, 0,
				   &t, L4_IPC_SHORT_MSG, &d1, &d2,
				   L4_IPC_NEVER, &result);

  for (;;)
    {
      while (! err)
	{
	  if (t.id.task != myself.id.task)
	    break;		/* silently drop the request */

	  d1 = d2 = (dword_t) -1;

	  if (is_failure && d1)	
	    {
	      /* we couldn't attach previous time -- try again */
	      if (irq_attach(irq))
		{
		  is_failure = 0;
		  is_attached = 1;
		}
	    }

	  if (! is_failure)
	    {
	      if (is_attached && d1)
		{
		  d1 = irq_detach(irq) ? 0 : 1;
		}
	      else if (!is_attached && !d1)
		{
		  d1 = irq_attach(irq) ? 0 : 1;
		  if (d1)
		    is_failure = 1;
		}
	    }

	  err = l4_i386_ipc_reply_and_wait(t, L4_IPC_SHORT_MSG, d1, d2,
					   &t, L4_IPC_SHORT_MSG, &d1, &d2,
					   L4_IPC_NEVER, &result);
	}

      err = l4_i386_ipc_wait(&t, L4_IPC_SHORT_MSG, &d1, &d2,
			     L4_IPC_NEVER, &result);

    }
}

static int irq_attach(unsigned irq)
{
  dword_t dummy;
  int err;
  l4_msgdope_t result;
  l4_threadid_t irq_th;

  /* first, register to the irq */
#ifdef __L4_STANDARD__
  irq_th.lh.low = irq + 1;
  irq_th.lh.high = 0;
#else 
# ifdef __L4_VERSION_X__
  irq_th.raw = irq + 1;
# else
#  warning undefined L4 version
# endif
#endif

  err = l4_i386_ipc_receive(irq_th, L4_IPC_SHORT_MSG,
			    &dummy, &dummy,
			    L4_IPC_TIMEOUT(0,0,0,1,0,0), /* rcv = 0,
							    snd = inf */
			    &result);

  return (err == L4_IPC_RETIMEOUT);
}

static int irq_detach(unsigned irq)
{
  dword_t dummy;
  l4_msgdope_t result;
  
  l4_i386_ipc_receive(L4_NIL_ID, 
		      L4_IPC_SHORT_MSG, &dummy, &dummy,
		      L4_IPC_TIMEOUT(0,0,0,1,0,0), /* rcv = 0,
						      snd = inf */
		      &result);

  return 1;
}
