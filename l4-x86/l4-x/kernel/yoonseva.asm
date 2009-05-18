include l4pre.inc 


  Copyright IBM, L4.YOONSEVA, 25,01,98, 1  
 
 
;*********************************************************************
;******                                                         ******
;******     Yoonho's and Seva's Real Mode INT n handler         ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 25.01.98    ******
;******                                                         ******
;*********************************************************************
 


  public real_mode_int_n



.nolist
include l4const.inc
include uid.inc
include ktype.inc
include adrspace.inc
include tcb.inc
include cpucb.inc
include segs.inc
include intrifc.inc
.list


ok_for x86


  
  assume ds:codseg



;-------------------------------------------------------------------------
;
;        descriptor types
;
;-------------------------------------------------------------------------
 
 
r32               equ  0100000000010000b
rw32              equ  0100000000010010b
r16               equ             10000b
rw16              equ             10010b
 
x32               equ  0100000000011000b
xr32              equ  0100000000011010b
x16               equ             11000b
xr16              equ             11010b
 



;----------------------------------------------------------------------------
;
;       descriptor entry
;
;----------------------------------------------------------------------------


descriptor macro dtype,dpl,dbase,dsize

  dw    lowword        (dsize-1)
  dw    lowword        dbase
  db    low highword   dbase
  db    low            (dtype+dpl+80h)
  db    high           (dtype + highword (dsize-1))
  db    high highword  dbase

  endm


 


;----------------------------------------------------------------------------
;
;       data
;
;----------------------------------------------------------------------------



              

pe_bit        equ 0
pg_bit        equ 31


intermediate_ds_64K_base_0      equ 8
intermediate_cs_64K_base_0      equ 16



                          dd    128 dup (0)
real_mode_stack           dd    0

pm_esp                    dd    0
pm_edi                    dd    0
pm_cr3                    dd    0
pm_gdt_ptr                df    0
pm_idt_ptr                df    0
         
intermediate_cr3          dd    0         
intermediate_gdt_ptr      dw    3*8-1
                          dd    offset intermediate_gdt


real_mode_idt_ptr         df    256*4-1




intermediate_gdt          dd    0,0            ; dummy seg

  descriptor rw16, 0, 0,  KB64                 ; 08 : 64 K data seg
  descriptor xr16, 0, 0,  KB64                 ; 10 : 64 K code seg






;----------------------------------------------------------------------------
;
;       Real Mode INT n handler
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       AH    n (Int # )
;       EDI   addr of int_pm
;
;       DS    linear space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       AL    instruction length
;
;----------------------------------------------------------------------------


real_mode_int_n:


  mov   ecx,cr3
  mov   [pm_cr3+PM],ecx
  
  mov   ecx,dword ptr ds:[kernel_proot]        ; switch to kernel_proot to access lowest MB identy-mapped
  mov   cr3,ecx
  mov   [intermediate_cr3+PM],ecx
  
  mov   [pm_esp+PM],esp
  mov   [pm_edi+PM],edi
  sgdt  fword ptr ds:[pm_gdt_ptr+PM]
  sidt  [pm_idt_ptr+PM]
  
  mov   esp,offset real_mode_stack
  

                                               ; load register set 
  mov   ecx,[edi+ip_eax]
  shl   ecx,16
  mov   cx,word ptr ds:[edi+ip_ecx]
  mov   edx,[edi+ip_edx]
  mov   ebx,[edi+ip_ebx]
  mov   ebp,[edi+ip_ebp]
  mov   esi,[edi+ip_esi]
  mov   edi,[edi+ip_edi]


  pushf
  push  offset return_from_bios
  
  movzx eax,al                                 ; push destination address of INT n handler
  push  [eax*4]
  

  lgdt  fword ptr ds:[intermediate_gdt_ptr+PM]
  lidt  [real_mode_idt_ptr+PM]
  
  jmpf32 run_identity_mapped_in_lowest_megabyte,intermediate_cs_64K_base_0
  
  
run_identity_mapped_in_lowest_megabyte:

  mov   al,intermediate_ds_64K_base_0
  mov   ah,0
  mov   ds,eax
  mov   ss,eax
  mov   es,eax
  mov   fs,eax
  mov   gs,eax
 
   
  mov   eax,cr0
  osp
  and   eax,NOT ((1 SHL pg_bit)+(1 SHL pe_bit))
  mov   cr0,eax
                                      
  jmpf16 (LOWWORD offset run_in_real_mode),0                    ; only for required for flushing prefetch que on 486
  
  
  
run_in_real_mode:         ; REAL MODE, 16-BIT MODE !

  sub   eax,eax
  mov   cr3,eax
  
  mov   ds,eax
  mov   ss,eax
  mov   fs,eax
  mov   gs,eax
  
  osp
  mov   eax,ebx           ; mov es,ebx SHR 16
  osp
  shr   eax,16
  mov   es,eax

  osp
  mov   eax,ecx           ; mov ax,ecx SHR 16
  osp
  shr   eax,16
  
    
  db    0CBh              ; RET FAR   call INT n handler

   
  
return_from_bios:         ; 16 bit mode!

  pushf
  osp
  shl   edx,16
  pop   edx               ; pop dx !
  osp
  rol   edx,16
  
  osp
  shl   eax,16
  mov   eax,ecx           ; mov ax,cx !
  osp
  mov   ecx,eax
  
  mov   eax,es
  osp
  shl   eax,16
  mov   eax,ebx
  osp
  mov   ebx,eax

  
  osp
  asp
  mov   eax,[intermediate_cr3]
  mov   cr3,eax

  osp
  asp
  lgdt  fword ptr ds:[intermediate_gdt_ptr]
 
  mov   eax,cr0
  osp
  or    eax,(1 SHL pg_bit)+(1 SHL pe_bit)
  mov   cr0,eax
  
  jmpf16 (LOWWORD offset back_in_protected_mode),intermediate_cs_64K_base_0
  
  
back_in_protected_mode:

  osp
  mov   eax,intermediate_ds_64K_base_0  
  mov   ds,eax
  
  osp             
  asp
  lgdt  [pm_gdt_ptr]
  osp
  asp
  lidt  [pm_idt_ptr]
  
  jmpf16 (LOWWORD offset back_in_LN_mode),phys_mem_exec
  
  
back_in_LN_mode:

  mov   eax,linear_kernel_space
  mov   ds,eax
  mov   es,eax
  mov   ss,eax
  sub   eax,eax
  mov   fs,eax
  mov   gs,eax
  
  mov   esp,[pm_esp]
  
  mov   eax,[pm_cr3]
  mov   cr3,eax


  mov   eax,[pm_edi+PM]
  
  
  mov   word ptr ds:[eax+ip_ecx],cx
  shr   ecx,16
  mov   word ptr ds:[eax+ip_eax],cx
  mov   word ptr ds:[eax+ip_edx],dx
  shr   edx,16
  mov   byte ptr ds:[eax+ip_eflags],dl
  mov   [eax+ip_ebx],ebx
  mov   [eax+ip_ebp],ebp
  mov   [eax+ip_esi],esi
  mov   [eax+ip_edi],edi
  
  
  ret
  





  code ends
  end