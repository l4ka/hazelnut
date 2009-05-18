#include <l4/types.h>
#include <l4/syscalls.h>
#include <l4/ipc.h>
#include <l4/kdebug.h>
#include <l4/librmgr.h>

#include <stdio.h>

#define LTHREAD_NO_RECEIVER 1
#define LTHREAD_NO_SENDER 0

l4_threadid_t sender_id, receiver_id;

unsigned char receiver_stack[1000];

extern inline void  
rdtsc(unsigned long *time)
{
  __asm__ volatile ("pushl	%%edx\n"
		    ".byte 0x0f, 0x31\n"
		    "popl	%%edx\n"
		    : "=a" (*time)
		    :
		    : "dx"
		    );
};

static inline void rdpmc(int no, qword_t* res)
{
    dword_t dummy;
    __asm__ __volatile__ (
	"rdpmc"
	: "=a"(*(dword_t*)res), "=d"(*((dword_t*)res + 1)), "=c"(dummy)
	: "c"(no)
	: "memory");
}


void 
start_receiver(void);
void 
pong(void);
void
ping();

void start_receiver(void)
{
  dword_t dummy;
  l4_msgdope_t result;
  l4_threadid_t foo_id;
  l4_sched_param_t schedparam;

  sender_id = receiver_id = l4_myself();
  receiver_id.id.task++;
  receiver_id.id.chief = sender_id.id.task;

  rmgr_init();
  if (rmgr_get_task(receiver_id.id.task)) {
    printf("(rmgr)create pong failed");
    exit(1);
  }

  printf("calling l4_task_new(%x.%x, %d, %p, %p, %x.%x);\n",
	 receiver_id.lh.high, receiver_id.lh.low,
	 255, &receiver_stack[255], pong, l4_myself().lh.high,
	 l4_myself().lh.low);
  receiver_id = l4_task_new(receiver_id, 255, 
			    (unsigned int)&receiver_stack[255], 
			    (unsigned int)pong, l4_myself()); 
  
  if (l4_is_nil_id(receiver_id)) {
    printf("create pong failed");
    exit(1);
  }

  l4_thread_switch(receiver_id);

  if (l4_i386_ipc_receive(receiver_id, L4_IPC_SHORT_MSG, &dummy, &dummy,
			  L4_IPC_NEVER, &result)) {
    printf("error while receiving pf");
    exit(1);
  }

  if (l4_i386_ipc_send(receiver_id, L4_IPC_SHORT_FPAGE,
		       0, /* send base */
		       l4_fpage(0, L4_WHOLE_ADDRESS_SPACE,
				L4_FPAGE_RW, L4_FPAGE_MAP).fpage,
		       L4_IPC_TIMEOUT(0, 1, 0, 0, 0, 0), 
		       &result)) {
    printf("error while mapping addr space");
    exit(1);
  }

#if 0
  foo_id = L4_INVALID_ID;
  l4_thread_schedule(receiver_id, (dword_t) -1, &foo_id, &foo_id, &schedparam);
  schedparam.sp.small = L4_SMALL_SPACE(32, 1);
  foo_id = L4_INVALID_ID;
  l4_thread_schedule(receiver_id, schedparam, &foo_id, &foo_id, &schedparam);
#endif
  /* one time around to synchronize with receiver */
  if (l4_i386_ipc_call(receiver_id, 
		       L4_IPC_SHORT_MSG, dummy, dummy,
		       L4_IPC_SHORT_MSG, &dummy, &dummy,
		       L4_IPC_NEVER, &result)) {
    printf("error while syncing with pong");
    exit(1);
  }
  printf("receiver ready");
}

#define RCV_MAN 255
#define RCV_EXP 6
extern long counter;
long ping_start, ping_stop;

void activate_fpu(void);
void activate_fpu(void)
{
	static double x = 4195835.0;
	static double y = 3145727.0;
	static long fdiv_bug;

	printf("Checking 386/387 coupling... \n");
	__asm__("fninit\n\t"
		"fldl %1\n\t"
		"fdivl %2\n\t"
		"fmull %2\n\t"
		"fldl %1\n\t"
		"fsubp %%st,%%st(1)\n\t"
		"fistpl %0\n\t"
		"fwait\n\t"
		"fninit"
		: "=m" (*&fdiv_bug)
		: "m" (*&x), "m" (*&y));
	if (!fdiv_bug) {
		printf("Ok, fpu using exception 16 error reporting.\n");
		return;
	}
}


int
main(void)
{
    qword_t cnt0,cnt1;

    start_receiver();

    while(1)
    {
#define ROUNDS 100000
	
	counter = ROUNDS;

	rdpmc(0,&cnt0);
	rdtsc(&ping_start);
	ping();
	rdtsc(&ping_stop);  
	rdpmc(0,&cnt1);
	printf("\ncycles: %ld (%lx, %lx)\n", (ping_stop-ping_start)/(ROUNDS*2), 
	       ping_stop, ping_start);
	printf("rdpmc(0) = %Ld\n", cnt0);
	printf("rdpmc(0) = %Ld\n", cnt1);
	printf("events: %Ld\n", ((long)(cnt1-cnt0)*100)/(ROUNDS*2));
      
#if 0
	printf("activating fpu");
	activate_fpu();
      
	counter = ROUNDS;
	rdtsc(&ping_start);
	ping();
	rdtsc(&ping_stop);  
	printf("cycles: %ld (%lx, %lx)\n", (ping_stop-ping_start)/(ROUNDS*2), 
	       ping_stop, ping_start);
#endif
	enter_kdebug("finished");
    }
    return 0;
}
