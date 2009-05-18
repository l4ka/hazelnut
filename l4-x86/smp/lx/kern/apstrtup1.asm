include l4pre.inc 
         


  Copyright IBM+UKA, L4.APSTART, 17,12,99, 31
 
;*********************************************************************
;******                                                         ******
;******         Application Processor Startup                   ******
;******                                                         ******
;******                                   Author:   M.Voelp     ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 17.12.99    ******
;******                                                         ******
;*********************************************************************
 
 
  public startup_ap


  extrn physical_kernel_info_page:dword
	
.nolist
  include l4const.inc
  include uid.inc	
.list
	
include adrspace.inc

.nolist
  include kpage.inc
  include apic.inc
  include cpucb.inc
include tcb.inc
	
.list

ok_for x86
	


	;; load shared tables:	gdt, idt, ldt
	;; enter protected mode, enable paging

	; Do code next:
	; AP Datastructure setup:
	; - control blocks: cpu schedule
	; - establish scheduler lists on APs: ready list soon wakeup
	; 
	; - test single processor behaviour on APs
	; - change Systemcall: thread switch
	; - extend IPC critical path
	; - extend thread schedule with dummy functionality: set processor number
	; - Implement Proxies and Mailboxes
	; - Implement send to Proxy 



	

;---------------------------------------------------------------------
;
;       AP Startup
;
;---------------------------------------------------------------------
; PRECONDITION:
;
;       protected mode
;	paging enabled
;	 
;	cs: physical mem executeable
;	ds,es,fs,gs,ss linear space  
;	
;       interrupts disabled
;       all APIC's but BSP-APIC disabled
;	waiting for INIT-IPI
;
;---------------------------------------------------------------------

  assume ds:codseg
  icode

	
			
;----------------------------------------------------------------------------
;
;    Bootup and synchronization of AP's
;
;----------------------------------------------------------------------------
; These methods insure to startup the kernel on a crashed / shutdown
; processor (i.e. after startup)
;
; PRECONDITION:	At least one Processor (at startup the BSP) must have
;		performed the startup of the nucleus including all structures
;		needed to bring up the system (i.e. IDT / GDT /...)
;----------------------------------------------------------------------------
	
;----------------------------------------------------------------------------
; Startup Application Processors
;----------------------------------------------------------------------------
; This method is run by the BSP / the processor detecting an AP-crash
;
; PRECONDITION:  EAX:	mask of the destination processors to be started
;		    ******* ?? exact value for different addr types ??
; POSTCONDITION: processor up waiting for task switch if 
;	  	 proc id bit is set in proc_avail 		
;		 (2^APIC_ID = 2^CPU_ID = proc ID bit in proc_avail)
;----------------------------------------------------------------------------

startup_ap:  
  bt ds:[cpu_feature_flags],on_chip_apic_bit
  IFS
  FI
	call prepare_ap_startup		; store proc state (IDTR ,...) to memory
	call initiate_startup		; initiate startup ipi
  ret

prepare_ap_startup:

   lea eax,[esp]		
   mov ds:[apinit_esp],eax
   

   ke 'wo bin ich'	
   sgdt ds:[apinit_gdt_vec]

   mov eax,cr3
   and eax,0FFFFFF00h

   lea edi,ds:[apinit_gdt_vec]
	
   mov ebx,[edi]
   mov ecx,ebx
   and ebx,0FFC00000h
   shr ebx,22	
   add eax,ebx

   ke 'pdir'
	
   mov eax,[eax]
   mov ebx,ecx
   and ebx,0003FF000h
   shr ebx,12
   add eax,ebx

   ke 'ptab' 
	
   mov ecx,ebx
   and ebx,00000FFFh
   mov eax,[eax]
   add eax,ebx

   mov [edi],eax

   ke 'phys'	
	
   sidt ds:[apinit_idt_vec]

   mov ds:[apinit_cs],cs
   mov ds:[apinit_ds],ds
	
   str ds:[apinit_tss]
   mov eax,cr3	
   mov ds:[apinit_cr3],eax

  ret

initiate_startup:
  sub eax,eax
  mov byte ptr ds:[eax],0E9h	;  ???
  mov ebx, offset application_processor_start-3
  mov word ptr ds:[eax+1],bx
  
  ke 'before IPI'	  
  ;<cd> send Init message to all processors (level assert) and delay for 2s
  mov ds:[local_apic+apic_icr], apic_init_msg + 0
  mov ecx, 1000000
  DO 
	dec ecx
	REPEATNZ
  OD

  ;<cd> send Init message to all processors (level deassert) and delay for 2s
  mov ds:[local_apic+apic_icr], apic_init_deassert_msg + 0
  mov ecx, 1000000
  DO 
	dec ecx
	REPEATNZ
  OD

  ;<cd> now send startup IPI
  mov ds:[local_apic+apic_icr], apic_startup_msg + 0
  ke 'after IPI'
	
  ;; wait for apic to finish ??
  mov ecx, 1000000		; some time
  DO
	dec ecx
	REPEATNZ
  OD

  ret
	
;----------------------------------------------------------------------------
; Application Processor Start
;----------------------------------------------------------------------------
; This method is run by the AP at startup
;
; PRECONDITION STARTUP:	real mode / disable intr / APIC not initialised
; POSTCONDITION: processor bit will be set in proc_avail (not immediately)
;
;----------------------------------------------------------------------------

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

        user_space_size        equ     linear_address_space_size

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




	
application_processor_start:
	
	sub	eax,eax		
	mov	ds,eax
	sub	ebx,ebx
	mov	[edi],eax
	;; 	jmp	$
		
	
	; Wrong code entry Point of AP Processor:
	; test with dec cs:[eax]
	; test without jump offset -3

  ;; load cr3 register with page directory base address 
  asp
  osp
  mov eax,cs:[apinit_cr3]
  mov cr3,eax


  asp
  osp
  mov esp,cs:[apinit_esp]

  asp
  osp
  lgdt cs:[apinit_gdt_vec]
	
  asp
  osp
  lidt cs:[apinit_idt_vec]

  mov eax,cr0	; Enable Protected Mode 

  or al,1			; protected mode

  osp
  bts eax,31			; paging

  osp	
  mov [edi],eax

	;;   jmp $

	;; put identity mapped code to 0 and jump to it
  sub ebx,ebx

  osp
  mov eax,0f22c0eah
  osp		
  mov [edi],eax
	
  add ebx,4
  mov [edi],offset ap_protected_mode

  sub eax,eax
  add ebx,2
  mov [edi],eax
	
  add ebx,4
  mov eax, 0008h
  mov [edi],eax

  db 0eah
  dd 00000000h
  dw 0008h
		
		
	;; mov cr0, eax		must be identity mapped 
	;; jmp far cs:offset	
  mov cr0,eax


	
ap_protected_mode:
	
  jmp $

;; Things to do for Startup AP's
	;;   movzx eax, cs:[apinit_ds]
	;;   mov ds,eax
	;;   mov eax, cs:[apinit_cr3]
	;;   mov cr3, eax
  
  ;; enable Paging
	;;   mov eax, cr0
	;;   bts eax, 31			; pg flag
	;;   mov cr0, eax

	
  ;; get Processor Number initially
  sub eax,eax
  inc eax
  lock xadd [physical_kernel_info_page+numb_proc],eax
  inc eax
  ;; Processor Number in eax 

  DO
	sub eax,eax
	REPEATNZ 
  OD
	

  ;; Wait for thread schedule (own processor)


	
  align 8
		  	
  apinit_ds		dw 0
  apinit_esp		dd 0
  apinit_gdt_vec	df 0
  apinit_idt_vec	df 0
  apinit_tss		dw 0
  apinit_cr3		dd 0		


initial_gdt   dd 0,0                   ; dummy seg

  descriptor rw32, dpl0, 0, <linear_address_space_size>     ; 08 : linear_kernel_space
  descriptor rw32, dpl3, 0,  user_space_size                ; 10 : linear space
  descriptor xr32, dpl3, 0,  user_space_size                ; 18 : linear space

  descriptor xr32, dpl0, PM, <physical_kernel_mem_size>     ; 20 : phys_exec
  descriptor rw32, dpl2, PM, <physical_kernel_mem_size>     ; 29 : phys_mem

  tss_base    equ offset cpu_tss_area
  tss_size    equ offset (iopbm - offset cpu_tss_area + sizeof iopbm)

  descriptor tsseg, dpl0, tss_base, tss_size                ; 30 : cpu0_tss
  descriptor tsseg, dpl0, tss_base, tss_size                ; 38 : cpu0_tss


end_of_initial_gdt  equ $


	
  icod ends
 code ends
end		










