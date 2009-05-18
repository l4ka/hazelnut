;----------------------------------------------------------------------------
;             
;       kernel type definition
;
;----------------------------------------------------------------------------
        			    
  
  
  IF target EQ x86
      
subtitle L4/x86 Version X

kernel_family      = lime_pip 
 
kernel_type        = x86   
 
kernel_char        = '5'									 

gver = 2

kernel_x2	   = 0

redirection        = 0
prio_sensitive_donation_on_send = 1
precise_cycles     = 0
random_sampling                 = 1      	
random_profiling                = 1   ; = 1 REQUIRES random_sampling = 1
fast_myself                     = 1
V4_clock_features               = 0

  ENDIF	


 
  


