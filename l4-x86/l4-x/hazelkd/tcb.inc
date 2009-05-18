
thread_control_block struc

    ipc_buffer		dd	0,0,0
    ipc_timeout		dd 	0
    rcv_descriptor		dd	0
    proot_ptr			dd	0
    myself			dd	0
    pager			dd	0


    thread_idt_base		dd	0
    thread_esp		dd	0
    fine_state		dd	0
    list_state		dd	0
    prio			dd	0
    timeslice			dd	0
    rem_timeslice		dd	0
    wakeup			dd	0,0 	; /* valid, iff (queue_state & TS_QUEUE_WAKEUP) */
	
    present_llink		dd	0,0		; /* valid, iff (list_state & is_PRESENT) */
    ready_llink		dd	0,0		; /* valid, iff (list_state & is_READY) */
    wakeup_llink		dd	0,0		; /* valid, iff (list_state & is_WAKEUP) */
    ressources   		dd	0

    copy_area1		dd	0
    copy_area2		dd	0
    send_queue_head		dd	0	
    sndq_llink		dd	0,0		; /* valid, iff (list_state & is_WAKEUP) */
    partner			dd 	0		; /* IPC-partner (if in IPC), otherwise partner we wait for, L4_NIL_ID if open wait */
    intr_pending		dd	0
    unwind_ipc_sp		dd	0

	ifdef SMP
    	cpu			dd	0
    	tcb_spinlock	dd	0
	endif

    scratch			dd	0
    magic			dd	0
   
thread_control_block	ends


tcb struc

 pl0_stack          dd (KB1)/4 dup (0)

tcb ends


llinks  struc
  
  pred                    dd    0
  succ                    dd    0

llinks  ends





;----------------------------------------------------------------------------
;
;       fine_states
;
;----------------------------------------------------------------------------

fine_state_bits  record  xfree:4,xxcpu:1,xnlk:1,xnpo:1,xnbu:1

nready  equ mask xnbu
npoll   equ mask xnpo
nlock   equ mask xnlk
xcpu	equ	mask xxcpu



running                   equ (     +nlock+npoll	            )	
locked_waiting            equ (		       npoll+nready     )
locked_running            equ (            npoll	            )
polling                   equ (	     nlock+		 nready     )
aborted			      equ (      nlock+npoll+nready       )
waiting				equ (-1)

xcpu_locked_waiting		equ (xcpu+       npoll+nready       )
xcpu_locked_running	      equ (xcpu+       npoll	            )



;------ list state masks ---------------------


is_ready		equ	01
is_present		equ   02
is_wakeup		equ	04
is_sendq		equ	08



;------ resources ----------------------------


com0_used		equ	01
com1_used		equ	02

no_copy_area	equ (-1)







;----------------------------------------------------------------------------
;
;                    tcb lea      (access tcb)
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       thread   thread id (low)
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reg     linear read / write addr of tcb
;
;----------------------------------------------------------------------------


lea___tcb macro reg,thread

 IFNB  <thread>
 IFDIF <reg>,<thread>
   mov reg,thread
 ENDIF
 ENDIF
 and reg,mask thread_no
 add reg,offset tcb_space
endm


