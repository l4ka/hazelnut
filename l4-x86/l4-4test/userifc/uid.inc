;----------------------------------------------------------------------------
;
;       thread & task id
;
;----------------------------------------------------------------------------
 
thread_low       record   ver1:4,  task_no:16, lthread_no:3, ver0:9
 
thread_lowx      record  xver1:4,  thread_no:19,            xver0:9
 
thread_high      record  depth:4, chief_no:16, site_no:12



threads          equ (1 SHL width thread_no)
lthreads         equ (1 SHL width lthread_no)

threads_per_task equ lthreads

tasks            equ (1 SHL width task_no)





;----------------------------------------------------------------------------
;
;       special tasks & threads
;
;----------------------------------------------------------------------------
 

kernel_task_no           equ 1

kernel_task              equ (kernel_task_no SHL task_no)


;----------------------------------------------------------------------------
;
;       special tasks & threads
;
;----------------------------------------------------------------------------
 
initial_version          equ 1

max_kernel_tcb           equ (kernel_task+(lthreads-1)*sizeof tcb+offset tcb_space)

sigma0_task              equ (2 SHL task_no+initial_version)
sigma0_pager             equ sigma0_task
sigma0_disk_driver       equ (sigma0_task+(1 SHL lthread_no))
sigma0_booter            equ (sigma0_task+(2 SHL lthread_no))

sigma1_task              equ (3 SHL task_no+initial_version)

booter_task              equ (4 SHL task_no+initial_version)
booter_lthread           equ (0 SHL lthread_no)
booter_thread            equ (booter_task+booter_lthread)

sigma2_task              equ (5 SHL task_no+initial_version)
sigma2_lthread           equ (0 SHL lthread_no)
sigma2_thread            equ (sigma2_task+sigma2_lthread)




;----------------------------------------------------------------------------
;
;                    mov task
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       thread   thread id (low)
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reg      task id (low)
;
;----------------------------------------------------------------------------


mov___task macro reg,thread

 IFNB  <thread>
 IFDIF <reg>,<thread>
   mov reg,thread
 ENDIF
 ENDIF
 and reg,NOT mask lthread_no
endm




;----------------------------------------------------------------------------
;
;                    mov lthread
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       thread   thread id (low)
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reg      lthread
;
;----------------------------------------------------------------------------



mov___lthread macro reg,thread

 IFNB  <thread>
 IFDIF <reg>,<thread>
   mov reg,thread
 ENDIF
 ENDIF
 and reg,mask lthread_no
 endm




;----------------------------------------------------------------------------
;
;                    set lthread
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       reg      thread id (low)
;       lthread  lthread
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reg      set to specified lthread
;
;----------------------------------------------------------------------------


set___lthread macro reg,lthread

 and reg,NOT mask lthread_no
 IFDIF <lthread>,<0>
   or  reg,lthread
 ENDIF
 endm



;----------------------------------------------------------------------------
;
;                    lno task
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       thread   thread id (low)
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reg      task no
;
;----------------------------------------------------------------------------


lno___task macro reg,thread

 IFNB  <thread>
 IFDIF <reg>,<thread>
   mov reg,thread
 ENDIF
 ENDIF
 and reg,mask task_no
 shr reg,task_no
 endm




;----------------------------------------------------------------------------
;
;                    lno thread / lthread
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       thread   thread id (low)
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reg      (l)thread no
;
;----------------------------------------------------------------------------


lno___thread macro reg,thread

 IFNB  <thread>
 IFDIF <reg>,<thread>
   mov reg,thread
 ENDIF
 ENDIF
 and reg,mask thread_no
 shr reg,thread_no
 endm



lno___lthread macro reg,thread

 IFNB  <thread>
 IFDIF <reg>,<thread>
   mov reg,thread
 ENDIF
 ENDIF
 and reg,mask lthread_no
 shr reg,lthread_no
 endm


