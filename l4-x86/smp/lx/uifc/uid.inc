


;----------------------------------------------------------------------------
;
;       thread & task id
;
;----------------------------------------------------------------------------
 
thread_id      	 record   chief_no:8,  task_no:8, lthread_no:6, ver:10

thread_id_x    	 record   xchief_no:8,            thread_no:14, xver:10



threads          equ (1 SHL width thread_no)
lthreads         equ (1 SHL width lthread_no)

threads_per_task equ lthreads

tasks            equ (1 SHL width task_no)

                                log2 <tasks>
log2_tasks       equ  log2_


max_lthread_no   equ (lthreads-1)
max_task_no      equ (tasks-1)   





;----------------------------------------------------------------------------
;
;       special tasks & threads
;
;----------------------------------------------------------------------------
 

kernel_task_no           equ 1 ; + pnr

kernel_task              equ (kernel_task_no SHL task_no)

ipc_transparent          equ 0
ipc_inhibited            equ 0FFFFFFFFh
ipc_locked               equ 0FFFFFFFEh


;----------------------------------------------------------------------------
;
;       special tasks & threads
;
;----------------------------------------------------------------------------
 
initial_version          equ 1

max_kernel_tcb           equ (kernel_task+(lthreads-1)*sizeof tcb+offset tcb_space)


sigma0_task_no	     equ 2 ;+ max_cpu-1
sigma1_task_no	     equ 3 ;+ max_cpu-1
booter_task_no	     equ 4 ;+ max_cpu-1

root_chief_no		     equ booter_task_no
max_clan_depth	     equ 15

sigma0_task              equ (sigma0_task_no SHL task_no+initial_version+root_chief_no SHL chief_no)

sigma1_task              equ (3 SHL task_no+initial_version+root_chief_no SHL chief_no)

booter_task              equ (4 SHL task_no+initial_version+root_chief_no SHL chief_no)
booter_lthread           equ (0 SHL lthread_no)
booter_thread            equ (booter_task+booter_lthread)




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
;                    mov chief
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       thread   thread id
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reg      chief id, task only, ver and chief undefined
;
;----------------------------------------------------------------------------


mov___chief macro reg,thread

 IFNB  <thread>
 IFDIF <reg>,<thread>
   mov reg,thread
 ENDIF
 ENDIF
 shr reg,chief_no-task_no
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


