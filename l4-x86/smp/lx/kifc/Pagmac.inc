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
 mov reg, task_no
 shl reg, max_cpu
 push edi
 lno___prc edi
 add reg, edi
 pop edi
 mov reg, ds:[(reg*8)+task_proot+4-8]
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
 push ecx
 mov  ecx, 1 SHL max_cpu
 shl  task_no, max_cpu
 DO
	mov  ds:[(task_no*8)+task_proot].switch_ptr,reg
	mov  ds:[(task_no*8)+task_proot].proot_ptr,reg
	inc task_no
    	dec ecx
	REPEATNZ
 OD
 pop ecx
 endm 


store_inactive_proot macro reg,task_no
 shl  task_no, max_cpu
 IFIDN <task_no>,<edi>
 	push esi
	lno___prc esi
	add task_no, esi
	pop edi
 ELSE
	push edi
	lno___prc edi
	add task_no, edi
	pop edi
 ENDIF
 mov  ds:[(task_no*8)+task_proot-8],reg
 mov  ds:[(task_no*8)+task_proot+4-8],0
 endm
 
 
chnge_proot macro reg,task_no
 shl  task_no, max_cpu
 IFIDN <task_no>,<edi>
 	push esi
	lno___prc esi
	add task_no, esi
	pop edi
 ELSE
	push edi
	lno___prc edi
	add task_no, edi
	pop edi
 ENDIF
 test byte ptr ds:[(task_no*8)+task_proot+3-8],80h
 IFZ  
    mov  ds:[(task_no*8)+task_proot-8].switch_ptr,reg
 FI   
 and  ds:[(task_no*8)+task_proot-8].proot_ptr,000000FFh   
 or   ds:[(task_no*8)+task_proot-8].proot_ptr,reg
 endm 


  

