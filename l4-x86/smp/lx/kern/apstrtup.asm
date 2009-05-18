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
  public release_all


  extrn physical_kernel_info_page:dword

  extrn ltr_pnr:near
  extrn enable_paging_mode:near	
  extrn init_sgmctr:near
  extrn init_intctr:near
  extrn init_emuctr:near
  extrn init_cpuctr:near
  extrn init_pagmap:near
  extrn init_schedcb:near
  extrn init_tcbman:near
  extrn init_dispatcher:near
  extrn init_adrsman:near
  extrn init_small_address_spaces:near
  extrn create_kernel_thread:near
	
				
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
  include tcb.inc
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
  mov eax,[eax]
  lock inc ds:[logical_info_page+prc_sema]
  DO
	cmp al,ds:[logical_info_page+prc_sema]
	REPEATNZ
	
  OD
  ;; synched
  lock inc ds:[logical_info_page+prc_release]
  lno___prc eax
  IFNZ eax,1
 	sub eax,eax
 	DO
 		cmp al,ds:[logical_info_page+prc_sema]
 		REPEATNZ
 	OD
  FI	
  ret

release_all:
  mov eax,[logical_info_page+numb_proc]
  mov eax,[eax]	
  DO
	cmp al,ds:[logical_info_page+prc_release]
	REPEATNZ
  OD
  sub eax,eax
  mov ds:[logical_info_page+prc_sema],al
  mov ds:[logical_info_page+prc_release],al
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
  ret	
	
  apinit_esp		dd 0
  apinit_idt_vec	df 0


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

  mov eax,(1 SHL max_cpu)
  IFAE eax,2
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

  FI
	
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

  ;<cd> enable protected mode
	
  ;load temporary gdt	
  asp
  osp
  lgdt fword ptr cs:[ap_gdt_vec]

  mov eax,cr0
  or ax,1			; set protected mode bit
  mov cr0,eax

  osp
  db 0eah			; jmp far
  dd offset ap_protected_mode
  dw 10h			; cs register


  ap_gdt_vec dw offset ap_temp_end_of_gdt - offset ap_temp_gdt -1
	     dd offset ap_temp_gdt
	     
  align 8
	
  ap_temp_gdt	dd 0,0		; 0 descriptor

	  descriptor rw32, dpl0, 0, <linear_address_space_size>     ; 08 : linear_kernel_space
	  descriptor xr32, dpl0, 0, <linear_address_space_size>     ; 10 : phys_exec
		
  ap_temp_end_of_gdt	equ $

;-------------------------------------------------------------------------------------
; application processor protected mode start
;-------------------------------------------------------------------------------------
; PRECONDITION
; cs,ds phys memory, paging disabled, intr disabled
; protected mode
;
;-------------------------------------------------------------------------------------
	
	
ap_protected_mode:
  sub   eax,eax
  mov   eax,08h
  mov   ds, eax
  mov   es, eax
  mov   ss, eax
  sub   eax,eax
  mov   fs,eax
  mov   gs,eax

  mov esp,eax			; 	[apinit_esp]	
  add esp,1024

  call  ltr_pnr

  call  enable_paging_mode
  call  init_sgmctr
  call  init_intctr
  call  init_emuctr	

  ke 'dual'
	
  call  init_cpuctr		; ??
  call  init_pagmap		; ??

  call synch_all

  call  init_schedcb
  call  init_tcbman
  call  init_dispatcher

  call  init_adrsman
  call  init_small_address_spaces  

  
;;  create thread (tcb = e0010500) thread 4 of kernel task
	;; send to thread 8 of kernel task
  mov ebp, 0e0010400h		; thread 4
  mov ecx, offset dual_thread_test
  call create_kernel_thread
  
  ke 'thread started'
  jmp $
	
  ke 'end dual'		

	; - test single processor behaviour on APs
	; - change Systemcall: thread switch
	; - extend IPC critical path
	; - extend thread schedule with dummy functionality: set processor number
	; - Implement Proxies and Mailboxes
	; - Implement send to Proxy 


  ke 'dual end2'
	
  jmp $



dual_thread_test:

	
dual_ipc_test:	
	sub ecx,ecx		; timeoutinfinity
	mov ebp,-1		; send only
	mov esi,ds:[0e0010800h+myself];
	
	rdtsc		; Measure START 
	mov ebx,eax		; 1/2
	mov edi,edx		; 1/2
	sub eax,eax		; send registers only 1

	int 30h

	rdtsc		; Measure END

	ke 'ipc send'
	
  jmp dual_ipc_test
	
  icod ends
 code ends
end		


