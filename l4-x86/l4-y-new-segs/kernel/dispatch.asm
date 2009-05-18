include l4pre.inc 
  

  Copyright IBM, L4.DISPATCH, 03,02,00, 9090, K
 
;*********************************************************************
;******                                                         ******
;******         Dispatcher                                      ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 03.02.00    ******
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


ok_for x86,pIII


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

  mov   edi,offset pulse_counter
  mov   ecx,(scheduler_control_block_size-(offset pulse_counter))/4
  sub   eax,eax
  cld
  rep   stosd
  
  mov   edi,offset dispatcher_table
  mov   ecx,dispatcher_table_size/4
  sub   eax,eax
  cld
  rep   stosd
  mov   dword ptr ds:[dispatcher_table],-1
  
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

  mov   ebp,esp
  and   ebp,-sizeof tcb

  mov   [ebp+rem_timeslice],1         ; dispatcher ts will never reach 0 !

  mov   ds:[system_clock_low],1

  mov   bl,thread_switch
  mov   bh,3 SHL 5
  mov   eax,offset switch_sc+KR
  call  define_idt_gate

  mov   bl,thread_schedule
  mov   bh,3 SHL 5
  mov   eax,offset thread_schedule_sc+KR
  call  define_idt_gate

  ret





init_dispatcher_tcb:

  mov   ebx,offset dispatcher_tcb

  mov   [ebx+prio],0

  llinit ebx,present

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


  inc   [present_chain_version+PM]

  test  [ebp+list_state],is_present
  IFZ
        mov   ecx,offset present_root       ; Attention: may already linked into
        llins ebp,ecx,eax,present           ;            the present chain by
  FI                                        ;            concurrent tcb faults


  IFZ   ebp,<offset dispatcher_tcb>
        call  init_dispatcher_tcb
  ELSE_


        test  [ebp+fine_state],nwake
        IFNZ
              mov   cl,ds:[system_clock_high]
              mov   [ebp+wakeup_high],cl
              test  [ebp+fine_state],nready
              IFZ
              test  [ebp+list_state],is_ready
              CANDZ
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

  inc   [present_chain_version+PM]          ; aborts concurrent parsing of
                                            ; the present chain
  mov   ebx,esp
  and   ebx,-sizeof tcb

  test  [ebp+list_state],is_present
  IFNZ
        lldel ebp,edx,eax,present
  FI

  test  [ebp+list_state],is_ready
  IFNZ
        call  delete_from_ready_list
  FI


  mov   edx,offset late_wakeup_link
  mov   cl,is_late_wakeup
  call  delete_from_single_linked_list

  mov   edx,offset soon_wakeup_link
  mov   cl,is_soon_wakeup
  call  delete_from_single_linked_list

  btr   [ebp+wakeup_low],31
  IFC
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

  test  [ebp+list_state],cl
  IFNZ
        push  esi
        push  edi

        mov   edi,offset dispatcher_tcb
        DO
              mov   esi,edi
              mov   edi,[edi+edx]
              test  edi,edi
              EXITZ
              cmp   edi,ebp
              REPEATNZ

              not   cl
              and   [edi+list_state],cl
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

  movzx eax,[ebx+prio]

  test  [ebx+list_state],is_ready
  IFZ
        mov   edi,[eax*4+dispatcher_table]
        test  edi,edi
        IFZ
              IFA   eax,ds:[highest_active_prio]
                    mov   ds:[highest_active_prio],eax
              FI      
              mov   [eax*4+dispatcher_table],ebx
              llinit ebx,ready
              ret
        FI
        
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


  test  [ebp+list_state],is_ready
  IFNZ
        movzx eax,[ebp+prio]
        IFNZ  ebp,[ebp+ready_llink].succ
        
              mov   edi,dword ptr ds:[eax*4+dispatcher_table]
              IFZ   edi,ebp
                    mov   edi,[ebp+ready_llink].succ
                    mov   dword ptr ds:[eax*4+dispatcher_table],edi
              FI
              lldel ebp,edi,edx,ready
              ret
        FI       
          
        and   [ebp+list_state],NOT is_ready
        sub   edi,edi
        mov   dword ptr ds:[eax*4+dispatcher_table],edi

        cmp   eax,ds:[highest_active_prio]     
        IFZ
              DO
                    mov   edi,dword ptr ds:[eax*4+dispatcher_table-4]
                    dec   eax
                    test  edi,edi
                    REPEATZ
              OD
              mov   ds:[highest_active_prio],eax      
              ret
              
        ELIFB
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
;         the last threads stackpointer is restored an a complete context
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

  mov   ebx,dispatcher_tcb
  jmp   short dispatch




  align 16



dispatch:
 
  mov   [ebx+thread_esp],esp


restart_dispatch:

  sti
  mov   esp,offset dispatcher_tcb+sizeof pl0_stack-sizeof int_pm_stack
  cli

  mov   eax,ds:[highest_active_prio]
  
  test  eax,eax
  jz    restart_dispatch
  
  mov   ebp,dword ptr ds:[eax*4+dispatcher_table]

  CORZ  ebp,ebx
  test  [ebp+fine_state],nready
  CORNZ
  IFLE  [ebp+rem_timeslice],0

        mov   ebp,[ebp+ready_llink].succ
        DO
              test  [ebp+fine_state],nready
              EXITZ
              
              call  delete_from_ready_list
              mov   ebp,edi
              
              test  eax,eax
              REPEATNZ
              
              jmp   restart_dispatch
        OD      

        mov   dword ptr ds:[eax*4+dispatcher_table],ebp

        mov   al,[ebp+rem_timeslice]
        IFLE  al,0
              add   al,[ebp+timeslice]
              mov   [ebp+rem_timeslice],al
        FI
  FI

  mov   esp,[ebx+thread_esp]
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
;       ESI   <>0 : donate, thread id (low)
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

  mov   ebp,esp
  and   ebp,-sizeof tcb
  mov   ebx,ebp

  mark__ready ebx

  mov   al,[ebp+coarse_state]
  and   al,nblocked+ndead
  cmp   al,nblocked+ndead
  xc    nz,sw_block

  push  offset switch_ret+KR

  lea___tcb ebp,esi
  cmp   ebp,dispatcher_tcb
  jbe   dispatch

  IFNZ  ebp,ebx
  test__page_writable ebp
  CANDNC
  CANDZ esi,[ebp+myself]

        mov   al,[ebp+fine_state]
        test  al,nready
        jz    switch_context

     ;;   and   al,NOT nwake
     ;;   IFZ   al,closed_wait
     ;;         cmp   [ebp+waiting_for],0
     ;;         jz    switch_context
     ;;   FI
  FI
  jmp   dispatch



  align 4



XHEAD sw_block

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

              push  offset timer_int_ret+KR

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
;       EAX   param word
;       EBX   ext preempter  / 0
;       ESI   thread id
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;
;
;----------------------------------------------------------------------------




thread_schedule_sc:

  tpre  trap2,ds,es

  mov   edx,esi
  lea___tcb esi

  cmp   [esi+myself],edx

  IFZ   ,,long
  
        mov   edi,esp
        and   edi,-sizeof tcb

        push  eax
        push  ebx

        mov   dl,[esi+timeslice]
        shl   edx,4
        add   edx,10
        shl   edx,8

        xchg  ebp,esi
        call  get_bottom_state
        xchg  ebp,esi
        
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
        IFB_  dl,0C0h
        test  [esi+fine_state],nready
        CANDNZ
              mov   ebx,[esi+waiting_for]
              IFZ   ebx,[esi+pager]
                    add   dl,10h
              FI
        FI

        shl   edx,12

        call  get_small_space
        mov   dh,al              
        
        mov   dl,[esi+prio]
        
        mov   ch,[edi+max_controlled_prio]

        pop   ebx
        pop   eax


  CANDBE dl,ch,long

        
                    
        IFNZ  eax,-1
        CANDBE al,ch

              push  eax
              
              cmp   al,1                                      ; al := max (al,1)
              adc   al,0                                      ;
              
              
              IFNZ  dl,al
                    test  [esi+list_state],is_ready
                    IFNZ
                          pushad
                          mov   ebp,esi
                          push  eax
                          call  delete_from_ready_list
                          pop   eax
                          mov   [esi+prio],al
                          mov   ebx,esi
                          call  insert_into_ready_list
                          popad
                    ELSE_
                          mov   [esi+prio],al
                    FI              
              FI         
              
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
              
              IFBE  ah,2*max_small_spaces-1
                    push  ebp
                    mov   ebp,esi
                    call  attach_small_space
                    pop   ebp
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

        mov   eax,[esi+cpu_clock_low]
        mul   edi
        add   edx,ecx
        movzx ecx,[esi+cpu_clock_high]
        imul  ecx,edi
        add   edx,ecx
        mov   ecx,eax


        mov   eax,ebx
        
        mov   esi,[esi+waiting_for]

  ELSE_
  
        sub   eax,eax
        dec   eax
        
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







induce_timeouts_at_present_waitees:
 
  pushad
  pushfd

  cli
  mov   cl,16

  mov   ebx,offset dispatcher_tcb
 
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


 