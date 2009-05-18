#ifndef MEMMAP_LOCK_H
#define MEMMAP_LOCK_H

extern int memmap_lock;

static inline dword_t
i386_xchg(dword_t *addr, dword_t newval)
{
  dword_t oldval;
  __asm__ __volatile__ ("xchg %0, (%2)\n\t"
			: "=r" (oldval)
			: "0" (newval), "r" (addr)
			: "memory"
			);
  return oldval;
}

static inline dword_t
i386_cmpxchg(dword_t *addr, dword_t oldval, dword_t newval)
{
  char ret;
  __asm__ __volatile__ ("cmpxchg %1,(%2) ; sete %b0"
			: "=a" (ret)
			: "q" (newval), "q" (addr), "0" (oldval)
			: "memory"
		       );
  return ret;
}

static inline void
enter_memmap_functions(int me, l4_threadid_t locker)
{
  if (i386_xchg(&memmap_lock, me) != -1)
    {
      int error;
      dword_t d;
      l4_msgdope_t result;
      if ((error = l4_i386_ipc_receive(locker, L4_IPC_SHORT_MSG, &d, &d,
				       L4_IPC_NEVER, &result)) ||
          (error = l4_i386_ipc_send   (locker, L4_IPC_SHORT_MSG, 0, 0,
				       L4_IPC_NEVER, &result)))
	panic("Error 0x%02x woken up\n", error);
    }
}

static inline void
leave_memmap_functions(int me, l4_threadid_t candidate)
{
  if (!i386_cmpxchg(&memmap_lock, me, -1))
    {
      int error;
      dword_t d;
      l4_msgdope_t result;
      if ((error = l4_i386_ipc_call(candidate,
				    L4_IPC_SHORT_MSG, 0, 0,
				    L4_IPC_SHORT_MSG, &d, &d,
				    L4_IPC_NEVER, &result)))
	panic("Error 0x%02x waking up\n", error);
    }
}

#endif

