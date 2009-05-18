include l4pre.inc 
                                                        		   
   	       														 	  
  Copyright IBM+UKA, L4.IPCMAN, 18,04,00, 9580, K 
 
 				    
;*********************************************************************
;******                                                         ******
;******         IPC Manager                                     ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 18.04.00    ******
;******                                                         ******
;*********************************************************************
 

  public init_ipcman
  public init_sndq
  public init_intr_control_block
  public ipcman_open_tcb															  
  public ipcman_close_tcb
  public ipcman_wakeup_tcb
; public ipcman_rerun_thread
  public restart_poll_all_senders
  public detach_intr
  public push_ipc_state
  public pop_ipc_state
  public cancel_if_within_ipc
  public get_bottom_state
  public ipc_update_small_space_size
  public ipc_critical_region_begin
  public ipc_critical_region_end
  public ipcman_sysexit_to_user_instruction
  public ipc_exit

  public kernel_ipc
  public id_nearest_sc



  extrn deallocate_ressources_ipc:near
  extrn deallocate_ressources_int:near
  extrn switch_context:near
  extrn dispatch:near
  extrn insert_into_ready_list:near
  extrn define_idt_gate:near
  extrn mask_hw_interrupt:near
  extrn map_fpage:near
  extrn grant_fpage:near
  extrn translate_address:near
  extrn irq0_intr:abs
  extrn irq15:abs
  extrn rdtsc_clocks:dword
  


.nolist
include l4const.inc
include uid.inc
include adrspace.inc
.list
include tcb.inc
.nolist
include schedcb.inc
include cpucb.inc
include intrifc.inc
include pagconst.inc
include pagmac.inc
.list
include msg.inc
.nolist
include small-as.inc
include syscalls.inc
include apic.inc
.list


ok_for pIII



  assume ds:codseg


ipc_kernel_stack struc

  ipc_edx           dd    0
  ipc_ebx           dd    0
  ipc_eax           dd    0
  ipc_esp           dd    0

ipc_kernel_stack ends


ipc_user_stack struc

  ipc_user_eip            dd    0
  ipc_user_cs             dd    0
  ipc_user_rcv_descriptor dd    0
  ipc_user_timeouts       dd    0

ipc_user_stack ends




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

  push  ds
  push  es
  push  linear_kernel_space
  pop   ds

  mov   ebx,esp
  and   ebx,-sizeof tcb

  movzx edx,al
  inc   edx

  mov   ebp,[(edx*4)+intr_associated_tcb-4*1]

  mov   al,[ebp+fine_state]

  and   al,nwait+nclos
  IFLE                                      ; Greater : nwait=SF=OV=0 and ZF=0
        IFZ  
              cmp [ebp+waiting_for],edx
        FI      
        jnz intr_pending
  FI

  mov   [ebp+fine_state],running

  cmp   ebx,dispatcher_tcb
  jz    intr_while_dispatching

  mark__interrupted ebx
  push  offset switch_from_intr+KR



transfer_intr:

  switch_thread int,ebx

  mov   [ebp+rem_timeslice],100 ;;;;;;;;;;;;;;;;;; --------------

  switch_space

  sub   eax,eax
  mov   ebx,eax
  mov   esi,edx
  mov   edi,ebx

  jmp   ipc_exit





intr_while_dispatching:

  mov   ebx,ds:[cpu_esp0]
  sub   ebx,sizeof tcb
  mov   esp,[ebx+thread_esp]
  cmp   ebp,ebx
  jnz   transfer_intr


  sub   eax,eax
  mov   ebx,eax
  mov   esi,edx
  mov   edi,ebx

  jmp   ipc_exit






intr_pending:
 
  test_intr_in_sndq ebp                        ; prevents multiple entry
  IFC                                          ; of intr into sendq
        insert_intr_first_into_sndq

        test  [ebp+fine_state],nready
        IFZ
        CANDNZ ebp,ebx
        CANDNZ ebx,dispatcher_tcb

              mark__interrupted ebx

              push  offset switch_from_intr+KR
              jmp   switch_context
        FI
  FI

  jmp   switch_from_intr



  align 16




switch_from_intr:

  ipost




;----------------------------------------------------------------------------
;
;       special P2 intr handling
;
;----------------------------------------------------------------------------

  IF    kernel_x2
  


        align 16


 irp irq,<0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15>

        align 8

intr_&irq&_P2:
 push fault
 pushad
 mov al,irq
 jmp short send_intr_P2

  endm
  
  
.list  



send_intr_P2:
  
  mov   ss:[local_apic+apic_eoi],0
  jmp   send_intr
  
  
  
  ENDIF
  


;****************************************************************************
;*****                                                                 ******
;*****                                                                 ******
;*****              IPC System Calls                                   ******
;*****                                                                 ******
;*****                                                                 ******
;****************************************************************************


        align 16



;----------------------------------------------------------------------------
;
;       INT 30h     IPC
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   snd descriptor
;       ECX   timeouts
;       EDX   snd.w0
;       EBX   snd.w1
;       EBP   rcv descriptor
;       ESI   dest
;       EDI   msg.w2
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX   msg.dope / completion code
;       ECX   -ud-
;       EDX   msg.w1
;       EBX   msg.w0
;       EBP   -ud-
;       ESI   source
;       EDI   msg.w2
;
;----------------------------------------------------------------------------

.erre   (PM SHR 24) LE hardware_ec



  kcod  ends


ipc_sc:

  push  linear_kernel_space
  pop   ds
  push  linear_kernel_space
  pop   es

  call  kernel_ipc
        
  push  linear_space
  pop   ds
  push  linear_space
  pop   es

  iretd




;  push  ebx
;
;  mov   ebx,ss:[esp+iret_esp+4]
;  sub   ebx,sizeof ipc_user_stack
;
;  mov   ds:[ebx+ipc_user_timeouts],ecx
;  mov   ds:[ebx+ipc_user_rcv_descriptor],ebp
;  mov   ds:[ebx+ipc_user_cs],linear_space_exec
;  mov   ecx,ss:[esp+iret_eip+4]
;  mov   ds:[ebx+ipc_user_eip],ecx
;
;  mov   ecx,ebx
;  pop   ebx
;
;  mov   esp,offset cpu_esp0
;
;  jmp   ipc_sysenter
 




  kcode


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
;       ESI   dest
;       EDI   msg.w2
;
;       <ESP>       EIP
;       <ESP+4>     CS
;       <ESP+8>     rcv descriptor
;       <ESP+12     timeouts
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX   msg.dope / completion code
;       ECX   -ud-
;       EDX   msg.w1
;       EBX   msg.w0
;       EBP   -ud-
;       ESI   source
;       EDI   msg.w2
;
;----------------------------------------------------------------------------



ipc_critical_region_begin:                     ; PF in critical region push ipc state
                                               ; even if fine_state is still 'running'



ipc_sysenter:          

  mov   esp,dword ptr ss:[esp]

  push  ecx


  mov   ebp,esp
  and   ebp,-sizeof tcb


  push  eax  
  push  ebx
  push  edx

  mov   ebx,ds:[ecx+ipc_user_rcv_descriptor]
  mov   ecx,ds:[ecx+ipc_user_timeouts]

  mov   [ebp+waiting_for],esi
  mov   [ebp+mword2],edi
  mov   [ebp+timeouts],ecx
  mov   [ebp+rcv_descriptor],ebx

  mov   ebx,ebp

  mov   edx,linear_kernel_space
  mov   ds,edx


ipc_restart:

  cmp   eax,virtual_space_size-MB4
  jae   receive_only
  
        and   eax,(NOT ipc_control_mask)+deceit+map_msg ;REDIR begin ----------------------------
                                                        ;
        and   ebp,mask task_no                          ;
        mov   edx,ebx                                   ;
                                                        ;
        shr   ebp,task_no-2                             ;
        and   edx,mask task_no                          ;
                                                        ;
        shr   edx,task_no-log2_tasks-2                  ;
                                                        ;
        mov   edx,[edx+ebp+redirection_table]           ;
                                                        ;
        cmp   edx,ipc_transparent                       ;
        xc    nz,redirect_or_lock_ipc,long              ;
                                                        ;REDIR ends ----------------------------
  mov   ebp,esi   


  and   ebp,mask thread_no  
  
  add   ebp,offset tcb_space
  
  mov   edi,[ebx+myself]
                                ;REDIR:   and   al,(NOT ipc_control_mask)+deceit+map_msg  
  
  mov	edx,[ebp+myself]
  xor	edi,esi

  test  edi,mask chief_no
  xc    nz,to_chief,long

  cmp   esi,edx
  jnz   ipc_dest_not_existent
  
  test  eax,deceit
  xc    nz,propagate_pre,long

  mov   dl,[ebp+fine_state]
  
  
  and   dl,nwait+nclos
  IFLE                                      ; Greater : nwait=SF=OV=0 and ZF=0
        IFZ   
              mov   esi,[ebp+waiting_for]
              mov   edi,[ebx+myself]   
              cmp   edi,esi      
        FI      
        xc    nz,pending_or_auto_propagating,long
  FI

 
  test  eax,0FFFFFFF2h
  xc    nz,ipc_long,long

 mov dl,al
 and dl,0F0h
 IFZ dl,0E0h
   ke 'cut'
 FI


  mov   edx,[ebx+rcv_descriptor]
  mov   ch,running
  
  mov   [ebp+fine_state],ch
  cmp   edx,virtual_space_size
  
  IF____xc ae,send_only,long

  ELSE__

        and   edx,nclos   
        mov   edi,dword ptr ds:[ebx+fine_state]
        shr   edi,zpolled+16
        test  edi,edx

        jnz   fetch_next
                
        and   cl,0Fh
        xc    nz,enter_wakeup_for_receive_timeout,long
        
        add   dl,closed_wait+nwake
        mov   edi,ebx
        
        mov   [ebx+fine_state],dl

        pop   edx
        pop   ebx  
  FI____
  
  
;;;;; switch_thread ipc,edi
  mov   cl,[edi+ressources]
  
  test  cl,cl
  jnz   deallocate_ressources_ipc

  public switch_thread_ipc_ret
  switch_thread_ipc_ret:

  lea   esi,[ebp+sizeof tcb]
  mov   [edi+thread_esp],esp
  
  mov   ds:[cpu_esp0],esi
  mov   esp,[ebp+thread_esp]
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  mov   esi,[edi+myself]
  
  test  eax,deceit+redirected
  xc    nz,propagate_or_redirected_post,long

  mov   edi,[edi+mword2]  
  

;;;;;;;;;;;;  switch_space

  mov   ecx,[ebp+thread_segment]
  mov   ds:[gdt+linear_space/8*8],ecx
  mov   ds:[gdt+linear_space_exec/8*8],ecx

  mov   ecx,[ebp+thread_segment+4]
  mov   ds:[gdt+linear_space/8*8+4],ecx
  add   ecx,0000FB00h-0000F300h
  mov   ds:[gdt+linear_space_exec/8*8+4],ecx

  IFNS
        mov   ecx,[ebp+thread_proot]

        IFNZ  dword ptr ds:[cpu_cr3],ecx
           
              mov   dword ptr ds:[cpu_cr3],ecx
              mov   dword ptr ds:[tlb_invalidated],ecx
              mov   cr3,ecx
        FI
  FI



ipc_exit:

  mov   ecx,ss:[esp+ipc_esp-2*4]
  mov   ebp,edx

  cmp   ecx,esp
  jae   kernel_ipc_exit

  mov   edx,offset sysexit_code+KR
  sysexit

  align 16

sysexit_code:

  mov   edx,ebp

  mov   ebp,linear_space
  mov   ds,ebp
  mov   es,ebp
  mov   ss,ebp
                      
  sti

ipcman_sysexit_to_user_instruction:

  db    0CAh
  dw    sizeof ipc_user_stack-2*4








;----------------------------------------------------------------------------
;
;       KERNEL IPC
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   snd descriptor
;       ECX   timeouts
;       EDX   snd.w0
;       EBX   snd.w1
;       EBP   rcv descriptor
;       ESI   dest
;       EDI   msg.w2
;
;       DS    kernel space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX   msg.dope / completion code
;       ECX   -ud-
;       EDX   msg.w1
;       EBX   msg.w0
;       EBP   -ud-
;       ESI   source
;       EDI   msg.w2
;
;----------------------------------------------------------------------------




kernel_ipc:

  push  eax
  push  ebx
  push  edx

  mov   ebx,esp
  and   ebx,-sizeof tcb

  mov   [ebx+waiting_for],esi
  mov   [ebx+mword2],edi
  mov   [ebx+timeouts],ecx
  mov   [ebx+rcv_descriptor],ebp

  jmp   ipc_restart


kernel_ipc_exit:

  add   esp,offset ipc_esp-2*4
  ret   








  




  


.errnz  open_wait - (closed_wait + nclos)
.errnz  nwait - 80h





XHEAD propagate_or_redirected_post

  mov   ecx,[edi+waiting_for]
  test  al,deceit
 
  mov   esi,[edi+myself]
  xret  z,long

  mov   esi,[edi+virtual_sender]
  xret  ,long





XHEAD send_only

  mov   [ebx+fine_state],running
  mov   edi,ebx
  pop   edx
  pop   ebx
  push  offset send_ok_ret+KR

  test  [edi+list_state],is_ready
  xret  nz,long

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
  xret  ,long






XHEAD propagate_pre

  mov   edi,[esp+ipc_esp]             ; must be read from user space,
  add   edi,[ebx+as_base]             ; potentially small space
  mov   edi,[edi+sizeof ipc_user_stack]        
  
  sub   edx,edx  
  mov   [ebx+propagatee_tcb],edx

  mov   edx,ebx
  mov   esi,ebp
  xor   edx,edi
  xor   esi,edi
  
  mov   [ebx+virtual_sender],edi
  
  test  edx,mask task_no                          ; propagation if deceited in dest or src task
  CORZ
  test  esi,mask task_no
  IFZ
        lea___tcb esi,edi
        
        test__page_writable esi
        IFNC
        CANDZ [esi+myself],edi
        mov   dl,[esi+fine_state]
        mov   edi,[esi+waiting_for]
        and   dl,NOT nwake
        CANDZ dl,closed_wait
        CANDZ [ebx+myself],edi
        
              mov   [ebx+propagatee_tcb],esi
              mov   edi,[ebp+myself]
              mov   [esi+waiting_for],edi
        FI      
        xret  ,long
  FI


                                                        ;REDIR begin --------------------------------
                                                        ;
  mov   edx,[ebx+myself]                                ;  ; always redirection if 
  mov   esi,[ebp+myself]                                ;  ;        myself = chief (dest) AND
  shl   edx,chief_no-task_no                            ;  ;        myself = chief (virtual sender)
  xor   esi,edx                                         ;
  xor   edx,edi                                         ;
  or    edx,esi                                         ;
  test  edx,mask chief_no                               ;                                        ;
  xret  z,long                                          ;
                                                        ;
  mov   edx,[ebx+myself]                                ;
  mov   esi,[ebp+myself]                                ;
  xor   esi,edx                                         ;
  xor   edx,edi                                         ;
  or    edx,esi                                         ;
                                                        ;
  test  edx,mask chief_no                               ; ; redirection only if within same clan
  IFZ                                                   ; ;       and redir path
        push  ecx                                       ; ;
        push  ebp                                       ;
                                                        ;
        and   ebp,mask task_no                          ;
        mov   esi,edi                                   ;
        shr   ebp,task_no-2                             ;
        mov   ecx,16                                    ;
        DO                                              ;
              and   esi,mask task_no                    ;
              shr   esi,task_no-log2_tasks-2            ;
                                                        ;
              mov   edx,[esi+ebp+redirection_table]     ;
              cmp   edx,ipc_transparent                 ;
              EXITZ                                     ;
              cmp   edx,ipc_inhibited                   ;
              EXITZ                                     ;                                                       ;
              cmp   edx,ipc_locked                      ;
              EXITZ                                     ;
              dec   ecx                                 ;
              EXITZ                                     ;
                                                        ;
              mov   esi,edx                             ;
              xor   edx,ebx                             ;
              test  edx,mask task_no                    ;
              REPEATNZ                                  ;
                                                        ;
              pop   ebp                                 ;
              pop   ecx                                 ;
              xret  ,long                               ;
        OD                                              ;
        pop   ebp                                       ;
        pop   ecx                                       ;
        and   al,NOT deceit                             ;
        xret  ,long                                     ;
  FI                                                    ;
                                                        ;
                                                        ;REDIR ends -----------------------------------

  
  push  eax  
  push  edi
  
  mov   esi,edi
  shr   edi,chief_no-task_no
  xor   edi,ebx
  test  edi,mask task_no
  setz  ah                      ; AH=1  <==>  I am chief of nchief(dest)  
  call  nchief                        
  shr   esi,chief_no-task_no
  xor   esi,ebx
  test  esi,mask task_no
  setz  al                      ; AL=1  <==>  I am chief of nchief(source)
  xor   al,ah
  
  pop   edi
  pop   eax

  IFZ  
        and   al,NOT deceit
  FI
  xret  ,long

  
  
  
  


XHEAD enter_wakeup_for_receive_timeout

  mov   [esp+ipc_eax],offset receive_timeout_ret+KR

  mov   edi,ecx
  and   edi,0FF000000h
  add   cl,2
  shr   edi,cl
  shr   edi,cl
  IFNZ

        add   edi,ds:[system_clock_low]
        mov   [ebx+wakeup_low],edi

        sub   dl,nwake

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


  mov   dl,running-(closed_wait+nwake)
  push  eax
  mark__ready ebx
  pop   eax
  xret  ,long


  align 4



send_ok_ret:

  sub   eax,eax
  jmp   ipc_exit




                                               ; PF in critical region push ipc state
ipc_critical_region_end:                       ; even if fine_state is still 'running'



  align 16


XHEAD ipc_long
 
  mov   [ebx+fine_state],locked_running  
  mov   [ebp+fine_state],locked_waiting
  
  and   eax,ipc_control_mask
  mov   esi,[esp+ipc_eax]
  
  mov   edx,[ebx+as_base]
  
  mov   [ebx+com_partner],ebp
  mov   [ebp+com_partner],ebx

  and   esi,NOT (deceit+map_msg)
  IFNZ
        mov   edi,[esi+edx+msg_dope]
        and   edi,0FFFFFF00h
        or    eax,edi
  FI


  mov   edi,[ebp+rcv_descriptor]


  test  edi,map_msg 	;;; test  al,map_msg    ---- Volkmar
  IFZ   ,,long

  test  eax,mask md_mwords-(3 SHL md_mwords)+mask md_strings + map_msg
  xret  z,long

  add   eax,ipc_cut
  and   edi,NOT (deceit+map_msg)
  
  CANDNZ ,,long

     ;   mov   ecx,linear_kernel_space
     ;   mov   es,ecx

        sub   eax,ipc_cut
        push  ebx
        
        push  eax
        mov   ecx,eax                                    
        
        shr   ecx,md_mwords
        
        lea   eax,[(ecx*4)+esi+msg_w3]
        cmp   eax,[ebx+as_size]
        xc    a,address_space_overflow_mwords_source,long

        add   esi,edx

        lea   eax,[(ecx*4)+edi+msg_w3]
        cmp   eax,[ebp+as_size]
        xc    a,address_space_overflow_mwords_dest,long

        mov   eax,[ebp+as_base]
        add   edi,eax
     
        test  eax,eax
     ;;;;cmp   eax,eax
        xc    z,mwords_through_com_space,long

    ;;    sti

        mov   edx,[edi+msg_size_dope]
        shr   edx,md_mwords
        cmp   ecx,edx
        xc    a,shorten_mwords,long

        mov   edx,edi        
        sub   ecx,2
        IFA
              push  esi
              add   esi,offset msg_w3
              add   edi,offset msg_w3
              
              IFAE  ecx,32/4
                    DO
                          mov   eax,[esi]
                        ;;  mov   ebx,[edi+32]
                          mov   ebx,[esi+4]
                          mov   [edi],eax
                          mov   [edi+4],ebx
                          mov   eax,[esi+8]
                          mov   ebx,[esi+12]
                          mov   [edi+8],eax
                          mov   [edi+12],ebx
                          mov   eax,[esi+16]
                          mov   ebx,[esi+20]
                          mov   [edi+16],eax
                          mov   [edi+20],ebx
                          mov   eax,[esi+24]
                          mov   ebx,[esi+28]
                          mov   [edi+24],eax
                          mov   [edi+28],ebx
                          add   esi,32
                          add   edi,32
                          sub   ecx,32/4
                          cmp   ecx,32/4
                          REPEATA
                    OD
              FI
              IFAE  ecx,4/4
                    DO
                          mov   eax,[esi]
                          mov   [edi],eax
                          add   esi,4
                          add   edi,4
                          sub   ecx,4/4
                          REPEATA
                    OD
              FI

              ;cld
              ;rep   movsd        

              pop   esi

        FI
        pop   eax
        pop   ebx

        test  eax,mask md_strings
        xc    nz,ipc_strings,long

        mov   edi,[edx+msg_rcv_fpage]

        unmrk_ressources ebx,com_used,in_partner_space

		test  edi,edi
		IFNZ
			  or	edi,map_msg
		FI

    ;;    cli

        mov   ecx,[ebx+timeouts]

  FI

  test  eax,map_msg
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

  or    al,ipc_cut
  
  mov   ecx,ebx                       ;
  xor   ecx,ebp                       ;
  test  ecx,mask task_no              ; ignore intra-task mapping
  xret  z,long                        ;

  test  edi,map_msg
  xret  z,long
  
  and   al,NOT ipc_cut

  pop   edx
  
  push  eax
  push  ebx
  push  edx


  mov   ecx,eax

  mov   eax,[esp+ipc_ebx+2*4]         ; w1, first snd fpage
  mov   ebx,edi                       ; rcv fpage -> ebx
  
  ;-------------- provisional translate impl -----------
  
  test  al,al
  IFZ
        call  translate_address
        mov   [esp+ipc_ebx+2*4],eax
        
        pop   edx
        pop   ebx
        pop   eax
        
        push  edx
        mov   ecx,[ebx+timeouts]
        
        xret  ,long
  FI
  
  ;-----------------------------------------------------      

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

              push  offset fpage_opn_ret+KR
              test  ch,fpage_grant
              jz    map_fpage
              jmp   grant_fpage

              align 16

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
              
              push  edx

              mov   ecx,[ebx+timeouts]

              xret  ,long
        FI

        add   esi,sizeof fpage_vector

        mov   edx,[esi+msg_w3].snd_base
        mov   eax,[esi+msg_w3].snd_fpage

        REPEAT
  OD

  pop   edx
  pop   ebx
  pop   eax

  push  edx
  
  mov   al,ipc_cut

  mov   ecx,[ebx+timeouts]
  xret  ,long





XHEAD shrink_snd_fpage

  add   eax,edx
  sub   edx,edx
  mov   cl,bl
  xret
 


XHEAD shorten_mwords

 ke 'shorten_mwords'
  mov   ecx,edx
  shl   eax,width md_mwords
  shrd  eax,ecx,width md_mwords
  or    al,ipc_cut
  xret  ,long





XHEAD address_space_overflow_mwords_source
  ke 'mw_source_ovfl'
  xret  ,long

XHEAD address_space_overflow_mwords_dest
  ke 'mw_dest_overfl'
  xret  ,long



;XHEAD prepare_small_source
;
;  lea   edx,[ecx*4+esi]
;  
;  sass__32 cmp,edx,MB4-offset msg_w3
;  IFB_
;        add   esi,eax
;        xret  ,long
;  FI
;  
;  lno___task eax,ebx
;  mov   eax,[eax*8+task_proot].proot_ptr
;  and   eax,0FFFFFF00h
;  mov   ds:[cpu_cr3],eax
;  mov   ds:[tlb_invalidated],al
;  mov   cr3,eax
;  xret  ,long
;  
  
  
        

                                          
               copy_long:

                 ;;   push  es
                 ;;   push  ds
                 ;;   pop   es
                 ;;   mov   eax,ecx
                 ;;   and   ecx,-8
                 ;;   and   eax,8-1
                 ;;   cld
                 ;;   rep   movsd  
                 ;;   mov   ecx,eax
                 ;;   pop   es
                 ;;   ret             
                 ;;
            
            copy:

              IFAE  ecx,32/4
                    DO
                          mov   eax,[esi]
                        ;;  mov   ebx,[edi+32]
                          mov   ebx,[esi+4]
                          mov   [edi],eax
                          mov   [edi+4],ebx
                          mov   eax,[esi+8]
                          mov   ebx,[esi+12]
                          mov   [edi+8],eax
                          mov   [edi+12],ebx
                          mov   eax,[esi+16]
                          mov   ebx,[esi+20]
                          mov   [edi+16],eax
                          mov   [edi+20],ebx
                          mov   eax,[esi+24]
                          mov   ebx,[esi+28]
                          mov   [edi+24],eax
                          mov   [edi+28],ebx
                          add   esi,32
                          add   edi,32
                          sub   ecx,32/4
                          cmp   ecx,32/4
                          REPEATA
                    OD
                    pop   ebp
                    pop   edx
              FI
              IFAE  ecx,4/4
                    DO
                          mov   eax,[esi]
                          mov   [edi],eax
                          add   esi,4
                          add   edi,4
                          sub   ecx,4/4
                          REPEATA
                    OD
              FI

              ret




XHEAD mwords_through_com_space         

              mov   eax,[ebp+thread_proot]
              IFNZ  eax,ds:[cpu_cr3]
              
                    mark__ressource ebx,com_used

                    mov   edx,[ebx+thread_proot]     ;;; may be not in its own address space
                    IFNZ  edx,ds:[cpu_cr3]           ;;; if source is small !!!!!
                          mov   ds:[cpu_cr3],edx
                          mov   cr3,edx
                    FI
                    
                    mov   [ebx+waddr],edi
                    mov   ebx,edi
                    
                    and   ebx,-MB4
                    and   edi,MB4-1
                    
                    shr   ebx,20
                    add   eax,PM
                    
                    add   ebx,eax
                    
                    add   edi,com0_base
                    
                    cmp   ds:[tlb_invalidated],0
                    
                    mov   eax,[ebx]
                    mov   ebx,[ebx+4]
                    
                    lea   edx,[edx+(com0_base SHR 20)+PM]
                    IFNZ
                    
                          or    eax,page_accessed+page_dirty
                          or    ebx,page_accessed+page_dirty
                          
                          cmp   [edx],eax
                          CORNZ
                          
                          cmp   [edx+4],ebx
                          IFNZ
                                push  eax
                                mov   eax,cr3
                                mov   cr3,eax
                                pop   eax
                          FI
                    FI      
                    or    eax,page_accessed+page_dirty
                    or    ebx,page_accessed+page_dirty
                          
                    mov   [edx],eax
                    mov   [edx+4],ebx

              ELSE_

                    mark__ressource ebx,in_partner_space

              FI  
              
              xret  ,long              



;-----------------------------------------------------------------------------------
;
;       ipc strings
;
;-----------------------------------------------------------------------------------
;
; ipc strings :
;
;       to first source string ;
;       to first dest string ;
;       IF no dest string THEN LEAVE WITH cut error FI ;
;       open dest string ;
;      
;       DO
;             copy segment := source segment RESTRICTED BY (dest segment.length, 4MB) ;
;             IF addresses are valid
;                   THEN  copy data
;             FI ;
;             set dest string length ;
;             source segment PROCEED BY copy segment.length ;
;             dest segment PROCEED BY copy segment.length ;
;
;             IF source segment exhausted
;                   THEN  to next source string ;
;                         IF no source string THEN LEAVE WITH done FI ;
;                         IF is master source string 
;                               THEN  to next master dest string ;
;                                     IF no dest string 
;                                           THEN LEAVE WITH cut error
;                                           ELSE open dest string
;                                     FI
;                         FI
;             ELIF dest segment exhausted
;                   THEN  to next dest string ;
;                         IF no dest string THEN LEAVE WITH cut error FI ;
;                         IF dest slave string
;                               THEN  open dest string
;                               ELSE  LEAVE WITH cut error
;                         FI
;             FI
;       OD .
;
;---------------------------------------------------------------------------------



              align 16



XHEAD ipc_strings

  or    al,ipc_cut

  mov   ch,ah
  and   ah,NOT (mask md_strings SHR 8)
  and   ch,mask md_strings SHR 8
  
  mov   cl,[edx+msg_size_dope].msg_strings
  and   cl,mask md_strings SHR 8
  xret  z,long
  or    ah,cl


  push  eax
  push  edx

  mov   eax,linear_kernel_space
  mov   es,eax

  mov   ebx,[esi+msg_size_dope]
  shr   ebx,md_mwords
  lea   ebp,[(ebx*4)+esi+msg_w3-2*4]

  mov   eax,[edx+msg_size_dope]
  shr   eax,md_mwords
  lea   edx,[(eax*4)+edx+msg_w3-2*4]

  mov   ebx,[ebp+str_len]
  mov   esi,[ebp+str_addr]

  mov   eax,[edx+buf_size]
  mov   edi,[edx+buf_addr]
  mov   [edx+str_addr],edi


  DO
        push  ecx

        mov   ecx,MB4
        cmp   eax,ecx
        cmovb ecx,eax
        cmp   ebx,ecx  
        cmovb ecx,ebx

  
        pushad
        mov   eax,edi
        mov   ebx,esi
        add   eax,ecx
        IFNC  ,,long
        add   ebx,ecx
        CANDNC ,,long
        CANDB eax,virtual_space_size,long
        CANDB ebx,virtual_space_size,long

              mov   ebx,esp
              and   ebx,-sizeof tcb
              mov   ebp,[ebx+com_partner]

              lea   eax,[esi+ecx]
              cmp   eax,[ebx+as_size]
              xc    a,address_space_overflow_str_source,long
              add   esi,[ebx+as_base]

              lea   eax,[edi+ecx]
              cmp   eax,[ebp+as_size]
              xc    a,address_space_overflow_str_dest,long

              mov   eax,[ebp+as_base]
              add   edi,eax
         
              test  eax,eax
         ;;;cmp eax,eax
              IFZ
                    mov   edx,[ebp+thread_proot]
                    IFNZ  edx,ds:[cpu_cr3]                    ;;
                          mov   edx,edi
                          and   edx,-MB4
                          sub   edi,edx
                          add   edi,com0_base
                          mov   eax,[ebx+waddr]
                          xor   eax,edx
                          test  eax,-MB4
                          xc    nz,string_to_com1_space,long
                    FI
              FI

              cld
              rep   movsb       ; as fast as movsd
  
        FI
        popad


        sub   eax,ecx
        sub   ebx,ecx
        add   edi,ecx
        add   ecx,esi
        IFNC
              mov   esi,ecx
        FI

        mov   ecx,[edx+buf_addr]
        sub   edi,ecx
        mov   [edx+str_len],edi
        add   edi,ecx

        pop   ecx


        test  ebx,ebx
        IFZ
              add   ebp,sizeof string_vector
              dec   ch
              EXITZ
        
              mov   ebx,[ebp+str_len]
              mov   esi,[ebp+str_addr]
              test  ebx,ebx
              REPEATNS

              and   ebx,7FFFFFFFh
              DO
                    add   edx,sizeof string_vector
                    dec   cl
                    OUTER_LOOP EXITZ
                    mov   eax,[edx+buf_size]
                    mov   edi,[edx+buf_addr]
                    test  eax,eax
                    REPEATS
              OD
              mov   [edx+str_addr],edi
              REPEAT
        FI

        test  eax,eax
        REPEATNZ

        add   edx,sizeof string_vector
        dec   cl
        EXITZ
  
        mov   eax,[edx+buf_size]
        mov   edi,[edx+buf_addr]
  
        test  eax,eax
        REPEATS
        mov   cl,0

  OD
  

  mov   ebx,esp
  and   ebx,-sizeof tcb

  pop   edx
  pop   eax

  mov   ebp,[ebx+com_partner]

  test  cl,cl
  IFNZ
        and   al,NOT ipc_cut
        sub   ah,cl
        inc   ah
 ELSE_
      ke 'cut_string'

  FI
  
  xret  ,long





XHEAD address_space_overflow_str_source
  ke 'str_source_ovfl'
  xret  ,long

XHEAD address_space_overflow_str_dest
  ke 'str_dest_overfl'
  xret  ,long







XHEAD string_to_com1_space

  push  ecx

  mark__ressource ebx,com_used

  shr   edx,16
  mov   word ptr [ebx+waddr],dx
  add   edi,com1_base-com0_base
  lea___pdir ecx,ebp
  mov   ecx,[ecx+edx]
  mov   edx,ecx
  and   dl,NOT page_user_permit
  xchg  ds:[pdir+(com1_base SHR 20)],edx
  cmp   edx,ecx
  mov   dword ptr ds:[pdir+(com1_base SHR 20)+4],0

  pop   ecx
  xret  z,long
  test  edx,edx
  xret  z,long

  mov   edx,cr3
  mov   cr3,edx
  xret  ,long




;XHEAD prepare_small_string_source
;
;  shl   eax,16
;  
;  lea   edx,[esi+ecx]
;  sass__32 cmp,edx,MB4
;  IFB_
;        add   esi,eax
;        xret  ,long
;  FI
;  
;  lno___task eax,ebx
;  mov   eax,[eax*8+task_proot].proot_ptr
;  mov   al,0
;  mov   ds:[cpu_cr3],eax
;  mov   ds:[tlb_invalidated],al
;  mov   cr3,eax
;  xret  ,long
  
  




        align 16



fetch_next:

  mov   [ebx+fine_state],locked_waiting
  
  pop   edx

  mov   esi,[ebx+myself]
  test  al,deceit
  IFNZ
        mov   esi,[ebx+virtual_sender]
  FI
  mov   edi,[ebx+mword2]
  
  mov   ecx,esp
  mov   esp,[ebp+thread_esp]

  push  eax                     ; eax         ;
  mov   eax,[ebx+waiting_for]                 ;
  push  eax                     ; ecx         ;
  push  edx                     ; edx         ;
  mov   eax,[ecx]                             ;
  push  eax                     ; ebx         ; pushad
  push  eax                     ; temp (esp)  ;
  push  eax                     ; ebp         ;
  push  esi                     ; esi         ;
  push  edi                     ; edi         ;
  push  offset received_ok_ret+KR

  mark__ready ebp

  mov   [ebp+thread_esp],esp
  lea   esp,[ecx+4]


  get_first_from_sndq

  IFC
        mov   dl,0
        mov   ebp,edx


        mov   [edx+fine_state],locked_running

        jmp   switch_context

  FI

  mov   [ebx+fine_state],running
  
  sub   edx,offset intrq_llink-1*8
  shr   edx,3
  sub   ebx,ebx
  mov   esi,edx
  mov   edi,ebx

  sub   eax,eax
  jmp   ipc_exit








ipc_dest_not_existent:

  sub   eax,eax
  mov   al,ipc_not_existent_or_illegal
  jmp   ipc_exit




nil_dest_not_existent_or_interrupt_attach_operation:

  sub   eax,eax
  pop   edx                     ; msg w0
  pop   ebx                     ; msg w1
  pop   ebp

  test  esi,esi
  IFZ
        mov   al,ipc_not_existent_or_illegal
        jmp   ipc_exit
  FI
  
  lea   ecx,[esi-1]
  
  mov   edi,ebx
  mov   esi,edx
  sub   ebx,ebx
  test  edx,edx
  IFNZ
  lea___tcb   edx,edx
  CANDZ [edx+myself],esi
        mov   ebx,edx
  FI
  
  call  attach_intr
  
  ke '??'
  jmp   ipc_exit      
               







  align 16                                        ;REDIR begins --------------------------
                                                  ;
                                                  ;
                                                  ;
XHEAD redirect_or_lock_ipc                        ;
                                                  ;
  IFNZ  edx,ipc_locked                            ;
                                                  ;
        mov   esi,edx                             ;
        or    al,redirected                       ;
                                                  ;
        xret  ,long                               ;
  FI                                              ;
                                                  ;
                                                  ;
                                                  ; ipc locked: wait and restart ipc
  pushad                                          ;
  sub   esi,esi                                   ;
  int   thread_switch                             ;
  popad                                           ;
                                                  ;
  mov   ebp,[ebx+rcv_descriptor]                  ;
  pop   edx                                       ;
  pop   ebx                                       ;
  pop   eax                                       ;
                                                  ;
  ke '??'                                         ;REDIR ends --------------------------
  jmp   ipc_exit


        




  align 16


XHEAD to_chief

  cmp   esi,intr_sources
  jbe   nil_dest_not_existent_or_interrupt_attach_operation
  
  cmp   esi,ipc_inhibited                               ;REDIR -------------------------
  jz    ipc_dest_not_existent                           ;REDIR -------------------------

  DO    
        mov   edi,[ebx+myself]
        shr   edi,chief_no-task_no
        xor   edi,esi
        test  edi,mask task_no
        EXITZ
        
        mov   edi,esi
        shr   edi,chief_no-task_no
        xor   edi,ebx
        test  edi,mask task_no
        EXITZ
        
        test__page_present ebp
        IFNC
        CANDNZ [ebp+coarse_state],unused_tcb
        mov   dl,[ebp+clan_depth]
        sub   dl,[ebx+clan_depth]
        CANDA
              or    al,redirected
              DO
                    shr   esi,chief_no-task_no
					and   esi,NOT mask lthread_no
                    lea___tcb esi,esi
                    mov   edi,[esi+myself]
                    mov   esi,edi
                    shr   edi,chief_no-task_no
                    xor   edi,ebx
                    test  edi,mask task_no
                    OUTER_LOOP EXITZ
                    
                    dec   dl
                    REPEATNZ
              OD
              mov   edi,[ebx+myself]
              xor   edi,esi
              test  edi,mask chief_no
              EXITZ
        FI
        
        or    al,redirected+from_inner_clan
        mov   esi,[ebx+myself]
        shr   esi,chief_no-task_no
  OD
  
  lea___tcb ebp,esi
  mov   edx,esi                             ; ensures that dest-id check succeeds
  xret  ,long
                
                    
              





XHEAD pending_or_auto_propagating

  cmp   ebx,ebp
  jz    sw_err3
  
  test  al,deceit
  IFNZ
        cmp   esi,[ebx+virtual_sender]
        xret  z,long
  FI

  
  mov   edi,[ebp+myself]
  DO
        test  [ebp+coarse_state],auto_propagating
        EXITZ
              
        add   ebp,sizeof tcb
        test__page_writable ebp
        EXITC
        
        mov   dl,[ebp+fine_state]
        test  dl,nwait
        REPEATNZ
        
        test  dl,nclos
        IFNZ
              cmp   [ebp+waiting_for],edi
              REPEATNZ
        FI      
        
        mov   edi,[ebp+myself]
        test  al,deceit
        IFNZ
        mov   esi,[ebx+propagatee_tcb]
        CANDNZ esi,0
              mov   [esi+waiting_for],edi
        ELSE_
              mov   [ebx+waiting_for],edi
        FI            
              
        xret  ,long
  OD 
 

  test  cl,0F0h
  IFNZ
        test  ecx,000FF0000h
        jz    send_timeout_ret
  FI

  mov   [ebx+com_partner],ebp

  insert_last_into_sndq

  shl   ecx,8
  mov   cl,ch
  shr   cl,4

  push  offset ret_from_poll+KR
  mov   ch,polling+nwake

  jmp   wait_for_ipc_or_timeout




sw_err3:
  pop   edx
  pop   edi
  sub   eax,eax
  mov   al,ipc_not_existent_or_illegal
  jmp   ipc_exit











ret_from_poll:

  push  linear_kernel_space
  pop   ds

  mov   ebx,esp
  and   ebx,-sizeof tcb
  mov   ebp,[ebx+com_partner]

  mov   eax,[esp+ipc_eax]
  mov   ecx,[ebx+timeouts]
  mov   esi,[ebx+waiting_for]

  IFZ   [ebx+fine_state],locked_running

        mov   [ebx+fine_state],running
        and   [ebp+fine_state],nclos
        or    [ebp+fine_state],closed_wait+nwake

        jmp   ipc_restart

  FI

  test  [ebx+fine_state],npoll
  IFZ
        mov   ebp,ebx
        delete_from_sndq
        mov   [ebx+fine_state],running
  FI


send_timeout_ret:

  add   esp,sizeof ipc_kernel_stack-2*4
  sub   eax,eax
  mov   al,ipc_timeout+ipc_s
  jmp   ipc_exit






;----------------------------------------------------------------------------



w_err:
  sub   eax,eax
  mov   al,ipc_not_existent_or_illegal
  jmp   ipc_exit




;----------------------------------------------------------------------------
;
;       RECEIVE
;
;----------------------------------------------------------------------------


  align 16






  
receive_only:
  
  pop   edx
  pop   eax

  mov   ebp,[ebx+rcv_descriptor]

  cmp   ebp,virtual_space_size
  jae   w_err

  test  ebp,nclos
  jz    receive_from

  test  [ebx+list_state],is_polled
  IFNZ

        get_first_from_sndq

        IFNC
              sub   edx,offset intrq_llink-1*8
              shr   edx,3
              sub   ebx,ebx
              mov   esi,edx
              mov   edi,ebx
              sub   eax,eax

              jmp   ipc_exit

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

  mov   dword ptr [esp],offset receive_timeout_ret+KR




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

  mov   ebp,esp
  and   ebp,-sizeof tcb

  push  eax

  mov   [ebp+fine_state],running

  sub   eax,eax
  mov   al,ipc_timeout
  jmp   ipc_exit



  align 16


received_ok_ret:

  popad
  jmp   ipc_exit






;----------------------------------------------------------------------------
;
;       RECEIVE FROM
;
;----------------------------------------------------------------------------


  align 16


receive_from:

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

        sub   edx,offset intrq_llink-1*8
        shr   edx,3
        sub   ebx,ebx
        mov   esi,edx
        mov   edi,ebx
        sub   eax,eax

        jmp   ipc_exit
  FI

  lea___tcb ebp,esi

  mov   edi,[ebx+myself]
  xor   edi,esi
  test  edi,mask chief_no
  IFNZ
        call  nchief
  FI

  cmp   [ebp+myself],esi
  jnz   short r_source_not_existent

  test  [ebp+fine_state],npoll
  IFZ
  CANDZ [ebp+com_partner],ebx

        delete_from_sndq

        mov   [ebp+fine_state],locked_running
        mark__ready ebp
        mov   [ebx+fine_state],locked_closed_waiting
        mov   [ebx+com_partner],ebp

        jmp   switch_context
  FI


  mov   ch,closed_wait+nwake
  jmp   wait_for_receive_from_or_timeout




 
r_source_not_existent:
  sub   eax,eax
  mov   al,ipc_not_existent_or_illegal
  jmp   ipc_exit




;----------------------------------------------------------------------------
;
;       nchief
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   thread / 0
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
; ESI=0 on input:
;
;       ESI   myself 
;
;
; ESI>0 on input:
;                        outside clan                       within clan
;
;       AL    redirected / redirected+from_inner_clan             0
;       ESI              chief                                 thread 
;
;       EDX,EDI          scratch
;
;----------------------------------------------------------------------------



        align 16




id_nearest_sc:

  mov   ebp,esp
  and   ebp,-sizeof tcb

  sub   eax,eax

  test  esi,esi
  IFZ
        mov   esi,[ebp+myself]
        iretd
  FI


  mov   ebx,ebp
  lea___tcb ebp,esi
  
  push  linear_kernel_space
  pop   ds

  sub   eax,eax
  call  nchief

  push  linear_space
  pop   ds

  iretd







nchief:                                           ; esi: dest, ebx: my tcb, ebp: dest tcb

  mov   al,0
  DO
        mov   edi,[ebx+myself]
        shr   edi,chief_no-task_no
        xor   edi,esi
        test  edi,mask task_no                   ; esi = chief(me)
        EXITZ
        
        mov   edi,esi
        shr   edi,chief_no-task_no
        xor   edi,ebx
        test  edi,mask task_no                   ; me = chief(esi)
        EXITZ
        
        test__page_present ebp
        IFNC
        CANDNZ [ebp+coarse_state],unused_tcb
        mov   dl,[ebp+clan_depth]
        sub   dl,[ebx+clan_depth]
        CANDA
              mov   al,redirected+from_inner_clan
              DO
                    shr   esi,chief_no-task_no
					and   esi,NOT mask lthread_no
                    lea___tcb esi,esi
                    mov   edi,[esi+myself]
                    mov   esi,edi
                    shr   edi,chief_no-task_no
                    xor   edi,ebx
                    test  edi,mask task_no
                    OUTER_LOOP EXITZ
               
                    dec   dl
                    REPEATNZ
              OD
        
              mov   edi,[ebx+myself]
              xor   edi,esi
              test  edi,mask chief_no
              IFZ
                    mov   al,redirected
                    ret
              FI
        FI
         
        mov   esi,[ebx+myself]
        shr   esi,chief_no-task_no
		and   esi,NOT mask lthread_no
        lea___tcb esi,esi
        mov   esi,[esi+myself] 
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
;       <EBP+fine_state>  is 'locked_running' or 'locked_waiting' or 'running'
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
  mov   eax,[ebp+mword2]
  push  eax
  mov   eax,[ebp+rcv_descriptor]
  push  eax
  mov   eax,[ebp+virtual_sender]
  push  eax
  mov   eax,[ebp+timeouts]
  shl   eax,8
  mov   ah,[ebp+fine_state]
  mov   [ebp+fine_state],running
  mov   al,[ebp+state_sp]
  push  eax

  mov   ecx,esp
  shr   ecx,2
  mov   [ebp+state_sp],cl
  
  IFNZ  ah,running
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
  FI
  
  sub   ecx,ecx
  test  esp,esp           ; NZ!
  
  jmp   edi        






pop_ipc_state:

  pop   edi

  pop   eax
  mov   [ebp+state_sp],al
  mov   [ebp+fine_state],ah
  shr   eax,8
  mov   byte ptr [ebp+timeouts+1],ah

  pop   eax
  mov   [ebp+virtual_sender],eax
  pop   eax
  mov   [ebp+rcv_descriptor],eax
  pop   eax
  mov   [ebp+mword2],eax
  pop   eax
  mov   [ebp+waiting_for],eax
  pop   eax
  mov   [ebp+com_partner],eax

  IFNZ  [ebp+fine_state],running
        test  [eax+fine_state],nlock
        CORNZ
        IFNZ  [eax+com_partner],ebp
        
              ke    '-pi_err'
        FI
  FI      
  jmp   edi






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
        IFZ   word ptr [ebp+sizeof tcb-sizeof ipc_kernel_stack].ipc_esp,linear_space
        mov   eax,[ebp+sizeof tcb-sizeof int_pm_stack].ip_error_code 
        CANDAE eax,min_icode
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
 
.erre   (linear_space AND 3) ne 0
.erre   (linear_space lt 1000h)       ; kernel start address MOD 64 K 



;----------------------------------------------------------------------------
;
;       cancel if within ipc
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
; POSTCONDITION:
;
;       AL    bottom state
;
;       {REGs - AL}    scratch
;
;       base waiting   :   ipc cancelled
;       base pending   :   ipc cancelled
;       base locked    :   ipc aborted, also of partner
;
;       ELSE           :   status unchanged
;
;----------------------------------------------------------------------------



cancel_if_within_ipc:

  test  [ebp+fine_state],npoll    
  IFZ
        push  eax
        push  ecx
        delete_from_sndq
        pop   ecx
        pop   eax
  FI      


  call  get_bottom_state

  push  eax
  
  test  al,nlock
  IFNZ
        test  al,nready
        IFNZ
              mov   al,ipc_cancelled
              call  reset_ipc
        FI
        pop   eax
        ret
  FI

  mov   al,ipc_aborted
  call  reset_ipc
  mov   ebp,ebx
  mov   al,ipc_aborted
  call  reset_ipc

  pop   eax
  ret




                                      
reset_ipc:

  pop   ecx

  test  [ebp+fine_state],nrcv
  IFNZ
        add   al,ipc_s
  FI
  movzx eax,al

  mov   [ebp+fine_state],running
  mov   ebx,ebp
  mark__ready ebx
  
  lea   esi,[ebp+sizeof tcb - (sizeof ipc_kernel_stack-2*4+2*4)]
  mov   ebx,offset reset_ipc_sysenter_ret+KR

  IFZ   word ptr [esi+ipc_esp],linear_space

        add   esi,(sizeof tcb-(sizeof iret_vec+2*4)) - (sizeof tcb-(sizeof ipc_kernel_stack-2*4+2*4))
        add   ebx,(offset reset_ipc_sc_ret+KR) - (offset reset_ipc_sysenter_ret+KR)
  FI

  mov   [esi+4],eax
  mov   [esi],ebx

  mov   [ebp+thread_esp],esi
  xor   esi,esp
  test  esi,mask thread_no
  IFZ
        xor   esp,esi
  FI

  jmp   ecx



reset_ipc_sysenter_ret:

  pop   eax
  ke    'reset_ipc_sysenter'
  jmp   ipc_exit



reset_ipc_sc_ret:

  pop   eax
  ke    'reset_ipc_sc'

  push  linear_space
  pop   ds
  push  linear_space
  pop   es

  iretd



.erre   (linear_space AND 3) ne 0
.erre   (linear_space lt 1000h)       ; kernel start address MOD 64 K 



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
;       EBP   tcb write addr, must be mapped !!
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
;;mov   dword ptr [eax],offset restart_poll+KR

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
        IFC                                   
              sub   esi,esi
        FI
        mov   [ebp+timeouts],esi            
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

  IF    kernel_x2
        push  eax
        lno___prc eax
        test  eax,eax
        pop   eax
        IFNZ
              push  eax
              push  ebx
              
              lea   eax,[ecx*2+io_apic_redir_table]
              mov   byte ptr ds:[io_apic+io_apic_select_reg],al
              lea   ebx,[ecx+irq0_intr]
           mov ebx,10000h  
              mov   ds:[io_apic+io_apic_window],ebx
              inc   al
              mov   byte ptr ds:[io_apic+io_apic_select_reg],al
              mov   eax,ds:[local_apic+apic_id]
              mov   ds:[io_apic+io_apic_window],eax
              
              lea   eax,[(ecx*intr1_intr0)+intr_0_P2+KR]
              lea   ebx,[ecx+irq0_intr]
              mov   bh,0 SHL 5
              call  define_idt_gate
              
              extrn p6_workaround_open_irq:near
              call  p6_workaround_open_irq
              
              pop   ebx
              pop   eax
              ret
        FI
  ENDIF            
              

  call  mask_hw_interrupt

  push  eax
  push  ebx
  lea   eax,[(ecx*intr1_intr0)+intr_0+KR]
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
;         THEN restart transfer long message   {edx,ebp are ok on stack !! }
;       ELIF locked waiting OR waiting
;         THEN restart waiting
;       ELIF locked waiting for non persisting {sender disappeared}
;         THEN restart receive timeout
;       FI
;
;----------------------------------------------------------------------------

        align 4


;ipcman_rerun_thread:
;
;  pushad
;
;  mov   al,[ebp+fine_state]
;  and   al,NOT nwake
;
;  CORZ  al,<locked_waiting AND NOT nwake>
;  IFZ   al,<closed_wait AND NOT nwake>
;        mov   dword ptr [ecx],offset receive_timeout_ret+KR
;        IFAE  [ebp+waiting_for],intr_sources
;              mov   [ebp+fine_state],running
;        FI
;
;  ELIFZ al,<open_wait AND NOT nwake>
;        mov   dword ptr [ecx],offset receive_timeout_ret+KR
;
;  ELIFZ al,<locked_running AND NOT nwake>
;  ELIFZ al,<polling AND NOT nwake>
;        sub   ecx,4
;        mov   dword ptr [ecx],offset ret_from_poll+KR
;
;  ELIFZ al,<running AND NOT nwake>
;        mov   dword ptr [ecx],offset send_ok_ret+KR
;
;  ELIFZ al,<aborted AND NOT nwake>                  
;  mov   al,[ebp+coarse_state]
;  and   al,nblocked+ndead
;  CANDZ al,ndead
;        mov   dword ptr [ecx],offset send_ok_ret+KR
;
;  ELSE_                                             
;        ke    'ill_mess_rerun'
;  FI
;
;  mov   [esp+6*4],ecx
;  mov   [esp+2*4],ebp
;  popad
;  ret




;----------------------------------------------------------------------------
;
;       update small_space_size
;
;----------------------------------------------------------------------------

.listmacro


ipc_update_small_space_size:

  update_small_space_size_immediates
  
  ret
  
  
  
.nolistmacro  
  
;----------------------------------------------------------------------------
;
;       init ipcman
;
;----------------------------------------------------------------------------


  icode



init_ipcman:

  mov   bh,3 SHL 5

  mov   bl,ipc
  mov   eax,offset ipc_sc+KR
  call  define_idt_gate

  mov   bl,id_nearest
  mov   eax,offset id_nearest_sc+KR
  call  define_idt_gate

  bt    ds:[cpu_feature_flags],sysenter_present_bit
  IFNC
        ke    '-processor does not support fast system calls'
  FI

  mov   ecx,sysenter_cs_msr
  mov   eax,kernel_exec
  sub   edx,edx
  wrmsr

  mov   ecx,sysenter_eip_msr
  mov   eax,offset ipc_sysenter+KR
  sub   edx,edx
  wrmsr

  mov   ecx,sysenter_esp_msr
  mov   eax,offset cpu_esp0
  sub   edx,edx
  wrmsr

  ret



  icod  ends




  code ends
  end
