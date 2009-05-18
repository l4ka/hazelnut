
;----------------------------------------------------------------------------
;
;       load proot
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       task_no
;       reg   proot addr of task (store only)
;
;       DS    linear space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reg contains proot of task (load only)
;
;----------------------------------------------------------------------------
 



load__proot macro reg,task_no

 ke  'load_proot'
 endm 




  
