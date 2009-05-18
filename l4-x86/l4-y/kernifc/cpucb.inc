  
  IF (kernel_type EQ x86) OR (kernel_type EQ pentium)
      
include cpucb5.inc

  ENDIF
  IF (kernel_type EQ pIII)

include cpucb6.inc

  ENDIF
  
