;----------------------------------------------------------------------------
;
;       xpdir / xptab     (calc pdir/ptab index out of address)
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       addr  linear address
;       reg   32 bit register
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reg   index within pdir/ptab corresponding to addr,
;             i.e. pdir/ptab + 4*reg hold pdir/ptab entry
;
;----------------------------------------------------------------------------


xpdir macro reg,addr

 IFDIF <reg>,<addr>
  mov reg,addr
 ENDIF
 shr  reg,22
 endm



xptab macro reg,addr

 IFDIF <reg>,<addr>
  mov reg,addr
 ENDIF
 and  reg,003FF000h
 shr  reg,12
 endm 
 
 
 


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
 




load__root macro reg,task_no

 mov reg,ds:[(task_no*4)+task_root]
 endm 


store_root macro reg,task_no

 mov  ds:[(task_no*4)+task_root],reg
 endm 


store_inactive_root macro reg,task_no

 mov  ds:[(task_no*4)+task_root],reg
 endm
 
 


  
