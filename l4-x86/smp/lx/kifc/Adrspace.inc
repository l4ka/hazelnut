;----------------------------------------------------------------------------
;             
;       kernel type definition
;
;----------------------------------------------------------------------------
       
  
  
  IF target EQ x86
      
subtitle L4/x86 Version X
 
kernel_type        = x86 
 
kernel_char        = '5'

gver = 2

kernel_x2	   = 0	

; max_cpu            = 2   ; defined as compiler constrait

include adrspac5.inc

  ENDIF	


 
  


