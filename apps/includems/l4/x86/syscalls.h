/**********************************************************************
 * (c) 1999, 2000 by University of Karlsruhe and Dresden University
 *
 * filename: l4/x86/syscalls.h
 * 
 */

#ifndef __L4_X86_SYSCALLS_H__ 
#define __L4_X86_SYSCALLS_H__ 


#define L4_FP_REMAP_PAGE	0x00	/* Page is set to read only */
#define L4_FP_FLUSH_PAGE	0x02	/* Page is flushed completly */
#define L4_FP_OTHER_SPACES	0x00	/* Page is flushed in all other */
					/* address spaces */
#define L4_FP_ALL_SPACES	0x80000000U
					/* Page is flushed in own address */ 
					/* space too */

#define L4_NC_SAME_CLAN		0x00	/* destination resides within the */
					/* same clan */
#define L4_NC_INNER_CLAN	0x0C	/* destination is in an inner clan */
#define L4_NC_OUTER_CLAN	0x04	/* destination is outside the */
					/* invoker's clan */

#define L4_CT_LIMITED_IO	0
#define L4_CT_UNLIMITED_IO	1
#define L4_CT_DI_FORBIDDEN	0
#define L4_CT_DI_ALLOWED	1


/*
 * L4 flex page unmap
 */
L4_INLINE void
l4_fpage_unmap(dword_t fpage, dword_t map_mask)
{
    __asm {
		mov	eax, fpage;
		mov	ecx, map_mask;
		push	ebp;
		int	0x32;
		pop	ebp;
    }
}


/*
 * L4 id myself
 */
L4_INLINE
l4_threadid_t l4_myself(void)
{
    l4_threadid_t temp_id;
    __asm {
		mov	esi, 0;
		push	ebp;
		int	0x31;
		pop	ebp;
		mov	temp_id, esi
    }
    return temp_id;
}


/*
 * L4 id next chief
 */

// ok
L4_INLINE int 
l4_nchief(l4_threadid_t destination, l4_threadid_t *next_chief)
{
    int tmp_type;
    __asm {
		mov	esi, destination.raw;
		push	ebp;
		int	0x31;
		pop	ebp;
		mov	ebx, next_chief
		mov	[ebx], esi;
		mov	tmp_type, eax;
    }
    return tmp_type;
}

/*
 * L4 lthread_ex_regs
 */
// ok
L4_INLINE void
l4_thread_ex_regs(l4_threadid_t destination, dword_t uip, dword_t usp,
		  l4_threadid_t *preempter, l4_threadid_t *pager,
		  dword_t *old_eflags, dword_t *old_eip, dword_t *old_esp)
{
    int tid = destination.id.thread;
    __asm {
		mov	eax, tid;
		mov	ecx, usp;
		mov	edx, uip;
		mov	esi, pager
		mov	esi, [esi];
		mov	edi, preempter;
		mov	edi, [edi];
		push	ebp;
		int	0x35;
		pop	ebp;
		mov	ebx, preempter;
		mov	[ebx], edi;
		mov	ebx, pager
		mov	[ebx], esi;
		mov	ebx, old_eip;
		mov	[ebx], edx;
		mov	ebx, old_esp;
		mov	[ebx], ecx;
		mov	ebx, old_eflags;
		mov	[ebx], eax;
    }
}


/*
 * L4 thread switch
 */

// ok
L4_INLINE void
l4_thread_switch(l4_threadid_t destination)
{
    __asm {
		mov	esi, destination;
		push	ebp;
		int	0x33;
		pop	ebp;
    }
}



/*
 * L4 thread schedule
 */
// ok
L4_INLINE cpu_time_t
l4_thread_schedule(l4_threadid_t dest, l4_sched_param_t param,
		   l4_threadid_t *ext_preempter, l4_threadid_t *partner,
		   l4_sched_param_t *old_param)
{
    union {
	cpu_time_t  time;
	dword_t t[2];
    } tmp;
	
    __asm {
		mov	eax, param;
		mov	edx, ext_preempter;
		mov	edx, [edx]
		mov	esi, dest;
		push	ebp;
		int	0x34;
		pop	ebp;
		mov	tmp.t[0], ecx;
		mov	tmp.t[4], edx;
		mov	ecx, old_param
		mov	[ecx], eax;
		mov	ecx, partner
		mov	[ecx], esi;
		mov	ecx, ext_preempter;
		mov	[ecx], ebx;
    }
    return tmp.time;
}

 

/*
 * L4 task new
 */
L4_INLINE l4_taskid_t 
l4_task_new(l4_taskid_t destination, dword_t mcp_or_new_chief, 
	    dword_t usp, dword_t uip, l4_threadid_t pager)
{
    l4_taskid_t temp_id;

    __asm {
		mov   ebx, pager;
		mov   eax, mcp_or_new_chief;
		mov   ecx, usp;
		mov   edx, uip;
		mov   esi, destination;
		push  ebp;
		int   0x36;
		pop   ebp;
		mov   temp_id, eax;
    }
    return temp_id;
}

#endif













