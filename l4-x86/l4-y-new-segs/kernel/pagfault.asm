include l4pre.inc 


  Copyright IBM, L4.PAGFAULT, 15,04,00, 9565, K
 
 
;*********************************************************************
;******                                                         ******
;******         Page Fault Handler                              ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 15.04.00    ******
;******                                                         ******
;*********************************************************************
 

  public init_pagfault
  public page_fault_handler
 

  extrn map_system_shared_page:near
 
  extrn kernel_ipc:near
  extrn ipc_exit:near
  extrn ipcman_sysexit_to_user_instruction:near
  extrn ipc_critical_region_begin:near
  extrn ipc_critical_region_end:near
  extrn tcb_fault:near
  extrn pagmap_fault:near
  extrn push_ipc_state:near
  extrn pop_ipc_state:near
  extrn cancel_if_within_ipc:near
  extrn tunnel_to:near
  extrn define_idt_gate:near
  extrn exception:near
  


.nolist 
include l4const.inc
include uid.inc
include adrspace.inc
include tcb.inc
include cpucb.inc
include intrifc.inc
include schedcb.inc
include syscalls.inc
.list
include pagconst.inc
include pagmac.inc
include pagcb.inc
.nolist
include msg.inc
.list



ok_for x86,pIII
 

  extrn set_small_pde_block_in_pdir:near
  

  assume ds:codseg





;****************************************************************************
;*****                                                                  *****
;*****                                                                  *****
;*****        PAGFAULT    INITITIALIZATION                              *****
;*****                                                                  *****
;*****                                                                  *****
;****************************************************************************





;-----------------------------------------------------------------------
;
;       init page fault handling
;
;-----------------------------------------------------------------------
; PRECONDITION:
;
;       pm32
;
;       DS,ES linear space
;
;-----------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX...EBP scratch
;
;-----------------------------------------------------------------------
 
  icode


 
init_pagfault:

  mov   eax,offset page_fault_handler+KR
  mov   bl,page_fault
  mov   bh,0
  call  define_idt_gate

  ret
 

  icod  ends



;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  kcode


;----------------------------------------------------------------------------
;
;                 Page Fault Handler
;
;----------------------------------------------------------------------------
;
;       Analyzes Page Faults and passes valid Page Faults to kernel.
;
;----------------------------------------------------------------------------
; Remark:
;
;       The linear addresses 1 MB ... 1 MB + 64 KB - 1 are aliased with
;       0 ... 64 KB - 1 to emulate 8086 wrap around.
;
;----------------------------------------------------------------------------




  align 16






page_fault_handler:

  cmp   ss:[esp+iret_eip+4],PM
  xc    ae,page_fault_pl0,long


  ipre  ec_present

  mov   ebp,esp

  mov   ebx,-1
  IFNZ
        mov   ebx,[ebp+ip_eip]
  FI


  mov   edx,cr2

  mov   cl,byte ptr [ebp+ip_error_code]
  if    fpopn_write gt page_fault_due_to_write_bit
        shl   cl,fpopn_write - page_fault_due_to_write_bit
  endif
  if    fpopn_write lt page_fault_due_to_write_bit
        shr   cl,page_fault_due_to_write_bit - fpopn_write
  endif
  and   cl,page_fault_due_to_write

  and   dl,NOT 3
  or    dl,cl

  and   ebp,-sizeof tcb
  
  cmp   edx,offset small_virtual_spaces
  xc    ae,perhaps_small_pf,long
    
  
  IFB_  edx,<virtual_space_size>,long

        test  [ebp+fine_state],nlock
        IFNZ
        cmp   ebx,-1
        xc    z,occurred_within_ipc,long
        CANDNZ
        
          ;;    mov   eax,[ebp+rcv_descriptor]   ; dirty! this branch might be entered
          ;;    push  eax                        ; if deceit_pre leads to PF even though
          ;;    push  ebp                        ; status is not (yet) locked_running

              mov   esi,[ebp+pager]
              sub   edi,edi
              sub   ecx,ecx
              sub   eax,eax
              mov   ebp,32*4+map_msg

              call  kernel_ipc
              
          ;;    pop   ebp                        ; see above
          ;;    pop   ebx
          ;;    mov   [ebp+rcv_descriptor],ebx

              test  al,ipc_error_mask
              mov   al,page_fault
              jnz   exception

              ipost
        FI


        test  [ebp+ressources],in_partner_space
        jnz   partner_space_fault


        call  push_ipc_state

        IFNZ
              push  edx
              push  ebp
              push  ds

              mov   esi,[ebp+pager]
              sub   edi,edi
              sub   eax,eax
              mov   ebp,32*4+map_msg

              call  kernel_ipc

              pop   ds
              pop   ebp
              pop   edx

              test  al,ipc_error_mask
              IFZ
             ;; test__page_present edx
             ;; CANDNC

                    call  pop_ipc_state

                    ipost
              FI
        FI
        jmp   cancel_if_within_ipc

  FI


  mov   al,page_fault
  cmp   ebx,-1
  jnz   exception

  mov   eax,edx

  cmp   eax,offset small_virtual_spaces+small_virtual_spaces_size
  jb    small_space_com_write_fault 

  cmp   eax,shared_table_base
  jb    short addressing_error

  cmp   eax,shared_table_base+shared_table_size-1
  jbe   shared_table_fault

  cmp   eax,offset iopbm
  jb    short addressing_error

  cmp   eax,offset iopbm+sizeof iopbm-1
  jbe   own_iopbm_fault

  cmp   eax,offset com0_space
  jb    short addressing_error

  cmp   eax,offset com1_space+com1_space_size-1
  jbe   com_space_write_fault






addressing_error:

internal_addressing_error:

  ke    '-inv_addr'



XHEAD   occurred_within_ipc

  mov   eax,[esp+ip_eip]
  IFAE  eax,offset ipc_critical_region_begin+KR
  CANDBE eax,offset ipc_critical_region_end+KR
        sub   eax,eax     ; Z !
  FI                      ; NZ else !
  xret  ,long



  
  
;----------------------------------------------------------------------------
;
;       small space page fault
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EDX   faulting virtual address
;       EBP   tcb write addr
;
;       DS    linear_kernel_space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EDX   faulting virtual address (may be rel to large space)
;
;       EAX,EBX,ECX,ESI,EDI           scratch
;
;----------------------------------------------------------------------------
;
;       PROC small table fault (INT CONST address, attributes) :
;
;         IF current pdir has ptab for this small space address
;            THEN handle pf (address MOD small space size, attributes)
;         ELIF related large space has pdir for related large address
;            THEN copy large pdes to small pdes
;            ELSE handle pf (address MOD small space size, attributes)
;                 (* instr restart will re-raise pf which then will be
;                    resolved by 'copy ...' *)
;         FI
;
;       ENDPROC small table fault
;
;----------------------------------------------------------------------------

  
  
XHEAD perhaps_small_pf

  cmp   edx,offset small_virtual_spaces+small_virtual_spaces_size
  xret  ae,long
      

  mov   cl,ds:[log2_small_space_size_DIV_MB4]
  mov   ch,cl

  xpdir eax,edx

  mov   edi,[ebp+as_base]
  xor   edi,edx
  shr   edi,22
  shr   edi,cl

  mov   cl,32-22
  sub   cl,ch

  test  edi,edi
  IFZ
        shl   edx,cl
        shr   edx,cl
        mov   esi,ebp
  ELSE_
        mov   esi,[ebp+com_partner]
  FI

  mov   edi,ds:[cpu_cr3]
  lea   edi,[(eax*4)+edi+PM]
  test  byte ptr [edi],page_present    
  xret  nz,long
  
 
  mov   esi,[esi+thread_proot] 
  add   esi,PM
 
  mov   eax,edx
  shl   eax,cl
  shr   eax,cl
  xpdir eax,eax
  test  byte ptr [(eax*4)+esi],page_present 
  xret  z,long

  mov   cl,ch       
  sub   cl,22-2-22
  shr   edi,cl
  shl   edi,cl
  call  set_small_pde_block_in_pdir
  
  ipost
  
  
    
  












;----------------------------------------------------------------------------
;
;       shared table fault
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   faulting virtual address
;       EDX   = EAX
;       EBP   tcb write addr
;
;       DS    user_space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,ESI,EDI        scratch
;
;----------------------------------------------------------------------------
;
;       PROC shared table fault (INT CONST address, attributes) :
;
;         IF kernel has ptab for this address CAND
;            actual task has no ptab for this address
;            THEN enter kernel ptab link into actual pdir
;            ELSE decode access {and enter into kernel pdir too}
;         FI
;
;       ENDPROC shared table fault
;
;----------------------------------------------------------------------------
; shared table INVARIANT:
;
;       all shared table ptabs are linked to kernel pdir
;
;----------------------------------------------------------------------------

  align 16


shared_table_fault:

  shr   eax,22
  lea   eax,[(eax*4)+PM]
  mov   edi,ds:[kernel_proot]
  mov   ebx,[eax+edi]
  test  bl,page_present
  IFNZ
        mov   edi,cr3
        and   edi,-pagesize
        xchg  [eax+edi],ebx
        test  bl,page_present
        IFZ
              ipost
        FI
  FI
  mov   eax,edx

  cmp   eax,offset tcb_space+tcb_space_size
  jb    tcb_fault
  
  cmp   eax,offset pdir_space
  jb    addressing_error

  cmp   eax,offset pdir_space+pdir_space_size
  jb    pdir_space_fault

  cmp   eax,offset chapter_map
  jb    addressing_error
  
  cmp   eax,offset chapter_map+(max_ptabs*chapters_per_page)
  jb    pagmap_fault
  
  jmp   addressing_error




;----------------------------------------------------------------------------
;
;       own iopbm fault
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   faulting virtual address
;       EDX   = EAX
;       EBP   tcb write addr
;
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,ESI,EDI        scratch
;




own_iopbm_fault:

  ke    'iopbm_fault'

  ret



;----------------------------------------------------------------------------
;
;       com space write fault
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   faulting virtual address
;       EDX   = EAX
;       EBP   tcb write addr
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,ESI,EDI        scratch
;
;----------------------------------------------------------------------------
;
;       PROC com space write fault (addr) :
;
;         calc addr in dest task ;
;         ensure ptab existing and write mapped in dest task ;
;         copy entry .
;
;         ensure ptab existing and write mapped in dest task :
;           REP
;             IF NOT ptab existing
;               THEN map ptab ;
;                    enter new ptab into comspace
;             ELIF NOT write mapped in dest task
;               THEN user space read write fault (dest task, dest task addr, write)
;               ELSE LEAVE
;          PER .
;
;        ENDPROC com space write fault
;
;----------------------------------------------------------------------------

        align 4


com_space_write_fault:

  

  mov   esi,[ebp+com_partner]               ; com partner is tcb address

  mov   edi,[ebp+waddr]
         
  mark__ressource ebp,com_used

  sub   eax,com0_base
  CORB
  IFAE  eax,MB8
        sub   eax,com1_base-com0_base
  FI
  sub   edx,com0_base
  shr   edx,23-1
  test  edx,10b
  IFNZ
        shl   edi,16
  FI      
  
  and   edi,-MB4
  add   eax,edi


  DO
        lea___pdir ebx,esi
        xpdir ecx,eax
        mov   ebx,[(ecx*4)+ebx]
        and   bl,NOT page_user_permit
        mov   edi,ebx

        and   bl,page_present+page_write_permit
        IFZ   bl,page_present+page_write_permit
        and   ebx,-pagesize
        xptab ecx,eax
        mov   ebx,dword ptr [(ecx*4)+ebx+PM]
        and   bl,page_present+page_write_permit
        CANDZ bl,page_present+page_write_permit

              mov   [(edx*4)+pdir+(offset com0_space SHR 20)],edi

              ipost
        FI

        push  esi

        mov   edi,ebp
        mov   ebp,[ebp+com_partner]
        call  tunnel_to

        add   byte ptr [eax],0

        xchg  edi,ebp
        call  tunnel_to

        pop   esi
        REPEAT
  OD






small_space_com_write_fault:

  mov   esi,[ebp+com_partner]               ; com partner is tcb address

  sub   eax,[esi+as_base]

  mov   edi,ebp
  mov   ebp,[ebp+com_partner]
  call  tunnel_to

  add   byte ptr [eax],0

  xchg  edi,ebp
  call  tunnel_to

  ipost



;----------------------------------------------------------------------------
;
;       partner-space write fault     (msg copy   small -> large, in large as
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   faulting virtual address
;       EDX   = EAX
;       EBP   tcb write addr
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,ESI,EDI        scratch
;
;----------------------------------------------------------------------------

        align 4


partner_space_fault:

  

  mov   edi,ebp
  mov   ebp,[ebp+com_partner]
  call  tunnel_to

  add   byte ptr [eax],0

  xchg  edi,ebp
  call  tunnel_to

  ipost



  kcod ends
;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||





;----------------------------------------------------------------------------
;
;       pdir space fault
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   faulting virtual address within pdir_space
;       EDX   = EAX
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,ESI,EDI        scratch
;
;----------------------------------------------------------------------------

        align 4


pdir_space_fault:

  sub   eax,offset pdir_space
  shr   eax,12
  load__root eax,eax

  mov   esi,edx
  call  map_system_shared_page

  ipost






;----------------------------------------------------------------------------
;
;                 Special PL0 Page Fault Handling
;
;----------------------------------------------------------------------------




iret_   equ 0CFh



.erre   KR ge PM


        align 16




XHEAD page_fault_pl0

 
  test  byte ptr ss:[esp+iret_eflags+4+2],(1 SHL (vm_flag-16)) 
  xret  nz,long 
 
  push  eax 

  test  esp,(sizeof tcb-1) AND (-512)
  IFZ
  CANDA esp,<offset tcb_space>
  CANDB esp,<offset tcb_space+tcb_space_size>
        ke    'esp < 512'
  FI

  mov   eax,ss:[esp+iret_eip+4+4] 

  IFZ   eax,offset ipcman_sysexit_to_user_instruction+KR

        mov   [esp+iret_eip+4+4],offset ipc_exit
        mov   [esp+iret_cs+4+4],kernel_exec
        btr   [esp+iret_eflags+4+4],i_flag
        mov   eax,[esp+iret_esp+4+4]
        mov   [esp+iret_esp+4+4+4],eax

        pop   eax
        xret  ,long
  FI


        mov   eax,cs:[eax]
                                          ; if PF happens at IRET (in PL0)
        IFZ   al,iret_                    ; new iret vector is dropped
                                          ; and faulting vector is taken
                                          ; instead. This ensures correct
                                          ; load of seg reg.
              mov   eax,ss:[esp+4]
              or    al,page_fault_from_user_level
              mov   ss:[esp+3*4+4+4],eax

              pop   eax
              add   esp,3*4+4             ; NZ !
              xret  ,long

        FI

        and   ah,NOT 7                    ;
        IFNZ  eax,0FF0040F6h              ; test byte ptr [reg],FF
        CANDNZ eax,0FF006080h             ; and  byte ptr [reg],FF
              pop   eax                   ; are skipped upon PF and result in C
              cmp   eax,eax               ;
              xret  ,long                 ; Z !
        FI 

        push  ebx
        push  ecx

        mov   ecx,cr3
        and   ecx,-pagesize
        mov   eax,cr2
        xpdir ebx,eax

        IFAE  eax,shared_table_base
        CANDB eax,shared_table_base+shared_table_size
              mov   ecx,ss:[kernel_proot]
        FI

        mov   ecx,dword ptr ss:[(ebx*4)+ecx+PM]
        test  cl,page_present
        IFNZ
        and   ecx,-pagesize
        xptab eax,eax
        test  byte ptr ss:[(eax*4)+ecx+PM],page_present
        CANDNZ
              and   byte ptr ss:[esp+iret_eflags+4*4],NOT (1 SHL c_flag)
        ELSE_
              or    byte ptr ss:[esp+iret_eflags+4*4],1 SHL c_flag
        FI
        add   ss:[esp+iret_eip+4*4],4

        pop   ecx
        pop   ebx
        pop   eax
        add   esp,4

        iretd









  code ends
  end