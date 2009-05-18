include l4pre.inc 
         


  Copyright IBM+UKA, L4.APSTART, 17,01,00, 31
 
;*********************************************************************
;******                                                         ******
;******         Application Processor Startup                   ******
;******                                                         ******
;******                                   Author:   M.Voelp     ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 17.01.00    ******
;******                                                         ******
;*********************************************************************
 
 
  public startup_ap
  public synch_all


  extrn physical_kernel_info_page:dword
  extrn init_sgmctr:near
  extrn tss_vec:dword
  extrn gdt_vec:dword
	
.nolist
  include l4const.inc
  include uid.inc
  include l4kd.inc
.list
	
include adrspace.inc

.nolist
  include kpage.inc
  include apic.inc
  include descript.inc

  include cpucb.inc		
.list
  include apstrtup.inc

ok_for x86

 assume ds:codseg

  icode


;-------------------------------------------------------------------------------------------
; handle not answering processors
;-------------------------------------------------------------------------------------------
	
check_fatal_cpu_exception:	
  ; Get number of processors out of MPconfig Table
  mov esi,ds:[logical_info_page+proc_data]
  add esi,PM
	
  sub ebx,ebx
  DO
	;<cd> processor data has entry type 0 located at first byte each 14h starting at proc_data
	mov eax,ds:[esi]
	inc ebx
	add esi,14h
	cmp al,0
	REPEATZ
  OD
  dec ebx	
  ;<cd> number of processors in the system in ebx
  ;<cd> wait for time to elaps or all processors up and running (aquiring their procnumber)

  mov ecx,100000h
  DO 
	dec ecx
	IFZ
		ke 'Fatal Error, some Processors does not respond'
	FI
	mov al,ds:[logical_info_page+numb_proc]
	cmp al,bl
	REPEATNZ
  OD
  ret  	

;-------------------------------------------------------------------------------------------
; synch all
;-------------------------------------------------------------------------------------------
; Waits for all processors to enter the code. All processors loop waiting until all reached 
; the loop.
;-------------------------------------------------------------------------------------------	
synch_all:
  mov eax,[logical_info_page+numb_proc]
  lock inc ds:[logical_info_page+prc_sema]
  DO
	cmp al,ds:[logical_info_page+prc_sema]
	REPEATNZ
  OD
  ret

		
;-------------------------------------------------------------------------------------------
; startup application processors
;-------------------------------------------------------------------------------------------	
		
startup_ap:  
  call prepare_ap_startup		; store proc state (IDTR ,...) to memory
  call initiate_startup		; initiate startup ipi
  ret
	
			
;-------------------------------------------------------------------------------------------
; prepare application processor startup
;-------------------------------------------------------------------------------------------
;
;  PRECONDITION:	ds linear space
;  temporarly implementation of startup code:		
;	store idt, tss, cr3 of BSP to share tables/data with all APs
;
;-------------------------------------------------------------------------------------------

prepare_ap_startup:

   lea eax,[esp]		
   mov ds:[apinit_esp],eax

   sidt ds:[apinit_idt_vec]
   str  ds:[apinit_tss]
   mov  eax,cr3	
   mov  ds:[apinit_cr3],eax

   mov ds:[apinit_cs],cs
   mov ds:[apinit_ds],ds
		
  ret	
	
  apinit_esp		dd 0
  apinit_idt_vec	df 0
  apinit_tss		dw 0
  apinit_cr3		dd 0		  	

;-------------------------------------------------------------------------------------------
; initiate startup
;-------------------------------------------------------------------------------------------
; initialises and starts up all APs. Startupcode at application_processor_start
;
; PRECONDITION
;	ds	linear space
;	apic initialised
; POSTCONDITION
;
;-------------------------------------------------------------------------------------------
	
initiate_startup:
  ;<cd> set entry point for the ap processor:	 jump to startup code 
  sub eax,eax
  mov byte ptr ds:[eax],0E9h	
  mov ebx, offset application_processor_start-3
  mov word ptr ds:[eax+1],bx

  ;<cd> enable apic (set software apic enable bit)
;  bts ds:[local_apic+apic_icr], 7 ; apic sw enabled
		  
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
  ;; wait for apic to finish ??

  mov ecx, 1000000		; some time
  DO
	dec ecx
	REPEATNZ
  OD

  call check_fatal_cpu_exception

  ret
	

	
;-------------------------------------------------------------------------------------------	
; application processor start
;-------------------------------------------------------------------------------------------
; PRECONDITION
;	real mode, interrupts disabled, apic disabled
; POSTCONDITION
;
;
;-------------------------------------------------------------------------------------------
	
application_processor_start:

  ;<cd> enable debug extension and page size extension for 4 MB pages
  mov eax, cr4	
  or eax, 16+8			; bit 4:PSE 3:Debug Extension
  mov cr4, eax			; <pr> BSP uses large Pages

  ;<cd> enable protected mode and paging

	;; esp setzen

	
  ;<cd> share page directory with BSP 
  asp
  osp
  mov eax,cs:[apinit_cr3]

  mov cr3,eax			; both arguments are always 32 bit


	
  ;load temporary gdt	
  asp
  osp
  lgdt fword ptr cs:[ap_gdt_vec]

  mov eax,cr0
  or ax,1			; set protected mode bit

  osp			
  bts eax,31			; set paging flag

  mov cr0,eax

  osp
  db 0eah			; jmp far
  dd offset ap_protected_mode
apinit_cs	dw 0


  ap_gdt_vec dw offset ap_temp_end_of_gdt - offset ap_temp_gdt -1
	     dd offset ap_temp_gdt
	     
	  

  IF kernel_type NE pentium

        user_space_size        equ     linear_address_space_size
  ELSE
        user_space_size        equ     (virtual_space_size + MB4)

  ENDIF
	

  align 8
	
  ap_temp_gdt	dd 0,0		; 0 descriptor

	  descriptor rw32, dpl0, 0, <linear_address_space_size>     ; 08 : linear_kernel_space
	  descriptor rw32, dpl3, 0,  user_space_size                ; 10 : linear space
	  descriptor xr32, dpl3, 0,  user_space_size                ; 18 : linear space

	  descriptor xr32, dpl0, PM, <physical_kernel_mem_size>     ; 20 : phys_exec
	  descriptor rw32, dpl2, PM, <physical_kernel_mem_size>     ; 29 : phys_mem
	
	  tss_base    equ offset cpu_tss_area
	  tss_size    equ offset (iopbm - offset cpu_tss_area + sizeof iopbm)
	
	  descriptor tsseg, dpl0, tss_base, tss_size                ; 30 : cpu0_tss
	  descriptor tsseg, dpl0, tss_base, tss_size                ; 38 : cpu0_tss
	
  ap_temp_end_of_gdt	equ $


  apinit_ds dw 0
	
;-------------------------------------------------------------------------------------
; application processor protected mode start
;-------------------------------------------------------------------------------------
; PRECONDITION
; cs,ds phys memory, paging disabled, intr disabled
; protected mode
;
;
	
	
ap_protected_mode:

  aquire_new_processor_number ebx

  lgdt fword ptr ds:[tss_vec]
  jmpf32 $+6,(max_cpu+1)*8
  mov  eax,(max_cpu+2)*8
  mov  ds,eax

  mov  es,eax
  shl  ebx,3
  ltr  bx

  lgdt  fword ptr ds:[gdt_vec]

  jmpf32 $+6,phys_mem_exec

  mov   eax,linear_kernel_space
  mov   ds,eax	
  mov   es,eax
  mov   ss,eax

  ;; load ds equal to bsp
	
  ;; status:	 PM, Paging, Code segment valid

  lidt ds:[apinit_idt_vec]
  
  mov esp,[apinit_esp]
  add esp,1024

  call synch_all

  ke 'synched'

ap_striptease:

  kd____disp <'PROCESSOR STATE SUMMARY',13,10>

  kd____disp <13,10,'DS  '>
  mov   eax,ds
  kd____outhex32 
  kd____disp <'    ESP  '>
  mov   eax,esp
  kd____outhex32 

  kd____disp <13,10,'ES  '>
  mov   eax,es
  kd____outhex32 
  kd____disp <'    CR0  '>
  mov   eax,cr0
  kd____outhex32 

  kd____disp <13,10,'SS  '>
  mov   eax,ss
  kd____outhex32 
  kd____disp <'    CR1  '>

  kd____disp <13,10,'TR  '>
  str   ax
  kd____outhex32 
  kd____disp <'    CR2  '>
  mov   eax,cr2
  kd____outhex32 

  kd____disp <13,10,'    '>
  mov   eax,0
  kd____outhex32 
  kd____disp <'    CR3  '>
  mov   eax,cr3
  kd____outhex32 

  kd____disp <13,10,'    '>
  mov   eax,0
  kd____outhex32 
  kd____disp <'    CR4  '>
  mov   eax,cr4
  kd____outhex32 

ke 'xx'
	
  jmp $



	;; precondition: 		
	;; cs,ds phys memory, paging disabled, intr disabled
	;; protected mode
	;;
	;;
  ke 'before dual start' 

  cli				; disable interrupts

  jmpf32 $+6,phys_mem_exec	; load cs with pm pl0 rx
  mov eax,phys_mem
  mov ds,eax			; load ds with pm pl0 rw
  mov es,eax
  sub eax,eax
  mov fs,eax
  mov gs,eax

	
  ke 'before dual start1'
  call  ltr_pnr			
		
  ke 'ltr reloaded'	

  call  enable_paging_mode	

  call  init_sgmctr
  call  init_intctr
  call  init_emuctr	

  ke 'after emuctr2'
	
  call  init_cpuctr		; ??
  call  init_pagmap		; ??


  ke 'before synch'
  ; synch				
	
  call  init_schedcb
  call  init_tcbman
  call  init_dispatcher

  ke 'end dual'		

	
	
  ; load gate descriptor table
  
  ;; load address of gdt in edi, eax = size (gdt)
	;;   lea___pno edi
	;;   shl edi,6			; log 2 gdt size (8*2*4 bytes = 64 bytes) = 6
	;;   mov eax,edi
	;;   dec eax
	;;   add edi, offset gdt + first_kernel_segment
  
  	  	
  	
	
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



  icod ends
 code ends
end		

