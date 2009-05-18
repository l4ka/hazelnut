
;----------------------------------------------------------------------------
;
;       load/store/change proot
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
 



x86_proot struc 

  switch_ptr  dd    0
  proot_ptr   dd    0
  
x86_proot ends

x86_prootx struc

              dd    0
  small_as    db    0
              db    0,0,0
              
x86_prootx ends


load__proot macro reg,task_no

 mov reg,ds:[(task_no*8)+task_proot+4]
 IFIDN <reg>,<eax>
  mov al,0
 ELSEIFIDN <reg>,<ebx>
  mov bl,0
 ELSEIFIDN <reg>,<ecx>
  mov cl,0
 ELSEIFIDN <reg>,<edx>
  mov dl,0
 ELSE 
  and reg,0FFFFFF00h   
 ENDIF 
 endm 


store_proot macro reg,task_no

 mov  ds:[(task_no*8)+task_proot].switch_ptr,reg
 mov  ds:[(task_no*8)+task_proot].proot_ptr,reg
 endm 


store_inactive_proot macro reg,task_no

 mov  ds:[(task_no*8)+task_proot],reg
 mov  ds:[(task_no*8)+task_proot+4],0
 endm
 
 
chnge_proot macro reg,task_no

 test byte ptr ds:[(task_no*8)+task_proot+3],80h
 IFZ  
    mov  ds:[(task_no*8)+task_proot].switch_ptr,reg
 FI   
 and  ds:[(task_no*8)+task_proot].proot_ptr,000000FFh   
 or   ds:[(task_no*8)+task_proot].proot_ptr,reg
 endm 


  
