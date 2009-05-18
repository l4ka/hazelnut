include l4pre.inc 

  Copyright GMD, L4.SGMCTR, 24,12,97, 300
 
 
;*********************************************************************
;******                                                         ******
;******         Segment Controller                              ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  24.02.90    ******
;******                                   modified: 24.12.97    ******
;******                                                         ******
;*********************************************************************
 
  public init_sgmctr
  
  
  extrn physical_kernel_info_page:dword
 





.nolist 
include l4const.inc
include adrspace.inc
include intrifc.inc
include uid.inc
include tcb.inc
include cpucb.inc
include kpage.inc
.list


ok_for i486,pentium,ppro,k6



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
 
ldtseg            equ  2
taskgate          equ  5
tsseg             equ  9
callgate          equ  0Ch
intrgate          equ  0Eh
trapgate          equ  0Fh
 
 
;---------------------------------------------------------------------------
;
;        descriptor privilege levels codes
;
;---------------------------------------------------------------------------
 
dpl0    equ (0 shl 5)
dpl1    equ (1 shl 5)
dpl2    equ (2 shl 5)
dpl3    equ (3 shl 5)



;----------------------------------------------------------------------------
;
;       descriptor entry
;
;----------------------------------------------------------------------------


descriptor macro dtype,dpl,dbase,dsize


IF dsize eq 0

  dw    0FFFFh
  dw    lowword        dbase
  db    low highword   dbase
  db    low            (dtype+dpl+80h)
  db    high           (dtype+8000h) + 0Fh
  db    high highword  dbase

ELSE
IF dsize AND -KB4

  dw    lowword        ((dsize SHR 12)-1)
  dw    lowword        dbase
  db    low highword   dbase
  db    low            (dtype+dpl+80h)
  db    high           ((dtype+8000h) + (highword ((dsize SHR 12)-1)) SHL 8)
  db    high highword  dbase

ELSE

  dw    lowword        (dsize-1)
  dw    lowword        dbase
  db    low highword   dbase
  db    low            (dtype+dpl+80h)
  db    high           dtype
  db    high highword  dbase

ENDIF
ENDIF

  endm


 
 

;****************************************************************************
;******                                                               *******
;******                                                               *******
;******                 Segment Controller Initialization             *******
;******                                                               *******
;******                                                               *******
;****************************************************************************



;-----------------------------------------------------------------------
;
;       init segment controller
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
;       DS : R/W 0..4GB
;       SS : R/W 0..4GB
;       CS : X/R 0..4GB, USE32
;
;-----------------------------------------------------------------------
; POSTCONDITION:
;
;       GDT     initialized
;       GDTR    initialized
;
;       LDTR    initialized
;
;       CS    linear_kernel_space_exec
;       DS    linear_space
;       ES    linear_space
;       FS    linear_space
;       GS    linear_space
;       SS    linear_space
;
;       EAX...EBP scratch
;
;----------------------------------------------------------------------
 

 assume ds:codseg

  icode

 
 
init_sgmctr:

  add   ds:[physical_kernel_info_page].kdebug_exception,PM

  mov   eax,ds
  mov   es,eax

  mov   edi,offset gdt + first_kernel_sgm
  mov   ecx,(sizeof gdt - first_kernel_sgm)/4
  sub   eax,eax
  cld
  rep   stosd

  mov   edi,offset gdt + first_kernel_sgm
  mov   esi,offset initial_gdt+8
  mov   ecx,(offset end_of_initial_gdt - (offset initial_gdt+8) +3) / 4
  rep   movsd

  lgdt  fword ptr ds:[gdt_vec]

  jmpf32 $+6+PM,linear_kernel_space_exec

  mov   eax,linear_kernel_space
  mov   ds,eax
  mov   es,eax
  mov   ss,eax
	sub	eax,eax
  mov   fs,eax
  mov   gs,eax
  lea   esp,[esp+PM]

  sub   eax,eax
  lldt  ax

  pop   eax
  add   eax,PM
  jmp   eax
 
 
        align 4


gdt_vec       dw sizeof gdt-1
              dd offset gdt

        align 4

  IF kernel_type EQ i486

        user_space_size        equ     linear_address_space_size
  ELSE
        user_space_size        equ     (virtual_space_size + MB4)

  ENDIF



.errnz  virtual_space_size AND (MB4-1)


.xall
initial_gdt   dd 0,0                   ; dummy seg

  tss_base    equ offset cpu_tss_area
  tss_size    equ offset (iopbm - offset cpu_tss_area + sizeof iopbm)
  
  descriptor tsseg, dpl0, tss_base, tss_size                ; 08 : cpu0_tss
  descriptor rw32, dpl2, PM, <physical_kernel_mem_size>     ; 10 : phys_mem

  descriptor xr32, dpl0, 0,  <linear_address_space_size>    ; 18 : phys_exec
 ; dw    lowword        (KB64-1)                            ; 18 : phys_exec
 ; dw    lowword        PM
 ; db    low highword   PM
 ; db    low            (xr32+dpl0+80h)
 ; db    high           xr32
 ; db    high highword  0 ; PM
  descriptor rw32, dpl0, 0, <linear_address_space_size>     ; 20 : linear_kernel_space

  descriptor rw32, dpl3, 0,  user_space_size                ; 28 : linear space
  descriptor xr32, dpl3, 0,  user_space_size                ; 30 : linear space

  



end_of_initial_gdt  equ $

  

  icod  ends


  code ends
  end




