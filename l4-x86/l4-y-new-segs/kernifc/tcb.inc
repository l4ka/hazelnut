new_tcb_version  equ 0f2h              ; 14.04.96 

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
 sndq_llink         dd 0,0   ; virtual sender, propagatee tcb

 ready_llink        dd 0,0
 com_partner        dd 0
 thread_proot       dd 0

 thread_seg         dd 0
                    dd 0
 as_base      	dd 0
 as_size            dd 0


 myself             dd 0
 waiting_for        dd 0
 waddr              dd 0
 mword2			dd 0
 
 fine_state         db 0
 coarse_state       db 0
 list_state         db 0
 ressources         db 0
 thread_esp         dd 0
 rcv_descriptor     dd 0
 timeouts           dd 0

 cpu_clock_low      dd 0
 cpu_clock_high     db 0
 wakeup_high        db 0
 thread_dr6         db 0
 state_sp           db 0
 pager              dd 0
 clan_depth 		db 0
 				db 0,0,0

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
propagatee_tcb   equ     (sndq_llink+4)   



tcb struc

 pl0_stack          dd (KB1)/4 dup (0)

tcb ends


;----------------------------------------------------------------------------
;
;       ressource bits
;
;----------------------------------------------------------------------------

ressource_bits   record rfree:3,in_partner_space:1,com_used:1,dr_used:1,M4_locked:1,x87_used:1


;.errnz  M4_locked eq (9-2)  ; i_flag - 2


;----------------------------------------------------------------------------
;
;       list state
;
;----------------------------------------------------------------------------

liste_state_bits  record zpres:1,zfree:2,zpolled:1,zpoll:1,zmwake:1,zswake:1,zready:1

is_present       equ mask zpres
is_polled        equ mask zpolled
is_polling       equ mask zpoll
is_late_wakeup   equ mask zmwake
is_soon_wakeup   equ mask zswake
is_ready         equ mask zready


;----------------------------------------------------------------------------
;
;       coarse states
;
;----------------------------------------------------------------------------

coarse_state_bits record ydead:1,yrestart:1,yautoprop:1,yfree:2,yiopl3:1,yv86:1,yblck:1

nblocked         equ mask yblck
v86              equ mask yv86
iopl3_right      equ mask yiopl3
restarting       equ mask yrestart
auto_propagating equ mask yautoprop
ndead            equ mask ydead
unused_tcb       equ 0



;----------------------------------------------------------------------------
;
;       fine_states
;
;----------------------------------------------------------------------------

fine_state_bits  record  xnwt:1,xnrcv:1,xfree:1,xnlk:1,xnpo:1,xnbu:1,xnwk:1,xncl:1

nwait   equ mask xnwt
nrcv    equ mask xnrcv
nlock   equ mask xnlk
npoll   equ mask xnpo
nready  equ mask xnbu
nclos   equ mask xncl
nwake   equ mask xnwk


running                   equ (-256+nwait+nrcv+nlock+npoll+       nclos+nwake)
locked_waiting            equ (-256+nwait+           npoll+nready+nclos+nwake)
locked_closed_waiting     equ (-256+nwait+           npoll+nready+      nwake)
locked_running            equ (-256+nwait+nrcv+      npoll+       nclos+nwake)
polling                   equ (-256+nwait+nrcv+nlock+      nready+nclos)
open_wait                 equ (           nrcv+nlock+npoll+nready+nclos)
closed_wait               equ (           nrcv+nlock+npoll+nready)
aborted                   equ (-256+nwait+nrcv+nlock+npoll+nready+nclos+nwake)



.erre  nwait      EQ 80h    ; nwt bit tested by IFS
.erre  nclos      EQ 01h    ; nclos bit part of dword aligned address





.erre            tcb_space_size GE (1 SHL (thread_no + width thread_no))
.errnz           sizeof tcb - (1 SHL thread_no)
.errnz           offset tcb_space AND (tcb_space_size-1)

.erre            sizeof task_root/4 GE tasks

;----------------------------------------------------------------------------
;
;       prios
;
;----------------------------------------------------------------------------


nil_prio      equ 0
min_prio      equ 1
max_prio      equ 255

prios         equ (max_prio-nil_prio+1)


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

dispatcher_table         equ (kbooter_tcb + sizeof tcb)
dispatcher_table_size    equ (prios*4)

.erre (dispatcher_table+dispatcher_table_size-dispatcher_tcb)/sizeof tcb LE lthreads

root_chief               equ ((booter_task AND mask task_no) SHL (chief_no-task_no)

max_root_tcb             equ (kbooter_tcb+(lthreads-1)*sizeof tcb)




.errnz  (offset sigma0_proot-offset task_root)/4 - sigma0_task_no

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
;       tcb     32 bit register containing tcb or id (low) of addressed task/thread
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

