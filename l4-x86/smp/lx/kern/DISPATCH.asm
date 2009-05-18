include l4pre.inc 
  

  Copyright IBM, L4.DISPATCH, 11,03,99, 9089, K
 
;*********************************************************************
;******                                                         ******
;******         Dispatcher                                      ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 11.03.99    ******
;******                                                         ******
;*********************************************************************
 

 
  public init_dispatcher	
  public init_schedcb		
  public start_dispatch
  public dispatch
  public rtc_timer_int
  public insert_into_ready_list
  public dispatcher_open_tcb
  public dispatcher_close_tcb
  public induce_timeouts_at_present_waitees


  extrn switch_context:near
  extrn ipcman_wakeup_tcb:near
  extrn get_bottom_state:near
  extrn define_idt_gate:near
  extrn init_rtc_timer:near
  

.nolist 
include l4const.inc
include uid.inc
include adrspace.inc
include tcb.inc
include intrifc.inc
include cpucb.inc
.list
include schedcb.inc
.nolist
include lbmac.inc
include syscalls.inc
include kpage.inc
include apic.inc
include pagconst.inc
.list


ok_for x86


  public apic_timer_int
        
  extrn attach_small_space:near
  extrn get_small_space:near
  extrn apic_millis_per_pulse:abs
  extrn apic_micros_per_pulse:abs


        align 4


present_chain_version     dd 0
    





;----------------------------------------------------------------------------
;
;       init schedcb data area
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       DS,ES   linear space
;
;       interrupt & memory controller have to be already initialized
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,EBP,ESI,EDI      scratch
;
;----------------------------------------------------------------------------

  assume ds:codseg

  icode



init_schedcb:

  ;<cd> fill schedule control block area with 0		<pr> schedcb needed per processor
  mov   edi,offset pulse_counter
  mov   ecx,(scheduler_control_block_size-(offset pulse_counter))/4
  sub   eax,eax
  cld
  rep   stosd

  ;<cd> clear dispatcher table	<pr> first tcb for each prio in list
  mov   edi,offset dispatcher_table
  mov   ecx,dispatcher_table_size/4
  sub   eax,eax			; haengt
  cld
  rep   stosd

  ;<cd> set thread of prio 0 to invalid: FFFFFFFF
  mov   dword ptr ds:[dispatcher_table],-1

  ;<cd> set highest active prio to 0
  mov   ds:[highest_active_prio],0

  ret


  icod  ends



;----------------------------------------------------------------------------
;
;       init dispatcher & dispatcher tcb
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       DS,ES   linear space
;
;       interrupt & memory controller have to be already initialized
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,EBP,ESI,EDI      scratch
;
;----------------------------------------------------------------------------

  assume ds:codseg


  icode



init_dispatcher:

  ;load address of dispatcher tcb 
  mov   ebp,esp
  and   ebp,-sizeof tcb

  ;<cd> set remaining timeslice in dispatcher tcb (baseaddress:	ebp) to 1. Ts will never reach 0
  mov   [ebp+rem_timeslice],1         ; dispatcher ts will never reach 0 !
	
  ;<cd> set lower byte system clock in schedcb to 1
  mov   ds:[system_clock_low],1

  lno___prc eax	
  IFZ eax,1	
	;<cd> define Interrupt vector for thread switch
	mov   bl,thread_switch
	mov   bh,3 SHL 5
	mov   eax,offset switch_sc
	call  define_idt_gate

	;<cd> define Interrupt vector for thread schedule
	mov   bl,thread_schedule
	mov   bh,3 SHL 5
	mov   eax,offset thread_schedule_sc
	call  define_idt_gate
  FI

  ret





init_dispatcher_tcb:

  mov   ebx,offset dispatcher_tcb

  ;<cd> set dispatchers priority to 0
  mov   [ebx+prio],0

  ;<cd> initialise present list and insert dispatcher
  llinit ebx,present

  ;<cd> initialise soon and late wakeup queues
  linit soon_wakeup
  linit late_wakeup

  ret

  
  
  
  icod  ends



;----------------------------------------------------------------------------
;
;       dispatcher_open_tcb
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb write addr
;
;----------------------------------------------------------------------------


dispatcher_open_tcb:
 
  pushad
  pushfd

  cli

  ;<cd> increase version of present chain
  inc   [present_chain_version+PM]

  ;<cd> if thread is not in present list
  test  [ebp+list_state],is_present
  IFZ
	;<cd> insert thread into present list
        mov   ecx,offset present_root       ; Attention: may already linked into
        llins ebp,ecx,eax,present           ;            the present chain by
  FI                                        ;            concurrent tcb faults

  ;<cd> if thread is dispatcher
  IFZ   ebp,<offset dispatcher_tcb>
	;<cd> init dispatcher tcb
        call  init_dispatcher_tcb
  ELSE_

	;<cd> if thread is wake
        test  [ebp+fine_state],nwake
        IFNZ
		;<cd> set wakeup high to system clock high 
		mov   cl,ds:[system_clock_high]
		mov   [ebp+wakeup_high],cl
		;<cd> if thread is ready and not in ready list
		test  [ebp+fine_state],nready
		IFZ
		test  [ebp+list_state],is_ready
		CANDZ
			;<cd> insert thread into ready list
			mov   ebx,ebp
			call  insert_into_ready_list
		FI
        FI
  FI      

  popfd
  popad
  ret


;----------------------------------------------------------------------------
;
;       dispatcher close tcb
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb write addr
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       tcb eliminated from all dispatcher lists
;
;----------------------------------------------------------------------------


dispatcher_close_tcb:
 
  pushad
  pushfd

  cli

  ;<cd> increase version of present list
  inc   [present_chain_version+PM]          ; aborts concurrent parsing of
                                            ; the present chain
  ;<cd> load current tcb address
  mov   ebx,esp
  and   ebx,-sizeof tcb

  ;<cd> if thread to close is present
  test  [ebp+list_state],is_present
  IFNZ
	;<cd> delete thread from present list
        lldel ebp,edx,eax,present
  FI

  ;<cd> if thread to close is ready
  test  [ebp+list_state],is_ready
  IFNZ
	;<cd> delete from ready list
        call  delete_from_ready_list
  FI

  ;<cd> delete from late wakeup list
  mov   edx,offset late_wakeup_link
  mov   cl,is_late_wakeup
  call  delete_from_single_linked_list

  ;<cd> delete from soon wakeup list
  mov   edx,offset soon_wakeup_link
  mov   cl,is_soon_wakeup
  call  delete_from_single_linked_list

  ;<cd> reset bit 31 in wakeup low
  btr   [ebp+wakeup_low],31
  IFC
	;<cd> load current system clock to wakeup high
        mov   al,ds:[system_clock_high]
        mov   [ebp+wakeup_high],al
  FI

;;lno___thread edx,ebp
;;test  [ebp+fine_state],nbusy
;;IFZ
;;      call  signal_scheduler_reactivation
;;ELSE_
;;      mov   eax,ebp
;;      test  [eax+fine_state],nwake
;;      IFZ
;;            mov   ecx,[eax+wakeup_low]
;;            mov   dx,[eax+wakeup_high]
;;            call  signal_scheduler_wakeup
;;      FI
;;      test  [eax+fine_state],nwait+nclos
;;      IFZ
;;            mov   ecx,[eax+waiting_for]
;;            lno___thread ecx,ecx
;;            call  signal_scheduler_waitfor
;;      FI
;;FI

  popfd
  popad
  ret






;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  kcode


;<cd> NOTE: all lists are processor local, so no synch is needed to perform operations on lists

;----------------------------------------------------------------------------
;
;       delete from single linked list
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb to be deleted, write addr
;       EDX   list offset
;       CL    list mask
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       tcb deleted from list
;
;----------------------------------------------------------------------------



delete_from_single_linked_list:

  ;<cd> if thread is in list (cl)
  test  [ebp+list_state],cl
  IFNZ
        push  esi
        push  edi

	;<cd> load dispatcher tcb address
        mov   edi,offset dispatcher_tcb
        klign 16
	;<cd> while any more threads in list (tcb != 0) and (list_tcb != own_tcb) 
        DO
	      ;<cd> load next tcb from list
              mov   esi,edi
              mov   edi,[edi+edx]
	      ;<cd> Exit loop if no more threads in list
              test  edi,edi
              EXITZ
	      ;<cd> repeat until list_tcb == thread to delete
              cmp   edi,ebp
              REPEATNZ

	      ;<cd> reset bit in list state of the tcb
              not   cl
              and   [edi+list_state],cl
	      ;<cd> list.previous = list.next
              mov   edi,[edi+edx]
              mov   [esi+edx],edi
        OD

        pop   edi
        pop   esi
  FI

  ret





;----------------------------------------------------------------------------
;
;       insert thread into ready list
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBX   thread tcb
;             *not* in ready list
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX, EDI scratch
;
;       in ready list
;
;----------------------------------------------------------------------------



insert_into_ready_list:

  ;<cd> load thread priority
  movzx eax,[ebx+prio]

  ;<cd> if thread is not yet in ready list
  test  [ebx+list_state],is_ready
  IFZ
	;<cd> load list root of thread's priority
        mov   edi,[eax*4+dispatcher_table]
	;<cd> if list is empty
        test  edi,edi
        IFZ
	      ;<cd> if thread's priority above current highest active priority
              IFA   eax,ds:[highest_active_prio]
		    ;<cd> set new highest active priority
                    mov   ds:[highest_active_prio],eax
              FI
	      ;<cd> set list root to thread's tcb 
              mov   [eax*4+dispatcher_table],ebx
	      ;<cd> initialise ready list and return
              llinit ebx,ready
              ret
        FI
	;<cd> else insert thread into ready list and return
        llins ebx,edi,eax,ready
        
        ret      
  
  FI                          
 
  ke 'ihhh'
  ret
  
  
;----------------------------------------------------------------------------
;
;       delete thread from ready list
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   thread tcb
;             *in* ready list
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX   next prio
;       EDI   next tcb
;
;       EDX   scratch
;
;       NOT in ready list
;
;----------------------------------------------------------------------------



delete_from_ready_list:

  ;<cd> if thread in ready list
  test  [ebp+list_state],is_ready
  IFNZ
	;<cd> get current threads prio
        movzx eax,[ebp+prio]
	;<cd> if current thread and its successor are not equal
        IFNZ  ebp,[ebp+ready_llink].succ
		;<cd> get head of ready list
                mov   edi,dword ptr ds:[eax*4+dispatcher_table]
		;<cd> if current thread is head of ready list
                IFZ   edi,ebp
			;<cd> set new head = successor of current thread
			mov   edi,[ebp+ready_llink].succ
			mov   dword ptr ds:[eax*4+dispatcher_table],edi
                FI
		;<cd> delete currnet thread and return
                lldel ebp,edi,edx,ready
                ret
        FI       

	;<cd> only one thread in ready list for this priority
	;<cd> mark thread is not in ready list
        and   [ebp+list_state],NOT is_ready
	;<cd> set head of ready list to nilthread
        sub   edi,edi
        mov   dword ptr ds:[eax*4+dispatcher_table],edi

	;<cd> if threads prio was highest active
        cmp   eax,ds:[highest_active_prio]     
        IFZ
		;<cd> search queue from top to button to find the next highest active thread
		DO
			mov   edi,dword ptr ds:[eax*4+dispatcher_table-4]
			dec   eax
			test  edi,edi
			REPEATZ
		OD
		;<cd> reset highest active priority thread and return
		mov   ds:[highest_active_prio],eax      
                ret
        ELIFB
		;<cd> else reload highest active priority 
		mov   eax,ds:[highest_active_prio]
		ret      
        FI      
        
  FI                          
 
  ke 'grrr'
  ret
  





;----------------------------------------------------------------------------
;
;       dispatcher thread
;
;----------------------------------------------------------------------------
;
;       REP
;         enable interrupts ;
;         disable interrupts ;
;         IF interrupted threads stack is empty
;           THEN get thread from busy que
;           ELSE pop thread from interrupted threads stack
;         FI ;
;         IF thread found
;           THEN switch to thread
;         FI
;       PER .
;
;----------------------------------------------------------------------------
; Remark: The dispatcher runs on its own thread, but for sake of efficiency
;         no complete switch_context (only a temporary stack switch) is used
;         for switching from any other thread to the dispatcher. Whenever a
;         new thread is picked up by the dispatcher or by a hardware interrupt
;         the last threads stackpointer is restored and a complete context
;         switch from the last to the new thread is executed. (Note that the
;         dispatcher thread can run in any address space.)
;         Pros:
;             1. Only one instead of two switches are necessary for dispatch,
;                especially at most one switching of address space.
;             2. If there is only one thread (and the dispatcher) busy in the
;                moment, address space switch and deallocate/allocate ressources
;                (numeric coprocesor) can be omitted.
;
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBX         tcb write address of actual thread ( <> dispatcher ! )
;
;       DS, ES      linear space
;
;       interrupts disabled
;
;-----------------------------------------------------------------------------





start_dispatch:

  ;<cd> load dispatcher_tcb address 
  mov   ebx,dispatcher_tcb
  jmp   short dispatch




  align 16



dispatch:

  ;<cd> save current thread pointer
  mov   [ebx+thread_esp],esp


restart_dispatch:

  ;<cd> enable interrupts (note: interrupts won't occur before next instruction)
  sti
  ;<cd> load dispatcher's stack pointer = end of tcb - interrupted stack
  mov   esp,offset dispatcher_tcb+sizeof pl0_stack-sizeof int_pm_stack

  ;<cd> at this point pending interrupts may pass
  ;<cd> disable interrupts 
  cli

  ;<cd> get highest active prio
  mov   eax,ds:[highest_active_prio]

  ;<cd> restart dispatching if only dispatcher thread is active
  test  eax,eax
  jz    restart_dispatch

  ;<cd> else get tcb of runnable thread with highest priority
  mov   ebp,dword ptr ds:[eax*4+dispatcher_table]
	
  lno___prc edx
  ;<cd> if current thread equals new thread
  CORZ  ebp,ebx
  ;<cd> or on different processor
  CORNZ [ebp+proc_id],edx
  ;<cd> or new thread is not ready 
  test  [ebp+fine_state],nready
  CORNZ
  ;<cd> or remaining timeslice of new thread less than 0
  IFLE  [ebp+rem_timeslice],0

	;<cd> search for next ready thread:
	;<cd> get next thread from ready list
        mov   ebp,[ebp+ready_llink].succ
	;<cd> do
        DO
	      ;<cd> exit if new thread is ready and on same processor
              IFZ [ebp+proc_id],edx
	            test  [ebp+fine_state],nready
                    EXITZ
	      FI

	      ;<cd> delete thread from ready list (returns next prio and tcb)
              call  delete_from_ready_list
              mov   ebp,edi

	      ;<cd> edx is scratch after deletion from ready list so reload it.
      	      lno___prc edx

	      ;<cd> while prio not 0 (only dispatcher thread has a prio of 0) 
              test  eax,eax
              REPEATNZ
	
	      ;<cd> restart dispatch loop
              jmp   restart_dispatch
        OD
	
	;<cd> update hread pointer to new thread
        mov   dword ptr ds:[eax*4+dispatcher_table],ebp

	;<cd> update remaining timeslice
        mov   al,[ebp+rem_timeslice]
        IFLE  al,0
              add   al,[ebp+timeslice]
              mov   [ebp+rem_timeslice],al
        FI
  FI

  ;<cd> load stack pointer of new thread
  mov   esp,[ebx+thread_esp]
	
  ;<cd> switch to new thread if different from current
  cmp   ebp,ebx
  jnz   switch_context

  ret


                   


;----------------------------------------------------------------------------
;
;       switch_sc
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   0 : dispatch
;
;       ESI   <>0 : if thread on same processor then donate, thread id (low)
;		       else dispatch     
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       REG                     scratch
;
;----------------------------------------------------------------------------



  align 16


switch_sc:
 
  tpre  switch_code,ds

  ;<cd> load tcb of current thread
  mov   ebp,esp
  and   ebp,-sizeof tcb

  mov   ebx,ebp

  ;<cd> mark source tcb ready
  mark__ready ebx

  ;<cd> if blocked or dead sw_block
  mov   al,[ebp+coarse_state]
  and   al,nblocked+ndead
  cmp   al,nblocked+ndead
  xc    nz,sw_block

  ;<cd> safe return point 
  push  offset switch_ret

  ;<cd> if switch to dispatcher, dispatch
  lea___tcb ebp,esi		
  cmp   ebp,dispatcher_tcb	
  jbe   dispatch		

  ;<cd> if destination tcb != source tcb 
  IFNZ  ebp,ebx
  ;<cd> cand dest_tcb writeable 
  test__page_writable ebp	
  CANDNC
  ;<cd> cand thread on same processor
  lno___prc eax
  CANDNZ [ebp+proc_id],eax	
  ;<cd> cand esi = id (dest tcb)
  CANDZ esi,[ebp+myself]
	;<cd> switch_context if ready
        mov   al,[ebp+fine_state]
        test  al,nready
        jz    switch_context 

     ;;   and   al,NOT nwake
     ;;   IFZ   al,closed_wait
     ;;         cmp   [ebp+waiting_for],0
     ;;         jz    switch_context
     ;;   FI
  FI
  ;<cd> dispatch
  jmp   dispatch	



  align 4



XHEAD sw_block
  ;<cd> if not virtual-86 mode and rpl of code not 3, set fine state not ready 
  bt    [esp+ip_eflags+4],vm_flag
  CORC
  test  byte ptr [esp+ip_cs+4],11b
  IFNZ
        or    [ebp+fine_state],nready
  FI
  xret






  align 16


switch_ret:

  tpost eax,ds,es





;#######################################

;----------------------------------------------------------------------------
;
;       timer interrupt
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       <ESP>    INTR return vector
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
;
;       PROC timer interrupt :
;
;         pulse counter DECR thousand div milliseconds per pulse ;
;         IF pulse counter <= 0
;           THEN pulse counter INCR pulses per second ;
;                timer tick ;
;                IF end of timeslice
;                  THEN mark busy (myself)
;                FI ;
;                inspect wakeup lists ;
;                IF wakeup pending COR end of timeslice
;                  THEN IF myself in kernel mode
;                         THEN mark pending dispatch
;                         ELSE dispatch
;                       FI
;                FI
;         FI .
;
;         delta t : milliseconds per pulse .
;
;         timer tick :
;           increment system clocks ;
;           cpu clock (me) INCR delta t ;
;           remaining timeslice (me) := max (remaining timeslice - delta t, 0) .
;
;         increment system clocks :
;           system clock offset INCR delta t ;
;           propagate new clock to ipcman ;
;           IF ready threads = 0
;             THEN idle clock INCR delta t
;             ELSE ready clock INCR (ready threads * delta t) ;
;                  IF kernel active
;                    THEN kernel clock INCR delta t
;                  FI
;           FI .
;
;         inspect wakeup lists :
;                  IF system clock MOD 1024 = 0
;                    THEN parse all present tcbs
;                  FI ;
;                  IF system clock MOD 128 = 0
;                    THEN parse mid term wakeup list
;                  FI ;
;                  IF system clock MOD 4 = 0
;                    THEN parse short term wakeup list
;                  FI .
;
;
;         parse short term wakeup list :
;           actual := first (short term wakeup) ;
;           WHILE actual <> nil REP
;             abort wakeup handling if necessary ;
;             IF fine state (actual).wait
;               THEN IF wakeup (actual) <= system clock offset
;                      THEN push interrupted (myself) ;  {happens only once!}
;                           remaining timeslice (actual) := intr timeslice length ;
;                           push interrupted (actual)
;                           delete from short term wakeup list (actual)
;                    FI
;               ELSE delete from short term wakeup list (actual)
;             FI
;             actual := next (short term wakeup)
;           PER .
;
;         parse mid term wakeup list :
;           actual := first (mid term wakeup) ;
;           WHILE actual <> nil REP
;             abort wakeup handling if necessary ;
;             IF fine state (actual).wait
;               THEN IF wakeup (actual) <= system clock offset + 128
;                      THEN delete from mid term wakeup list (actual)
;                           insert into short term wakeup list (actual)
;                    FI
;               ELSE delete from mid term wakeup list (actual)
;             FI
;             actual := next (mid term wakeup)
;           PER .
;
;         parse long term wakeup list :
;           actual := first (present) ;
;           WHILE actual <> nil REP
;             abort wakeup handling if necessary ;
;             IF fine state (actual).wait
;               THEN IF wakeup (actual) <= system clock offset + 128
;                      THEN insert into mid term wakeup list (actual)
;                    FI
;             FI
;             actual := next (present)
;           PER .
;
;----------------------------------------------------------------------------




                                               ; for Pentium and higher, presence of APIC
                                               ; is considered to be default. RTC timer intr
              kcod  ends                       ; for non 486 processors therfore not in kcode          
                                               ; segment.
                                               ; NOTE: RTC timer will be used if no APIC !


        
  align 16



rtc_timer_int:

  ipre  fault
 
  reset_rtc_timer_intr


  mov   esi,offset system_clock_low

  sub   [esi+pulse_counter-offset system_clock_low],rtc_thousand_div_millis
  ja    timer_int_iret
  
  add   [esi+pulse_counter-offset system_clock_low],rtc_pulses_per_second
  mov   edx,rtc_millis_per_pulse
  mov   ecx,rtc_micros_per_pulse

  jmp   timer_int
        



              kcode
        
        
        
              align 16
        


       
        
apic_timer_int:

  ipre  fault    
              
  mov   esi,offset system_clock_low
  
  mov   edx,apic_millis_per_pulse
  mov   ecx,apic_micros_per_pulse
  
  sub   eax,eax
  mov   ds:[local_apic+apic_eoi],eax



  
  
timer_int:  
  
  mov   ebx,esp
  and   ebx,-sizeof tcb

  mov   edi,offset user_clock+offset logical_info_page
 
  add   dword ptr ds:[esi],edx
  xc    c,inc_system_clock_high

  add   dword ptr ds:[edi],ecx
  xc    c,inc_user_clock_high

  add   [ebx+cpu_clock_low],edx
  xc    c,inc_cpu_clock_high


  sub   ds:[late_wakeup_count],dl
  xc    c,late_wakeup,long

  sub   esi,esi

  sub   ds:[soon_wakeup_count],dl
  xc    c,soon_wakeup


  IFNZ  ebx,dispatcher_tcb

        sub   [ebx+rem_timeslice],dl

        CORLE
        test  esi,esi
        IFNZ

              mark__ready ebx

              mov   al,[ebx+coarse_state]
              and   al,nblocked+ndead
              cmp   al,nblocked+ndead
              IFNZ
                    or   [ebx+fine_state],nready
              FI

              push  offset timer_int_ret

              test  esi,esi
              jz    dispatch

              mov   ebp,esi
              jmp   switch_context


              timer_int_ret:
        FI
  FI



timer_int_iret:

  ipost
  
  


XHEAD inc_system_clock_high

  inc   [esi+system_clock_high-offset system_clock_low]
  xret


XHEAD inc_user_clock_high

  inc   dword ptr ds:[edi+4]
  xret


XHEAD inc_cpu_clock_high

  inc   [ebx+cpu_clock_high]
  xret





XHEAD soon_wakeup

  mov   ds:[soon_wakeup_count],soon_wakeup_interval

  movl__root ebp,soon_wakeup
  mov   eax,ds:[system_clock_low]
  DO
        movl__next ebp,ecx,soon_wakeup
        xret  z

        test  [ebp+fine_state],nwake
        IFZ
              cmp   [ebp+wakeup_low],eax
              REPEATG

              IFNZ  ebx,dispatcher_tcb
              mov   al,[ebx+prio]
              CANDA [ebp+prio],al

                    mov   esi,ebp
                    mark__interrupted ebx
              FI
              call  ipcman_wakeup_tcb
                                                      ;mark__ready ebp
        FI
        ldel  ebp,ecx,soon_wakeup
        REPEAT
  OD





  kcod ends

;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||



XHEAD   late_wakeup

  mov   ds:[late_wakeup_count],late_wakeup_interval

  sub   ds:[late_late_wakeup_count],1
  xc    c,late_late_wakeup,long

  mov   eax,ds:[system_clock_low]
  add___eax late_wakeup_interval
  movl__root ebp,late_wakeup
  clign 16
  DO
        movl__next ebp,ecx,late_wakeup
        xret  z,long
 
        test  [ebp+fine_state],nwake
        IFZ
              cmp   [ebp+wakeup_low],eax
              REPEATG

              test  [ebp+list_state],is_soon_wakeup
              IFZ
                    lins  ebp,esi,soon_wakeup
              FI
        FI
        ldel  ebp,ecx,late_wakeup
        REPEAT
  OD






XHEAD   late_late_wakeup

  mov   ds:[late_late_wakeup_count],late_late_wakeup_interval/late_wakeup_interval
  mov   eax,ds:[system_clock_low]
  add   eax,late_late_wakeup_interval
  push  edx
  sub   dl,dl
  mov   esi,ds:[present_chain_version+PM]

  mov   ebp,offset dispatcher_tcb
  clign 16
  DO

        sub   dl,4
        xc    c,permit_interrupts

        mov   ebp,[ebp+present_llink].succ
        cmp   ebp,offset dispatcher_tcb
        EXITZ

        test  [ebp+fine_state],nwake
        REPEATNZ

        test  [ebp+list_state],is_late_wakeup
        REPEATNZ

        cmp   [ebp+wakeup_low],eax
        REPEATG

        lins  ebp,esi,late_wakeup
        REPEAT

late_late_wakeup_od:

  OD
  pop   edx

  xret  ,long




XHEAD permit_interrupts

  sti
  nop
  nop
  cli
  cmp   esi,ds:[present_chain_version+PM]
  xret  z

  mov   ds:[late_late_wakeup_count],10
  jmp   late_late_wakeup_od




 
;----------------------------------------------------------------------------
;
;       thread schedule sc
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   param word	(bits 16..19 proc no)
;       EBX   ext preempter  / 0
;       ESI   dest thread id
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;	
;	EAX   old param word	(bits 16..19 proc no)
;	ECX   time low
;	EDX   time high
;	EBX   old preempter
;	ESI   partner
;
;----------------------------------------------------------------------------




thread_schedule_sc:

  tpre  trap2,ds,es

  ;<cd> get scheduler's tcb = current thread
  mov   edi,esp
  and   edi,-sizeof tcb

  mov   ch,[edi+max_controlled_prio]

thread_schedule_proxy_code:	; Precondition:	 tpre, thread schedule sc, ch 0 mcp proc_id already set
	
  ;<cd> load address of tcb in esi
  mov   edx,esi
  lea___tcb esi

  ;<cd> if target thread on same processor
  lno___prc edi
	;; edi
	;; eax:	16-19 proc_id

	;; (eax.procid = esi.procid || eax.procid = 0) => no migration
	;; schedule on eax.procid || (if eax.procid = 0: esi.procid)

  ;<cd> test for migration
  push eax
  shr eax,16
  and eax,0Fh
  IFNZ al,0
  CANDNZ [esi+proc_id],eax
	ke 'migrate thread (esi) to processor (al)'
	;; the remaining registers are in use
  FI	
  pop eax
				
  ;<cd> schedule if on destination CPU
  IFZ [esi+proc_id],edi,long
	;<cd> schedule thread normally

	;<cd> if thread exists and tid is valid
	cmp   [esi+myself],edx
	IFZ   ,,long

		;<cd> save current values to  old param word = edx, old ext preemter = e
		push  eax
	        push  ebx

		;<cd> param Word = tsAsSSPr:	timeslice(8) A(4) ThreadState(4) SmallSpaceNr(8) Prio(8)
	        mov   dl,[esi+timeslice]
		shl   edx,4
	        add   edx,10
		shl   edx,8
		
	        xchg  ebp,esi
		call  get_bottom_state	; gets fine state in eax, com partner in ebx 
	        xchg  ebp,esi

		;<cd> store current thread_state in dl  
	        mov   dl,0F0h
		IFZ   al,running
	              mov   dl,00h
		FI
	        IFZ   al,locked_running
		      mov   dl,40h
	        FI
		IFZ   al,locked_waiting
			mov   dl,80h
	        FI
	        test  al,nwait
		IFZ   al,closed_wait
	              mov   dl,0C0h
		FI
	        IFZ   al,polling
		      mov   dl,0D0h
	        FI
		;<cd> if locked or running and not running
		IFB_  dl,0C0h
	        test  [esi+fine_state],nready
		CANDNZ
			;<cd> ongoing IPC with:	pager, int preempter, ext preempter 
			mov   ebx,[esi+waiting_for]
			IFZ   ebx,[esi+pager]
				add   dl,10h
			ELIFZ ebx,[esi+int_preempter]
				add   dl,20h
			ELIFZ ebx,[esi+ext_preempter]
				add   dl,30h
			FI
		FI

	        shl   edx,12		

		;<cd> get small space number
		call  get_small_space
	        mov   dh,al              

		;<cd> get priority
	        mov   dl,[esi+prio]
        
	        pop   ebx
		pop   eax

	;<cd> cand prio <= max controlled prio
	CANDBE dl,ch,long


		;<cd> set external preemter
		IFNZ  ebx,-1
	              mov   [esi+ext_preempter],ebx
		FI
        
		;<cd> if param word valid
		IFNZ  eax,-1
		;<cd> cand new prio <= max controlled prio
		CANDBE al,ch

			;<cd> parse new param word and set values
			push  eax

			;<cd> prio >= 1 (Prio 0 reservered for dispatcher thread)
			cmp   al,1                                      ; al := max (al,1)
			adc   al,0                                      ;
              
			;<cd> if current prio != new prio 
			IFNZ  dl,al
				;<cd> if in ready list
				test  [esi+list_state],is_ready
				IFNZ
					;<cd> delete from ready list
					pushad
					mov   ebp,esi
					push  eax
					call  delete_from_ready_list
					pop   eax
					;<cd> set new prio
					mov   [esi+prio],al
					mov   ebx,esi
					;<cd> reinsert to ready list
					call  insert_into_ready_list
					popad
				ELSE_
				;<cd> else set new prio 
					mov   [esi+prio],al
				FI              
			FI         

			;<cd> set new timeslice 
		        mov   ecx,eax
	                shr   ecx,20
		        and   cl,0Fh
	                add   cl,2
		        shr   eax,cl
	                shr   eax,cl
		        IFA   eax,127
				mov   al,127
			FI
			mov   [esi+timeslice],al
              
			pop   eax

			;<cd> attach new small space
			IFB_  ah,max_small_spaces
		            call  attach_small_space
			FI
        
	        FI

		mov   ebx,edx

	        mov   edi,1000

		sub   ecx,ecx
	        test  [esi+fine_state],nwake
		IFZ
	              dec   ecx
		      mov   eax,[esi+wakeup_low]
	              sub   eax,ds:[system_clock_low]
		      mul   edi
	              mov   al,dl
		      test  eax,0FF0000FFh
	              IFZ
		            rol   eax,16
			    add   cl,80h
	              FI
		      test  al,al
	              IFZ
		            rol   eax,8
			    add   cl,40h
	              FI
		      test  al,0F0h
	              IFZ
		            shl   eax,4
			    add   cl,20h
	              FI
		      test  al,0C0h
	              IFZ
		            shl   eax,2
			    add   cl,10h
	              FI
		      mov   ch,cl
	              shl   ecx,16
		FI

		;<cd> read current time values
	        mov   eax,[esi+cpu_clock_low]
		mul   edi
	        add   edx,ecx
		movzx ecx,[esi+cpu_clock_high]
	        imul  ecx,edi
		add   edx,ecx
	        mov   ecx,eax

	        mov   eax,ebx

		;<cd> get external preempter
		mov   ebx,[esi+ext_preempter]

		;<cd> get communication partner
	        mov   esi,[esi+waiting_for]

	ELSE_
	;<cd> else return with invalid param word 
		sub   eax,eax
	        dec   eax
        
	FI
	tpost ,ds,es
  ELSE_
	ke 'thread schedule sc mp case'
	
	; setup proxy thread command:
	; eax: param word, ebx: ext preempter, esi: dest thread, edi: scheduler thread, ch: mcp
	; send cp ipc to proxy thread
	; block scheduler in wait for cp-ipc from proxy thread
	; eax: old param word, ebx old ext preempter, ecx,edx: time low / high, esi: comm partner

  FI  
  tpost ,ds,es	










;----------------------------------------------------------------------------
;
;       induce timeouts at present waitees
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   thread id (low)
;       EDI   thread id (high)
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       all present threads waiting for <ESI,EDI> got receive timeout
;
;----------------------------------------------------------------------------

        align 16



XHEAD permit_interrupts_while_inducing_timeouts

  mov   eax,[present_chain_version+PM]
  sti
  mov   cl,16
  mov   cl,cl                   ; due to 486 bug (1 cycle enabled too short)
  cli
  cmp   [present_chain_version+PM],eax
  xret  z

  pop   eax                     ; restart induce_timeouts_at_present_waitees
  popfd                         ; if someone changed the present chain by
  popad                         ; interrupt



        clign 4




induce_timeouts_at_present_waitees:
 
  pushad
  pushfd

  cli
  mov   cl,16

  mov   ebx,offset dispatcher_tcb
 
  clign 16
  DO
        mov   ebx,[ebx+present_llink].succ
        cmp   ebx,offset dispatcher_tcb
        EXITZ

        dec   cl
        xc    z,permit_interrupts_while_inducing_timeouts

        cmp   [ebx+waiting_for],esi
        REPEATNZ

        test  [ebx+coarse_state],ndead
        REPEATZ

        mov   al,[ebx+fine_state]
        test  al,npoll
        IFNZ
              and   al,nwait+nlock+nclos
              cmp   al,nlock
              REPEATNZ
        FI

        mov   ebp,ebx
        call  ipcman_wakeup_tcb
        REPEAT
  OD


  popfd
  popad
  ret










        code ends
        end


 