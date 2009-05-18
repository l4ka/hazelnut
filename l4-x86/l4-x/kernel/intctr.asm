include l4pre.inc 

 
  Copyright IBM, L4.INTCTR, 18,12,00, 61
 
 
;*********************************************************************
;******                                                         ******
;******         Interrupt Controller                            ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 18.12.00    ******
;******                                                         ******
;*********************************************************************


 
  public init_intctr
  public define_idt_gate
  public emu_lidt_eax
  public exception
 
 
 ;extrn intr_from_v86:near
  extrn shutdown_thread:near
  extrn machine_check_exception:near
 
.nolist 
include l4const.inc 
include uid.inc
include ktype.inc
include adrspace.inc
include tcb.inc
include pagmac.inc
.list
include segs.inc
include intrifc.inc
include syscalls.inc
.nolist
include kpage.inc
.list
 

  extrn make_own_address_space_large:near


ok_for x86
 
 
  icode

 
idtvec  dw sizeof idt-1
        dd offset idt
 
 
        align 4
 
 
 
r32               equ  0100000000010000b
rw32              equ  0100000000010010b
r16               equ             10000b
rw16              equ             10010b
 
x32               equ  0100000000011000b
xr32              equ  0100000000011010b
x16               equ             11000b
xr16              equ             11010b
 
ldtseg            equ  2
taskgate          equ  5
tsseg             equ  9
callgate          equ  0Ch
intrgate          equ  0Eh
trapgate          equ  0Fh
 

ar_byte        record dpresent:1,dpl:2,dtype:4,daccessed:1

d_bit          equ 22

 
;---------------------------------------------------------------------------
;
;        descriptor privilege levels codes
;
;---------------------------------------------------------------------------
 
dpl0    equ 0 shl 5
dpl1    equ 1 shl 5
dpl2    equ 2 shl 5
dpl3    equ 3 shl 5



;-----------------------------------------------------------------------
;
;       init interrupt controller
;
;-----------------------------------------------------------------------
; PRECONDITION:
;
;       protected mode
;
;       paging enabled, adrspace established
;
;       disable interrupt
;
;       DS,ES  linear space
;
;-----------------------------------------------------------------------
; POSTCONDITION:
;
;       IDT     initialized
;       IDTR    initialized
;
;       EAX...EBP scratch
;
;----------------------------------------------------------------------
 

 assume ds:codseg
 
 
init_intctr:

  sub   eax,eax
  mov   edi,offset idt
  mov   ecx,sizeof idt/4
  cld
  rep   stosd

  mov   bl,0
  mov   esi,offset initial_idt+PM
  DO
        mov   eax,[esi]
        mov   bh,[esi+4]
        call  define_idt_gate
        inc   bl
        add   esi,5
        cmp   esi,offset end_of_initial_idt+PM
        REPEATB
  OD

  lidt  fword ptr ds:[idtvec+PM]

  ret


 
 
        align 4


initial_idt    dd offset divide_error_handler
               db dpl3
               dd offset initial_debug_exception_handler
               db dpl0
               dd 0
               db dpl0
               dd offset breakpoint_handler
               db dpl3
               dd offset overflow_handler
               db dpl3
               dd offset bound_check_handler
               db dpl3
               dd offset invalid_opcode_handler
               db dpl0
               dd 0
               db dpl0
               dd offset double_fault_handler
               db dpl0
               dd 0
               db dpl0
               dd 0
               db dpl0
               dd offset seg_not_present_handler
               db dpl0
               dd offset stack_exception_handler
               db dpl0
               dd 0
               db dpl0
               dd 0
               db dpl0
               dd 0
               db dpl0
               dd offset co_error_handler
               db dpl0
               dd offset alignment_check_handler
               db dpl0
               dd offset machine_check_exception
               db dpl0

end_of_initial_idt  equ $


  icod  ends



;--------------------------------------------------------------------------
;
;       define idt gate
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX    offset
;       BL     interrupt number
;       BH     dpl
;
;       DS     linear_space
;
;---------------------------------------------------------------------------

 
 assume ds:codseg
 
 
define_idt_gate:
 
  push  ebx
  push  edi

  movzx edi,bl
  shl   edi,3
  add   edi,offset idt

  shld  ebx,eax,16
  rol   ebx,16
  rol   eax,16
  mov   ax,phys_mem_exec
  rol   eax,16
  mov   bl,0
  add   bh,80h+intrgate

  mov   [edi],eax
  mov   [edi+4],ebx

  pop   edi
  pop   ebx

  ret



;--------------------------------------------------------------------------
;
;       multi level interrupt switches
;
;--------------------------------------------------------------------------

  assume ds:nothing


  icode


initial_debug_exception_handler:


  ipre  debug_ec

  mov   al,debug_exception
  jmp   exception


  icod  ends




        align 16




multi_handler macro intr,icode

  align 4

intr&_handler:

  IFIDN <icode>,<ec_present>
        mov   byte ptr ss:[esp+3],hardware_ec
  ELSE
        push  icode
  ENDIF
  pushad
  mov   al,intr
  jmp   short multi_exception
  endm




  multi_handler divide_error,fault
  multi_handler breakpoint,trap1
  multi_handler overflow,fault
  multi_handler bound_check,fault
  multi_handler invalid_opcode,fault
  multi_handler double_fault,ec_present
  multi_handler stack_exception,ec_present
  multi_handler seg_not_present,ec_present
  multi_handler co_error,fault
  multi_handler alignment_check,ec_present




;----------------------------------------------------------------------------
;
;       std exception handler
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       stack like ipre, (multi exception: but seg regs not yet pushed!)
;
;       AL     intr number
;
;----------------------------------------------------------------------------






        align 16





multi_exception:

  push  ds
  push  es


exception:


  mov   ebp,esp
  and   ebp,-sizeof tcb

  and   eax,000000FFh


  CORZ  al,seg_not_present      ; ensures that seg regs are valid
  CORZ  al,stack_exception      ;
  IFZ   al,general_protection   ; recall: linear_space is only valid
      
        movzx ebx,word ptr ss:[esp+ip_error_code]
        
        IFB_  ebx,sizeof gdt 
                                               ;         data segment
           ;   mov   ecx,linear_space           ;
           ;   CORNZ [esp+ip_ds],ecx            ;
           ;   mov   edx,es                     ;
           ;   IFNZ  edx,ecx                    ;
           ;         test  byte ptr ss:[esp+ip_cs],11b
           ;         IFNZ                       ;
           ;               mov   [esp+ip_ds],ecx;     do not update DS
           ;               mov   [esp+ip_es],ecx;     do not update ES
           ;                                    ;     if within kernel !
           ;               ipost                ;     (DS might be kernel seg)
           ;         FI                         ;
           ;   FI                               ;


              IFBE  word ptr ss:[esp+ip_ds],3
                    mov   [esp+ip_ds],linear_space
                    ipost
              FI
              IFBE  word ptr ss:[esp+ip_es],3
                    mov   [esp+ip_es],linear_space
                    ipost
              FI

              if    random_sampling OR fast_myself
                    mov   edx,gs
                    IFNZ  edx,sampling_space
                          push  sampling_space
                          pop   gs
                          ipost
                    FI
              endif

 
              test  ebx,ebx
              IFZ   
              test  byte ptr [esp+ip_eflags+2],1 SHL (vm_flag-16) 
              CANDZ 

              lno___task edi,esp 
              test  byte ptr ss:[edi*8+task_proot+3].switch_ptr,80h 
              CANDNZ 

                    push  linear_kernel_space
                    pop   ds 
              
                    call  make_own_address_space_large 
              
                    ipost
              FI 
        FI       
  FI


  cmp   esp,PM
  jae   kd_exception

  test  byte ptr [esp+ip_cs],11b
  jz    kd_exception

  mov   ebp,[ebp+thread_idt_base]
  test  ebp,ebp
  jz    short perhaps_kd_exception


                                      ; note: define_pl3_idt ensures that
  lea   edi,[eax*8+ebp]               ; idt_base is always valid
                                      ; (inside virtual_space)
  mov   ebx,[edi+4]
  mov   bx,[edi]

  test  ebx,ebx
  jz    short perhaps_kd_exception
  cmp   ebx,virtual_space_size
  ja    short perhaps_kd_exception



  mov   edx,[esp+ip_esp]

  bt    [esp+ip_eflags],vm_flag
  IFC
        ke    'v86_exc'
  FI
  ;;;;; jc    intr_from_v86

  sub   edx,3*4
  jc    short perhaps_kd_exception
  
  mov   edi,edx

  IFAE   al,8
  CANDBE al,17
  CANDNZ al,16
        sub   edi,4
        jc    short perhaps_kd_exception
        movzx eax,word ptr [esp+ip_error_code]
        mov   [edi],eax
  FI

  mov   eax,[esp+ip_eip]
  mov   [edx+iret_eip],eax
  mov   cx,[esp+ip_cs]
  mov   [edx+iret_cs],cx
  mov   eax,[esp+ip_eflags]
  mov   [edx+iret_eflags],eax

  btr   eax,t_flag
  mov   [esp+ip_eflags],eax
  mov   [esp+ip_eip],ebx
  mov   [esp+ip_cs],cx
  mov   [esp+ip_esp],edi

  ipost







perhaps_kd_exception:


  movzx ebx,ss:[logical_info_page].kdebug_permissions.max_task
  test  ebx,ebx
  IFNZ
        lno___task ecx,esp
        cmp   ecx,ebx
        ja    shutdown_thread
  FI        



kd_exception:

  push  eax
  push  0
  jmp   dword ptr ss:[logical_info_page].kdebug_exception








;----------------------------------------------------------------------------
;
;       emulate LIDT [EAX]
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   instruction SHR 8
;       EDI   REG addr
;
;       DS    linear space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       AL    instruction length
;       EBP   scratch
;
;----------------------------------------------------------------------------






emu_lidt_eax:

  mov   ebp,esp
  and   ebp,-sizeof tcb

  mov   eax,ss:[edi+7*4]

  CORA  eax,<virtual_space_size-6>
  mov   eax,[eax+2]
  IFA   eax,<virtual_space_size-32*8>
        sub   eax,eax
  FI

  mov   [ebp+thread_idt_base],eax

  mov   eax,3
  ret





 
 
 
 

  code ends
  end
