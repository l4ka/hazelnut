new_tcb_version  equ 043h              ; 30.12.96 

empty_tcb        equ 0FFh
          

 
thread_control_block struc

 soon_wakeup_link   dd 0
 late_wakeup_link   dd 0
 wakeup_low         dd 0
 rem_timeslice      db 0
 timeslice          db 0
 max_controlled_prio db 0
 prio               db 0

 sndq_root          dd 0,0
 sndq_llink         dd 0,0   ; virtual sender


 ready_llink        dd 0,0
                    dd 0
 task_pdir          dd 0

 com_partner        dd 0
 waddr              dd 0
 cpu_clock_low      dd 0
 cpu_clock_high     db 0
 wakeup_high        db 0
 thread_dr6         db 0
 state_sp           db 0


 myself             dd 0
 chief              dd 0
 thread_state       dd 0
 thread_proot       dd 0
 
                    db 0
 coarse_state       db 0
 list_state         db 0
 resources          db 0
 thread_esp         dd 0
 rcv_descriptor     dd 0
 ipc_control        dd 0


 pager              dd 0
 ext_preempter      dd 0
 int_preempter      dd 0
 ext_preempter_prio db 0
 int_preempter_prio db 0
                    db 0
 small_as           db 0
                    

 thread_dr0         dd 0
 thread_dr1         dd 0
 thread_dr2         dd 0
 thread_dr3         dd 0

 thread_idt_base    dd 0
 present_llink      dd 0,0
 thread_dr7         dd 0

 reg_387            dd 108/4 dup (0)

 tcb_id             db 'TCB'
 tcb_version        db 0


thread_control_block ends


virtual_sender   equ     sndq_llink



tcb struc

 pl0_stack          dd 512/4 dup (0)

tcb ends


  
kernel_stack_bottom equ (sizeof tcb-4)



auxiliary_thread_control_block struc

                    db kernel_stack_bottom-sizeof int_pm_stack+offset ip_aux_state dup (0)
  aux_state         db 0
                    dd 0                  
                    
auxiliary_thread_control_block ends
                    



;----------------------------------------------------------------------------
;
;       ressource bits
;
;----------------------------------------------------------------------------

ressource_bits   record rfree:4,xis_polled:1,com_used:1,dr_used:1,x87_used:1



is_polled        equ mask xis_polled


;----------------------------------------------------------------------------
;
;       list state
;
;----------------------------------------------------------------------------

liste_state_bits  record zpres:1,zfree:4,zmwake:1,zswake:1,zready:1

is_present       equ mask zpres
is_late_wakeup   equ mask zmwake
is_soon_wakeup   equ mask zswake
is_ready         equ mask zready


;----------------------------------------------------------------------------
;
;       coarse states
;
;----------------------------------------------------------------------------

coarse_state_bits record ydead:1,yrestart:1,yfree:5,yiopl3:1

iopl3_right   equ mask yiopl3
restarting    equ mask yrestart
ndead         equ mask ydead
unused_tcb    equ 0



;----------------------------------------------------------------------------
;
;       thread_states   (special contents of 'waiting_for')
;
;----------------------------------------------------------------------------


ready         equ 0
dead          equ 1
locked        equ 2
polling       equ 3
waiting_inner equ 4
waiting_outer equ 5
waiting_none  equ 7
waiting_any   equ -1

;waiting_tid  100h <= tid < 80000000h
;waiting_int   40h <= int < 100h 


;----------------------------------------------------------------------------
;
;       auxilary thread_states   (only valid during in kernel)
;
;----------------------------------------------------------------------------


aux_state_bits    record xfree:6,xipcmem:1,xwake:1


wakeup_active       equ mask xwake
ipc_mem_active      equ mask xipcmem

    

 

.errnz           tcb_space_size - (1 SHL (thread_no + width thread_no))
.errnz           sizeof tcb - (1 SHL thread_no)
.errnz           offset tcb_space AND (tcb_space_size-1)


;----------------------------------------------------------------------------
;
;       special tasks & threads
;
;----------------------------------------------------------------------------
 
dispatcher_lthread       equ (0 SHL lthread_no)
dispatcher_thread        equ (kernel_task+dispatcher_lthread+initial_version)
dispatcher_tcb           equ (kernel_task+dispatcher_lthread+offset tcb_space)

kbooter_lthread          equ (1 SHL lthread_no)
kbooter_thread           equ (kernel_task+kbooter_lthread+initial_version)
kbooter_tcb              equ (kernel_task+kbooter_lthread+offset tcb_space)


root_chief               equ (booter_task AND mask task_no)

max_root_tcb             equ (kbooter_tcb+(lthreads-1)*sizeof tcb)

;;max_kdebug_tcb           equ ((sigma2_thread AND mask thread_no)+offset tcb_space)
max_kdebug_tcb           equ (((threads-1) SHL thread_no)+offset tcb_space)



.erre (kernel_task+offset tcb_space) GE (offset kernel_firewall+kernel_firewall_size)



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



;----------------------------------------------------------------------------
;
;                    load pdir address
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       tcb     reg containing tcb or id (low) of addressed task/thread
;
;       DS      linear space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reg   32 bit register, contains address (r/w) of pdir
;
;----------------------------------------------------------------------------


lea___pdir macro reg,tcb

 IFNB  <tcb>
 IFDIF <reg>,<tcb>
   mov reg,tcb
 ENDIF
 ENDIF
 and reg,mask task_no
 shr reg,task_no-12
 add reg,offset pdir_space
 endm

