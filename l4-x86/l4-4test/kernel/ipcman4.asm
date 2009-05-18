include l4pre.inc 
  

  Copyright GMD, L4.IPCMAN.4, 27,07,96, 158, K
 
 
;*********************************************************************
;******                                                         ******
;******         IPC Manager                                     ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  22.07.90    ******
;******                                   modified: 27.07.96    ******
;******                                                         ******
;*********************************************************************
 

  public init_ipcman
  public init_sndq
  public init_intr_control_block
  public ipcman_open_tcb
  public ipcman_close_tcb
  public ipcman_wakeup_tcb
  public ipcman_rerun_thread
  public restart_poll_all_senders
  public detach_intr
  public push_ipc_state
  public pop_ipc_state
  public cancel_if_within_ipc
  public cancel_ipc
  public get_bottom_state

  public ipc_sc
  public id_nearest_sc



  extrn deallocate_resources_ipc:near
  extrn deallocate_resources_int:near
  extrn switch_context:near
  extrn dispatch:near
  extrn insert_into_ready_list:near
  extrn define_idt_gate:near
  extrn mask_hw_interrupt:near
  extrn map_fpage:near
  extrn grant_fpage:near
  extrn irq0_intr:abs
  extrn irq15:abs


.nolist
include l4const.inc
include uid.inc
include adrspace.inc
include intrifc.inc
include tcb.inc
include schedcb.inc
include cpucb.inc
include pagconst.inc
.list
include msg.inc
.nolist
include syscalls.inc
.list



ok_for i486



  assume ds:codseg



;----------------------------------------------------------------------------
;
;20.02.95 jl:  flexpage messages (temp mapping) introduced
;
;
;----------------------------------------------------------------------------




;----------------------------------------------------------------------------
;
;       interrupt associated threads
;
;----------------------------------------------------------------------------


intr1_intr0   equ 8




intr_control_block struc

                          db offset intr_cb dup (?)
 
  intr_associated_tcb     dd intr_sources dup (?)

intr_control_block ends 
 



;----------------------------------------------------------------------------
;
;       init intr control block
;
;----------------------------------------------------------------------------
;
;       EAX   bit n = 0 : intr usable
;                   = 1 : intr reserved for kernel
;
;----------------------------------------------------------------------------


  icode


init_intr_control_block:

  pushad

  sub   ecx,ecx
  DO
        shr   eax,1
        sbb   ebx,ebx
        mov   [(ecx*4)+intr_associated_tcb],ebx

        inc   ecx
        cmp   ecx,intr_sources
        REPEATB
  OD

  popad
  ret


  icod  ends



;----------------------------------------------------------------------------
;
;       send ques
;
;----------------------------------------------------------------------------
; send que INVARIANT:
;
;       All tcbs in send ques are present in RAM !!!
;
;       (So insert/delete will never induce paging!)
;       (Swapping out such a tcb must delete it from the que.)
;
;----------------------------------------------------------------------------




;----------------------------------------------------------------------------
;
;       init send que
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb write addr
;
;       DS    linear space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       send que of tcb initialized empty
;
;----------------------------------------------------------------------------


init_sndq:

  push  ebp
  and   [ebp+list_state],NOT is_polled
  add   ebp,offset sndq_root
  mov   [ebp].tail,ebp
  mov   [ebp].head,ebp
  pop   ebp
  ret



;----------------------------------------------------------------------------
;
;       insert last into send que
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   send que, (write addr of tcb)
;       EBX   tcb of thread to be entered
;
;       DS    linear space
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EDX,EDI   scratch
;
;       EBX thread entered into EBP send que
;
;----------------------------------------------------------------------------


insert_last_into_sndq macro

 or  [ebp+list_state],is_polled

 lea edi,[ebp+sndq_root]
 lea edx,[ebx+sndq_llink]
 mov eax,[edi].tail

 mov [edi].tail,edx
 mov [edx].pred,eax
 mov [eax].succ,edx
 mov [edx].succ,edi

 endm


;----------------------------------------------------------------------------
;
;       insert intr first into send que
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   send que, (write addr of tcb)
;       EDX   intr id
;
;       DS    linear space
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,ECX,EDX   scratch
;
;       intr id entered into EDX send que
;
;----------------------------------------------------------------------------


insert_intr_first_into_sndq macro

 or  [ebp+list_state],is_polled

 lea edx,[(edx*8)+intrq_llink-8*1]
 lea ecx,[ebp+sndq_root]

 mov [edx].pred,ecx
 mov eax,[ecx].head
 mov [ecx].head,edx
 mov [edx].succ,eax
 mov [eax].pred,edx

 endm


.erre   offset intrq_llink GE (offset tcb_space+tcb_space_size)



;----------------------------------------------------------------------------
;
;       get first from send que
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBX   send que, (write addr of tcb)    must not be empty !!
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       C:    EDX   deleted first thread (tcb write addr), BL undefined !
;
;       NC:   EDX   deleted first intr (intr_tab_addr)
;
;       ECX,ESI     scratch
;
;----------------------------------------------------------------------------


get_first_from_sndq macro

 lea esi,[ebx+sndq_root]
 mov edx,[esi].head
 mov ecx,[edx].succ

 mov [esi].head,ecx
 mov [ecx].pred,esi

 IFZ ecx,esi
     and [ebx+list_state],NOT is_polled
 FI

 cmp edx,offset intrq_llink

 endm




;----------------------------------------------------------------------------
;
;       test intr in send que
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       reg   send que, (write addr of tcb)    must not be empty !!
;
;       DS    linear space
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       C:    no intr waiting
;
;       NC:   intr waiting in send que (first position)
;
;----------------------------------------------------------------------------


test_intr_in_sndq macro reg

 cmp [reg+sndq_root],offset intrq_llink

 endm





;----------------------------------------------------------------------------
;
;       delete from send que
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   to be deleted, (tcb write addr), must be within a snd que!
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,ECX   scratch
;
;       EBP thread deleted from send que
;
;----------------------------------------------------------------------------


delete_from_sndq macro

 mov eax,[ebp+sndq_llink].succ
 mov ecx,[ebp+sndq_llink].pred

 mov [eax].pred,ecx
 mov [ecx].succ,eax

 IFZ eax,ecx
     and [eax+list_state-offset sndq_root],NOT is_polled
 FI

 endm



;----------------------------------------------------------------------------
;
;       join send ques
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb of source sndq (not empty)
;       EDI   tcb of dest sndq (may be empty)
;
;       DS    linear space
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,ESI   scratch
;
;       source sndq empty, old joined to des sndq
;
;----------------------------------------------------------------------------


join_sndqs macro

 and [ebp+list_state],NOT is_polled
 or  [edi+list_state],is_polled

 lea eax,[edi+sndq_root]
 mov ebx,[edi+sndq_root].tail

 lea esi,[ebp+sndq_root]
 mov ecx,[esi].head
 mov ebp,[esi].tail

 mov [esi].head,esi
 mov [esi].tail,esi

 mov [eax].tail,ebp
 mov [ebp].succ,eax
 mov [ebx].succ,ecx
 mov [ecx].pred,ebx

 endm










;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  kcode


;****************************************************************************
;*****                                                                 ******
;*****                                                                 ******
;*****              Interrupt Handling                                 ******
;*****                                                                 ******
;*****                                                                 ******
;****************************************************************************



        align 16


 irp irq,<0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15>

        align 8

intr_&irq:
 push fault
 pushad
 mov al,irq
 jmp short send_intr

  endm
  
.list

.erre offset intr_1 - offset intr_0 EQ intr1_intr0
.erre offset intr_2 - offset intr_1 EQ intr1_intr0
.erre offset intr_3 - offset intr_2 EQ intr1_intr0
.erre offset intr_4 - offset intr_3 EQ intr1_intr0
.erre offset intr_5 - offset intr_4 EQ intr1_intr0
.erre offset intr_6 - offset intr_5 EQ intr1_intr0
.erre offset intr_7 - offset intr_6 EQ intr1_intr0
.erre offset intr_8 - offset intr_7 EQ intr1_intr0
.erre offset intr_9 - offset intr_8 EQ intr1_intr0
.erre offset intr_10 - offset intr_9 EQ intr1_intr0
.erre offset intr_11 - offset intr_10 EQ intr1_intr0
.erre offset intr_12 - offset intr_11 EQ intr1_intr0
.erre offset intr_13 - offset intr_12 EQ intr1_intr0
.erre offset intr_14 - offset intr_13 EQ intr1_intr0
.erre offset intr_15 - offset intr_14 EQ intr1_intr0







        align 16
        



send_intr:


  mov   ebx,esp
  and   ebx,-sizeof tcb

  movzx edx,al
  inc   edx

  mov   ebp,[(edx*4)+intr_associated_tcb-4*1]

  test [ebp+fine_state],nwait+nclos
  IFPE
        IFZ
        CANDZ [ebp+waiting_for],edx
            cmp [ebp+waiting_for+4],0
        FI
        jnz intr_pending
  FI

  mov   [ebp+fine_state],running

  cmp   ebx,dispatcher_tcb
  jz    short intr_while_dispatching

  mark__interrupted ebx
  push  offset switch_from_intr+PM



transfer_intr:

  switch_thread int,ebx

  xor   ebx,ebp
  shr   ebp,task_no

  test  ebx,mask task_no
  IFNZ

        switch_space

  FI

  pop   eax

  sub   eax,eax
  mov   ebx,eax
  mov   esi,edx
  mov   edi,ebx

  iretd





intr_while_dispatching:

  mov   ebx,ds:[cpu_esp0]
  sub   ebx,sizeof tcb
  mov   esp,[ebx+thread_esp]
  cmp   ebp,ebx
  jnz   transfer_intr

  pop   eax

  sub   eax,eax
  mov   ebx,eax
  mov   esi,edx
  mov   edi,ebx

  iretd






intr_pending:
 
  test_intr_in_sndq ebp                        ; prevents multiple entry
  IFC                                          ; of intr into sendq
        insert_intr_first_into_sndq

        test  [ebp+fine_state],nready
        IFZ
        CANDNZ ebp,ebx
        CANDNZ ebx,dispatcher_tcb

              mark__interrupted ebx

              push  offset switch_from_intr+PM
              jmp   switch_context
        FI
  FI

; jmp   switch_from_intr



  klign 16




switch_from_intr:

.listmacroall
  ipost
.nolistmacro



;****************************************************************************
;*****                                                                 ******
;*****                                                                 ******
;*****              IPC System Calls                                   ******
;*****                                                                 ******
;*****                                                                 ******
;****************************************************************************






;----------------------------------------------------------------------------
;
;       IPC
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   snd descriptor
;       ECX   timeouts
;       EDX   snd.w0
;       EBX   snd.w1
;       EBP   rcv descriptor
;       ESI   dest.high
;       EDI   dest.low
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX   msg.dope / completion code
;       ECX   -ud-
;       EDX   msg.w1
;       EBX   msg.w0
;       EBP   -ud-
;       ESI   source.low
;       EDI   source.high
;
;----------------------------------------------------------------------------

.erre   (PM SHR 24) LE hardware_ec






ipc_dest_not_existent:

  sub   eax,eax
  mov   al,ipc_not_existent_or_illegal
  pop   ebp
  pop   ebp
  iretd






XHEAD deceit_pre

  test  al,redirected
  IFNZ
        push  eax
        push  ebp

        mov   edi,[esp+iret_esp+(4+2)*4]
        mov   esi,[edi]
        mov   edi,[edi+4]
        mov   [ebx+virtual_sender],esi
        mov   [ebx+virtual_sender+4],edi
        mov   ah,al
        call  nchief
        xor   al,ah
        cmp   al,from_inner_clan

        pop   ebp
        pop   eax
  CANDZ
        lea   edi,[ebx+virtual_sender-offset myself]
        xret
  FI

  mov   edi,ebx
  and   al,NOT deceit
  xret






  align 16




ipc_sc:

  push  eax
  push  ebx

  mov   ebx,esp
  and   ebx,-sizeof tcb

  cmp   eax,virtual_space_size-MB4
  jae   receive_only

  and   al,(NOT ipc_control_mask)+deceit+map_msg

  mov   [ebx+waiting_for],esi
  mov   [ebx+waiting_for+4],edi
  mov   [ebx+rcv_descriptor],ebp

  cmp   [ebx+chief],edi
  xc    nz,to_chief,long

  lea___tcb ebp,esi

  cmp   [ebp+myself],esi
  jnz   ipc_dest_not_existent
  cmp   [ebp+chief],edi
  jnz   ipc_dest_not_existent

  mov   edi,ebx

  test  al,deceit
  xc    nz,deceit_pre

  test  [ebp+fine_state],nwait+nclos
  IFPE
        IFZ
        mov   esi,[edi+myself]
        CANDZ [ebp+waiting_for],esi
              mov   esi,[edi+chief]
              cmp   [ebp+waiting_for+4],esi
        FI
        jnz   pending
  FI



  cmp___eax deceit
  xc    a,ipc_long,long



  mov   [ebp+fine_state],running

  mov   edi,[ebx+rcv_descriptor]

  cmp   edi,virtual_space_size
  IF____xc ae,send_only

  ELSE__
        mov   ch,closed_wait+nwake
        test  edi,nclos
        IFNZ
              mov   ch,open_wait+nwake
              test  [ebx+list_state],is_polled
              jnz   fetch_next
        FI
        and   cl,0Fh
        xc    nz,enter_wakeup_for_receive_timeout
        mov   [ebx+fine_state],ch

        mov   edi,ebx
        pop   ebx
  FI____


  switch_thread ipc,edi

  test  al,deceit+redirected
  xc    nz,deceit_or_redirected_post

  mov   esi,ebp
  shr   ebp,task_no

  xor   esi,edi
  test  esi,mask task_no

  mov   esi,[edi+myself]
  mov   edi,[edi+chief]

  IFNZ

        switch_space

  FI

  pop   ebp
  iretd







XHEAD send_only

  mov   [ebx+fine_state],running
  mov   edi,ebx
  pop   ebx
  push  offset send_ok_ret+PM

  test  [edi+list_state],is_ready
  xret  nz

  IFDEF ready_llink
        push  eax
        push  ebx
        mov   ebx,edi
        call  insert_into_ready_list
        mov   edi,ebx
        pop   ebx
        pop   eax
  ELSE
        lins  edi,esi,ready
  ENDIF
  xret



XHEAD deceit_or_redirected_post

  mov   ecx,[edi+waiting_for]
  mov   esi,[edi+waiting_for+4]
  mov   [esp],esi

  test  al,deceit
  xret  z

  add   edi,offset virtual_sender-offset myself
  xret






XHEAD enter_wakeup_for_receive_timeout

  mov   edi,ecx
  and   edi,0FF000000h
  add   cl,2
  shr   edi,cl
  shr   edi,cl
  IFNZ

        add   edi,ds:[system_clock_low]
        mov   [ebx+wakeup_low],edi

        sub   ch,nwake

        cmp   cl,5+2
        xret  le,long

        movi  edi,<offset soon_wakeup_link>
        cmp   cl,7+2
        mov   cl,is_soon_wakeup
        IFLE
              add   edi,offset late_wakeup_link-soon_wakeup_link
              mov   cl,is_late_wakeup
        FI

        test  [ebx+list_state],cl
        xret  nz,long

        linsr ebx,ecx,edi,cl
        xret  ,long
  FI


  mov   ch,running
  push  eax
  mark__ready ebx
  pop   eax
  mov   dword ptr [esp+4],offset receive_timeout_ret
  xret  ,long


  align 4



send_ok_ret:

  pop   eax
  sub   eax,eax
  iretd






  align 16


XHEAD ipc_long
 
  mov   [ebx+timeouts],ecx

  mov   [ebx+fine_state],locked_running
  mov   [ebp+fine_state],locked_waiting

  and   al,ipc_control_mask

  mov   esi,[esp+4]
  mov   edi,[ebp+rcv_descriptor]

  and   esi,NOT (deceit+map_msg)
  IFNZ
        mov   bl,al
        mov   eax,[esi+msg_dope]
        mov   al,bl
        mov   bl,0
  FI


  test  edi,map_msg
  IFZ   ,,long

  test  eax,mask md_mwords-(2 SHL md_mwords)+mask md_strings + map_msg
  xret  z,long

  add   al,ipc_cut
  and   edi,NOT (deceit+map_msg)
  CANDNZ ,,long


        sub   al,ipc_cut

        mov   [ebx+com_partner],ebp
        mov   [ebp+com_partner],ebx

        sti


        push  edx

        mov   edx,edi

        mov   ecx,ebx
        xor   ecx,ebp
        test  ecx,mask task_no
        IFNZ
              mark__ressource ebx,com_used

              and   edi,-MB4
              sub   edx,edi
              add   edx,offset com0_space
              shr   edi,20
              lea___pdir ecx,ebp
              mov   ecx,[edi+ecx]
              and   cl,NOT page_user_permit
              mov   dword ptr [ebx+waddr0],edi    ; waddr1 := 0 is ok here

              sub   edi,edi
              mov   ds:[pdir+(com0_base SHR 20)],ecx
              mov   ds:[pdir+(com0_base SHR 20)+4],edi
        FI


        mov   ecx,eax
        shr   ecx,md_mwords

        mov   edi,[edx+msg_size_dope]
        shr   edi,md_mwords

        cmp   ecx,edi
        xc    a,shorten_mwords,long
        sub   ecx,2
        IFA
              lea   esi,[esi+msg_w2]
              lea   edi,[edx+msg_w2]
              cld                              ; overrun into kernel area
              rep   movsd                      ; prohibited by firewall PF


; push  eax
;
; test  ecx,ecx
; IFNZ
;       mov   eax,[edi+16]
;       DO
;             mov   eax,[esi]
;             mov   [edi],eax
;             add   esi,4
;             add   edi,4
;             dec   cl
;             test  cl,11b
;             REPEATNZ
;       OD
;
; test  ecx,ecx
; CANDNZ
;
;       DO
;             mov   eax,[edi+16]
;
;             mov   eax,[esi]
;             mov   [edi],eax
;             mov   eax,[esi+4]
;             mov   [edi+4],eax
;             mov   eax,[esi+8]
;             mov   [edi+8],eax
;             mov   eax,[esi+12]
;             mov   [edi+12],eax
;
;             add   edi,16
;             add   esi,16
;             sub   ecx,16/4
;             REPEATA
;       OD
; FI
; pop   eax

              sub   edi,edx
              sub   esi,edi
        FI

        test  ah,mask md_strings SHR 8
        xc    nz,ipc_strings,long

        mov   edi,[edx+msg_rcv_fpage]

        cli

        unmrk_ressource ebx,com_used
 
        mov   ecx,[ebx+timeouts]
        pop   edx

  FI

  test  al,map_msg
  xret  z,long



              ;-------------------------------------------------------------
              ;
              ;           IPC MAP
              ;
              ;-------------------------------------------------------------
              ;
              ;
              ;     EAX   msg dope + cc
              ;     ECX   scratch
              ;     EDX   w0
              ;     EBX   snd tcb
              ;     EBP   rcv tcb
              ;     ESI   snd msg pointer / 0
              ;     EDI   rcv fpage
              ;
              ;--------------------------------------------------------------

  mov   ecx,ebx                       ;
  xor   ecx,ebp                       ;
  test  ecx,mask task_no              ; ignore intra-task mapping
  xret  z,long                        ;


  push  eax
  push  ebx
  push  edx

  mov   ecx,eax

  mov   eax,[esp+3*4]                 ; w1, first snd fpage
  mov   ebx,edi                       ; rcv fpage -> ebx

  DO
        push  ecx
        push  ebx
        push  esi

        mov   ch,al                   ; ch: opn

        mov   esi,-1 SHL log2_pagesize
        mov   edi,esi

        mov   cl,bl
        shr   cl,2
        sub   cl,log2_pagesize
        IFNC
              shl   edi,cl

              mov   cl,al
              shr   cl,2
              sub   cl,log2_pagesize
        CANDNC
              shl   esi,cl

              and   eax,esi

              xor   esi,edi
              and   edx,esi

              test  esi,edi
              xc    nz,shrink_snd_fpage          ; snd fpage > rcv fpage
              and   edi,ebx
              add   edi,edx

              push  ebp

              mov   edx,ebp

              push  offset fpage_opn_ret+PM
              test  ch,fpage_grant
              jz    map_fpage
              jmp   grant_fpage

              klign 16

              fpage_opn_ret:

              pop   ebp
        FI

        pop   esi
        pop   ebx
        pop   ecx

        EXITC

        sub   ecx,2 SHL md_mwords
        IFBE
              pop   edx
              pop   ebx
              pop   eax

              mov   ecx,[ebx+timeouts]

              xret  ,long
        FI

        add   esi,sizeof fpage_vector

        mov   edx,[esi+msg_w2].snd_base
        mov   eax,[esi+msg_w2].snd_fpage

        REPEAT
  OD

  pop   edx
  pop   ebx
  pop   eax

  mov   al,ipc_cut

  mov   ecx,[ebx+timeouts]
  xret  ,long





XHEAD shrink_snd_fpage

  add   eax,edx
  sub   edx,edx
  mov   cl,bl
  xret
 


XHEAD shorten_mwords

  mov   ecx,edi
  shl   eax,width md_mwords
  shrd  eax,ecx,width md_mwords
  or    al,ipc_cut
  xret  ,long






              align 16



XHEAD ipc_strings

  mov   ch,ah
  and   ch,mask md_strings SHR 8
  mov   cl,[edx+msg_size_dope].msg_strings
  and   cl,mask md_strings SHR 8
  IFA   ch,cl
        mov   ch,cl
        or    al,ipc_cut
        and   ah,NOT (mask md_strings SHR 8)
        add   ah,cl
        test  cl,cl
        xret  z,long
  FI

  push  edx

  mov   edi,[edx+msg_size_dope]
  shr   edi,md_mwords
  lea   edi,[(edi*4)+edx+msg_w2]

  mov   edx,[esi+msg_size_dope]
  shr   edx,md_mwords
  lea   esi,[(edx*4)+esi+msg_w2]

  DO
        push  ecx
        push  esi
        push  edi

        mov   ecx,[esi+str_len]
        cmp   ecx,[edi+buf_size]
        IFA
              mov   ecx,[edi+buf_size]
              or    al,ipc_cut
        FI
        mov   esi,[esi+str_addr]
        mov   edi,[edi+buf_addr]

        mov   edx,ebx
        xor   edx,ebp
        test  edx,mask task_no
        IFNZ
              mov   edx,edi
              and   edx,-MB4
              sub   edi,edx
              add   edi,com0_base
              shr   edx,20
              cmp   [ebx+waddr0],dx
              xc    nz,string_to_com1_space
        FI

        mov   dl,cl
        and   dl,4-1
        shr   ecx,2
        cld                                    ; overrun into kernel_area
        rep   movsd                            ; prohibited by firewall PF
        mov   cl,dl                            ;
        rep   movsb


        pop   edi
        pop   esi
        pop   ecx

        add   esi,sizeof string_vector
        add   edi,sizeof string_vector
        dec   ch
        REPEATNZ
  OD

  pop   edx
  sub   edi,edx
  sub   esi,edi
  xret  ,long



XHEAD string_to_com1_space

  push  ecx

  mark__ressource ebx,com_used

  mov   [ebx+waddr1],dx
  add   edi,com1_base-com0_base
  lea___pdir ecx,ebp
  mov   ecx,[ecx+edx]
  mov   edx,ecx
  and   dl,NOT page_user_permit
  xchg  ds:[pdir+(com1_base SHR 20)],edx
  cmp   edx,ecx
  mov   dword ptr ds:[pdir+(com1_base SHR 20)+4],0

  pop   ecx
  xret  z
  test  edx,edx
  xret  z

  mov   edx,cr3
  mov   cr3,edx
  xret







        align 16



fetch_next:

  mov   esi,[ebx+myself]
  mov   edi,[ebx+chief]
  test  al,deceit
  IFNZ
        mov   esi,[ebx+virtual_sender]
        mov   edi,[ebx+virtual_sender+4]
  FI
  mov   ecx,esp
  mov   esp,[ebp+thread_esp]

  push  eax                     ; eax         ;
  mov   eax,[ebx+waiting_for]                 ;
  push  eax                     ; ecx         ;
  push  edx                     ; edx         ;
  mov   eax,[ecx]                             ;
  push  eax                     ; ebx         ; pushad
  push  eax                     ; temp (esp)  ;
  mov   eax,[ebx+waiting_for+4]               ;
  push  eax                     ; ebp         ;
  push  esi                     ; esi         ;
  push  edi                     ; edi         ;
  push  offset received_ok_ret+PM

  mark__ready ebp

  mov   [ebp+thread_esp],esp
  lea   esp,[ecx+4]


  get_first_from_sndq

  IFC
        mov   dl,0
        mov   ebp,edx

        mov   [edx+fine_state],locked_running
        mov   [ebx+fine_state],locked_waiting

        jmp   switch_context

  FI

  sub   edx,offset intrq_llink-1*8
  shr   edx,3
  sub   ebx,ebx
  mov   esi,edx
  mov   edi,ebx

  pop   eax
  sub   eax,eax
  iretd







nil_dest_not_existent:

  sub   eax,eax
  mov   al,ipc_not_existent_or_illegal
  pop   ebx
  pop   ebp
  iretd




  align 16


XHEAD to_chief

  test  esi,esi
  jz    nil_dest_not_existent

  mov   ebp,[ebx+chief]
  xor   ebp,edi
  test  ebp,mask site_no
  IFZ
        xor   ebp,edi
        xor   ebp,esi
        test  ebp,mask task_no
        xret  z,long

        mov   ebp,[ebx+myself]
        xor   ebp,edi
        test  ebp,mask task_no
        xret  z,long

  mov   ebp,edi
  or    ebp,NOT mask depth
  sub   ebp,[ebx+chief]
  CANDNC
  shr   ebp,32 - width depth
  CANDA ebp,1
        dec   ebp
        DO
              lea___tcb edi,edi
              mov   esi,[edi+myself]
              mov   edi,[edi+chief]
              dec   ebp
              REPEATNZ
        OD
        mov   ebp,[ebx+myself]
        xor   ebp,edi
        or    al,redirected
        test  ebp,mask thread_no
        xret  z,long
  FI

  or    al,redirected+from_inner_clan
  mov   ebp,[ebx+chief]
  lea___tcb ebp,ebp
  mov   esi,[ebp+myself]
  mov   edi,[ebp+chief]

  xret  ,long






  align 16


pending:
 
  pop   edi
                                ; eax          ;  already pushed
  cmp   ebx,ebp
  jz    short sw_err3
 
  push  ecx                     ; ecx          ;
  push  edx                     ; edx          ;
  push  edi                     ; ebx          ;
  push  edi                     ; temp (esp)   ;
  mov   edi,[ebx+rcv_descriptor];              ;  pushad
  push  edi                     ; ebp          ;
  mov   esi,[ebx+waiting_for]   ;              ;
  mov   edi,[ebx+waiting_for+4] ;              ;
  push  esi                     ; esi          ;
  push  edi                     ; edi          ;


  test  cl,0F0h
  IFNZ
        test  ecx,000FF0000h
        jz    short send_timeout_ret
  FI

  mov   [ebx+com_partner],ebp

  insert_last_into_sndq

  shl   ecx,8
  mov   cl,ch
  shr   cl,4

  push  offset ret_from_poll+PM
  mov   ch,polling+nwake

  jmp   wait_for_ipc_or_timeout




sw_err3:
  sub   eax,eax
  mov   al,ipc_not_existent_or_illegal
  pop   ebp
  iretd










ret_from_poll:

  mov   ebx,esp
  and   ebx,-sizeof tcb
  mov   ebp,[ebx+com_partner]

  IFZ   [ebx+fine_state],locked_running

        mov   [ebx+fine_state],running
        mov   [ebp+fine_state],closed_wait
        mov   eax,[ebx+myself]
        mov   [ebp+waiting_for],eax
        mov   eax,[ebx+chief]
        mov   [ebp+waiting_for+4],eax

        popad
        jmp   ipc_sc

  FI

  test  [ebx+fine_state],npoll
  IFZ
        mov   ebp,ebx
        delete_from_sndq
        mov   [ebx+fine_state],running
  FI


send_timeout_ret:

  popad
  sub   eax,eax
  mov   al,ipc_timeout+ipc_s
  iretd






;----------------------------------------------------------------------------



w_err:
  pop   eax
  sub   eax,eax
  mov   al,ipc_not_existent_or_illegal
  iretd




;----------------------------------------------------------------------------
;
;       RECEIVE
;
;----------------------------------------------------------------------------


  align 16





receive_only:

  pop   eax

  cmp   ebp,virtual_space_size
  jae   w_err

  mov   [ebx+timeouts],ecx
  mov   [ebx+rcv_descriptor],ebp

  test  ebp,nclos
  jz    receive_from

  test  [ebx+list_state],is_polled
  IFNZ

        get_first_from_sndq

        IFNC
              pop   eax

              sub   edx,offset intrq_llink-1*8
              shr   edx,3
              sub   ebx,ebx
              mov   esi,edx
              mov   edi,ebx
              sub   eax,eax

              iretd
        FI

        mov   dl,0
        mark__ready edx
        mov   [ebx+fine_state],locked_waiting
        mov   [ebx+com_partner],edx
        mov   [edx+fine_state],locked_running
        mov   [edx+com_partner],ebx
        mov   ebp,edx
        jmp   switch_context
  FI


  mov   ch,open_wait+nwake



wait_for_receive_or_timeout:

  mov   ebp,ebx



wait_for_receive_from_or_timeout:

  mov   dword ptr [esp],offset receive_timeout_ret




wait_for_ipc_or_timeout:

  and   cl,0Fh
  IFNZ

        mov   edi,ecx
        and   edi,0FF000000h
        IFZ
              ret
        FI
        sub   ch,nwake
        add   cl,2
        shr   edi,cl
        shr   edi,cl
        add   edi,ds:[system_clock_low]
        mov   [ebx+wakeup_low],edi
        cmp   cl,5+2
        IFG
              movi  edi,<offset soon_wakeup_link>
              cmp   cl,7+2
              mov   cl,is_soon_wakeup
              IFLE
                    add   edi,offset late_wakeup_link-soon_wakeup_link
                    mov   cl,is_late_wakeup
              FI
        test  [ebx+list_state],cl
        CANDZ
              linsr ebx,eax,edi,cl
        FI
  FI

  mov   al,[ebp+timeslice]
  mov   [ebp+rem_timeslice],al

  mov   [ebx+fine_state],ch

  test  [ebp+fine_state],nready
  jz    switch_context
  jmp   dispatch





receive_timeout_ret:

  mov   ebx,esp
  and   ebx,-sizeof tcb

  mov   [ebx+fine_state],running

  sub   eax,eax
  mov   al,ipc_timeout

  iretd



  align 16


received_ok_ret:

  popad
  add   esp,4
  iretd






;----------------------------------------------------------------------------
;
;       RECEIVE FROM
;
;----------------------------------------------------------------------------


  align 16


receive_from:

  mov   [ebx+waiting_for],esi
  mov   [ebx+waiting_for+4],edi



  IFB_  esi,intr_sources+1

        test_intr_in_sndq ebx

        IFC
              mov   ch,closed_wait+nwake
              mov   edi,ecx
              and   edi,0FF00000Fh
              IFNZ
                    cmp   edi,15
              FI
              jae   wait_for_receive_or_timeout

              call  detach_intr
              mov   ecx,esi
              dec   ecx
              IFNS
              CANDZ [(ecx*4)+intr_associated_tcb],0
                    call  attach_intr
                    pop   eax
                    jmp   receive_timeout_ret
              FI
              jmp   w_err
        FI


        get_first_from_sndq

        pop   eax

        sub   edx,offset intrq_llink-1*8
        shr   edx,3
        sub   ebx,ebx
        mov   esi,edx
        mov   edi,ebx
        sub   eax,eax

        iretd
  FI


  IFNZ  [ebx+chief],edi
        call  nchief
  FI

  lea___tcb ebp,esi

  cmp   [ebp+myself],esi
  jnz   short r_source_not_existent
  cmp   [ebp+chief],edi
  jnz   short r_source_not_existent

  test  [ebp+fine_state],npoll
  IFZ
  CANDZ [ebp+com_partner],ebx

        delete_from_sndq

        mov   [ebp+fine_state],locked_running
  ;;;;; mark__ready ebp
        mov   [ebx+fine_state],locked_waiting
        mov   [ebx+com_partner],ebp

        jmp   switch_context
  FI


  mov   ch,closed_wait+nwake
  jmp   wait_for_receive_from_or_timeout




 
r_source_not_existent:
  sub   eax,eax
  mov   al,ipc_not_existent_or_illegal
  pop   ebx
  pop   ebp
  iretd




;----------------------------------------------------------------------------
;
;       nchief
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   thread (low)   / 0
;       EDI   thread (high)  / undef
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
; ESI=0 on input:
;
;       ESI   myself (low)
;       EDI   myself (high)
;
;
; ESI>0 on input:
;                        outside clan                       within clan
;
;       AL    redirected / redirected+from_inner_clan             0
;       ESI              chief (low)                        thread (low)
;       EDI              chief (high)                       thread (high)
;
;       ECX,EDX     scratch
;
;----------------------------------------------------------------------------



        align 16




id_nearest_sc:

  mov   ebx,esp
  and   ebx,-sizeof tcb

  sub   eax,eax

  test  esi,esi
  IFZ
        mov   esi,[ebx+myself]
        mov   edi,[ebx+chief]

        iretd
  FI

  sub   eax,eax
  call  nchief

  iretd



  align 16


nchief:
 
  mov   al,0

  DO
        mov   ebp,[ebx+chief]
        xor   ebp,edi
        test  ebp,mask site_no
        IFZ

              xor   ebp,edi
              xor   ebp,esi
              test  ebp,mask task_no
              EXITZ
              mov   ebp,[ebx+myself]
              xor   ebp,edi
              test  ebp,mask task_no
              EXITZ

              mov   ebp,edi
              sub   ebp,[ebx+chief]
              IFNC
              shr   ebp,32 - width depth
              CANDA ebp,1
                    dec   ebp
                    DO
                          lea___tcb edi,edi
                          mov   esi,[edi+myself]
                          mov   edi,[edi+chief]
                          dec   ebp
                          REPEATNZ
                    OD
                    mov   ebp,[ebx+myself]
                    xor   ebp,edi
                    test  ebp,mask thread_no
                    IFZ
                          mov   al,redirected+from_inner_clan
                          ret
                    FI
              FI
        FI

        mov   esi,[ebx+chief]
        lea___tcb ebp,esi
        mov   esi,[ebp+myself]
        mov   edi,[ebp+chief]
        mov   al,redirected
  OD

  ret







  kcod ends
;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||




;----------------------------------------------------------------------------
;
;       push / pop complete ipc state
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb addr
;
;       interrupts disabled !
;
;----------------------------------------------------------------------------
; push PRECONDITION:
;
;       <EBP+fine_state>  is 'locked_running' or 'locked_waiting'
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       pushed / popped
;
;       EAX,EDI  scratch
;
;----------------------------------------------------------------------------
; push POSTCONDITION:
;
;       NZ:   ECX   timeouts for page fault RPC
;
;        Z:         PF timeout is 0, ECX scratch
;
;
;       <EBP+fine_state>  is 'running'
;
;----------------------------------------------------------------------------

        align 16



push_ipc_state:

  pop   edi

  mov   eax,[ebp+com_partner]
  push  eax
  mov   eax,[ebp+waiting_for]
  push  eax
  mov   eax,[ebp+waiting_for+4]
  push  eax
  mov   eax,[ebp+rcv_descriptor]
  push  eax
  mov   eax,[ebp+timeouts]
  shl   eax,8
  mov   ah,[ebp+fine_state]
  mov   [ebp+fine_state],running
  mov   al,[ebp+state_sp]
  push  eax

  mov   eax,esp
  shr   eax,2
  mov   [ebp+state_sp],al

  mov   ecx,[ebp+com_partner]
  test  [ebp+fine_state],nrcv
  mov   ecx,[ecx+timeouts]
  IFNZ
        rol   ch,4
  FI
  mov   cl,ch
  and   cl,0F0h
  shr   ch,4
  or    cl,ch
  mov   ch,cl
  rol   ecx,16
  mov   cl,1
  mov   ch,1
  ror   ecx,16
  cmp   cl,15*16+15

  jmp   edi






pop_ipc_state:

  pop   edi

  pop   eax
  mov   [ebp+state_sp],al
  mov   [ebp+fine_state],ah
  shr   eax,8
  mov   byte ptr [ebp+timeouts+1],ah

  pop   eax
  mov   [ebp+rcv_descriptor],eax
  pop   eax
  mov   [ebp+waiting_for+4],eax
  pop   eax
  mov   [ebp+waiting_for],eax
  pop   eax
  mov   [ebp+com_partner],eax

  test  [eax+fine_state],nlock
  IFZ
  CANDZ [eax+com_partner],ebp

        jmp   edi
  FI

  ke    '-pi_err'




;----------------------------------------------------------------------------
;
;       get bottom state
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb write addr
;
;       interrupts disabled !
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX   fine state (bottom)
;       EBX   com partner (bottom) iff state is 'locked'
;
;----------------------------------------------------------------------------



get_bottom_state:
 
  movzx eax,[ebp+state_sp]
  test  eax,eax
  IFZ 
        mov   eax,[ebp+sizeof tcb-sizeof int_pm_stack].ip_error_code 
        IFAE  eax,min_icode
        CANDBE eax,max_icode
 
              mov   eax,running
              ret 
        FI
 
        movzx eax,[ebp+fine_state]
        ret
  FI
 
  DO
        lea   ebx,[(eax*4)+ebp]
        mov   al,[ebx]
        test  al,al
        REPEATNZ
  OD
  mov   al,[ebx+1]
  mov   ebx,[ebx+4*4]
  ret
 
 
 
 
 
;----------------------------------------------------------------------------
;
;       cancel if within ipc   /    cancel ipc 
;
;----------------------------------------------------------------------------
; cancel if within ipc PRECONDITION:
;
;       EBP   tcb write addr
;
;       interrupts disabled !
;
;
;----------------------------------------------------------------------------
; cancel ipc PRECONDITION:
;
;       AL    bottom state of thread 
;       EBP   tcb write addr
;
;       interrupts disabled !
;
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       REGs  scratch
;
;       base waiting   :   ipc cancelled
;       base pending   :   ipc cancelled
;       base locked    :   ipc aborted, also of partner
;
;       ELSE           :   status unchanged
;
;----------------------------------------------------------------------------



cancel_if_within_ipc:

  call  get_bottom_state


cancel_ipc:

  test  al,nlock
  IFNZ
        test  al,nready
        IFNZ
              mov   al,ipc_cancelled
              call  reset_ipc
        FI
        ret
  FI

  mov   al,ipc_aborted
  call  reset_ipc
  mov   ebp,ebx
  mov   al,ipc_aborted
  call  reset_ipc

  ret





reset_ipc:

  pop   ecx

  lea   esi,[ebp+sizeof tcb-sizeof iret_vec-2*4]
  test  [ebp+fine_state],nrcv
  IFNZ
        add   al,ipc_s
  FI
  movzx eax,al
  mov   [esi+4],eax
  mov   dword ptr [esi],offset reset_ipc_ret

  mov   [ebp+fine_state],running
  mov   ebx,ebp
  mark__ready ebx

  mov   [ebp+thread_esp],esi
  xor   esi,esp
  test  esi,mask thread_no
  IFZ
        xor   esp,esi
  FI

  jmp   ecx





reset_ipc_ret:

  pop   eax
  iretd




;----------------------------------------------------------------------------
;
;       ipcman wakeup tcb
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb write addr
;
;       DS    linear space
;
;       interrupts disabled !
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       if locked no change, else
;       state of thread set to 'running', deleted from sendq if necessary
;
;----------------------------------------------------------------------------




ipcman_wakeup_tcb:
 
  test  [ebp+fine_state],nlock
  IFNZ
        test  [ebp+fine_state],npoll
        IFZ
              push  eax
              push  ecx

              delete_from_sndq

              pop   ecx
              pop   eax
        FI
        mov   [ebp+fine_state],running
        push  eax
        push  edi
        mark__ready  ebp
        pop   edi
        pop   eax
  FI

  ret



;----------------------------------------------------------------------------
;
;       ipcman open tcb
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EDP   tcb write addr, must be mapped !!
;
;       DS    linear space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EDP   reentered into snd que if necessary
;
;----------------------------------------------------------------------------




ipcman_open_tcb:
 
  pushfd
  cli

  test  [ebp+fine_state],npoll
  IFZ
        call  enforce_restart_poll
  FI

  popfd
  ret


;----------------------------------------------------------------------------
;
;       ipcman close tcb
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   to be deleted, (tcb write addr)
;             must be mapped !!
;
;       DS    linear space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EBP thread deleted from send que if contained
;
;----------------------------------------------------------------------------



ipcman_close_tcb:
 
  pushad
  pushfd

  cli

  mov   al,[ebp+fine_state]

  test  al,npoll
  IFZ
        delete_from_sndq
  ;;;;; lno___thread ebx,eBp
  ;;;;; call  signal_scheduler_reactivation
  FI

  mov   eax,[ebp+sndq_root].head
  and   eax,-sizeof tcb
  IFNZ  eax,ebp

;;;;    mov   edi,scheduler_tcb
;;;;    join_sndqs
  FI

  popfd
  popad
  ret




;----------------------------------------------------------------------------
;
;       restart poll all senders          (special routine for schedule)
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBX   tcb address
;
;----------------------------------------------------------------------------


restart_poll_all_senders:
 
  ke '-n'

; pushad
; pushfd
;
; DO
;       cli
;       test  [ebx+list_state],is_polled
;       EXITZ
;
;       get_first_from_sndq
;       IFNC
;             ke    'flushed_intr'
;       FI
;       mov   dl,0
;
;       test  [edx+fine_state],npoll
;       IFZ
;             mov   ebp,edx
;             call  enforce_restart_poll
;       FI
;
;       sti
;       REPEAT
; OD
;
; popfd
; popad
; ret



;----------------------------------------------------------------------------
;
;       enforce restart poll
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb address, mapped
;
;       tcb not open AND fine state = polling
;
;----------------------------------------------------------------------------


enforce_restart_poll:
 
  pushad
			
  lea___esp eax,ebp
;;mov   dword ptr [eax],offset restart_poll

  mov   ebx,ebp
  mark__ready ebx

  mov   al,running
  xchg  [ebp+fine_state],al

  test  al,nwake
  IFZ
        mov   esi,[ebp+wakeup_low]
        movzx edi,[ebp+wakeup_high]
        pushfd
        cli
        mov   eax,ds:[system_clock_low]
        movzx ebx,ds:[system_clock_high]
        popfd                                  ; Rem: change of NT impossible

        sub   esi,eax
        sbb   edi,ebx
        IFC                                    ;Ž92-12-08
              sub   esi,esi
        FI
        mov   [ebp+timeouts],esi            ;..Ž
  FI

  popad
  ret




;----------------------------------------------------------------------------
;
;       attach interrupt
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBX   tcb write addr
;       ECX   intr no  (0...intr_sources-1)
;
;----------------------------------------------------------------------------


attach_intr:
 

  mov   [(ecx*4)+intr_associated_tcb],ebx

  call  mask_hw_interrupt

  push  eax
  push  ebx
  lea   eax,[(ecx*intr1_intr0)+intr_0+PM]
  lea   ebx,[ecx+irq0_intr]
  mov   bh,0 SHL 5
  call  define_idt_gate
  pop   ebx
  pop   eax

  ret



;----------------------------------------------------------------------------
;
;       detach interrupt
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBX   tcb write addr
;
;----------------------------------------------------------------------------


detach_intr:
 
  push  ecx

  sub   ecx,ecx
  DO
        IFZ   [ecx+intr_associated_tcb],ebx
              mov   [ecx+intr_associated_tcb],0
              shr   ecx,2
              call  mask_hw_interrupt
              EXIT
        FI
        add   ecx,4
        cmp   ecx,sizeof intr_associated_tcb
        REPEATB
  OD

  pop   ecx
  ret



;----------------------------------------------------------------------------
;
;       ipcman rerun tcb
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ECX   rerun esp real
;       EDX   tcb addr virtual (not mapped !)
;       EBP   tcb addr real
;
;       DS,ES linear space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       ECX   rerun esp real (may have changed !)
;       EBP   tcb addr real (may have changed !)
;
;       tcb restarted as far as ipcman is concerned
;
;----------------------------------------------------------------------------
; Algorithm:
;
;       IF special kernel ipc active
;         THEN pop original tcb status
;       FI ;
;       IF locked running {message transfer running}
;         THEN restart transfer long message   {ebp,edx are ok on stack !! }
;       ELIF locked waiting OR waiting
;         THEN restart waiting
;       ELIF locked waiting for non persisting {sender disappeared}
;         THEN restart receive timeout
;       FI
;
;----------------------------------------------------------------------------

        align 4


ipcman_rerun_thread:

  pushad

  mov   al,[ebp+fine_state]
  and   al,NOT nwake

  CORZ  al,<locked_waiting AND NOT nwake>
  IFZ   al,<closed_wait AND NOT nwake>
        mov   dword ptr [ecx],offset receive_timeout_ret
        IFAE  [ebp+waiting_for],intr_sources
              mov   [ebp+fine_state],running
        FI

  ELIFZ al,<open_wait AND NOT nwake>
        mov   dword ptr [ecx],offset receive_timeout_ret

  ELIFZ al,<locked_running AND NOT nwake>
  ELIFZ al,<polling AND NOT nwake>
        sub   ecx,4
        mov   dword ptr [ecx],offset ret_from_poll

  ELIFZ al,<running AND NOT nwake>
        mov   dword ptr [ecx],offset send_ok_ret

  ELIFZ al,<aborted AND NOT nwake>                   ;Ž92-12-21..
  test  [ebp+coarse_state],ndead
  CANDNZ
        mov   dword ptr [ecx],offset send_ok_ret

  ELSE_                                              ;..Ž
        ke    'ill_mess_rerun'
  FI

  mov   [esp+6*4],ecx
  mov   [esp+2*4],ebp
  popad
  ret






;----------------------------------------------------------------------------
;
;       init ipcman
;
;----------------------------------------------------------------------------


  icode



init_ipcman:

  mov   bh,3 SHL 5

  mov   bl,ipc
  mov   eax,offset ipc_sc+PM
  call  define_idt_gate

  mov   bl,id_nearest
  mov   eax,offset id_nearest_sc+PM
  call  define_idt_gate

  ret


  icod  ends




  code ends
  end
