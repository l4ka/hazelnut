include l4pre.inc 
   
  
  Copyright GMD, L4.ADRSMAN.4, 16, 07, 96, 25
 
;*********************************************************************
;******                                                         ******
;******         Address Space Manager                           ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  07.03.90    ******
;******                                   modified: 16.07.96    ******
;******                                                         ******
;*********************************************************************
 

      
  public init_adrsman
  public init_sigma_1
  public sigma_1_installed
  public create_kernel_including_task


  extrn create_thread:near
  extrn delete_thread:near
  extrn insert_into_fresh_frame_pool:near
  extrn flush_address_space:near
  extrn gen_kernel_including_address_space:near
  extrn define_idt_gate:near


.nolist
include l4const.inc
include adrspace.inc
include intrifc.inc
include uid.inc
include tcb.inc
include cpucb.inc
include pagconst.inc
include pagmac.inc
include schedcb.inc
include syscalls.inc
include kpage.inc
.list

ok_for i486,ppro



  assume ds:codseg





;----------------------------------------------------------------------------
;
;       init address space manager
;
;----------------------------------------------------------------------------


  icode



init_adrsman:

  mov   eax,kernel_task         ; ensuring that first ptab for pdir space
  lea___pdir eax,eax            ; becomes allocated before task creation
  mov   eax,[eax]               ;
  
  mov   bh,3 SHL 5

  mov   bl,task_new
  mov   eax,offset task_new_sc+PM
  call  define_idt_gate

  ret


  icod  ends




;----------------------------------------------------------------------------
;
;       task new sc
;
;       delete/create task (incl. creation of lthread 0 of new task
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX      new chief / mcp
;       ECX      initial ESP                             of lthread 0
;       EDX      initial EIP                             of lthread 0
;       EBX      pager.low
;       EBP      pager.high
;       ESI      task id.low
;       EDI      task id.high
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       ESI      new task id (low)     /    0
;       EDI      new task id (high     /    0
;
;       ECX,EDX,ESI,EDI,EBP scratch
;
;       task created,
;       lthread 0 created and started at PL3 with:
;
;                   EAX...EBP            0
;                   ESP                  initial ESP
;                   EIP                  initial EIP
;                   DS...GS              linear space
;                   CS                   linear space exec
;
;----------------------------------------------------------------------------


task_new_failed:

  ke    'tfail'

  sub   esi,esi
  sub   edi,edi

  add   esp,3*4
  tpost eax




task_new_sc:

  tpre  trap2,ds,es

  and   esi,NOT mask lthread_no

  push  esi
  push  ebx
  push  ebp

  mov   ebp,esp
  and   ebp,-sizeof tcb

  mov   ebx,[ebp+myself]
  xor   ebx,edi
  test  ebx,mask chief_no
  IFZ
        mov   ebx,[ebp+chief]
        cmp   ebx,mask depth
        jae   task_new_failed
        xor   ebx,edi
        test  ebx,mask site_no
  FI
  jnz   task_new_failed

  lea___tcb ebx,esi
  test__page_present ebx
  IFNC
  CANDNZ [ebx+coarse_state],unused_tcb

        xor   esi,[ebx+myself]
        test  esi,NOT (mask ver1 + mask ver0)
        IFZ
              cmp   [ebx+chief],edi
        FI
        jnz   task_new_failed


        pushad                                  ;-------------------------
                                                ;
        lno___task edi,ebx                      ;
        load__proot edi,edi                     ;  delete task
                                                ;
        mov   cl,lthreads                       ;
        DO                                      ;
              test__page_present ebx            ;
              IFNC                              ;
                    mov   ebp,ebx               ;
                    call  delete_thread         ;
              FI                                ;
              add   ebx,sizeof tcb              ;
              dec   cl                          ;
              REPEATNZ                          ;
        OD                                      ;
                                                ;
        call  flush_address_space               ;
                                                ;
        add   edi,PM                            ;
        mov   ecx,virtual_space_size SHR 22     ;
        DO                                      ;
              sub   eax,eax                     ;
              cld                               ;
              repe  scasd                       ;
              EXITZ                             ;
                                                ;
              mov   eax,[edi-4]                 ;
              call  insert_into_fresh_frame_pool;
              REPEAT                            ;
        OD                                      ;
                                                ;
        lea   eax,[edi-PM]                      ;
        call  insert_into_fresh_frame_pool      ;
                                                ;
        popad                                   ;--------------------------


  ELSE_

        push  eax
        push  ecx

        lno___task ecx,ebx
        load__proot ecx,ecx
        lno___task eax,ebp
        add   eax,ds:[empty_proot]
        cmp   eax,ecx

        pop   ecx
        pop   eax
        jnz   task_new_failed
  FI

 
  IFZ   <dword ptr [esp+4]>,0

        and   eax,mask task_no
        and   edi,NOT mask task_no
        or    edi,eax

        lno___task ebx
        shr   eax,task_no
        add   eax,ds:[empty_proot]
        store_inactive_proot eax,ebx

        add   esp,4*4
        iretd
  FI


  IFA   al,[ebp+max_controlled_prio]
        mov   al,[ebp+max_controlled_prio]
  FI
  shl   eax,16
  mov   ah,[ebp+prio]
  mov   al,[ebp+timeslice]

  lno___task edi,ebx
  mov   esi,ds:[empty_proot]
  store_proot esi,edi

  pop   edi
  pop   esi

  xchg  ebp,ebx
  push  ebx
  call  create_thread
  pop   ebx

  pop   esi

  mov   [ebp+myself],esi
  mov   eax,[ebx+myself]
  mov   edi,[ebx+chief]
  and   eax,mask task_no
  and   edi,NOT mask task_no
  or    edi,eax
  IFNZ  eax,root_chief
        add   edi,(1 SHL depth)
  FI
  mov   [ebp+chief],edi

  tpost eax,ds,es







  icode




create_kernel_including_task:


  lea___tcb ebp,eax

  call  gen_kernel_including_address_space

  push  eax

  mov   ecx,[ebx].ktask_stack
  mov   edx,[ebx].ktask_start
  mov   eax,(255 SHL 16) + (16 SHL 8) + 10
  mov   esi,sigma0_task
  mov   edi,root_chief

  call  create_thread

  pop   eax

  lno___task ebp
  store_proot eax,ebp

  ret



  icod  ends




;----------------------------------------------------------------------------
;
;       sigma_1
;
;----------------------------------------------------------------------------


sigma_1_installed   db false





init_sigma_1:

  ret







        code ends
        end