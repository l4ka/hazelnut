include schedcb2.inc



;----------------------------------------------------------------------------
;
;       timeslice, ticker & wakeups
;
;----------------------------------------------------------------------------


std_timeslice_length       equ 10



;----------------------------------------------------------------------------
;
;       double linked lists
;
;----------------------------------------------------------------------------


double_linked_list struc

  succ           dd 0
  pred           dd 0

double_linked_list ends


double_linked_list_root struc

  head           dd 0
  tail           dd 0

double_linked_list_root ends



;----------------------------------------------------------------------------
;
;       double linked list init
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       list     link offset in tcb
;
;----------------------------------------------------------------------------

llinit macro tcb,list

 or  [tcb+list_state],is_&list&
 mov [tcb+list&_llink].head,tcb
 mov [tcb+list&_llink].tail,tcb
 endm



;----------------------------------------------------------------------------
;
;       double linked list ins
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       entry    tcb to be inserted (REG)
;       root     addr of root (REG)
;       temp     REG
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       temp      scratch
;
;----------------------------------------------------------------------------


llins macro entry,root,temp,list

 or  [entry+list_state],is_&list&
 mov [entry+list&_llink].pred,root
 mov temp,[root+list&_llink].succ
 mov [root+list&_llink].succ,entry
 mov [entry+list&_llink].succ,temp
 mov [temp+list&_llink].pred,entry
 endm




;----------------------------------------------------------------------------
;
;       double linked list del
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       entry    tcb to be deleted (REG)
;       temp...  REG
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       temp_succ   succ of deleted entry
;       temp_pred   pred of deleted entry
;
;       [deleted].succ = 0
;
;----------------------------------------------------------------------------


lldel macro entry,temp_succ,temp_pred,list

 and  [entry+list_state],NOT is_&list&
 mov  temp_succ,[entry+list&_llink].succ
 mov  temp_pred,[entry+list&_llink].pred
 mov  [temp_succ+list&_llink].pred,temp_pred
 mov  [temp_pred+list&_llink].succ,temp_succ
 endm






;----------------------------------------------------------------------------
;
;       single linked lists
;
;----------------------------------------------------------------------------


;----------------------------------------------------------------------------
;
;       single linked list init
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       list     link offset in tcb
;
;----------------------------------------------------------------------------

linit macro list

 mov  ds:[dispatcher_tcb+&list&_link],0
 endm



;----------------------------------------------------------------------------
;
;       single linked list root / next / first
;
;----------------------------------------------------------------------------
; next PRECONDITION:
;
;       list     link offset in tcb
;       tcb      first / next tcb in list
;       pred     predecessing tcb in list
;
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; root  POSTCONDITION:
;
;       tcb      root
;
;----------------------------------------------------------------------------
; next  POSTCONDITION:
;
;       pred     predecessing tcb in list
;
;       tcb      NZ:  first/next tcb
;                Z:   end of list
;
;----------------------------------------------------------------------------
; first POSTCONDITION:
;
;       tcb      first
;
;----------------------------------------------------------------------------


movl__root macro tcb,list

 mov  tcb,offset dispatcher_tcb
 endm


movl__next macro tcb,pred,list

 mov  pred,tcb
 mov  tcb,[tcb+&list&_link]
 test tcb,tcb
 endm



;----------------------------------------------------------------------------
;
;       single linked list ins
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       list     link offset in tcb
;       tcb      tcb to be inserted
;       temp     REG
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       temp      scratch
;
;----------------------------------------------------------------------------


lins macro tcb,temp,list

 or   [tcb+list_state],is_&list&
 mov  temp,tcb
 xchg temp,ds:[dispatcher_tcb+&list&_link]
 mov  [tcb+&list&_link],temp
 endm



linsr macro tcb,temp,list,listmask

 or   [tcb+list_state],listmask
 mov  temp,tcb
 xchg temp,ds:[dispatcher_tcb+list]
 mov  [tcb+list],temp
 endm



;----------------------------------------------------------------------------
;
;       single linked list del
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       list     link offset in tcb
;       tcb      tcb to be deleted
;       pred     predecessing tcb in list
;
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       tcb      pred
;       pred     scratch
;
;----------------------------------------------------------------------------


ldel macro tcb,pred,list

 and  [tcb+list_state],NOT is_&list&
 mov  tcb,[tcb+&list&_link]
 mov  [pred+&list&_link],tcb
 mov  tcb,pred
 endm




;----------------------------------------------------------------------------
;
;       mark ready
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       tcb   tcb (write address) to be marked as ready
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX, EDI  scratch
;
;       thread marked ready
;
;----------------------------------------------------------------------------


mark__ready macro tcb

  test  [tcb+list_state],is_ready
  IFZ
    IFDIF <tcb>,<ebx>
      push ebx
      mov  ebx,tcb
    ENDIF
    call  insert_into_ready_list
    IFDIF <tcb>,<ebx>
      pop  ebx
    ENDIF
  FI
  endm




;----------------------------------------------------------------------------
;
;       mark interrupted
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       tcb   =  EBX    tcb (write address) to be marked as ready
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       thread marked interrupted (also ready)
;
;       EAX   scratch
;
;----------------------------------------------------------------------------


mark__interrupted macro tcb

  mark__ready tcb

  endm






