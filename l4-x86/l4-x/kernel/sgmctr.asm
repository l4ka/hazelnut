include l4pre.inc 

  Copyright IBM, L4.SGMCTR, 01,09,00, 20
 
 
;*********************************************************************
;******                                                         ******
;******         Segment Controller                              ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 01.09.00    ******
;******                                                         ******
;*********************************************************************
 
  public init_sgmctr





.nolist 
include l4const.inc
include uid.inc
include ktype.inc
include adrspace.inc
include tcb.inc
include cpucb.inc
include segs.inc
.list


ok_for x86






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
  db    high           ((dtype+8000h) + highword ((dsize SHR 12)-1))
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
;       CS    phys_mem_exec
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

  jmpf32 $+6,phys_mem_exec

  mov   eax,linear_kernel_space
  mov   ds,eax
  mov   es,eax
  mov   ss,eax
  lea   esp,[esp+PM]

  sub   eax,eax
  lldt  ax

  ret
 
 
        align 4


gdt_vec       dw sizeof gdt-1
              dd offset gdt

        align 4

  IF kernel_type NE pentium

        user_space_size        equ     linear_address_space_size
  ELSE
        user_space_size        equ     (virtual_space_size + MB4)

  ENDIF



.errnz  virtual_space_size AND (MB4-1)


.xall
initial_gdt   dd 0,0                   ; dummy seg

  descriptor rw32, dpl0, 0, <linear_address_space_size>     ; 08 : linear_kernel_space
  descriptor rw32, dpl3, 0,  user_space_size                ; 10 : linear space
  descriptor xr32, dpl3, 0,  user_space_size                ; 18 : linear space

  descriptor xr32, dpl0, PM, <256*MB> ; special for kdebug_alias <physical_kernel_mem_size>     ; 20 : phys_exec
  descriptor rw32, dpl2, PM, <physical_kernel_mem_size>     ; 29 : phys_mem

  tss_base    equ offset cpu_tss_area
  tss_size    equ offset (iopbm - offset cpu_tss_area + sizeof iopbm)

  descriptor tsseg, dpl0, tss_base, tss_size                ; 30 : cpu0_tss

  if random_sampling OR fast_myself
        descriptor rw32, dpl3, 44, 44                       ; 38 : sampling space
  endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;descriptor tsseg, dpl0, tss_base, tss_size                ; 38 : cpu0_tss


end_of_initial_gdt  equ $


  icod  ends


  code ends
  end




