
;-------------------------------------------------------------------------------------------
; aquire new processor number
;-------------------------------------------------------------------------------------------	
; Warning: May only be called once for each processor. 
; PRECONDITION:	physical kernel info page established.
;
; POSTCONDITION: new processor number in reg
;
;-------------------------------------------------------------------------------------------		
aquire_new_processor_number macro reg
  sub reg,reg
  inc reg
  lock xadd [physical_kernel_info_page+numb_proc],reg
  inc reg
endm
