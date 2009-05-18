;----------------------------------------------------------------------------
;
;       kernel type definition
;
;----------------------------------------------------------------------------

  IF target EQ 486
      
subtitle L4/486
 
kernel_type        = i486 
 
gver = 3

kernel_x2	   = 0	


include adrspac4.inc


  ENDIF
  
  
  
  IF target EQ pentium
      
subtitle L4/Pentium
 
kernel_type        = pentium 
 
kernel_char        = '5'

gver = 3

kernel_x2	   = 0	


include adrspac5.inc

  ENDIF	



  IF target EQ pentium_x2
      
subtitle L4/Pentium/x2
 
kernel_type        = pentium 
 
gver = 3

kernel_x2	   = 2	


include adrspac5.inc


  ENDIF
  


  IF target EQ amd_k6
      
subtitle L4/K6
 
kernel_type        = amd_k6 
 
gver = 3

kernel_x2	   = 0	


include adrspac5.inc

  ENDIF	

