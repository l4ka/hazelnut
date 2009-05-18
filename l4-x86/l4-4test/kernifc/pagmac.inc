;----------------------------------------------------------------------------
;
;       linktab 
;
;----------------------------------------------------------------------------
 
 
linktab_entry_for_ptab struc

  pmap_link   dd 0
  pdir_link   dd 0
  
linktab_entry_for_ptab ends


linktab_entry_for_data_page struc

  ptab_link   dd 0
              dd 0
              
linktab_entry_for_data_page ends



linksize      equ (sizeof linktab_entry_for_ptab)

               
 


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
 



 IF kernel_type NE i486



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
 mov  edi,task_no
 call update_thread_proot
 pop  edi
 endm 


  



 ENDIF

