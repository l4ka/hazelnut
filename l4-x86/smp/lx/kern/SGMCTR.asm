include l4pre.inc 

  Copyright IBM, L4.SGMCTR, 03,09,97, 19
 
 
;*********************************************************************
;******                                                         ******
;******         Segment Controller                              ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 03.09.97    ******
;******                                                         ******
;*********************************************************************
 
  public init_sgmctr
  public ltr_pnr

  public gdt_vec
  public tss_vec 
	
  extrn physical_kernel_info_page:dword


.nolist 
include l4const.inc
include uid.inc
include adrspace.inc
include tcb.inc
include cpucb.inc
include descript.inc
include kpage.inc
.list
include apstrtup.inc


ok_for x86

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

  ;<cd> delete space for gdt in linear adress space
  mov   edi,offset gdt + first_kernel_sgm
  mov   ecx,(sizeof gdt - first_kernel_sgm)/4
  sub   eax,eax
  cld
  rep   stosd

  ;<cd> copy initial GDT into GDT space

  mov   edi,offset gdt + first_kernel_sgm
  mov   esi,offset initial_gdt+8
  mov   ecx,(offset end_of_initial_gdt - (offset initial_gdt+8) +3) / 4
  rep   movsd

  ;<cd> load GDT

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


;----------------------------------------------------------------------
; Load Task Register with Processor Number
;----------------------------------------------------------------------
; PRECONDITION:  protected mode
;		 cs points to code segment
;		 ds points to data segment
; POSTCONDITION: segment registers reloaded
;		 tr loaded
;		 registers scratch
;----------------------------------------------------------------------

	
ltr_pnr:
  ;<cd> store current segment registers and gdt

  mov [tss_old_cs],cs
  mov [tss_old_ds],ds
  mov [tss_old_es],es
  mov [tss_old_fs],fs
  mov [tss_old_gs],gs
  mov [tss_old_ss],ss

  sgdt fword ptr ds:[tss_old_gdt_vec]
  
  ;<cd> create temporary GDT for loading processor number into TR

  mov   edi,offset tss_gdt
  sub   eax,eax
  stosd
  stosd

  ;<cd> use one task descriptor for each processor

  mov   ecx, (1 SHL max_cpu)
  tss_loop:
	mov   esi,offset desc_tss0
	movsd
	movsd
  loop  tss_loop

  ;<cd> we need two additional descriptors for physical memory and linear kernel space
  ;<cd> one is loaded into CS for execution, the other one is used to locate the final
  ;<cd> descriptor table
  ;<cd> use segments of cs and ds for code and data descriptors


  mov eax,offset tss_old_gdt_ofs
  	
  mov     esi,cs	
  add     esi,[tss_old_gdt_ofs]
  movsd
  movsd
  mov     esi,ds	
  add     esi,[tss_old_gdt_ofs]
  movsd
  movsd

  aquire_new_processor_number ebx

  mov edx, offset tss_gdt 
	 
  lgdt fword ptr ds:[tss_vec]
  jmpf32 $+6,((1 SHL max_cpu)+1)*8
  mov  eax,((1 SHL max_cpu)+2)*8
  mov  ds,eax
  mov  es,eax
  shl  ebx,3
  ltr  bx

  ;<cd> reload old GDT

  lgdt  fword ptr ds:[tss_old_gdt_vec]

  db 0EAh
  dd $+6
  tss_old_cs dw 0

  mov   eax,linear_kernel_space
  mov   ds,[tss_old_ds]
  mov   es,[tss_old_es]
  mov   fs,[tss_old_fs]
  mov   gs,[tss_old_gs]
  mov   ss,[tss_old_ss]
		
ret
	
;<ds> temporary space for gdt and other segments
tss_old_gdt_vec dw 0
tss_old_gdt_ofs dd 0
	
tss_old_ds      dw 0
tss_old_es      dw 0
tss_old_fs      dw 0
tss_old_gs      dw 0
tss_old_ss      dw 0	
	
	
;<cd> special GDT with a list of task state segments
;<cd> used to load TR with processor number
 
        align 4

tss_vec dw 24+(1 SHL max_cpu)*8
        dd offset tss_gdt

        align 4  
 
tss_gdt dd (((1 SHL max_cpu)+3)*2) dup (?)

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

desc_lksp:  descriptor rw32, dpl0, 0, <linear_address_space_size>     ; 08 : linear_kernel_space
  descriptor rw32, dpl3, 0,  user_space_size                ; 10 : linear space
  descriptor xr32, dpl3, 0,  user_space_size                ; 18 : linear space

desc_phex:  descriptor xr32, dpl0, PM, <physical_kernel_mem_size>     ; 20 : phys_exec
  descriptor rw32, dpl2, PM, <physical_kernel_mem_size>     ; 29 : phys_mem

  tss_base    equ offset cpu_tss_area
  tss_size    equ offset (iopbm - offset cpu_tss_area + sizeof iopbm)

desc_tss0:  descriptor tsseg, dpl0, tss_base, tss_size                ; 30 : cpu0_tss
  descriptor tsseg, dpl0, tss_base, tss_size                ; 38 : cpu0_tss

  tss_ldt_base equ offset tss_ldt
  tss_ldt_size equ 9*8

;  descriptor ldtseg, dpl0, tss_ldt_base, tss_ldt_size       ; 40 : tss_ldt
  descriptor ldtseg, dpl0, 0, 24

end_of_initial_gdt  equ $

  icod  ends


  code ends
  end




