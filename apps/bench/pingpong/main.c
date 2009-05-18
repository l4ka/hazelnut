#include <l4/l4.h>

#include <l4/helpers.h>
#include <l4io.h>


//#define PERFMON

#define PING_PONG_PRIO 128

int MIGRATE = 0;
int INTER_AS = 1;
int SMALL_AS = 0;


#if 1
extern "C" void memset(char* p, char c, int size)
{
    for (;size--;)
	*(p++)=c;
}
#endif


static inline void rdpmc(int no, qword_t* res)
{
#if (__ARCH__ == x86)
    dword_t dummy;
    __asm__ __volatile__ (
#if defined(PERFMON)
	"rdpmc"
#else
	""
#endif
	: "=a"(*(dword_t*)res), "=d"(*((dword_t*)res + 1)), "=c"(dummy)
	: "c"(no)
	: "memory");
#endif
}


#if defined(CONFIG_ARCH_X86)
#define rdtsc(x)	__asm__ __volatile__ ("rdtsc" : "=a"(x): : "edx");
#else
#define rdtsc(x)	x = 0
#endif



#define WHATTODO				\
		"sub	%%ebp,%%ebp	\n\t"	\
		"sub	%%ecx,%%ecx	\n\t"	\
		"sub	%%eax,%%eax	\n\t"	\
		IPC_SYSENTER



l4_threadid_t master_tid;
l4_threadid_t ping_tid;
l4_threadid_t pong_tid;


dword_t ping_stack[1024];
dword_t pong_stack[1024];

void pong_thread(void)
{
#if (__ARCH__ != x86)
    dword_t dummy;
    l4_msgdope_t dope;
#endif
    l4_threadid_t t = ping_tid;

    if (MIGRATE)
	while (!get_current_cpu())
	    /* wait */;

    while(1)
#if (__ARCH__ != x86)
	l4_ipc_call(t,
		    0,
		    3, 2, 1,
		    0,
		    &dummy, &dummy, &dummy,
		    L4_IPC_NEVER, &dope);
#else
		__asm__ __volatile__ (
		    "pushl	%%ebp	\n\t"
		    WHATTODO
		    WHATTODO
		    WHATTODO
		    WHATTODO
		    WHATTODO
		    WHATTODO
		    WHATTODO
		    WHATTODO
		    "popl	%%ebp	\n\t"
		    :
		    : "S"(t.raw)
		    : "eax", "ecx", "edx", "ebx", "edi");
#endif
}

void ping_thread(void)
{
    dword_t in, out;
#ifdef PERFMON
    qword_t cnt0,cnt1;
#endif
    dword_t dummy;
    l4_msgdope_t dope;
    l4_threadid_t t = pong_tid;
    dword_t go = 1;

    /* Wait for pong thread to come up. */
    l4_ipc_receive(t, 0,
		   &dummy, &dummy, &dummy,
		   L4_IPC_NEVER, &dope);

    while (go)
    {
	for (int j = 10; j--;)
	{
	    dword_t i;
#define ROUNDS 100000
	    i = ROUNDS;
#define FACTOR (8)
	    i /= FACTOR;	i *= FACTOR;
#ifdef PERFMON
	    rdpmc(0,&cnt0);
#endif
	    rdtsc(in);
	    register l4_threadid_t t = pong_tid;
	    for (;i; i -= FACTOR)
	    {
#if (__ARCH__ != x86)
		for (int j = 0; j < FACTOR; j++)
		    l4_ipc_call(t,
			    0,
			    1, 2, 3,
			    0,
			    &dummy, &dummy, &dummy,
			    L4_IPC_NEVER, &dope);
#else
		__asm__ __volatile__ (
		    "pushl	%%ebp	\n\t"
		    WHATTODO
		    WHATTODO
		    WHATTODO
		    WHATTODO
		    WHATTODO
		    WHATTODO
		    WHATTODO
		    WHATTODO
		    "popl	%%ebp	\n\t"
		    :
		    : "S"(t.raw)
		    : "eax", "ecx", "edx", "ebx", "edi");
#endif
	    }
	    rdtsc(out);
#ifdef PERFMON
	    rdpmc(0,&cnt1);
	    printf("rdpmc(0) = %Ld\n", cnt0);
	    printf("rdpmc(1) = %Ld\n", cnt1);
	    printf("events: %ld.%02ld\n",
		   (((long)(cnt1-cnt0) )/(ROUNDS*2)),
		   (((long)(cnt1-cnt0)*100)/(ROUNDS*2))%100);
#endif
	    printf("%d cycles (%d)\n",
		   (out-in),
		   (unsigned int) (((long)(out-in) )/(ROUNDS*2)));
	    outdec((out-in)/2); kd_display(" cycles\\r\\n");
	}
	enter_kdebug("done");

	for (;;)
	{
	    outstring("What now?\r\n"
		      "  g - Continue\r\n"
		      "  q - Quit/New measurement\r\n\r\n");
	    char c = kd_inchar();
	    if ( c == 'g' ) { break; }
	    if ( c == 'q' ) { go = 0; break; }
	}
    }

    /* Tell master that we're finished. */
    l4_ipc_send(master_tid, 0, 0, 0, 0, L4_IPC_NEVER, &dope);

    for (;;)
	l4_sleep(0);
    /* NOTREACHED */
}


#define PAGE_BITS 	12
#define PAGE_SIZE	(1 << PAGE_BITS)
#define PAGE_MASK	(~(PAGE_SIZE-1))

dword_t pager_stack[512];

void pager(void)
{
    l4_threadid_t t;
    l4_msgdope_t dope;
    dword_t dw0, dw1, dw2;
    dword_t map = 2;
    
    while (4)
    {
	l4_ipc_wait(&t, 0, &dw0, &dw1, &dw2, L4_IPC_NEVER, &dope);
	
	while(5)
	{
	    /* touch page */
	    // *((volatile dword_t*) dw0) = *((volatile dword_t*) dw0);
	    dw0 &= PAGE_MASK;
	    dw1 = dw0 | 0x32;
	    l4_ipc_reply_and_wait(t, (void *) map,
				  dw0, dw1, dw2,
				  &t, 0,
				  &dw0, &dw1, &dw2,
				  L4_IPC_NEVER, &dope);
	    if (L4_IPC_ERROR(dope))
	    {
		printf("%s: error reply_and_wait (%x)\n",
		       __FUNCTION__, dope.raw);
		break;
	    }
	}
    }
}

int main(void)
{
    l4_threadid_t thepager, foo;
    l4_msgdope_t dope;
    dword_t dummy;

//    printf("KVER=%x\n", ({int v; asm("hlt" : "=a"(v)); v;}));

//    printf("KVER=%x\n", l4_get_kernelversion());

//    enter_kdebug("xxx");
//    __asm__ __volatile__ ("int $30;jmp 1f;.ascii \"huhu\";1:");
//    enter_kdebug("yyy");

    printf("KTEST using the following code sequence for IPC:\n");
    outstring("KTEST using the following code sequence for IPC:\n");
    printf("->->->->->-\n"); outstring("->->->->->-\n\r\t");
    printf("%s", IPC_SYSENTER); outstring(IPC_SYSENTER);
    printf("\r-<-<-<-<-<-\n"); outstring("\r-<-<-<-<-<-\n");

    /* Create pager */
    thepager = master_tid = l4_myself();
    thepager.id.thread = 1;
    l4_thread_ex_regs(thepager,
		      (dword_t) pager,
		      (dword_t) pager_stack + sizeof(pager_stack),
		      &foo, &foo, &dummy, &dummy, &dummy);

    for (;;)
    {
	for (;;)
	{
	    outstring("\nPlease select ipc type:\n");
	    outstring("\r\n"
		      "0: INTER-AS\r\n"
		      "1: INTER-AS (small)\r\n"
		      "2: INTRA-AS\r\n"
		      "3: XCPU\r\n");
	    char c = kd_inchar();
	    if (c == '0') { INTER_AS=1; MIGRATE=0; SMALL_AS=0; break; };
	    if (c == '1') { INTER_AS=1; MIGRATE=0; SMALL_AS=1; break; };
	    if (c == '2') { INTER_AS=0; MIGRATE=0; SMALL_AS=0; break; };
	    if (c == '3') { INTER_AS=0; MIGRATE=1; SMALL_AS=0; break; };
	}

	extern dword_t _end, _start;
	for (ptr_t x = (&_start); x < &_end; x++)
	{
	    dword_t q;
	    q = *(volatile dword_t*)x;
	}

	ping_tid = pong_tid = master_tid;
    
	if (INTER_AS)
	{
	    ping_tid.id.task += 1;
	    pong_tid.id.task += 2;
	    ping_tid.id.thread = 0;
	    pong_tid.id.thread = 0;

	    l4_task_new(pong_tid, 255,
			(dword_t) &pong_stack[1024],
			(dword_t) pong_thread, thepager);

	    l4_task_new(ping_tid, 255,
			(dword_t) &ping_stack[1024],
			(dword_t) ping_thread, thepager);

#ifdef PING_PONG_PRIO
	    l4_set_prio(ping_tid, PING_PONG_PRIO);
	    l4_set_prio(pong_tid, PING_PONG_PRIO);
#endif

	    if (SMALL_AS)
	    {
		l4_set_small(ping_tid, l4_make_small_id(2, 0));
		l4_set_small(pong_tid, l4_make_small_id(2, 1));
	    }

	    if (MIGRATE)
		l4_migrate_thread(pong_tid, 1);
	}
	else
	{
	    ping_tid.id.thread = 2;
	    pong_tid.id.thread = 3;
	    foo = get_current_pager(master_tid);
    
	    l4_thread_ex_regs(pong_tid,
			      (dword_t) pong_thread,
			      (dword_t) pong_stack + sizeof(pong_stack),
			      &foo, &foo, &dummy, &dummy, &dummy);

	    l4_thread_ex_regs(ping_tid,
			      (dword_t) ping_thread,
			      (dword_t) ping_stack + sizeof(ping_stack),
			      &foo, &foo, &dummy, &dummy, &dummy);
   
	    if (MIGRATE)
		l4_migrate_thread(pong_tid, 1);
	}

	/* Wait for measurement to finish. */
	l4_ipc_receive(ping_tid, 0,
		       &dummy, &dummy, &dummy,
		       L4_IPC_NEVER, &dope);

	if (INTER_AS)
	{
	    l4_task_new(ping_tid, 0, 0, 0, L4_NIL_ID);
	    l4_task_new(pong_tid, 0, 0, 0, L4_NIL_ID);
	}
    }

    for (;;)
	enter_kdebug("EOW");
}
