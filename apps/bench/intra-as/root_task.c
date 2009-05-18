#include <l4/l4.h>
#include <l4io.h>


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
	"rdpmc"
	: "=a"(*(dword_t*)res), "=d"(*((dword_t*)res + 1)), "=c"(dummy)
	: "c"(no)
	: "memory");
#endif
}









dword_t in, out;

dword_t pong_stack[128];
l4_threadid_t src;

void pong_thread()
{
    dword_t dummy;
    l4_msgdope_t dope;
    l4_threadid_t t = src;

    while(1)
#if (__ARCH__ != x86)
	l4_ipc_call(src,
		    0,
		    1, 2, 3,
		    0,
		    &dummy, &dummy, &dummy,
		    L4_IPC_NEVER, &dope);
#else
    {
	__asm__ __volatile__ (
	    "sub	%%ebp, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (t.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(t.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "sub	%%ebp, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (t.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(t.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "sub	%%ebp, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (t.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(t.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "sub	%%ebp, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (t.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(t.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "sub	%%ebp, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (t.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(t.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "sub	%%ebp, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (t.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(t.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "sub	%%ebp, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (t.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(t.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "sub	%%ebp, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (t.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(t.raw)
	    : "edx", "ebx", "edi");
    }
#endif
}




int main()
{
    l4_threadid_t tid;

    printf("KTEST using the following code sequence for IPC:\n");
    outstring("KTEST using the following code sequence for IPC:\n");
    printf("->->->->->-\n"); outstring("->->->->->-\n\r");
    printf(IPC_SYSENTER); outstring(IPC_SYSENTER);
    printf("\r-<-<-<-<-<-\n"); outstring("\r-<-<-<-<-<-\n");

    qword_t cnt0,cnt1;

    l4_threadid_t foo;
    dword_t dummy;
    l4_msgdope_t dope;

    src = tid = l4_myself();
    tid.id.thread = 3;
    
    l4_thread_ex_regs(tid,
		      (dword_t) pong_thread, (dword_t) pong_stack + sizeof(pong_stack),
		      &foo, &foo, &dummy, &dummy, &dummy);

    /* wait for thread to come up */
    l4_ipc_receive(tid, 0,
		   &dummy, &dummy, &dummy,
		   L4_IPC_NEVER, &dope);

    while (14101973)
    {
	for (int j = 10; j--;)
	{

	dword_t i;
#define ROUNDS 100000

	i = ROUNDS;
#define FACTOR (8)
	i /= FACTOR;	i *= FACTOR;
	outdec(i); kd_display(": ");
    rdpmc(0,&cnt0);
	__asm__ __volatile__ ("rdtsc" : "=a"(in): : "edx");
	for (;i; i -= FACTOR)
	{
#if (__ARCH__ != x86)
	    l4_ipc_call(tid,
			0,
			0, 0, 0,
			0,
			&dummy, &dummy, &dummy,
			L4_IPC_NEVER, &dope);
#else
	__asm__ __volatile__ (
	    "mov	$0, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (tid.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(tid.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "mov	$0, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (tid.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(tid.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "mov	$0, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (tid.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(tid.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "mov	$0, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (tid.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(tid.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "mov	$0, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (tid.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(tid.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "mov	$0, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (tid.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(tid.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "mov	$0, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (tid.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(tid.raw)
	    : "edx", "ebx", "edi");
	__asm__ __volatile__ (
	    "mov	$0, %%ebp	\n\t"
	    IPC_SYSENTER
	    : "=a" (dummy), "=c" (dummy), "=S" (tid.raw)
	    : "a"(0), "c" (L4_IPC_NEVER.raw), "S"(tid.raw)
	    : "edx", "ebx", "edi");
#endif
	}
	__asm__ __volatile__ ("rdtsc" : "=a"(out): : "edx");
    rdpmc(0,&cnt1);
    printf("rdpmc(0) = %Ld\n", cnt0);
    printf("rdpmc(0) = %Ld\n", cnt1);
	printf("events: %ld.%02ld\n",
	       (((long)(cnt1-cnt0) )/(ROUNDS*2)),
	       (((long)(cnt1-cnt0)*100)/(ROUNDS*2))%100);
	printf("%d cycles RTT\n", out-in);
	outdec(out-in); kd_display(" cycles RTT\\r\\n");
	};
	enter_kdebug("done");
    }
}


