

include l4pre.inc 
         


  Copyright IBM+UKA, L4.MPCONF, 07,01,00, 31
 
;*********************************************************************
;******                                                         ******
;******         Multi Processor Configuration                   ******
;******                                                         ******
;******                                   Author:   M.Voelp     ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 7.1.00      ******
;******                                                         ******
;*********************************************************************
 
 
  public determine_mp_system_type



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
.list

ok_for x86
	
;*********************************************************************
;******                                                         ******
;******         Multi Processor Configuration                   ******
;******                                                         ******
;******                                                         ******
;*********************************************************************

  assume ds:codseg
  icode
	

;---------------------------------------------------------------------
;
;	determin mp system type
;
;---------------------------------------------------------------------

determine_mp_system_type:	
  ;<cd> Find that MP Floating Pointer Structure
  ;<cd> Looking for _PM_ = 5F504D5F	
  ;<cd> First look at 1. kb EBDA 09fc00h
  ;<cd> Then in last kb of System Memory
  ;<cd> Last in 0F0000h
 
  mov esi, 09fc00h
  call find_float_struc
  IFZ eax,5F504D5Fh
 	call mp_float_struc	
  ELSE_	

	;<cd> Look at last kb of System Memory
	mov eax, [edi+main_mem].mem_end				; System Memory    
	sub eax, 400h
	mov esi, eax
	call find_float_struc
  	IFZ eax,5F504D5Fh
 		call mp_float_struc	
	ELSE_
	        ;<cd> Look at kb starting at 0f0000h 
		mov esi, 0F0000h
		call find_float_struc
		IFZ eax,5F504D5Fh
 			call mp_float_struc  
		ELSE_
			;<cd> Nothing of that stuff was found	
	        	mov [physical_kernel_info_page].mpfloatstruc, 00h
	        	mov [physical_kernel_info_page].mpconftbl, 00h
	        	mov [physical_kernel_info_page].proc_data, 00h
		FI
	FI
  FI
  ret

;-------------------------------------------------------------------------
;
; search for _PM_ in the first kb pointed to by esi 
;
;-------------------------------------------------------------------------

find_float_struc:
  mov ecx, 00h		
  DO	
	mov eax, [esi]
	cmp eax, 5F504D5Fh	;<cd> look for _PM_
	EXITZ
	inc esi			
	inc ecx
	cmp ecx,10000h		;<cd> repeat until (1kb == ecx)
	REPEATB
  OD
  ret

;----------------------------------------------------------------------------
;
; MP_float_structure found:	
; look for the MP_configuration table, processor data
;
;----------------------------------------------------------------------------	
	
mp_float_struc:	       	; MP floating point structure found at esi
  mov [physical_kernel_info_page].mpfloatstruc, esi
  add esi, 04h
  mov ebx, [esi]
  mov [physical_kernel_info_page].mpconftbl, ebx
  IFZ ebx,0			; No MP Configuration Table exists
	mov al,[esi+07h]	
	IFNZ al,0
		;; dammed intel standard configuration is used
		;; MP Standard type in al
	CANDB al, 08h
		call determine_standard_configuration
	ELSE_
	FI
  ELSE_
	mov eax,ebx		; MPconftbl in ebx
	add eax, 02Ch
	mov [physical_kernel_info_page].proc_data, eax ; Start adress of proc info in eax  
  FI
  ret				

;----------------------------------------------------------------------------
; ******** NOT YET IMPLEMENTED *********
; determine standard configuration
; No MP_configuration table present. Build needed entries of configuration 
; table out of the standard type:	
; Dual machine, local APIC ID == 0, 1
;----------------------------------------------------------------------------

;; PRECONDITION id of standard configuration in al
determine_standard_configuration:
 	; <imp>
	;; build MP table out of scratch
	;; Construct a new MP Configuration Table and link it to the needed position
  ret
	
  icod ends
 code ends
end





