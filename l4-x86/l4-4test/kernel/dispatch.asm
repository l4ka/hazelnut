include l4pre.inc 
    
                           
  Copyright GMD, L4.DISPATCH, 31,01,97, 200, K
 
;*********************************************************************
;******                                                         ******
;******         Dispatcher                                      ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  07.03.90    ******
;******                                   modified: 27.07.96    ******
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


  extrn switch_context:near
  extrn ipcman_wakeup_tcb:near
  extrn get_bottom_state:near
  extrn define_idt_gate:near
  extrn flush_tcb:near
  extrn init_rtc_timer:near
  extrn physical_kernel_info_page:dword
  

.nolist 
include l4const.inc
include uid.inc
include adrspace.inc
include intrifc.inc
include tcb.inc
include cpucb.inc
include intrifc.inc
.list
include schedcb.inc
.nolist
include lbmac.inc
include syscalls.inc
include kpage.inc
include pagconst.inc
include apic.inc
.list


ok_for i486,pentium,ppro,k6


  IF    kernel_type NE i486
  
        public apic_timer_int
        
        extrn attach_small_space:near
        extrn get_small_space:near
        extrn apic_millis_per_pulse:abs
        extrn apic_micros_per_pulse:abs
  ENDIF      


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

  mov   ds:[scheduled_tcb],offset dispatcher_tcb

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

  mov   ds:[scheduled_tcb],offset dispatcher_tcb

  mov   ds:[system_clock_low],1

  mov   bl,thread_switch
  mov   bh,3 SHL 5
  mov   eax,offset switch_sc+PM
  call  define_idt_gate

  mov   bl,thread_schedule
  mov   bh,3 SHL 5
  mov   eax,offset thread_schedule_sc+PM
  call  define_idt_gate

  ret





init_dispatcher_tcb:

  mov   ebx,offset dispatcher_tcb

  mov   [ebx+prio],0

  llinit ebx,present
  llinit ebx,ready

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

  IFZ   ebp,<offset dispatcher_tcb>
        call  init_dispatcher_tcb
  FI


  inc   [present_chain_version+PM]

  test  [ebp+list_state],is_present
  IFZ
        mov   ecx,offset present_root       ; Attention: may already linked into
        llins ebp,ecx,eax,present           ;            the present chain by
  FI                                        ;            concurrent tcb faults



  test  byte ptr [ebp+ipc_control],0F0h
  CORZ
  IFBE  [ebp+thread_state],locked
  
        mov   cl,ds:[system_clock_high]
        mov   [ebp+wakeup_high],cl
        IFZ   [ebp+thread_state],ready
        test  [ebp+list_state],is_ready
        CANDZ
              call  insert_into_ready_list
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
        lldel ebp,edx,eax,ready
        IFZ   ebp,ds:[scheduled_tcb]
              mov   cl,[edx+prio]
              IFAE  cl,[eax+prio]
                    mov   eax,edx
              FI
              mov   ds:[scheduled_tcb],eax
        FI
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
        klign 16
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
;       schedule running thread
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   thread tcb
;             *not* in ready list
;
;       SS    linear kernel space
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       in ready list
;
;----------------------------------------------------------------------------



insert_into_ready_list:


 test ss:[ebp+list_state],is_ready
 IFNZ
  ke 'ihhh'
 FI

  push  eax
  push  edi
  
  mov   al,ss:[ebp+prio]
  mov   edi,ebp

  mov   ebp,ss:[scheduled_tcb]

  IFA   al,ss:[ebp+prio]

        mov   ss:[scheduled_tcb],edi

  ELSE_
        DO
              mov   ebp,ss:[ebp+ready_llink].pred
              cmp   al,ss:[ebp+prio]
              REPEATB
        OD
  FI

  llins_ss edi,ebp,eax,ready

  mov   ebp,edi
  
  pop   edi
  pop   eax
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

  tpre  switch_code

  mov   ebp,esp
  and   ebp,-sizeof tcb
  
  mov   eax,ebp
  mov   edi,esi

  mark__ready ebp

  push  offset switch_ret+PM

  lea___tcb edi,edi
  cmp   edi,dispatcher_tcb
  jbe   dispatch

  mov   ebp,edi
  
  IFNZ  edi,eax
  test__page_writable ebp
  CANDNC

        cmp   [ebp+thread_state],ready
        mov   ebp,eax
        jz    switch_context

  FI
  jmp   dispatch


  align 4







switch_ret:

  tpost eax








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
;                moment, address space switch and deallocate/allocate resources
;                (numeric coprocesor) can be omitted.
;
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBX         tcb write address of actual thread ( <> dispatcher ! )
;
;       SS          linear kernel space
;
;       interrupts disabled
;
;-----------------------------------------------------------------------------





start_dispatch:

  mov   ebp,dispatcher_tcb
  jmp   short dispatch




  align 16



dispatch:
 
  mov   ebx,ebp
  mov   ss:[ebp+thread_esp],esp

  DO
        sti
        mov   esp,offset dispatcher_tcb+sizeof pl0_stack-sizeof int_pm_stack
        cli

        mov   ebp,ss:[scheduled_tcb]

        CORZ  ebp,ebx
        CORNZ ss:[ebp+thread_state],ready
        IFLE  ss:[ebp+rem_timeslice],0

              mov   al,ss:[ebp+prio]
              mov   ebp,ss:[ebp+ready_llink].succ
              DO
                    cmp   al,ss:[ebp+prio]
                    xc    a,perhaps_back
                    
                    cmp   ss:[ebp+thread_state],ready
                    EXITZ
                    
                    lldel_ss ebp,edi,edx,ready
                    mov   ebp,edx
                    REPEAT
              OD      
                         

              mov   ss:[scheduled_tcb],ebp

              mov   al,ss:[ebp+rem_timeslice]
              IFLE  al,0
                    add   al,ss:[ebp+timeslice]
                    mov   ss:[ebp+rem_timeslice],al
              FI
        FI

        cmp   ebp,dispatcher_tcb
        REPEATZ
  OD        
  
  mov   edi,ebp
  mov   ebp,ebx
  
  cmp   edi,ebp
                
  mov   esp,ss:[ebp+thread_esp]
  jnz   switch_context
  ret



 
XHEAD perhaps_back
  
  cmp   ss:[ebp+thread_state],ready 
  xret  nz

  DO
        mov   ebp,ss:[ebp+ready_llink].pred
        cmp   al,ss:[ebp+prio]
        REPEATBE
  OD

  mov   ah,ss:[ebp+prio]
  mov   edx,ebp
  mov   ebp,ss:[ebp+ready_llink].succ
  mov   al,ss:[ebp+prio]
  cmp   al,ah
  xret  a
  mov   ebp,edx
  xret  





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




  IF    kernel_type NE i486
                                               ; for Pentium and higher, presence of APIC
                                               ; is considered to be default. RTC timer intr
              kcod  ends                       ; for non 486 processors therfore not in kcode          
                                               ; segment.
                                               ; NOTE: RTC timer will be used if no APIC !
  ENDIF
        
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



  IF    kernel_type NE i486
  
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

  ENDIF


  
  
timer_int:  
  
  mov   ebp,esp
  and   ebp,-sizeof tcb

  mov   edi,offset user_clock+offset logical_info_page
 
  add   dword ptr ds:[esi],edx
  xc    c,inc_system_clock_high

  add   dword ptr ds:[edi],ecx
  xc    c,inc_user_clock_high

  add   [ebp+cpu_clock_low],edx
  xc    c,inc_cpu_clock_high


  sub   ds:[late_wakeup_count],dl
  xc    c,late_wakeup,long

  sub   edi,edi

  sub   ds:[soon_wakeup_count],dl
  xc    c,soon_wakeup

  IFNZ  ebp,dispatcher_tcb

        sub   [ebp+rem_timeslice],dl

        CORLE
        test  edi,edi
        IFNZ

              mark__ready ebp

              push  offset timer_int_ret+PM

              test  edi,edi
              jz    dispatch
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

  inc   [ebp+cpu_clock_high]
  xret





XHEAD soon_wakeup

  mov   ds:[soon_wakeup_count],soon_wakeup_interval

  movl__root ebx,soon_wakeup
  mov   eax,ds:[system_clock_low]
  DO
        movl__next ebx,ecx,soon_wakeup
        xret  z

        test  byte ptr [ebx+ipc_control],0F0h
        IFNZ
        CANDA [ebx+thread_state],locked

              cmp   [ebx+wakeup_low],eax
              REPEATG

              IFNZ  ebp,dispatcher_tcb
              mov   al,[ebp+prio]
              CANDA [ebx+prio],al

                    mov   edi,ebx
                    mark__interrupted ebp
              FI
              call  ipcman_wakeup_tcb
                                                      ;mark__ready ebp
        FI
        ldel  ebx,ecx,soon_wakeup
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
  movl__root ebx,late_wakeup
  clign 16
  DO
        movl__next ebx,ecx,late_wakeup
        xret  z,long
 
        test  byte ptr [ebx+ipc_control],0F0h
        IFNZ
        CANDA [ebx+thread_state],locked

              cmp   [ebx+wakeup_low],eax
              REPEATG

              test  [ebx+list_state],is_soon_wakeup
              IFZ
                    lins  ebx,esi,soon_wakeup
              FI
        FI
        ldel  ebx,ecx,late_wakeup
        REPEAT
  OD






XHEAD   late_late_wakeup

  mov   ds:[late_late_wakeup_count],late_late_wakeup_interval/late_wakeup_interval
  mov   eax,ds:[system_clock_low]
  add   eax,late_late_wakeup_interval
  push  edx
  sub   dl,dl
  mov   esi,ds:[present_chain_version+PM]

  mov   ebx,offset dispatcher_tcb
  clign 16
  DO

        sub   dl,4
        xc    c,permit_interrupts

        mov   ebx,[ebx+present_llink].succ
        cmp   ebx,offset dispatcher_tcb
        EXITZ

        test  byte ptr [ebx+ipc_control],0F0h
        REPEATZ
        cmp   [ebx+thread_state],locked
        REPEATBE

        test  [ebx+list_state],is_late_wakeup
        REPEATNZ

        cmp   [ebx+wakeup_low],eax
        REPEATG

        lins  ebx,esi,late_wakeup
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
;       EBX   ext preempter (low)  / 0
;       EBP   ext preempter (high) / 0
;       ESI   thread id (low)
;       EDI   thread id (high)
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
  IFZ
        cmp   [esi+chief],edi
  FI

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
        IFZ   eax,ready
              mov   dl,00h
        FI
     ;;   IFZ   eax,  ????
     ;;         mov   dl,40h
     ;;   FI
        IFZ   eax,locked
              mov   dl,80h
        FI
        IFZ   eax,polling
              mov   dl,0D0h
        FI
        IFAE  eax,waiting_none
              mov   dl,0C0h
        FI
        IFB_  dl,0C0h 
              mov   eax,[esi+thread_state]
              IFZ   eax,polling
                    mov   eax,[esi+com_partner]
              FI
              test  eax,eax
              IFNZ
                    IFZ   eax,[esi+pager]
                          add   dl,10h
                    ELIFZ eax,[esi+int_preempter]
                          add   dl,20h
                    ELIFZ eax,[esi+ext_preempter]
                          add   dl,30h
                    FI
              FI      
        FI

        shl   edx,12

        IF    kernel_type NE i486
              call  get_small_space
              mov   dh,al              
        ENDIF            
        
        mov   dl,[esi+prio]
        
        mov   ch,[edi+max_controlled_prio]

        pop   ebx
        pop   eax


  CANDBE dl,ch,long



        IFNZ  ebx,-1
              mov   [esi+ext_preempter],ebx
              mov   [esi+ext_preempter+4],ebp
        FI
        

        IFNZ  eax,-1
        CANDBE al,ch

              push  eax
              
              cmp   al,1                                      ; al := max (al,1)
              adc   al,0                                      ;
              
              mov   [esi+prio],al
              
              IFNZ  dl,al
              test  [esi+list_state],is_ready
              CANDNZ
                    pushad
                    mov   ebp,esi
                    lldel ebp,ecx,edx,ready
                    IFZ   ebp,ds:[scheduled_tcb]
                          mov   al,[ecx+prio]
                          IFBE  al,[edx+prio]
                                mov   ecx,edx
                          FI
                          mov   ds:[scheduled_tcb],ecx
                    FI
                    call  insert_into_ready_list
                    popad
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
              
              IF    kernel_type NE i486
              
                    IFB_  ah,max_small_spaces
                          call  attach_small_space
                    FI
              
              ENDIF            
        
        FI
        mov   ebx,edx


        mov   edi,1000

        sub   ecx,ecx
   ;;     test  [esi+fine_state],nwake
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
        
        mov   ebx,[esi+ext_preempter]
      ;;  mov   ebp,[esi+ext_preempter+4]

      ;;  mov   edi,[esi+waiting_for+4]
      ;;  mov   esi,[esi+waiting_for]

  ELSE_
  
        sub   eax,eax
        dec   eax
        
  FI
  
  tpost ,ds,es







        code ends
        end


 