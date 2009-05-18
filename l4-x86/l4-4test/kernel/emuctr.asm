include l4pre.inc 


  Copyright GMD, L4.EMUCTR, 03,07,96, 40028
 
 
;*********************************************************************
;******                                                         ******
;******         Emulation Controller                            ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  06.06.91    ******
;******                                   modified: 03.07.96    ******
;******                                                         ******
;*********************************************************************
 
 
; 31.10.94  jl  :  rdmsr, wrmsr  emulated on pentium for pl3 threads in kernel task

  public init_emuctr


  extrn define_idt_gate:near
  extrn exception:near

 ;public signal_virtual_interrupt

  extrn emu_load_dr:near
  extrn emu_store_dr:near
  extrn emu_lidt_eax:near

 ;extrn sti_v86emu:near
 ;extrn cli_v86emu:near
 ;extrn lock_v86emu:near
 ;extrn pushf_v86emu:near
 ;extrn pushfd_v86emu:near
 ;extrn popf_v86emu:near
 ;extrn popfd_v86emu:near
 ;extrn int_v86emu:near
 ;extrn iret_v86emu:near



.nolist 
include l4const.inc
include uid.inc
include adrspace.inc
include intrifc.inc
include tcb.inc
include cpucb.inc
include syscalls.inc
.list
 

ok_for i486,pentium,ppro,k6


cli_    equ     0FAh
sti_    equ     0FBh
lock_   equ     0F0h
pushf_  equ     09Ch
popf_   equ     09Dh
int_    equ     0CDh
iret_   equ     0CFh

osp_    equ     066h
asp_    equ     067h
rep_    equ     0F3h
repne_  equ     0F2h
es_     equ     026h
ss_     equ     036h
cs_     equ     02Eh
ds_     equ     03Eh

nop_    equ     090h

ldr_    equ     230Fh
sdr_    equ     210Fh
scr_    equ     200Fh

wrmsr_  equ     300Fh
rdmsr_  equ     320Fh



  align 16


;v86_emu_tab     dd gp_exception                   ;  0
;                dd gp_exception                   ;  1
;                dd gp_exception                   ;  2
;                dd gp_exception                   ;  3
;                dd gp_exception                   ;  4
;                dd gp_exception                   ;  5
;                dd gp_exception                   ;  6
;                dd gp_exception                   ;  7
;                dd gp_exception                   ;  8

;v86_emu_tab     dd gp_exception                   ;  0
;                dd int_v86emu                     ;  1
;                dd iret_v86emu                    ;  2
;                dd pushf_v86emu                   ;  3
;                dd popf_v86emu                    ;  4
;                dd cli_v86emu                     ;  5
;                dd sti_v86emu                     ;  6
;                dd lock_v86emu                    ;  7
;                dd osp_v86operation               ;  8



nil             equ 0
int_op          equ 1
iret_op         equ 2
pushf_op        equ 3
popf_op         equ 4
cli_op          equ 5
sti_op          equ 6
lock_op         equ 7
os_pre          equ 8




opcode_type     db 16 dup (0)                                        ; 80
                db 0,0,0,0,0,0,0,0, 0,0,0,0,pushf_op,popf_op,0,0     ; 90
                db 16 dup (0)                                        ; A0
                db 16 dup (0)                                        ; B0
                db 0,0,0,0,0,0,0,0, 0,0,0,0,0,int_op,0,iret_op       ; C0
                db 16 dup (0)                                        ; D0
                db 16 dup (0)                                        ; E0
                db lock_op,0,0,0,0,0,0,0                             ; F0
                db 0,0,cli_op,sti_op,0,0,0,0                         ;*F8





;----------------------------------------------------------------------------
;
;       init emu ctr
;
;----------------------------------------------------------------------------



 assume ds:codseg,ss:codseg


  icode


init_emuctr:

  mov   bl,general_protection
  mov   bh,0 SHL 5
  mov   eax,offset general_protection_handler+PM
  call  define_idt_gate

 ; bt    ds:[cpu_feature_flags],enhanced_v86_bit
 ; IFC
 ;                                db    0Fh,20h,0E0h
 ;     ; mov   eax,cr4
 ;       bts   eax,0              ; enable enhanced v86 features
 ;     ; mov   cr4,eax
 ;                                db    0Fh,22h,0E0h
 ; FI
 
  ret


  icod  ends


;----------------------------------------------------------------------------
;
;       signal virtual interrupt
;
;----------------------------------------------------------------------------


  align 16



;signal_virtual_interrupt:
;
;  push  ebp
;  mov   ebp,esp
;  and   ebp,-sizeof tcb
;
;  lea___ip_bottom ebp,ebp
;  bts   [ebp+iv_eflags],vip_flag
;
;  pop   ebp
;  ret




;----------------------------------------------------------------------------
;
;       general protection handler
;
;----------------------------------------------------------------------------


  align 16


;XHEAD virtual_interrupt_pending
;
;  test  eax,(1 SHL vif_flag)
;  xret  z
;
;  btr   [esp+iv_eflags],vip_flag
;  mov   al,nmi
;  jmp   exception



           

  align 16




general_protection_handler:

  ipre  ec_present,no_ds_load 

  test  byte ptr [esp+ip_cs],11b
  IFNZ
  mov   ebp,[esp+ip_error_code]
  test  ebp,0000FFFFh
  CANDZ
 
     ;   mov   eax,ss:[esp+ip_eflags]
     ;   test  eax,(1 SHL vm_flag)
     ;   IFNZ
     ;                                        ; note: V86 has always large 
     ;         test  eax,(1 SHL vip_flag)           ;       space! 
     ;         xc    nz,virtual_interrupt_pending   ;
	   ;
     ;         movzx eax,[esp+iv_ip]
     ;         movzx esi,[esp+iv_cs]
     ;         add   esi,esi
     ;         mov   eax,ds:[eax+(esi*8)]
	   ;
     ;         cmp   al,0Fh
     ;         jz    short prefix_0F_operation
	   ;
     ;         movzx ebp,al
     ;         movzx ebp,ss:[ebp+opcode_type-80h+PM]
     ;         jmp   ss:[(ebp*4)+v86_emu_tab+PM]
     ;   FI
 
        test  byte ptr [esp+ip_cs],11b
        IFNZ 
              mov   esi,[esp+ip_eip]
              mov   eax,ds:[esi]
 
              cmp   al,0Fh
              jz    short prefix_0F_operation
        FI 
  FI 


gp_exception:

  mov   al,general_protection
  jmp   exception





        align 16


;;osp_v86operation:
;;
;;cmp   ah,pushf_
;;jz    pushfd_v86emu
;;cmp   ah,popf_
;;jz    popfd_v86emu

  clign 4









prefix_0F_operation:


  lea   edi,[esp+ip_edi]
  shr   eax,8

  IFZ   al,<HIGH ldr_>
        call  emu_load_dr

  ELIFZ al,<HIGH sdr_>
        call  emu_store_dr

  ELIFZ ax,1801h
        call  emu_lidt_eax

  IF    kernel_type NE i486
  ELIFZ al,<HIGH rdmsr_>
        call  emu_rdmsr
  ELIFZ al,<HIGH wrmsr_>
        call  emu_wrmsr
  ENDIF            

  ELSE_
        jmp   gp_exception

  FI
  movzx eax,al
  add   [esp+ip_eip],eax


  clign 16


gp_return:

  ipost


  IF    kernel_type NE i486
  
  
emu_rdmsr:

  mov   ecx,ss:[edi+ip_ecx-ip_edi]
  rdmsr
  mov   ss:[edi+ip_eax-ip_edi],eax
  mov   ss:[edi+ip_edx-ip_edi],edx
  mov   al,2
  
  ret
  
  
  
emu_wrmsr:

  mov   eax,ss:[edi+ip_eax-ip_edi]
  mov   ecx,ss:[edi+ip_ecx-ip_edi]
  mov   edx,ss:[edi+ip_edx-ip_edi]
  wrmsr
  mov   al,2
  ret
  
  
  ENDIF  
   





  code ends
  end
