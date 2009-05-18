 /* 
 * $Id: x-lid.c,v 1.1 2001/03/07 11:30:51 voelp Exp $
 */

#ifndef __LNX_LId_TYPE_H__ 
#define __LNX_LId_TYPE_H__ 

//#include <l4/compiler.h>
#define L4_INLINE inline


typedef struct {
  unsigned version____:10;
  unsigned lthread____:6;
  unsigned task____:8;
  unsigned chief____:8;
} LnThreadId____;

typedef struct {
  unsigned version____:10;
  unsigned thread____:14;
  unsigned chief____:8;
} LnThreadId_____;

typedef struct {
  unsigned intr____:10;
  unsigned zero____:22;
} LnInterruptId____;


typedef union {LnThreadId____ t; LnThreadId_____ t_; unsigned w;} LnThreadId ;

typedef LnThreadId LId;



#define LId_Nil   	         ((LId) {0} ) 
#define LId_Invalid	         ((LId) {0xffffffff} )


//-------- Relational Operators on LId s -----------


L4_INLINE int LId_Equal (LId LeftThreadId, LId RightThreadId)
{
  return ( LeftThreadId.w == RightThreadId.w);
}

L4_INLINE int LId_IsNil (LId ThreadId)
{
  return LId_Equal (ThreadId, LId_Nil);
}

L4_INLINE int IsInvalid_LId (LId ThreadId)
{
  return LId_Equal (ThreadId, LId_Invalid);
}


//------- Tasks and LIds -------------------


#define LId_MaxTasks               256
#define LId_MaxTaskNo              (LId_MaxTasks-1)

L4_INLINE int LId_TaskNo (LId ThreadId)
{
  return ThreadId.t.task____;
}

L4_INLINE LId LId_Task (LId ThreadId, int TaskNo)
{
  ThreadId.t.task____ = TaskNo ;
  return ThreadId ;
}


L4_INLINE int LId_SameTask (LId left, LId right)
{
  return (LId_TaskNo (left) == LId_TaskNo (right));
}


//------- LThreads and LIds -------------------


#define LId_MaxLThreads            64
#define LId_MaxLThreadNo           (LId_MaxLThreads-1)


L4_INLINE int LId_LThreadNo (LId ThreadId)
{
  return ThreadId.t.lthread____;
}

L4_INLINE LId LId_LThread (LId ThreadId, int LThreadNo)
{
  ThreadId.t.lthread____ = LThreadNo ;
  return ThreadId ;
}

L4_INLINE LId LId_FirstLThread (LId ThreadId)
{
  ThreadId.t.lthread____ = 0 ;
  return ThreadId ;
}

L4_INLINE LId LId_NextLThread (LId ThreadId)
{
  ThreadId.t.lthread____ ++ ;
  return ThreadId ;
}

L4_INLINE LId LId_MaxLThread (LId ThreadId)
{
  ThreadId.t.lthread____ = LId_MaxLThreadNo ;
  return ThreadId ;
}


//------- Threads and LIds -------------------


#define LId_MaxThreads             (LId_MaxTasks*LId_MaxLThreads)
#define LId_MaxThreadNo            (LId_MaxThreads-1)


L4_INLINE int LId_ThreadNo (LId ThreadId)
{
  return ThreadId.t_.thread____;
}

//L4_INLINE LId LId_Thread (LId ThreadId, int ThreadNo)
//{
//  return (LnThreadId_X) ThreadId.t.thread____;
//}



//------- Chiefs and LIds -------------------


#define LId_MaxChiefs              LId_MaxTasks
#define LId_MaxChiefNo             (LId_MaxChiefs-1)


L4_INLINE int LId_ChiefNo (LId ThreadId)
{
  return ThreadId.t.chief____;
}

L4_INLINE LId LId_Chief (LId ThreadId, int ChiefNo)
{
  ThreadId.t.chief____ = ChiefNo ;
  return ThreadId ;
}


//------- Versions and LIds -------------------

#define LId_MaxVersions            1024
#define LId_MaxVersionNo           (LId_MaxVersions-1)



L4_INLINE int LId_VersionNo (LId ThreadId)
{
  return ThreadId.t.version____;
}


L4_INLINE LId LId_Version (LId ThreadId, int VersionNo)
{
  ThreadId.t.version____ = VersionNo ;
  return ThreadId ;
}

L4_INLINE LId LId_FirstVersion (LId ThreadId)
{
  ThreadId.t.version____ = 0 ;
  return ThreadId ;
}

L4_INLINE LId LId_NextVersion (LId ThreadId)
{
  ThreadId.t.version____ ++ ;
  return ThreadId ;
}

L4_INLINE LId LId_MaxVersion (LId ThreadId)
{
  ThreadId.t.version____ = LId_MaxVersionNo ;
  return ThreadId ;
}


//------- Interrupts and LIds -------------------


#define LId_MaxInterrupts          16
#define LId_MaxInterruptNo         (LId_MaxInterrupts-1)


L4_INLINE int LId_IsInterrupt (LId ThreadId)
{
  return ( ( ThreadId.w < (LId_MaxInterruptNo+1)) && (ThreadId.w > 0) ) ;
}

L4_INLINE int LId_InterruptNo (LId ThreadId)
{
  return ThreadId.w-1 ;
}

L4_INLINE LId LID_Interrupt (int InterruptNo)
{
  return (LId) {InterruptNo+1} ;
}

L4_INLINE LId LId_FirstInterrupt (void)
{
  return (LId) {1} ;
}

L4_INLINE LId LId_NextInterrupt (LId ThreadId)
{
  return (LId) {ThreadId.w+1} ;
}

L4_INLINE LId LId_MaxInterrupt (void)
{
  return (LId) {LId_MaxInterruptNo+1} ;
}



#endif /* __LNX_LId_TYPE_H__ */ 


