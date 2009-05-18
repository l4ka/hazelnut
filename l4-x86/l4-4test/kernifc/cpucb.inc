
  IF kernel_type EQ i486
      
include cpucb4.inc

  ENDIF
   
  
  IF kernel_type EQ pentium
      
include cpucb5.inc

  ENDIF


  IF kernel_type EQ k6
      
include cpucb5.inc

  ENDIF
  
