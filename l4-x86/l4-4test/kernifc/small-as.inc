


;----------------------------------------------------------------------------
;
;       cmp with small_address_space_size
;
;----------------------------------------------------------------------------

sass32opn macro opn,reg,labpre,labno,value

  IFIDN <reg>,<eax>
    labpre&labno equ $+1
  ELSE  
    labpre&labno equ $+2
  ENDIF  
  opn   reg,value

  endm
  
  
sass8opn macro opn,reg,labpre,labno,value

  IFIDN <reg>,<al>
    labpre&labno equ $+1
  ELSE  
    labpre&labno equ $+2
  ENDIF  
  opn   reg,value

  endm



sass32x=0
sass8x=0
sass8y=0


sass__32 macro opn,reg,value
  sass32x = sass32x+1
  sass32opn opn,reg,sass32x,%sass32x,value
 endm


sass__8 macro opn,reg,value
  sass8x = sass8x+1
  sass8opn opn,reg,sass8x,%sass8x,value
 endm


sass__8MB4 macro opn,reg,value
  sass8y = sass8y+1
  sass8opn opn,reg,sass8y,%sass8y,value
 endm





;----------------------------------------------------------------------------
;
;       update small address space size immediates
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       CL    log2 (old small_address_space_size)
;       CH    log2 (new small_address_space_size)
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX, EBX    scratch
;
;----------------------------------------------------------------------------



update_small_space_size_immediates macro

 IF (sass32x+sass8y) GE 1
  sub eax,eax
  inc eax
  mov ebx,eax
  shl ebx,cl
  xchg cl,ch
  shl eax,cl
  xchg cl,ch
  sub eax,ebx
 ENDIF 
     
 IF sass32x GE 1
  db  01h,05h    ; add   dword ptr ds:[sass32x1+PM],eax
  dd  sass32x1+PM
 ENDIF
 IF sass32x GE 2
  db  01h,05h    ; add   dword ptr ds:[sass32x2+PM],eax
  dd  sass32x2+PM
 ENDIF
 IF sass32x GE 3
  db  01h,05h    ; add   dword ptr ds:[sass32x3+PM],eax
  dd  sass32x3+PM
 ENDIF
 IF sass32x GE 4
  db  01h,05h    ; add   dword ptr ds:[sass32x4+PM],eax
  dd  sass32x4+PM
 ENDIF
 IF sass32x GE 5
  db  01h,05h    ; add   dword ptr ds:[sass32x5+PM],eax
  dd  sass32x5+PM
 ENDIF
 IF sass32x GE 6
  db  01h,05h    ; add   dword ptr ds:[sass32x6+PM],eax
  dd  sass32x6+PM
 ENDIF
 IF sass32x GE 7
  db  01h,05h    ; add   dword ptr ds:[sass32x7+PM],eax
  dd  sass32x7+PM
 ENDIF

 IF sass8y GE 1
  sar eax,22
  db  00h,05h    ; add   byte ptr ds:[offset sass8y1+PM],al
  dd  sass8y1+PM
 ENDIF
 IF sass8y GE 2
  db  00h,05h    ; add   byte ptr ds:[offset sass8y2+PM],al
  dd  sass8y2+PM
 ENDIF
 IF sass8y GE 3
  db  00h,05h    ; add   byte ptr ds:[offset sass8y3+PM],al
  dd  sass8y3+PM
 ENDIF
 IF sass8y GE 4
  db  00h,05h    ; add   byte ptr ds:[offset sass8y4+PM],al
  dd  sass8y4+PM
 ENDIF
 IF sass8y GE 5
  db  00h,05h    ; add   byte ptr ds:[offset sass8y5+PM],al
  dd  sass8y5+PM
 ENDIF
 IF sass8y GE 6
  db  00h,05h    ; add   byte ptr ds:[offset sass8y6+PM],al
  dd  sass8y6+PM
 ENDIF
 IF sass8y GE 7
  db  00h,05h    ; add   byte ptr ds:[offset sass8y7+PM],al
  dd  sass8y7+M
 ENDIF
 
 IF sass8x GE 1
  mov  al,ch
  sub  al,cl       
  db  00h,05h    ; add   byte ptr ds:[offset sass8x1+PM],al
  dd  sass8x1+PM
 ENDIF
 IF sass8x GE 2
  db  00h,05h    ; add   byte ptr ds:[offset sass8x2+PM],al
  dd  sass8x2+PM
 ENDIF
 IF sass8x GE 3
  db  00h,05h    ; add   byte ptr ds:[offset sass8x3+PM],al
  dd  sass8x3+PM
 ENDIF
 IF sass8x GE 4
  db  00h,05h    ; add   byte ptr ds:[offset sass8x4+PM],al
  dd  sass8x4+PM
 ENDIF
 IF sass8x GE 5
  db  00h,05h    ; add   byte ptr ds:[offset sass8x5+PM],al
  dd  sass8x5+PM
 ENDIF
 IF sass8x GE 6
  db  00h,05h    ; add   byte ptr ds:[offset sass8x6+PM],al
  dd  sass8x6+PM
 ENDIF
 IF sass8x GE 7
  db  00h,05h    ; add   byte ptr ds:[offset sass8x7+PM],al
  dd  sass8x7+PM
 ENDIF

.erre sass32x LE 7

 endm

