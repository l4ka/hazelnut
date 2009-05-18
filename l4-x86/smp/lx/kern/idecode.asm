include lnpre.inc 
 

  Copyright IBM, LN.IDECODE, 09,02,98, 1
 
  dcode
 
;*********************************************************************
;******                                                         ******
;******         Instruction Decoder                             ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  18.01.98    ******
;******                                   modified: 12.02.98    ******
;******                                                         ******
;*********************************************************************
 


  public init_idecode


  extrn trace_eip:near
  extrn trace_data:near
  extrn define_idt_gate:near


.nolist 
include lnconst.inc
include uid.inc
include adrspace.inc
include tcb.inc
include cpucb.inc
include intrifc.inc
include syscalls.inc
.list
 

ok_for pentium,ppro


cachelinesize equ 32






;-----------------------------------------------------------------------
;
;        int identifier
;
;-----------------------------------------------------------------------
  
debug_exception   equ    1


;----------------------------------------------------------------------------
;
;       intr stack descriptions
;
;----------------------------------------------------------------------------


intr_stack struc

  intr_edi    dd 0
  intr_esi    dd 0
  intr_ebp    dd 0
              dd 0
  intr_ebx    dd 0
  intr_edx    dd 0
  intr_ecx    dd 0
  intr_eax    dd 0

  intr_eip    dd 0
  intr_cs     dw 0,0
  intr_eflags dd 0
  intr_esp    dd 0
  intr_ss     dw 0,0

intr_stack ends




idt_descriptor            df 0

idecode_idt_descriptor    df 0





opcode_type   record opc_type:4,data_width:2,access_type:2


mod_rm        equ   0 SHL opc_type
dir_mem       equ   1 SHL opc_type
pushx         equ   2 SHL opc_type
popx          equ   3 SHL opc_type
esi_access    equ   4 SHL opc_type
edi_access    equ   5 SHL opc_type
esi_edi_acc   equ   6 SHL opc_type

group1_4      equ   8 SHL opc_type
group4_8      equ   9 SHL opc_type

special       equ  13 SHL opc_type
prefix        equ  14 SHL opc_type 
_0F           equ  15 SHL opc_type 


opc_handler   dd    mod_rm_handler
              dd    dir_mem_handler
              dd    pushx_handler
              dd    popx_handler
              dd    esi_handler
              dd    edi_handler
              dd    esi_edi_handler
              dd    0
              dd    group1_4_handler
              dd    group5_8_handler
              dd    0
              dd    0
              dd    0
              dd    special_opcode
              dd    prefix_opcode
              dd    _0F_handler
              
              



byte_operand  equ   0 SHL data_width
word_operand  equ   1 SHL data_width
dword_operand equ   2 SHL data_width
qword_operand equ   3 SHL data_width

read_access   equ   01b
write_access  equ   10b
   


___           equ   0

r__           equ   read_access
w__           equ   write_access
x__           equ   read_access+write_access

rEb           equ   mod_rm+byte_operand+read_access
rEw           equ   mod_rm+word_operand+read_access
rEv           equ   mod_rm+dword_operand+read_access
rEq           equ   mod_rm+qword_operand+read_access
wEb           equ   mod_rm+byte_operand+write_access
wEw           equ   mod_rm+word_operand+write_access
wEv           equ   mod_rm+dword_operand+write_access
xEb           equ   mod_rm+byte_operand+read_access+write_access
xEv           equ   mod_rm+dword_operand+read_access+write_access

rDb           equ   dir_mem+byte_operand+read_access
rDv           equ   dir_mem+dword_operand+read_access
wDb           equ   dir_mem+byte_operand+write_access
wDv           equ   dir_mem+dword_operand+write_access

Uv            equ   pushx+dword_operand
Ov            equ   popx+dword_operand
UEv           equ   pushx+dword_operand+read_access
OEv           equ   popx+dword_operand+write_access
Uq            equ   pushx+qword_operand
Oq            equ   popx+qword_operand
UEq           equ   pushx+qword_operand+read_access
OEq           equ   popx+qword_operand+write_access

rXb           equ   esi_access+byte_operand+read_access
rXv           equ   esi_access+dword_operand+read_access
rYb           equ   edi_access+byte_operand+read_access
rYv           equ   edi_access+dword_operand+read_access
wYb           equ   edi_access+byte_operand+write_access
wYv           equ   edi_access+dword_operand+write_access
rZb           equ   esi_edi_acc+byte_operand+read_access
rZv           equ   esi_edi_acc+dword_operand+read_access
xZb           equ   esi_edi_acc+byte_operand+write_access+read_access
xZv           equ   esi_edi_acc+dword_operand+write_access+read_access

Eb1           equ   group1+byte_operand
Ev1           equ   group1+dword_operand
Eb2           equ   group2+byte_operand
Ev2           equ   group2+dword_operand
Eb3           equ   group3+byte_operand
Ev3           equ   group3+dword_operand
gr4           equ   group4
gr5           equ   group5
gr6           equ   group6
gr7           equ   group7
gr8           equ   group8



group1        equ   group1_4+00b
group2        equ   group1_4+01b
group3        equ   group1_4+10b
group4        equ   group1_4+11b
group5        equ   group4_8+00b
group6        equ   group4_8+01b
group7        equ   group4_8+10b
group8        equ   group4_8+11b


_xx           equ   prefix+1
_cs           equ   prefix+2
_ss           equ   prefix+3
_ds           equ   prefix+4
_es           equ   prefix+5
_fs           equ   prefix+6
_gs           equ   prefix+7


prefix_handler      dd    0
                    dd    _xx_handler
                    dd    _cs_handler
                    dd    _ss_handler
                    dd    _ds_handler
                    dd    _es_handler
                    dd    _fs_handler
                    dd    _gs_handler
                    
                    
                    


Ua            equ   special+0  ; pusha
Oa            equ   special+1  ; popa
Of            equ   special+2  ; popf
it3           equ   special+3  ; int 3
itn           equ   special+4  ; int n
ito           equ   special+5  ; into
bnd           equ   special+6  ; bound
irt           equ   special+7  ; iret   
xlt           equ   special+8  ; xlat
fD9           equ   special+9  ; FP D9
fDB           equ   special+10 ; FP DB
fDD           equ   special+11 ; FP DD
fDF           equ   special+12 ; FP DF
cx8           equ   special+13 ; cmpxchg8




special_handler     dd    pusha_handler
                    dd    popa_handler
                    dd    popf_handler
                    dd    int_3_handler
                    dd    int_n_handler
                    dd    into_handler
                    dd    bound_handler
                    dd    iret_handler
                    dd    xlat_handler
                    dd    FP_D9_handler
                    dd    FP_DB_handler
                    dd    FP_DD_handler
                    dd    FP_DF_handler
                    dd    cmpxchg8_handler
                    
                                                   


;           00  01  02  03  04  05  06  07   08  09  0A  0B  0C  0D  0E  0F

opc1    db xEb,xEv,rEb,rEv,___,___, Uv, Ov, xEb,xEv,rEb,rEv,___,___, Uv,_0F   ; 00
        db xEb,xEv,rEb,rEv,___,___, Uv, Ov, xEb,xEv,rEb,rEv,___,___, Uv, Ov   ; 10
        db xEb,xEv,rEb,rEv,___,___,_es,___, xEb,xEv,rEb,rEv,___,___,_cs,___   ; 20
        db xEb,xEv,rEb,rEv,___,___,_ss,___, rEb,rEv,rEb,rEv,___,___,_ds,___   ; 30
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; 40
        db  Uv, Uv, Uv, Uv, Uv, Uv, Uv, Uv,  Ov, Ov, Ov, Ov, Ov, Ov, Ov, Ov   ; 50
        db  Ua, Oa,bnd,___,_fs,_gs,_xx,_xx,  Uv,rEv, Uv,rEv,wYb,wYv,rXb,rXv   ; 60
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; 70
        db Eb1,Ev1,Ev1,Ev1,rEb,rEv,xEb,xEv, wEb,wEv,rEb,rEv,wEv,___,rEw,OEv   ; 80
        db ___,___,___,___,___,___,___,___, ___,___, Uq,___, Uv, Of,___,___   ; 90
        db rDb,rDv,wDb,wDv,xZb,xZv,rZb,rZv, ___,___,wYb,wYv,rXb,rXv,rYb,rYv   ; A0
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; B0
        db Eb2,Ev2, Ov, Ov,rEq,rEq,wEb,wEv,  Uv, Ov, Oq, Oq,it3,itn,ito,irt   ; C0
        db Eb2,Ev2,Eb2,Ev2,___,___,___,xlt, rEv,fD9,rEv,fDB,rEq,fDD,rEw,fDF   ; D0
        db ___,___,___,___,___,___,___,___,  Uv,___,___,___,___,___,___,___   ; E0
        db _xx,___,_xx,_xx,___,___,Eb3,Ev3, ___,___,___,___,___,___,gr4,gr5   ; F0



;           00  01  02  03  04  05  06  07   08  09  0A  0B  0C  0D  0E  0F

opc2    db gr6,___,___,rEw,___,___,___,___, ___,___,___,___,___,___,___,___   ; 00
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; 10
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; 20
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; 30
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; 40
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; 50
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; 60
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; 70
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; 80
        db ___,___,___,___,___,___,___,___, wEb,wEb,wEb,wEb,wEb,wEb,wEb,wEb   ; 90
        db  Uv, Ov,___,rEv,xEv,xEv,___,___,  Uv, Ov,___,xEv,xEv,xEv,___,rEv   ; A0
        db xEb,xEv,rEq,xEv,rEq,rEv,rEb,rEw, ___,___,gr8,xEv,rEv,rEv,rEb,rEw   ; B0
        db xEb,xEv,___,___,___,___,___,cx8, ___,___,___,___,___,___,___,___   ; C0
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; D0
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; E0
        db ___,___,___,___,___,___,___,___, ___,___,___,___,___,___,___,___   ; F0


;          000 001 010 011 100 101 110 111  

grpx    db x__,x__,x__,x__,x__,x__,x__,r__  ; 1
        db x__,x__,x__,x__,x__,x__,___,x__  ; 2
        db r__,___,x__,x__,r__,r__,r__,r__  ; 3
        db xEb,xEb,___,___,___,___,___,___  ; 4
        db xEv,xEv,UEv,UEq,rEv,rEq,UEv,___  ; 5
        db wEw,wEw,rEw,rEw,rEw,rEw,___,___  ; 6
        db ___,___,___,___,wEw,___,rEw,___  ; 7
        db ___,___,___,___,rEv,xEv,xEv,xEv  ; 8







;----------------------------------------------------------------------------------
;
;       instruction decoder
;
;----------------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   instruction address
;       EBP   pointer to intr_...
;
;                                              
;




idecode_handler:

  pushad
  
  mov   eax,dr6
  test  ah,40h
  jnz   non_single_step_debug_exception

  call  trace_eip
  
  sub   eax,eax
  sub   edx,edx
  
  
idecode1:

  mov   al,ds:[esi]
  inc   esi
  
  mov   al,ss:[eax+opc1+PM]
  mov   ch,al
  shr   eax,opc_type
  IFNZ  ch,___
        jmp   ss:[eax*4+opc_handler+PM]
  FI
  ret
        
        
        
    
  
  
XHEAD decode_sib_byte

  push  ecx
  
  mov   al,ds:[esi]
  inc   esi
  
  mov   cl,al
  mov   ch,al
  shr   cl,6
  and   al,111b SHL 3
  shr   al,3
  and   ch,7
  and   al,7
  xor   ch,7
  xor   al,7
  
  IFNZ  al,100b XOR 7
        mov   eax,ss:[eax*4+ebp+intr_edi]
        shl   eax,cl
        add   edi,eax
  FI

  mov   al,ch
  
  pop   ecx
  
  cmp   al,100b XOR 7
  xret  nz
  
  call  implicit_ss
  xret      
  
  
  
  

mod_rm_handler:
    
  mov   al,ds:[esi]
  inc   esi
  
  mov   dh,al
  and   al,7
  shr   dh,6
  xor   al,7
  
  IFZ   dh,11b
        ret
  FI


  sub   edi,edi  
  
  cmp   al,100b XOR 7
  xc    z,decode_sib_byte
  
  IFZ   al,101b XOR 7
        IFZ   dh,0                             ; no base, 32-bit offset
              add   edi,ds:[esi]               ; 
              add   esi,4                      ;
        ELSE_
              call  implicit_ss                ; base: ss:ebp
              add   edi,ss:[eax*4+ebp+intr_edi]  ;
        FI                                     ;
  ELSE_
        add   edi,ss:[eax*4+ebp+intr_edi]        ; base: ds:reg
  FI                                           ;
  
  cmp   cl,01b
  IFZ
        movsx edx,byte ptr ds:[esi]            ; 8-bit offset
        inc   esi                              ;
        add   edi,edx                          ;
  ELIFA   
        add   edi,ds:[esi]                     ; 32-bit offset
        add   esi,4                            ;
  FI                                           ;
  
  


access_data:

  and   edx,-8
  IFZ
        mov   edx,ds
        and   edx,-8
  FI
  
  mov   ah,byte ptr ss:[edx+gdt+7]
  mov   al,byte ptr ss:[edx+gdt+4]
  shl   eax,16
  mov   ax,word ptr ss:[edx+gdt+2]
  add   edi,eax

  mov   cl,ch
  and   ch,mask access_type
  and   cl,mask data_width
  shr   cl,data_width
  mov   edx,1
  shl   edx,cl
  add   edx,edi
  
  xor   edx,edi
  test  edx,-cachelinesize
  jz    trace_data
  
  call  trace_data
  add   edi,cachelinesize
  jmp   trace_data

  
  
  
  
  
  
implicit_ss:

  test  dl,dl
  jnz   short explicit_ss
  ret
  
  
  
explicit_ss:
  
  mov   dl,byte ptr ss:[ebp+intr_ss]      
  test  byte ptr ss:[ebp+intr_cs],11b
  IFNZ
        ret
  FI
        
  push  eax
  mov   eax,ss
  mov   dl,al
  pop   eax
  ret       
  
  
  
  
  
  
  
          
  
  
  

dir_mem_handler:

  add   edi,ds:[esi]
  add   esi,4
  jmp   access_data
  
  
  
  

pushx_handler:

  push  ecx
  push  edx
  push  edi
  
  and   ch,NOT mask access_type
  or    ch,write_access
  
  lea   edi,[ebp+intr_esp]
  test  byte ptr ss:[ebp+intr_cs],11b
  IFNZ
        mov   edi,ss:[ebp+intr_esp]
  FI
        
  call  explicit_ss      
  sub   edi,4
  call  access_data
  
  pop   edi
  pop   edx
  pop   ecx
  
  test  ch,mask access_type
  jnz   mod_rm_handler
  
  ret
  
  


popx_handler:

  push  ecx
  push  edx
  push  edi
  
  and   ch,NOT mask access_type
  or    ch,read_access

  lea   edi,[ebp+intr_esp]
  test  byte ptr ss:[ebp+intr_cs],11b
  IFNZ
        mov   edi,ss:[ebp+intr_esp]
  FI

  call  explicit_ss
  call  access_data
  
  pop   edi
  pop   edx
  pop   ecx
  
  test  ch,mask access_type
  jnz   mod_rm_handler
  
  ret
  




esi_handler:

  mov   edi,ss:[ebp+intr_esi]
  jmp   access_data



esi_edi_handler:

  push  ecx
  push  edx
  
  and   ch,NOT mask access_type
  or    ch,read_access
  mov   edi,ss:[ebp+intr_esi]
  call  access_data
  
  pop   edx
  pop   ecx
  
  test  ch,write_access
  IFNZ
        and   ch,NOT read_access
  FI      


edi_handler:

  mov   edx,es
  mov   edi,ss:[ebp+intr_edi]
  jmp   access_data
  
    








_0F_handler:

  mov   al,ds:[esi]
  inc   esi
  
  mov   al,ss:[eax+opc2+PM]
  mov   ch,al
  shr   eax,opc_type
  IFNZ  ch,___
        jmp   ss:[eax*4+opc_handler+PM]
  FI
  ret





  
group1_4_handler:

  and   ch,11b
  jmp   short group_n_handler
  
  
  
group5_8_handler:

  and   ch,11b
  add   ch,4
  
group_n_handler:

  shl   ch,3
  mov   al,ds:[esi]
  shr   al,3
  and   al,7
  add   al,ch
  mov   al,ss:[eax+grpx+PM]  

  mov   ch,al
  shr   eax,opc_type
  IFNZ  ch,___
        jmp   ss:[eax*4+opc_handler+PM]
  FI
  ret





prefix_opcode:

  mov   al,ch
  and   al,0Fh
  jmp   ss:[eax*4+prefix_handler+PM]
  



_xx_handler:

  ret
  

_ss_handler:
  call  explicit_ss
  jmp   idecode1
  
  
_ds_handler:
  mov   edx,ds
  jmp   idecode1      
 
_cs_handler:
  mov   edx,cs
  jmp   idecode1      
 
_es_handler:
  mov   edx,es
  jmp   idecode1 
  
_fs_handler:
  mov   edx,fs
  jmp   idecode1      
 
_gs_handler:
  mov   edx,gs
  jmp   idecode1      
 
       
 
  
  
  



special_opcode:

  mov   al,ch
  and   al,0Fh
  jmp   ss:[eax*4+special_handler+PM]
  
  
  
  
  
  
  
  
pusha_handler:

  mov   ch,qword_operand+write_access
  
  lea   edi,[ebp+intr_esp]
  test  byte ptr ss:[ebp+intr_cs],11b
  IFNZ
        mov   edi,ss:[ebp+intr_esp]
  FI
        
  call  explicit_ss      
 
  mov   cl,4
  DO
        push  ecx
        push  edx
        push  edi
        sub   edi,2*4
        call  access_data
        pop   edi
        pop   edx
        pop   ecx
        dec   cl
        REPEATNZ
  OD
  ret      
  



popa_handler:

  mov   ch,qword_operand+read_access
  
  lea   edi,[ebp+intr_esp]
  test  byte ptr ss:[ebp+intr_cs],11b
  IFNZ
        mov   edi,ss:[ebp+intr_esp]
  FI
        
  call  explicit_ss      
 
  mov   cl,4
  DO
        push  ecx
        push  edx
        push  edi
        call  access_data
        pop   edi
        pop   edx
        pop   ecx
        add   edi,2*4
        dec   cl
        REPEATNZ
  OD
  ret      
  



popf_handler:
  
  CORNZ esi,offset idecode_off_popfd
  test  byte ptr ss:[ebp+intr_cs],11b
  IFNZ
        sub   eax,eax
        call  ensure_single_step_on
  FI
  mov   ch,dword_operand
  jmp   popx_handler
  
  
  

int_3_handler:

  ke    'int 3'
  ret
  
  
int_n_handler:

  ke    'int n'
  ret
  
into_handler:

  ke    'into'
  ret
  
  
bound_handler:

  ke    'bound'
  ret
  
  
iret_handler:

  mov   eax,2*4
  call  ensure_single_step_on
  
  mov   ch,qword_operand
  jmp   popx_handler
  
  
  
xlat_handler:

  ke    'xlat'
  ret


FP_D9_handler:
FP_DB_handler:
FP_DD_handler:
FP_DF_handler:

  ke    'FP instr'
  ret
  
  
cmpxchg8_handler:

  ke    'cmpx8'
  ret



    



ensure_single_step_on:

  lea   edi,[ebp+intr_esp]
  test  byte ptr ss:[ebp+intr_cs],11b
  IFZ
        or    byte ptr ss:[eax+ebp+intr_esp+1],1 SHL (t_flag-8)
        ret
  FI
  
  mov   edi,ss:[ebp+intr_esp]
  or    byte ptr ds:[eax+edi+1],1 SHL (t_flag-8)
  ret  



;--------------------------------------------------------------------------
;
;       init idecoder
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       DS     linear kernel space
;
;---------------------------------------------------------------------------


init_idecode:

  pushad
  
  mov   edi,offset idecode_idt_descriptor+PM
  mov   word ptr [edi],idt_entries*8-1
  mov   dword ptr [edi+2],offset idecode_idt+PM
  
  mov   edi,offset idecode_idt+PM+(debug_exception*8)
  mov   dword ptr [edi],offset idecode_handler
  mov   dword ptr [edi+4],offset idecode_handler+PM

  mov   edi,offset idecode_idt+PM
  mov   ecx,idt_entries
  DO
        mov   word ptr ds:[edi+2],phys_mem_exec
        mov   word ptr ds:[edi+4],8E00h
        add   edi,8
        dec   ecx
        REPEATNZ
  OD
  
  popad
  ret
  


;--------------------------------------------------------------------------
;
;       instruction decode on/off
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       SS     linear_kernel_space
;
;---------------------------------------------------------------------------


idecode_on:

  sidt  ss:[idt_descriptor+PM]
  lidt  ss:[idecode_idt_descriptor+PM]
  
  pushfd
  or    byte ptr ss:[esp+1],1 SHL (t_flag-8)
  popfd
  
  ret
  
  
  
idecode_off:

  pushfd
  and    byte ptr ss:[esp+1],NOT (1 SHL (t_flag-8))
  idecode_off_popfd:
  popfd
  
  lidt  ss:[idt_descriptor+PM]
  
  ret
          
  
  
non_single_step_debug_exception:

  and   ah,NOT 40h
  mov   dr6,eax
  popad
  
  pushfd
  push  eax
  push  ebx
  
  mov   ebx,dword ptr ss:[idt_descriptor+2+PM]
  
  mov   eax,ss:[ebx+debug_exception*8]
  mov   ebx,ss:[ebx+debug_exception*8+4]
  
  mov   bx,ax
  shr   eax,16
  
  xchg  ss:[esp+1*4],eax
  xchg  ss:[esp],ebx
  
  iretd

  
  
  
  
          
.listmacro          

 FOR nnn,<0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17>
 
  idt_call_&nnn:
  push  nnn
  jmp   idt_call
  endm    

.list

.nolistmacro  
  
  
idt_call:

  sub   esp,2*4
  pushad
  
  mov   ebx,ss:[esp+intr_eflags]
  

idt_call_ebx:  
  
  pushfd
  pop   eax
  or    ah,1 SHL (t_flag-8)  
  mov   ss:[esp+intr_eflags],eax
  
  shl   ebx,3
  add   ebx,dword ptr ss:[idt_descriptor+2+PM]
  
  mov   eax,ss:[ebx]
  mov   ebx,ss:[ebx+4]
  
  test  bh,bh
;;;  IFS
  
  mov   bx,ax
  shr   eax,16
  
  mov   dword ptr ss:[esp+intr_cs],eax
  mov   ss:[esp+intr_eip],ebx
  
  popad
  iretd




gp_handler:

  sub   esp,2*4
  pushad
    
  mov   eax,dword ptr ss:[esp+8*4]
  
;;  test  al,mask error_code_idt_bit
  IFNZ
  CANDA ax,<word ptr ss:[idecode_idt_descriptor+PM]>
  CANDB ax,<word ptr ss:[idt_descriptor+PM]>
    
;;;        and   eax,mask error_code_selector_index
        add   eax,dword ptr ss:[idt_descriptor+2+PM]      
              
        mov   ebx,ss:[eax+4]
        mov   eax,ss:[eax]
  
        test  bh,bh
        IFS
              and   bh,11b
              mov   bl,byte ptr ss:[esp+intr_cs+3*4]
              shr   bh,5
              and   bl,11b
              IFBE  bl,bh
              
                    pushfd
                    pop   ecx
                    mov   ss:[esp+intr_eflags],ecx
                    mov   bx,ax
                    shr   eax,16
                    mov   dword ptr ss:[esp+intr_cs],eax
                    mov   ss:[esp+intr_eip],ebx
                    
                    popad
                    iretd
              FI
        ELSE_        
              popad
              add   esp,2*4
              push  seg_not_present
              jmp   idt_call             
        FI      
  FI              
                           
  popad
  add   esp,2*4
  push  general_protection
  jmp   idt_call
  
  
  
  



  


  
                         
  
  
              align 16
              
.listmacro
  

idecode_idt   dd    idt_call_0,idt_call_0+PM
              dd    idecode_handler,idecode_handler+PM
              FOR   nnn,<2,3,4,5,6,7,8,9,10,12>
              dd    idt_call_&nnn,idt_call_&nnn+PM
              endm
              dd    gp_handler,gp_handler+PM
              FOR   nnn,<14,15,15,16,17>
              dd    idt_call_&nnn,idt_call_&nnn+PM
              endm
              
idt_entries   equ (($-idecode_idt)/8)              


.nolistmacro






  dcod ends
  code ends
  end
