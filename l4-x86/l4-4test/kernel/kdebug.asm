include l4pre.inc

 
  dcode

  Copyright GMD, L4.KDEBUG, 01,01,97, 200


;*********************************************************************
;******                                                         ******
;******                L4 Kernel Debug                          ******
;******                                                         ******
;******                                                         ******
;******                                                         ******
;******                                   created:  26.03.91    ******
;******                                   modified: 16.07.96    ******
;******                                                         ******
;*********************************************************************
 

.nolist
include l4const.inc
include uid.inc
include adrspace.inc
include intrifc.inc
include tcb.inc
include cpucb.inc
include schedcb.inc
include syscalls.inc
include pagconst.inc
include pagmac.inc
include pagcb.inc
include pnodes.inc
include l4kd.inc
.list


ok_for i486,pentium,ppro,k6



;94-11-15   jl   new:  no remote kdebug io before '+' char received remote



  public init_default_kdebug
  public default_kdebug_exception
  public default_kdebug_end


  extrn init_kdio:near
  extrn open_debug_keyboard:near
  extrn close_debug_keyboard:near
  extrn set_remote_info_mode:near
  extrn init_debug_screen:near
  extrn open_debug_screen:near
  extrn kd_outchar:near
  extrn kd_incharety:near
  extrn kd_inchar:near
  extrn kd_kout:near
  extrn old_pic1_imr:byte
  extrn irq0_intr:abs

  extrn first_lab:byte
  extrn kcod_start:byte
  extrn cod_start:byte
  extrn dcod_start:byte
  extrn scod_start:byte
  extrn kernelstring:byte

  IF    kernel_x2
        extrn enter_single_processor_mode:near
        extrn exit_single_processor_mode:near
  ENDIF      




  assume ds:codseg
 

;----------------------------------------------------------------------------
;
;       screen
;
;----------------------------------------------------------------------------


lines         equ 25
columns       equ 80


;----------------------------------------------------------------------------
;
;       kd intr area
;
;----------------------------------------------------------------------------


  IF kernel_type EQ i486
        additional_kd_save_area_size  equ 8
  ELSE
        additional_kd_save_area_size  equ 4
  ENDIF
  
  
kd_save_area struc

  kd_es       dw    0,0
  kd_ds       dw    0,0

kd_save_area ends





kdpre  macro

  push  0
  pushad
  push  ds
  push  es

  endm



kdpost macro

  pop   es
  pop   ds
  popad
  add   esp,4
  iretd

  endm









;----------------------------------------------------------------------------
;
;       data
;
;----------------------------------------------------------------------------

              align 4

kdebug_sema          dd 0

kdebug_esp           dd 0
kdebug_text          dd 0


kdebug_segs_struc struc

  ds_sreg     dw 0
  es_sreg     dw 0

kdebug_segs_struc ends


                    align 4

breakpoint_base     dd 0

breakpoint_thread   dd 0
no_breakpoint_thread dd 0

bx_low              dd 0
bx_high             dd 0
bx_addr             dd 0
bx_size             db 0

debug_exception_ko_flag     db false
debug_exception_active_flag db false

                    align 4

debug_exception_handler dd 0



page_fault_prot_state db 0
ipc_prot_state	  db 0

niltext             db 0

                    align 4

page_fault_low      dd 0
page_fault_high     dd 0FFFFFFFFh


page_fault_handler  dd 0

ipc_handler		dd 0


timer_intr_handler  dd 0

monitored_exception_handler     dd 0
monitored_ec_min                dw 0
monitored_ec_max                dw 0
monitored_exception             db 0
exception_monitoring_flag       db false
                                db 0,0


kdebug_buffer       db 32 dup (0)

                    db 'HISTORY:'
history             dd 32 dup (0)
                    db ':END' 
history_pointer     dd    offset history+PM

;----------------------------------------------------------------------------
;
;       init kdebug
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       paging enabled
;
;       SS    linear kernel space
;
;----------------------------------------------------------------------------


  icode


init_default_kdebug:

  mov   al,'a'
  call  init_kdio
  ret


  icod  ends


;----------------------------------------------------------------------------
;
;       prep ds / prep ds & eax
;
;----------------------------------------------------------------------------


prep_ds_es:

  IFAE  esp,<physical_kernel_mem_size>
        push  phys_mem
        pop   ds
        push  phys_mem
        pop   es
  FI

  ret



;----------------------------------------------------------------------------
;
;       kdebug IO call
;
;----------------------------------------------------------------------------

        align 4

kdebug_io_call_tab  dd    kd_outchar  ;  0
                    dd    outstring   ;  1
                    dd    outcstring  ;  2
                    dd    clear_page  ;  3
                    dd    cursor      ;  4
 
                    dd    outhex32    ;  5
                    dd    outhex20    ;  6
                    dd    outhex16    ;  7
                    dd    outhex12    ;  8
                    dd    outhex8     ;  9
                    dd    outhex4     ; 10
                    dd    outdec      ; 11
 
                    dd    kd_incharety; 12
                    dd    kd_inchar   ; 13
                    dd    inhex32     ; 14
                    dd    inhex16     ; 15
                    dd    inhex8      ; 16
                    dd    inhex32     ; 17

kdebug_io_calls     equ 18




kdebug_io_call:

  IFAE  esp,<physical_kernel_mem_size>
        push  phys_mem
        pop   ds
        push  phys_mem
        pop   es
  FI

  movzx ebx,ah
  IFB_  ebx,kdebug_io_calls

        mov   eax,[ebp+ip_eax]
        mov   ebx,[ebx*4+kdebug_io_call_tab]
        IFA   esp,<physical_kernel_mem_size>
              add   ebx,PM
        FI      
        call  ebx
        mov   [ebp+ip_eax],eax
  ELSE_
        mov   al,ah
        call  kd_kout
  FI

  jmp   ret_from_kdebug




void:

  ret



;----------------------------------------------------------------------------
;
;       kdebug display
;
;----------------------------------------------------------------------------


kdebug_display:

  IFAE  esp,<physical_kernel_mem_size>
        push  phys_mem
        pop   ds
        push  phys_mem
        pop   es
  FI

  lea   eax,[ebx+2]
  call  outstring

  jmp   ret_from_kdebug



;----------------------------------------------------------------------------
;
;       outstring
;
;----------------------------------------------------------------------------
; outstring PRECONDITION:
;
;       EAX   string addr (phys addr or linear addr (+PM))
;             string format: len_byte,text
;
;----------------------------------------------------------------------------
; outcstring PRECONDITION:
;
;       EAX   string addr (phys addr or linear addr (+PM))
;             string format: text,00
;
;----------------------------------------------------------------------------
 
 
outstring:

  and   eax,NOT PM

  mov   cl,[eax]
  inc   eax

  mov   ebx,eax
  IFNZ  cl,0
        DO
              mov   al,[ebx]
              call  kd_outchar
              inc   ebx
              sub   cl,1
              REPEATNZ
        OD
  FI
 
  ret



outcstring:

  and   eax,NOT PM

  mov   cl,255
  mov   ebx,eax
  IFNZ  cl,0
        DO
              mov   al,[ebx]
              test  al,al
              EXITZ
              call  kd_outchar
              inc   ebx
              sub   cl,1
              REPEATNZ
        OD
  FI
 
  ret
 
 
 
;----------------------------------------------------------------------------
;
;       cursor
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       AL    x
;       AH    y
;
;----------------------------------------------------------------------------


cursor:
 
  push  eax
  mov   al,6
  call  kd_outchar
  mov   al,byte ptr ss:[esp+1]
  call  kd_outchar
  pop   eax
  jmp   kd_outchar



;----------------------------------------------------------------------------
;
;       clear page
;
;----------------------------------------------------------------------------


clear_page:
 
  push  eax
  push  ebx

  mov   bl,lines-1
  mov   al,1
  call  kd_outchar
  DO
        mov   al,5
        call  kd_outchar
        mov   al,10
        call  kd_outchar
        dec   bl
        REPEATNZ
  OD
  mov   al,5
  call  kd_outchar

  pop   ebx
  pop   ecx
  ret


 
 
 
;----------------------------------------------------------------------------
;
;       outhex
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       AL / AX / EAX   value
;
;----------------------------------------------------------------------------


outhex32:

  rol   eax,16
  call  outhex16
  rol   eax,16


outhex16:

  xchg  al,ah
  call  outhex8
  xchg  al,ah


outhex8:

  ror   eax,4
  call  outhex4
  rol   eax,4



outhex4:

  push  eax
  and   al,0Fh
  add   al,'0'
  IFA   al,'9'
        add   al,'a'-'0'-10
  FI
  call  kd_outchar
  pop   eax
  ret



outhex20:

  ror   eax,16
  call  outhex4
  rol   eax,16
  call  outhex16
  ret



outhex12:

  xchg  al,ah
  call  outhex4
  xchg  ah,al
  call  outhex8
  ret





;----------------------------------------------------------------------------
;
;       outdec
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   value
;
;----------------------------------------------------------------------------


outdec:

  push  eax
  push  edx

  sub   edx,edx
  push  ebx
  mov   ebx,10
  div   ebx
  pop   ebx
  test  eax,eax
  IFNZ
        call  outdec
  FI
  mov   al,'0'
  add   al,dl
  call  kd_outchar

  pop   edx
  pop   eax
  ret


;----------------------------------------------------------------------------
;
;       inhex
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       AL / AX / EAX   value
;
;----------------------------------------------------------------------------


inhex32:

  push  ecx
  mov   cl,8
  jmp   short inhex


inhex16:

  push  ecx
  mov   cl,4
  jmp   short inhex


inhex8:

  push  ecx
  mov   cl,2


inhex:

  push  edx

  sub   edx,edx
  DO
        kd____inchar

        IFZ   al,'.'
        CANDZ ebx,17
        CANDA cl,2

              call  kd_outchar
              call  inhex8
              and   eax,lthreads-1
              shl   edx,width lthread_no
              add   edx,eax
              EXIT
        FI

        mov   ch,al
        sub   ch,'0'
        EXITC
        IFA   ch,9
              sub   ch,'a'-'0'-10
              EXITC
              cmp   ch,15
              EXITA
        FI
        call  kd_outchar
        shl   edx,4
        add   dl,ch
        dec   cl
        REPEATNZ
  OD
  mov   eax,edx

  pop   edx
  pop   ecx
  ret







;----------------------------------------------------------------------------
;
;       show
;
;----------------------------------------------------------------------------


show macro string,field,aoff

xoff=0
IFNB <aoff>
xoff=aoff
ENDIF

  kd____disp  <string>
  IF sizeof field eq 1
        mov   al,[esi+field+xoff]
        kd____outhex8
  ENDIF
  IF sizeof field eq 2
        mov   ax,[esi+field+xoff]
        kd____outhex16
  ENDIF
  IF sizeof field eq 4
        mov   eax,[esi+field+xoff]
        kd____outhex32
  ENDIF
  IF sizeof field eq 8
        mov   eax,[esi+field+xoff]
        kd____outhex32
        mov   al,' '
        kd____outchar
        mov   eax,[esi+field+xoff+4]
        kd____outhex32
  ENDIF
  endm


;----------------------------------------------------------------------------
;
;       kdebug exception  (kernel exception)
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       stack like ipre
;
;----------------------------------------------------------------------------






default_kdebug_exception:


  kdpre

  lea   ebp,[esp+additional_kd_save_area_size]


  IFAE  ebp,<physical_kernel_mem_size>
        push  phys_mem
        pop   ds
  FI
  
  IF    kernel_x2
        call  enter_single_processor_mode
  ENDIF      

  movzx eax,[ebp+ip_code]
  lea   esi,[(eax*2)+id_table]

  IFZ   al,3
        mov   ebx,[ebp+ip_eip]
        and   ebx,NOT PM

        IFZ   [ebp+ip_cs],linear_space_exec
        CANDA ebp,<offset tcb_space>

              push  ds
              push  es
              push  eax

              push  ds
              pop   es
              push  linear_space
              pop   ds
              mov   edi,offset kdebug_buffer
              push  edi
              mov   al,sizeof kdebug_buffer
              DO
                    mov   ah,0
                    test__page_present ebx
                    IFNC
                          mov   ah,ds:[ebx]
                    FI
                    mov   es:[edi],ah
                    inc   ebx
                    inc   edi
                    dec   al
                    REPEATNZ
              OD
              pop   ebx

              pop   eax
              pop   es
              pop   ds
        FI

        mov   ax,[ebx]
        cmp   al,3Ch           ; cmp al
        jz    kdebug_io_call
        cmp   al,90h           ; nop
        jz    kdebug_display


        inc   ebx
        IFZ   ah,4
        CANDZ <byte ptr [ebx+4]>,<PM SHR 24>
              mov  ebx,[ebx+1]
              add  ebx,4
        FI

  ELIFAE al,8
  CANDBE al,17
  CANDNZ al,16

        mov    cl,12
        mov    edi,offset ec_exception_error_code
        DO
               mov     eax,ss:[ebp+ip_error_code]
               shr     eax,cl
               and     eax,0Fh
               IFB_    al,10
                       add     al,'0'
               ELSE_
                       add     al,'a'-10
               FI
               mov     [edi],al
               inc     edi
               sub     cl,4
               REPEATNC
        OD
        mov    ax,[esi]
        mov    word ptr [ec_exception_id],ax
        mov    ebx,offset ec_exception_string+PM
  ELSE_
        mov    ax,[esi]
        mov    word ptr [exception_id],ax
        mov    ebx,offset exception_string+PM
  FI


  cli

  IFAE  ebp,<physical_kernel_mem_size>
        mov   edi,phys_mem
        mov   ds,edi
        mov   es,edi
        and   ebx,NOT PM

        DO
              mov   edi,[kdebug_sema]
              test  edi,edi
              EXITZ
              xor   edi,esp
              and   edi,-sizeof tcb
              EXITZ
              pushad
              push  ds
              push  es
              sub   esi,esi
              int   thread_switch
              pop   es
              pop   ds
              popad
              REPEAT
        OD
        mov   [kdebug_sema],ebp
  FI


  push  [kdebug_esp]
  push  [kdebug_text]

  mov   [kdebug_esp],ebp
  mov   [kdebug_text],ebx


;;call  open_debug_keyboard
  call  open_debug_screen


  kd____disp  <6,lines-1,0,13,10>
  mov   ecx,columns-12
  DO
        mov   al,'-'
        kd____outchar
        RLOOP
  OD
  mov   eax,[ebp+ip_eip]
  kd____outhex32

  kd____disp  <'=EIP',13,10,6,lines-1,6>
  call  out_id_text

  DO
        call  kernel_debug
        cmp   bl,'g'
        REPEATNZ
  OD

  pop   [kdebug_text]
  pop   [kdebug_esp]

  mov   [kdebug_sema],0

  IFZ   [ebp+ip_code],debug_exception
  mov   eax,dr7
  test  al,10b
  CANDNZ
  shr   eax,16
  test  al,11b
  CANDZ
        bts   [ebp+ip_eflags],r_flag
  FI




ret_from_kdebug:

  IF    kernel_x2
        call  exit_single_processor_mode
  ENDIF      

  kdpost







id_table db 'DVDBNM03OVBNUD07DF09TSNPSFGPPF15FPAC'

exception_string        db 14,'L4 Kernel: #'
exception_id            db 'xx'

ec_exception_string     db 21,'L4 Kernel: #'
ec_exception_id         db 'xx ('
ec_exception_error_code db 'xxxx)'




;----------------------------------------------------------------------------
;
;       kernel debug
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       DS,ES    phys mem
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       BL       exit char
;
;       EAX,EBX,ECX,EDX,ESI,EDI,EBP  scratch
;
;----------------------------------------------------------------------------


kernel_debug:
 
  push  ebp

  call  open_debug_keyboard
  call  open_debug_screen
  DO
        kd____disp  <6,lines-1,0,10>
        call  get_kdebug_cmd

        DO
              cmp   al,'h'
              IFNZ
                    cmp   al,'g'
              FI
              jz    exit_kdebug

              IFZ   al,'a'
                    call  display_module_addresses
              ELIFZ al,'b',long
                    call  set_breakpoint
              ELIFZ al,'t',long
                    call  display_tcb
                    cmp   al,0
                    REPEATNZ
              ELIFZ al,'d',long
                    call  display_mem
                    cmp   al,0
                    REPEATNZ
              ELIFZ al,'p',long
                    call  display_ptabs
                    cmp   al,0
                    REPEATNZ
              ELIFZ al,'m',long
                    call  display_mappings
                    cmp   al,0
                    REPEATNZ
              ELIFZ al,'k',long
                    call  display_kernel_data
              ELIFZ al,'P',long
                    call  page_fault_prot
              ELIFZ al,'X',long
                    call  monit_exception
              ELIFZ al,'I'
                    call  ipc_prot
              ELIFZ al,'R'
                    call  remote_kd_intr
              ELIFZ al,'v'
                    call  virtual_address_info
              ELIFZ al,'i'
                    call  port_io
              ELIFZ al,'o'
                    call  port_io
              ELIFZ al,'H'
                    call  halt_current_thread
              ELIFZ al,'T'
                    call  tracing_on_off
              ELIFZ al,' '
                    call  out_id_text
              ELIFZ al,'*'
                    call  init_debug_screen
              ELIFZ al,'V'
                    call  set_video_mode
              ELIFZ al,'y'
                    call  special_test
              ELSE_
                    call  out_help
              FI
        OD
        REPEAT
  OD

exit_kdebug:

  call  close_debug_keyboard
  mov   bl,al
  kd____disp  <13,10>

  pop   ebp

  ret



get_kdebug_cmd:

  IF    kernel_x2
        kd____disp  <6,lines-1,0,'L4KD('>
        lno___prc eax
        add   al,'a'
        kd____outchar
        kd____disp  <'): '>
  ELSE      
        kd____disp  <6,lines-1,0,'L4KD: '>
  ENDIF     
  kd____inchar
  push  eax
  IFAE  al,20h
        kd____outchar
  FI
  pop   eax

  ret



;----------------------------------------------------------------------------
;
;       out id text
;
;----------------------------------------------------------------------------


out_id_text:

  mov   al,'"'
  kd____outchar
  mov   eax,[esp]
  and   eax,PM
  mov   eax,cs:[eax+kdebug_text]
  kd____outstring
  mov   al,'"'
  kd____outchar
  ret


;----------------------------------------------------------------------------
;
;       help
;
;----------------------------------------------------------------------------

out_help:

  kd____disp  <13,10,'a  :  all modules,    axxxx     :  find module and rel addr'>
  kd____disp  <13,10,'b  :  show bkpnt,     bi/w/a    :  set instr/wr/rdwr bkpnt'>
  kd____disp  <13,10,'                      b-/t/x    :  reset/strict bkpnt (thr/adr)'>
  kd____disp  <13,10,'t  :  current tcb,    txxxxx    :  tcb of thread xxxx'>
  kd____disp  <13,10,'                      txxx.yy   :  task xxx, lthread yy'>
  kd____disp  <13,10,'d  :  dump mem,       dxxxxxxxx :  dump memory'>
  kd____disp  <13,10,'p  :  dump ptab,      pxxx      :  ptabs (pdir) of task xxxx'>
  kd____disp  <13,10,'                      pxxxxx000 :  ptab at addr xxxxx000'>
  kd____disp  <13,10,'m  :  dump mappings,  mxxxx     :  mappings of frame xxxx'>
  kd____disp  <13,10,'k  :  display kernel data'>
  kd____disp  <13,10,'P  :  page fault prot P+ : on, P-: off, P* : on,running'>
  kd____disp  <13,10,'X  :  monit exception X+ xx [yyyy,zzzz]: on, X-: off'>
  kd____disp  <13,10,'I  :  ipc prot I+ : on, I-: off, I* : on,running'>
  kd____disp  <13,10,'R  :  kdebug activation by remote ESC'>
  kd____disp  <13,10,'i  :  in  port        i1/2/4xxxx:  byte/word/dword'>
  IF    kernel_type NE i486
  kd____disp  <13,10,'          apic        ia/i/xxxx :  apic/io apic'>
  ENDIF
  kd____disp  <13,10,'o  :  out port/apic   o...'>
  kd____disp  <13,10,'H  :  halt current thread'>
  kd____disp  <13,10,'H  :  halt current thread'>
  kd____disp  <13,10,'T  :  single step     T+ : on, T-: off'>
  kd____disp  <13,10,'V  :  video mode      a:automatic, c:cga, m:monochrom, h:hercules'>
  kd____disp  <13,10,'   :  id text'>

  ret


;----------------------------------------------------------------------------
;
;       set video mode
;
;----------------------------------------------------------------------------

set_video_mode:

  kd____inchar
  IFZ   al,'T'
        call  kd_outchar
        mov   eax,2F8h SHL 12 + 7 SHL 8 + 3
        call  set_remote_info_mode
  ELIFZ al,'-'
        call  kd_outchar
        sub   eax,eax
        call  set_remote_info_mode

  ELSE_
        push  eax
        kd____outchar
        pop   eax
        call  init_kdio
  FI

  ret



;----------------------------------------------------------------------------
;
;       halt current thread
;
;----------------------------------------------------------------------------

halt_current_thread:

  kd____disp  <' are you sure? '>
  kd____inchar
  CORZ  al,'j'
  IFZ   al,'y'
        kd____outchar
        call  exit_kdebug
        sub   eax,eax
        mov   ss:[kdebug_sema+PM],eax

        mov   ebp,esp
        and   ebp,-sizeof tcb

        mov   ss:[ebp+coarse_state],unused_tcb
        mov   ss:[ebp+thread_state],dead

        DO
              sub   esi,esi
              int   thread_switch
              REPEAT
        OD

  FI
  mov   al,'n'
  kd____outchar
  ret


;----------------------------------------------------------------------------
;
;       display_module_addresses
;
;----------------------------------------------------------------------------


display_module_addresses:

  kd____inhex16
  test  eax,eax
  IFNZ  ,,long

        mov   esi,offset first_lab
        IFB_  eax,<offset cod_start>
              DO
                    cmp   word ptr [esi+8],'C('
                    EXITNZ
                    movzx edi,word ptr [esi]
                    test  edi,edi
                    IFZ
                          call  to_next_lab
                          REPEAT
                    FI
                    cmp   eax,edi
                    EXITB
                    mov   ebx,edi
                    mov   edx,esi
                    call  to_next_lab
                    REPEAT
              OD
        ELSE_
              DO
                    cmp   word ptr [esi+8],'C('
                    EXITNZ
                    movzx edi,word ptr [esi+2]
                    cmp   eax,edi
                    EXITB
                    mov   ebx,edi
                    mov   edx,esi
                    call  to_next_lab
                    REPEAT
              OD
        FI
        mov   esi,edx
        sub   eax,ebx
        IFNC
              push  eax
              mov   ah,lines-1
              mov   al,20
              kd____cursor
              call  display_module
              kd____disp  <' : '>
              pop   eax
              kd____outhex16
        FI


  ELSE_ long

        kd____clear_page
        mov   al,0
        mov   ah,1
        kd____cursor

        mov   eax,offset kernelstring
        kd____outcstring
        kd____disp <13,10,10,'kernel: '>

        mov   esi,offset first_lab

        DO
              cmp   word ptr [esi+8],'C('
              EXITNZ

              movzx edi,word ptr [esi+2]
              IFZ   edi,<offset dcod_start>
                    kd____disp <13,'kdebug: '>
              ELIFZ edi,<offset scod_start>
                    kd____disp <13,'sigma:  '>
              FI

              call  display_module
              kd____disp  <13,10,'        '>

              call  to_next_lab
              REPEAT
        OD
  FI
  ret


to_next_lab:

  add   esi,7
  DO
        inc   esi
        cmp   byte ptr [esi],0
        REPEATNZ
  OD
  DO
        inc   esi
        cmp   byte ptr [esi],0
        REPEATNZ
  OD
  inc   esi
  ret




display_module:

  movzx eax,word ptr [esi]
  test  eax,eax
  IFZ
        kd____disp <'     '>
  ELSE_
        kd____outhex16
        kd____disp <','>
  FI
  movzx eax,word ptr [esi+2]
  kd____outhex16
  mov   al,' '
  kd____disp <' (C) '>

  lea   ebx,[esi+8+3]
  mov   eax,ebx
  kd____outcstring
  kd____disp <', '>

  DO
        cmp   byte ptr [ebx],0
        lea   ebx,[ebx+1]
        REPEATNZ
  OD
  mov   eax,ebx
  kd____outcstring

  mov   edx,[esi+4]

  kd____disp <' '>  

  mov   eax,edx
  and   eax,32-1
  IFB_  eax,10
        push  eax
        mov   al,'0'
        kd____outchar
        pop   eax
  FI
  kd____outdec
  mov   al,'.'
  kd____outchar
  mov   eax,edx
  shr   eax,5
  and   eax,16-1
  IFB_  eax,10
        push  eax
        mov   al,'0'
        kd____outchar
        pop   eax
  FI
  kd____outdec
  mov   al,'.'
  kd____outchar
  mov   eax,edx
  shr   eax,5+4
  and   eax,128-1
  add   eax,90
  IFAE  eax,100
        sub   eax,100
  FI
  IFB_  eax,10
        push  eax
        mov   al,'0'
        kd____outchar
        pop   eax
  FI
  kd____outdec

  mov   al,' '
  kd____outchar
  mov   eax,edx
  shr   eax,16+10
  kd____outdec
  mov   al,'.'
  kd____outchar
  mov   eax,edx
  shr   eax,16
  and   ah,3
  kd____outdec

  ret



;----------------------------------------------------------------------------
;
;       set breakpoint
;
;----------------------------------------------------------------------------


set_breakpoint:

  kd____inchar
  IFZ   al,13

        mov   ah,lines-1
        mov   al,20
        kd____cursor

        mov   eax,dr7
        test  al,10b
        IFZ
              mov   al,'-'
              kd____outchar
        ELSE_
              shr   eax,8
              mov   al,'I'
              test  ah,11b
              IFNZ
                    mov   al,'W'
                    test  ah,10b
                    IFNZ
                          mov    al,'A'
                    FI
                    kd____outchar
                    mov   eax,dr7
                    shr   eax,18
                    and   al,11b
                    add   al,'1'
              FI
              kd____outchar
              kd____disp  <' at '>
              mov   ebx,dr0
              mov   eax,[breakpoint_base]
              test  eax,eax
              IFNZ
                    sub   ebx,eax
                    kd____outhex32
                    mov   al,'+'
                    kd____outchar
              FI
              mov   eax,ebx
              kd____outhex32
        FI
        ret
  FI

  IFZ   al,'-'
        kd____outchar
        sub   eax,eax
        mov   dr7,eax
        mov   dr6,eax
        mov   [bx_size],al
        mov   [debug_exception_ko_flag],false

        mov   eax,[debug_exception_handler]
        test  eax,eax
        IFNZ
              mov   bl,debug_exception
              call  set_exception_handler
              mov   [debug_exception_handler],0
        FI
        ret
  FI

  push  eax
  IFZ   [debug_exception_handler],0
        mov   bl,debug_exception
        call  get_exception_handler
        mov   [debug_exception_handler],eax
  FI
  mov   bl,debug_exception
  mov   eax,offset kdebug_debug_exception_handler
  call  set_exception_handler
  pop   eax

  IFZ   al,'b'
        kd____outchar
        kd____inhex32
        mov   [breakpoint_base],eax
        ret
  FI

  CORZ  al,'i'
  CORZ  al,'w'
  IFZ   al,'a'
        sub   ecx,ecx
        IFNZ  al,'i'
              cmp   al,'a'
              setz  cl
              lea   ecx,[(ecx*2)+1]
              kd____outchar
              kd____inchar
              IFZ   al,'2'
                    or    cl,0100b
              ELIFZ al,'4'
                    or    cl,1100b
              ELSE_
                    mov   al,'1'
              FI
        FI
        kd____outchar
        shl   ecx,16
        mov   cx,202h
        kd____disp  <' at: '>
        mov   eax,[breakpoint_base]
        test  eax,eax
        IFNZ
              kd____outhex32
              mov   al,'+'
              kd____outchar
        FI
        kd____inhex32
        add   eax,[breakpoint_base]
        mov   dr0,eax
        mov   dr7,ecx
        sub   eax,eax
        mov   dr6,eax

        ret
  FI

  IFZ   al,'x',long
        kd____outchar
        kd____inchar
        sub   al,'0'
        CORBE
        CORZ  al,3
        IFA   al,4
              mov   [bx_size],0
              mov   al,'-'
              kd____outchar
              ret
        FI

        mov   [bx_size],al
        add   al,'0'
        kd____outchar
        kd____disp  <' monit:'>
        kd____inhex32
        mov   [bx_addr],eax
        kd____disp  <'  ['>
        kd____inhex32
        mov   [bx_low],eax
        mov   al,','
        kd____outchar
        kd____inhex32
        mov   [bx_high],eax
        mov   al,']'
        kd____outchar
        ret
  FI

  IFZ   al,'t'
        kd____outchar
        kd____inhex16
        mov   [breakpoint_thread],eax
        ret
  FI

  IFZ   al,'T'
        kd____outchar
        kd____inhex16
        mov   [no_breakpoint_thread],eax
        ret
  FI

  IFZ   al,'k'
        kd____outchar
        mov   [debug_exception_ko_flag],true
        ret
  FI

  ret




kdebug_debug_exception_handler:


  push  eax
  mov   eax,dr6
  and   al,NOT 1b
  mov   dr6,eax

  lno___thread eax,esp

  IFZ   cs:[no_breakpoint_thread+PM],eax
        pop   eax
        bts   [esp+iret_eflags],r_flag
        iretd
  FI

  IFNZ  cs:[breakpoint_thread+PM],0
  cmp   cs:[breakpoint_thread+PM],eax
  CANDNZ
        pop   eax
        bts   [esp+iret_eflags],r_flag
        iretd
  FI
  pop   eax

  IFA   esp,max_physical_memory_size
  
        push  eax
        push  ebx
  
        mov   ebx,ss:[history_pointer+PM]
        mov   eax,[esp+iret_eip+2*4]
        mov   ss:[ebx],eax
        
        add   ebx,4
        IFAE  ebx,<offset history+sizeof history+PM>
              mov   ebx,offset history+PM
        FI
        mov   ss:[history_pointer+PM],ebx
        sub   eax,eax
        mov   ss:[ebx],eax
        
        pop   ebx
        pop   eax
  FI              
  
  
  call  check_monitored_data
  IFNZ
        iretd
  FI      
  
  

  IFZ   cs:[debug_exception_ko_flag+PM],true
        pushad
        kd____disp  <13,10,'#DB:'>
        lno___thread eax,esp
        kd____outhex16
        popad
        bts   [esp+iret_eflags],r_flag
        iretd
  FI
  jmp   cs:[debug_exception_handler+PM]



;----------------------------------------------------------------------------
;
;       check monitored data
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       Z     monitored data meets condition
;
;       NZ    no data monitored OR NOT monitored data meets condition
;
;----------------------------------------------------------------------------


check_monitored_data:

  IFAE   esp,<offset tcb_space>,long
  CANDB  esp,<offset tcb_space+tcb_space_size>,long
  CANDNZ ss:[bx_size+PM],0,long
  CANDZ  ss:[debug_exception_active_flag+PM],false,long

        pushad

        mov   ss:[debug_exception_active_flag+PM],true
        mov   ebx,ss:[bx_addr+PM]
        mov   ecx,ss:[bx_low+PM]
        mov   edx,ss:[bx_high+PM]
        mov   al,ss:[bx_size+PM]
        IFZ   al,1
              movzx eax,byte ptr ss:[ebx]
        ELIFZ al,2
              movzx eax,word ptr ss:[ebx]
        ELSE_
              mov   eax,ss:[ebx]
        FI
        mov   ss:[debug_exception_active_flag+PM],false

        IFBE  ecx,edx
              CORB  eax,ecx
              IFA   eax,edx
                    popad
                    test  esp,esp              ; NZ !
                    ret
              FI
        ELSE_
              IFA   eax,edx
              CANDB eax,ecx
                    popad
                    test  esp,esp              ; NZ !
                    ret
              FI
        FI
        popad
  FI
  
  cmp   eax,eax                                ; Z !
  ret




;----------------------------------------------------------------------------
;
;       display tcb
;
;----------------------------------------------------------------------------


display_tcb:

  CORB  esp,<offset tcb_space>
  IFAE  esp,<offset tcb_space+tcb_space_size>

        kd____clear_page
        mov   ebp,[kdebug_esp]
        add   ebp,sizeof tcb/2
        mov   esi,ebp
        call  display_regs_and_stack
        ret
  FI

  push  ds
  push  es
  mov   eax,linear_kernel_space
  mov   ds,eax
  mov   es,eax

  mov   ebp,esp
  and   ebp,-sizeof tcb

  kd____inhext
  test  eax,eax
  IFZ
        mov   esi,ebp
  ELSE_
        IFB_  eax,threads
              shl   eax,thread_no
        FI
        lea___tcb esi,eax
  FI

  test__page_present esi
  IFC
        kd____disp  <' not mapped, force mapping (y/n)? '>
        kd____inchar
        IFNZ  al,'y'
        CANDNZ al,'j'
              mov   al,'n'
              kd____outchar
              mov   al,0
              pop   es
              pop   ds
              ret
        FI
        or    byte ptr [esi],0
  FI

  kd____clear_page

  kd____disp  <6,0,0,'thread: '>
	lea	ebx,[esi+myself]
	call	show_thread_id

	kd____disp <'  Chief: '>
  mov	eax,[esi+chief]
	lno___thread eax,eax
	kd____outhex16


  IFNZ  [esi+resources],0
        kd____disp  <6,0,53,'resrc: '>
        mov   al,[esi+resources]
        test  al,mask x87_used
        IFNZ
              push  eax
              kd____disp <'num '>
              pop   eax
        FI
        test  al,mask dr_used
        IFNZ
              push  eax
              kd____disp <'dr '>
              pop   eax
        FI
        and   al,NOT (x87_used+dr_used)
        IFNZ
              kd____outhex8
        FI
  FI


  show  <6,1,0,'state : '>,coarse_state
  kd____disp  <', '>

  mov   ebx,[esi+thread_state]
  IFZ   ebx,ready
        kd____disp <'ready'>
  ELIFZ ebx,dead         
        kd____disp <'dead'>
  ELIFZ ebx,locked       
        kd____disp <'lock'>
  ELIFZ ebx,polling      
        kd____disp <'poll'>
  ELSE_
        kd____disp <'wait '>
        IFZ   ebx,waiting_inner
              kd____disp <'inner'>
        ELIFZ ebx,waiting_outer
              kd____disp <'outer'>
        ELIFZ ebx,waiting_none 
              kd____disp <'none'>
        ELIFZ ebx,waiting_any  
              kd____disp <'any'>
        ELSE_
              lea   ebx,[esi+thread_state]      
              call  show_thread_id
        FI
  FI              

  show  <6,1,53,'lists: '>,list_state

  show  <6,0,72,'prio: '>,prio
  IFNZ  [esi+max_controlled_prio],0
        show  <6,1,73,'mcp: '>,max_controlled_prio
  FI

  movzx eax,[esi+state_sp]
  shl   eax,2
  IFNZ
        push  eax
        kd____disp <6,2,42,'state_sp: '>
        pop   eax
        kd____outhex12
  FI


  kd____disp  <6,4,0, 'sndq    : '>
  lea   ecx,[esi+sndq_root]
  call  show_llinks
  mov   al,' '
  kd____outchar
  lea   ecx,[esi+sndq_llink]
  call  show_llinks

  show  <6,3,40,' rcv descr: '>,rcv_descriptor
  show  <6,4,40,' ipc contr: '>,ipc_control

  show  <6,3,60,'   partner: '>,com_partner
  kd____disp  <6,4,60,'   waddr0/1: '>
  mov   eax,[esi+waddr]
  kd____outhex32


  kd____disp  <6,5,0, 'cpu time: '>
  mov   al,[esi+cpu_clock_high]
  kd____outhex8
  mov   eax,[esi+cpu_clock_low]
  kd____outhex32

  show  <' timeslice: '>,rem_timeslice
  mov   al,'/'
  kd____outchar
  mov   al,[esi+timeslice]
  kd____outhex8

  kd____disp <6,7,0, 'pager   : '>
  lea   ebx,[esi+pager]
  call  show_thread_id

  show  <6,7,28, 'pdir:  '>,task_pdir
  mov   eax,[esi+task_pdir]
  IFNZ  eax,[esi+thread_proot]
        show	<6,8,28, 'proot: '>,thread_proot
  FI
  IFNZ  [esi+small_as],0
        show	<6,9,28, 'small: '>,small_as
  FI      
              

  kd____disp <6,8,0, 'ipreempt: '>
  lea   ebx,[esi+int_preempter]
  call  show_thread_id

  kd____disp <6,9,0, 'xpreempt: '>
  lea   ebx,[esi+ext_preempter]
  call  show_thread_id

  kd____disp  <6, 7,50, 'prsent lnk: '>
  test  [esi+list_state],is_present
  IFNZ
        lea   ecx,[esi+present_llink]
        call  show_llinks
  FI
  kd____disp  <6, 8,50, 'ready link: '>
  IFDEF ready_llink
        test  [esi+list_state],is_ready
        IFNZ
              lea   ecx,[esi+ready_llink]
              call  show_llinks
        FI
  ELSE
        lea   ecx,[esi+ready_link]
        call  show_link
        kd____disp  <6,9,50, 'intr link : '>
        lea   ecx,[esi+interrupted_link]
        call  show_link
  ENDIF

  kd____disp  <6,10,50, 'soon wakeup lnk: '>
  test  [esi+list_state],is_soon_wakeup
  IFNZ
        lea   ecx,[esi+soon_wakeup_link]
        call  show_link
  FI
  kd____disp  <6,11,50, 'late wakeup lnk: '>
  test  [esi+list_state],is_late_wakeup
  IFNZ  
        lea   ecx,[esi+late_wakeup_link]
        call  show_link
  FI      

  IFNZ  [esi+thread_idt_base],0
        kd____disp  <6,7,63,'IDT: '>
        mov   eax,[esi+thread_idt_base]
        kd____outhex32
  FI

  mov   eax,[esi+thread_dr7]
  test  al,10101010b
  IFZ   ,,long
  test  al,01010101b
  CANDNZ
        kd____disp  <6,9,63,'DR7: '>
        mov   eax,[esi+thread_dr7]
        kd____outhex32
        kd____disp  <6,10,63,'DR6: '>
        mov   al,[esi+thread_dr6]
        mov   ah,al
        and   eax,0000F00Fh
        kd____outhex32
        kd____disp  <6,11,63,'DR3: '>
        mov   eax,[esi+thread_dr3]
        kd____outhex32
        kd____disp  <6,12,63,'DR2: '>
        mov   eax,[esi+thread_dr2]
        kd____outhex32
        kd____disp  <6,13,63,'DR1: '>
        mov   eax,[esi+thread_dr1]
        kd____outhex32
        kd____disp  <6,14,63,'DR0: '>
        mov   eax,[esi+thread_dr0]
        kd____outhex32
  FI


  call  display_regs_and_stack

  pop   es
  pop   ds
  ret




show_thread_id:

  IFZ   <dword ptr [ebx]>,0

        kd____disp <'--'>
  ELSE_
        mov   eax,[ebx]
  	lno___thread eax,eax
  	push  eax
  	kd____outhex16
		mov	al,' '
  	kd____outchar
  	pop   eax

  	push  eax
  	shr   eax,width lthread_no
  	kd____outhex12
  	mov   al,'.'
  	kd____outchar
  	pop   eax  	
  	and   al,lthreads-1
  	kd____outhex8

		mov	al,'-'
		kd____outchar

		push	ebx
		mov	eax,[ebx]
		mov	ebx,eax
		and	eax,mask ver0
		shr	ebx,ver1
		shl	ebx,ver0
		add	eax,ebx
		kd____outhex16
		pop	ebx
  FI

  ret




show_llinks:

  mov   eax,[ecx].succ
  test  eax,eax
  IFNZ
  CANDNZ eax,-1
        call  show_link
        mov   al,1Dh
        kd____outchar
        add   ecx,offset pred
        call  show_link
        sub   ecx,offset pred
  FI
  ret



show_link:

  mov   eax,[ecx]
  IFAE  eax,<offset tcb_space>
  CANDB eax,<offset tcb_space+tcb_space_size>
        lno___thread eax,eax
        kd____outhex16
        ret
  FI
  IFAE  eax,<offset intrq_llink>
  CANDB eax,<offset intrq_llink+sizeof intrq_llink>
        push  eax
        kd____disp  <' i'>
        pop   eax
        sub   eax,offset intrq_llink
        shr   eax,3
        kd____outhex8
        ret
  FI
  IFAE  eax,<offset dispatcher_tcb>
  CANDB eax,<offset dispatcher_tcb+sizeof tcb>
        kd____disp  <' -- '>
        ret
  FI
  IFAE  eax,<offset intrq_llink>
  CANDB eax,<scheduler_control_block_size>
        kd____disp  <' -- '>
        ret
  FI
  test  eax,eax
  IFZ
        kd____disp  <' -- '>
        ret
  FI
  kd____outhex32
  ret




show_reg macro txt,reg

  kd____disp  <txt>
  mov   eax,[ebp+ip_&reg]
  kd____outhex32
  endm



show_sreg macro txt,sreg

  kd____disp  <txt>
  mov   ax,[ebp+ip_&sreg]
  kd____outhex16
  endm




display_regs_and_stack:

  IFZ   esi,ebp
  call  $+5
  pop   eax
  and   eax,PM
  mov   eax,cs:[eax+kdebug_esp]
  test  eax,eax
  CANDNZ
        mov   ebx,eax
        mov   ecx,eax
        lea   ebp,[eax+sizeof int_pm_stack-sizeof iret_vec]
  ELSE_
        mov   ebx,[esi+thread_esp]
        test  bl,11b
        CORNZ
        CORB  ebx,esi
        lea   ecx,[esi+sizeof tcb]
        IFAE  ebx,ecx
              sub   ebx,ebx
        FI
        sub   ecx,ecx                 ; EBX : stack top
        mov   ebp,ebx                 ; ECX : reg pointer / 0
  FI                                  ; EBP : cursor pointer

; IFAE  ebx,KB256
;       CORB  ebx,esi
;       lea   eax,[esi+sizeof pl0_stack]
;       IFAE  ebx,eax
;             mov   al,0
;             ret
;       FI
; FI


  DO
        pushad
        call  show_regs_and_stack
        popad

        call  get_kdebug_cmd
        cmp   al,20h
        EXITAE

        IFZ   al,2
              add   ebp,4
        FI
        IFZ   al,8
              sub   ebp,4
        FI
        IFZ   al,10
              add   ebp,32
        FI
        IFZ   al,3
              sub   ebp,32
        FI
        mov   edx,ebp
        and   edx,-sizeof tcb
        add   edx,sizeof pl0_stack-4
        IFA   ebp,edx
              mov   ebp,edx
        FI
        IFB_  ebp,ebx
              mov   ebp,ebx
        FI

        IFZ   al,13
              lea   ecx,[ebp-(sizeof int_pm_stack-sizeof iret_vec)]
              IFB_  ecx,ebx
                    mov   ecx,ebx
              FI
        FI

        REPEAT
  OD
  ret



show_regs_and_stack:

  test  ecx,ecx
  IFNZ  ,,long
        push  ebp
        mov   ebp,ecx
        show_reg <6,11,0, 'EAX='>,eax
        show_reg <6,12,0, 'EBX='>,ebx
        show_reg <6,13,0, 'ECX='>,ecx
        show_reg <6,14,0, 'EDX='>,edx
        show_reg <6,11,14,'ESI='>,esi
        show_reg <6,12,14,'EDI='>,edi
        show_reg <6,13,14,'EBP='>,ebp

        call  $+5
        pop   eax
        and   eax,PM
        IFZ   ebp,cs:[eax+kdebug_esp]

              kd____disp <6,11,28,'DS='>
              mov   ax,[ebp-additional_kd_save_area_size].kd_ds
              kd____outhex16
              kd____disp <6,12,28,'ES='>
              mov   ax,[ebp-additional_kd_save_area_size].kd_es
              kd____outhex16
        ELSE_
              kd____disp <6,11,28,'       ',6,12,28,'       '>
        FI
        pop   ebp
  FI
                                                 
  kd____disp  <6,14,14,'ESP='>
  mov   eax,ebp
  kd____outhex32

  test  ebp,ebp
  IFNZ  ,,long

        lea   ebx,[esi+sizeof pl0_stack-8*8*4]
        IFA   ebx,ebp
              mov   ebx,ebp
        FI
        and   bl,-32
        mov   cl,16
        DO
              mov   ah,cl
              mov   al,0
              kd____cursor
              mov   eax,ebx
              kd____outhex12
              mov   al,':'
              kd____outchar
              mov   al,5
              kd____outchar
              add   ebx,32
              inc   cl
              cmp   cl,16+8
              REPEATB
        OD
        lea   ebx,[esi+sizeof pl0_stack]
        sub   ebx,ebp
        IFC
              sub   ebx,ebx
        FI
        shr   ebx,2
        DO
              cmp   ebx,8*8
              EXITBE
              sub   ebx,8
              REPEAT
        OD
        sub   ebx,8*8
        neg   ebx
        DO
              mov   eax,ebx
              and   al,7
              imul  eax,9
              IFAE  eax,4*9
                    add   eax,3
              FI
              add   eax,6
              mov   ah,bl
              shr   ah,3
              add   ah,16
              kd____cursor
              mov   eax,[ebp]
              kd____outhex32
              inc   ebx
              add   ebp,4
              cmp   ebx,8*8
              REPEATB
        OD
  FI

  ret



;----------------------------------------------------------------------------
;
;       display mem
;
;----------------------------------------------------------------------------


display_mem:

  mov   [dump_area_base],0
  mov   [dump_area_size],linear_address_space_size

  kd____inhex32
  test  eax,eax
  IFZ
  mov   eax,ss
  CANDZ eax,linear_kernel_space
        kd____disp <' Gdt/Idt/Task/Linktab/Pnodes? '>
        kd____inchar
        IFZ   al,'g'
              mov   eax,offset gdt
        ELIFZ al,'i'
              mov   eax,offset idt
        ELIFZ al,'l'
              mov   eax,ss:[linktab_base]
        ELIFZ al,'p'
              mov   eax,ss:[free_pnode_root]
        ELSE_
              sub   eax,eax
        FI
  FI                        
                          
  mov   esi,eax
  mov   edi,eax

  kd____clear_page

  push  esi
  push  edi
  mov   ebp,offset dump_dword
  DO
        mov   al,'d'
        call  dump
        IFZ   al,13
        CANDNZ edx,0
              pop   eax
              pop   eax
              push  esi
              push  edi
              mov   edi,edx
              mov   esi,edx
              REPEAT
        FI
        IFZ   al,1
              pop   edi
              pop   esi
              push  esi
              push  edi
              REPEAT
        FI

  OD
  pop   esi
  pop   edi

  push  eax
  mov   ah,lines-1
  mov   al,0
  kd____cursor
  kd____disp  <'L4KD: ',5>
  pop   eax
  push  eax
  IFAE  al,20h
        kd____outchar
  FI
  pop   eax

  ret



dump_dword:

  call  display_dword
  mov   ebx,esi
  ret




;----------------------------------------------------------------------------
;
;       display ptab
;
;----------------------------------------------------------------------------


display_ptabs:

  mov   [dump_area_size],pagesize

  kd____inhex32

  test  eax,eax
  IFZ
        mov   eax,cr3
  ELIFB eax,tasks
  mov   ebx,cr0
  bt    ebx,31
  CANDC
        push  ds
        push  linear_kernel_space
        pop   ds
        shl   eax,task_no
        add   eax,offset tcb_space
        test__page_present eax
        pop   ds
  CANDNC      
        mov   eax,ss:[eax+task_pdir]
  FI
  and   eax,-pagesize
  mov   [dump_area_base],eax

  kd____clear_page

  DO
        mov   esi,[dump_area_base]

        mov   edi,esi
        mov   ebp,offset dump_pdir
        DO
              mov   al,'p'
              call  dump

              cmp   al,13
              EXITNZ long

              test  edx,edx
              REPEATZ

              push  esi
              push  edi
              push  ebp
              mov   esi,edx
              mov   edi,edx
              mov   ebp,offset dump_ptab
              xchg  [dump_area_base],edx
              push  edx
              DO
                    mov   al,'p'
                    call  dump

                    IFZ   al,'m'
                          mov   eax,esi
                          shr   eax,log2_pagesize
                          imul  eax,linksize
                          add   eax,ss:[linktab_base]
                          mov   eax,ss:[eax].pmap_link
                          test  eax,eax
                          IFZ
                                mov   al,'-'
                                kd____outchar
                                REPEAT
                          FI               
                          push  esi
                          push  edi
                          push  ebp
                          mov   ebp,offset dump_page
                          add   esi,eax
                          mov   edi,esi
                          and   edi,-pagesize
                          mov   eax,edi
                          xchg  [dump_area_base],eax
                          push  eax
                          mov   al,'d'
                          call  dump  
                          pop   [dump_area_base]       
                          pop   ebp
                          pop   edi
                          pop   esi
                          cmp   al,1
                          REPEATZ
                          EXIT
                    FI

                    cmp   al,13
                    EXITNZ

                    test  edx,edx
                    REPEATZ

                    push  esi
                    push  edi
                    push  ebp
                    mov   esi,edx
                    mov   edi,esi
                    mov   ebp,offset dump_page
                    xchg  [dump_area_base],edx
                    push  edx
                    DO
                          mov   al,'d'
                          call  dump
                          cmp   al,13
                          REPEATZ
                    OD
                    pop   [dump_area_base]
                    pop   ebp
                    pop   edi
                    pop   esi

                    cmp   al,1
                    REPEATZ
              OD

              pop   [dump_area_base]
              pop   ebp
              pop   edi
              pop   esi

              cmp   al,1
              REPEATZ
        OD

        cmp   al,1
        REPEATZ
  OD

  push  eax
  mov   ah,lines-1
  mov   al,0
  kd____cursor
  kd____disp  <'L4KD: ',5>
  pop   eax
  push  eax
  IFAE  al,20h
        kd____outchar
  FI
  pop   eax

  ret




dump_pdir:

  push  esi
  mov   eax,cr0
  bt    eax,31
  IFC
        add   esi,PM
  FI
  call  display_dword
  pop   esi

  and   edx,-pagesize

  mov   ebx,esi
  and   ebx,pagesize-1
  shl   ebx,22-2
  mov   [virt_4M_base],ebx

  ret



dump_ptab:

  push  esi
  mov   eax,cr0
  bt    eax,31
  IFC
        add   esi,PM
  FI
  call  display_dword
  pop   esi

  and   edx,-pagesize

  mov   ebx,esi
  and   ebx,pagesize-1
  shl   ebx,12-2
  add   ebx,[virt_4M_base]
  mov   [virt_4K_base],ebx

  ret




dump_page:

  push  esi
  mov   eax,cr0
  bt    eax,31
  IFC
        add   esi,PM
  FI
  call  display_dword
  pop   esi

  mov   ebx,esi
  and   ebx,pagesize-1
  add   ebx,[virt_4K_base]

  ret


  align 4


virt_4M_base  dd 0
virt_4K_base  dd 0

dump_area_base dd 0
dump_area_size dd -1

dump_type     db 'd'


;----------------------------------------------------------------------------
;
;       dump
;
;----------------------------------------------------------------------------
;PRECONDITION:
;
;       AL    dump type
;       ESI   actual dump dword address (0 mod 4)
;       EDI   begin of dump address (will be 8*4-aligned)
;       EBP   dump operation
;
;----------------------------------------------------------------------------
;POSTCONDITION:
;
;       ESI   actual dump dword address (0 mod 4)
;       EDI   begin of dump address (will be 8*4-aligned)
;       EBP   dump operation
;
;       EBX,EDX can be loaded by dump operation
;
;       EAX,ECX scratch
;
;----------------------------------------------------------------------------

dumplines      equ (lines-1)


dump:

  mov   [dump_type],al

  mov   al,0
  DO
        mov   ecx,[dump_area_base]
        IFB_  esi,ecx
              mov   esi,ecx
        FI
        IFB_  edi,ecx
              mov   edi,ecx
        FI
        add   ecx,[dump_area_size]
        sub   ecx,4
        IFA   esi,ecx
              mov   esi,ecx
        FI
        sub   ecx,dumplines*8*4-4
        IFA   edi,ecx
              mov   edi,ecx
        FI

        and   esi,-4

        IFB_  esi,edi
              mov   edi,esi
              mov   al,0
        FI
        lea   ecx,[edi+dumplines*8*4]
        IFAE  esi,ecx
              lea   edi,[esi-(dumplines-1)*8*4]
              mov   al,0
        FI
        and   edi,-8*4

        IFZ   al,0

              push  esi
              mov   esi,edi
              mov   ch,lines-dumplines-1
              DO
                    mov   cl,0
                    mov   eax,ecx
                    kd____cursor
                    mov   eax,esi
                    kd____outhex32
                    mov   al,':'
                    kd____outchar
                    add   cl,8+1

                    DO
                          call  ebp
                          add   esi,4
                          add   cl,8+1
                          cmp   cl,80
                          REPEATB
                    OD

                    inc   ch
                    cmp   ch,lines-1
                    EXITAE
                    mov   eax,[dump_area_base]
                    add   eax,[dump_area_size]
                    dec   eax
                    cmp   esi,eax
                    REPEATB
              OD
              pop   esi
        FI

        mov   ecx,esi
        sub   ecx,edi
        shr   ecx,2

        mov   ch,cl
        shr   ch,3
        add   ch,lines-dumplines-1
        mov   al,cl
        and   al,8-1
        mov   ah,8+1
        IFZ   [dump_type],'c'
              mov   ah,4
        FI
        imul  ah
        add   al,9
        mov   cl,al

        mov   eax,ecx
        kd____cursor

        call  ebp
        kd____disp <6,lines-1,0,'L4KD: '>
        mov   al,[dump_type]
        kd____outchar
        mov   al,'<'
        kd____outchar
        mov   eax,ebx
        kd____outhex32
        mov   al,'>'
        kd____outchar
        kd____disp <6,lines-1,columns-35,'++KEYS: ',24,' ',25,' ',26,' ',27,' Pg',24,' Pg',25,' CR Home    '>
        IFZ   ebp,<offset dump_ptab>
              kd____disp <6,lines-1,columns-3,3Ch,'m',3Eh>
              mov   eax,ss
              IFZ   eax,linear_kernel_space
                    kd____disp <6,lines-1,24>
                    push  edi
                    shr   edi,log2_pagesize
                    imul  edi,linksize
                    add   edi,ss:[linktab_base]
                    mov   eax,ss:[edi].pdir_link
                    shr   eax,log2_pagesize
                    IFNZ
                          kd____disp <'pdir:'>
                          sub   eax,PM SHR log2_pagesize
                          kd____outhex20
                    FI
                    mov   eax,ss:[edi].pmap_link
                    pop   edi
                    test  eax,eax
                    IFNZ
                          kd____disp <' pmap:'>            
                          add   eax,edi
                          shr   eax,log2_pagesize
                          kd____outhex20
                    FI
              FI      
        FI
        mov   eax,ecx
        kd____cursor

        kd____inchar

        IFZ   al,2
              add   esi,4
        FI
        IFZ   al,8
              sub   esi,4
        FI
        IFZ   al,10
              add   esi,8*4
        FI
        IFZ   al,3
              sub   esi,8*4
        FI
        IFZ   al,11h
              add   esi,dumplines*8*4 AND -100h
              add   edi,dumplines*8*4 AND -100h
              mov   al,0
        FI
        IFZ   al,10h
              sub   esi,dumplines*8*4 AND -100h
              sub   edi,dumplines*8*4 AND -100h
              mov   al,0
        FI
        IFZ   al,' '
              mov   al,[dump_type]
              IFZ   al,'d'
                    mov   al,'b'
              ELIFZ al,'b'
                    mov   al,'c'
              ELIFZ al,'c'
                    mov   al,'p'
              ELSE_
                    mov   al,'d'
              FI
              mov   [dump_type],al
              kd____clear_page
              mov   al,0
        FI

        cmp   al,1
        EXITZ
        cmp   al,13
        REPEATB
  OD

  ret





display_dword:

  mov   eax,esi
  lno___task ebx,esp
  call  page_phys_address

  IFZ
        IFZ   [dump_type],'c'
              kd____disp <250,250,250,250>
        ELSE_
              kd____disp <250,250,250,250,250,250,250,250,250>
        FI
        sub   edx,edx
        ret
  FI

  mov   edx,[eax]

  mov   al,[dump_type]
  IFZ   al,'d'
        IFZ   edx,0
              kd____disp <'       0'>
        ELIFZ edx,-1
              kd____disp <'      -1'>
              sub   edx,edx
        ELSE_
              mov   eax,edx
              kd____outhex32
        FI
        mov   al,' '
        kd____outchar
        ret
  FI
  IFZ   al,'b'
        mov   al,dl
        kd____outhex8
        mov   al,dh
        kd____outhex8
        shr   edx,16
        mov   al,dl
        kd____outhex8
        mov   al,dh
        kd____outhex8
        sub   edx,edx
        mov   al,' '
        kd____outchar
        ret
  FI
  IFZ   al,'c'
        call  out_dump_char
        shr   edx,8
        call  out_dump_char
        shr   edx,8
        call  out_dump_char
        shr   edx,8
        call  out_dump_char
        sub   edx,edx
        ret
  FI
  IFZ   al,'p',long

        IFZ   edx,0
              kd____disp <'    -    '>
        ELSE_
              test  edx,0F0000000h
              CORNZ
              test  dl,page_present
              IFZ
                    mov   eax,edx
                    kd____outhex32
                    mov   al,' '
                    kd____outchar
                    ret
              FI
              call  dump_pte
        FI
        ret

  FI

  sub   edx,edx
  ret





out_dump_char:

  mov   al,dl
  IFB_  al,20h
        mov   al,'.'
  FI
  kd____outchar
  ret




dump_pte:


  mov   eax,edx
  shr   eax,28
  IFZ
        mov   al,' '
  ELIFB al,10
        add   al,'0'
  ELSE_
        add   al,'A'-10
  FI            
  kd____outchar
  mov   eax,edx
  test  dl,superpage
  IFZ
        shr   eax,12
        kd____outhex16
  ELSE_
        shr   eax,22
        kd____outhex8
        mov   al,'/'
        bt    edx,shadow_ptab_bit
        IFC
              mov   al,'*'
        FI
        kd____outchar
        mov   al,'4'      
        kd____outchar
  FI
  mov   al,'-'
  kd____outchar
  test  dl,page_write_through
  IFNZ
        mov   al,19h
  FI
  test  dl,page_cache_disable
  IFNZ
        mov   al,17h
  FI
  kd____outchar
  mov   al,'r'
  test  dl,page_write_permit
  IFNZ
        mov   al,'w'
  FI
  test  dl,page_user_permit
  IFZ
        sub   al,'a'-'A'
  FI
  kd____outchar
  mov   al,' '
  kd____outchar

  ret










;----------------------------------------------------------------------------
;
;       display mappings
;
;----------------------------------------------------------------------------


display_mappings:

  IFB_  esp,<offset tcb_space>
        ret
  FI

  kd____inhex32
  shl   eax,log2_pagesize



display_mappings_of:

  IF    kernel_type NE i486         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ret                         ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ELSE                              ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  

  push  ds
  push  es

  push  linear_kernel_space
  pop   ds
  push  linear_kernel_space
  pop   es


  mov   esi,eax

  IF    kernel_type EQ i486
  
        IFA   eax,GB1

              mov   ebx,M4_pnode_base
              DO
                    mov   ecx,[ebx].pte_ptr
                    test  ecx,ecx
                    IFNZ
                          mov   edi,[ecx]
                          and   edi,-pagesize
                          mov   edi,dword ptr [edi+PM]
                          sub   edi,GB1
                          xor   edi,eax
                          test  edi,-1 SHL 22
                          EXITZ
                    FI

                    add   ebx,sizeof pnode
                    cmp   ebx,M4_pnode_base + high4M_pages * sizeof pnode
                    REPEATB

                    pop   es
                    pop   ds
                    ret
              OD

              mov   eax,ecx
        FI
        
  ENDIF        


  shr   eax,log2_pagesize-log2_size_pnode
  and   eax,-sizeof pnode
  lea   edi,[eax+pnode_base]
  mov   ebx,edi

  kd____clear_page
  sub   eax,eax
  kd____cursor

  kd____disp <'phys frame: '>
  mov   eax,esi
  kd____outhex32

  kd____disp <'  cache: '>
  mov   eax,[edi+cache0]
  kd____outhex32
  mov   al,','
  kd____outchar
  mov   eax,[edi+cache1]
  kd____outhex32

  kd____disp <13,10,10>

  mov   cl,' '
  DO
        mov   eax,edi
        kd____outhex32
        kd____disp <'  '>
        mov   al,cl
        kd____outchar

        kd____disp <'  pte='>
        mov   eax,[edi+pte_ptr]
        kd____outhex32
        kd____disp <' ('>
        mov   eax,[edi+pte_ptr]
        mov   edx,[eax]
        call  dump_pte
        kd____disp <') v=...'>
        mov   eax,[edi+pte_ptr]
        and   eax,pagesize-1
        shr   eax,2
        kd____outhex12
        kd____disp <'000   ',25>
        mov   eax,[edi+child_pnode]
        kd____outhex32
        mov   al,' '
        kd____outchar
        IFNZ  edi,ebx
              mov   eax,[edi+pred_pnode]
              kd____outhex32
              mov   al,29
              kd____outchar
              mov   eax,[edi+succ_pnode]
              kd____outhex32
        FI

        kd____inchar

        IFZ   al,10
              mov   cl,25
              mov   edi,[edi+child_pnode]
        ELIFZ al,8
        CANDNZ edi,ebx
              mov   cl,27
              mov   edi,[edi+pred_pnode]
        ELIFZ al,2
        CANDNZ edi,ebx
              mov   cl,26
              mov   edi,[edi+succ_pnode]
        ELSE_
              EXIT
        FI
        kd____disp <13,10>

        and   edi,-sizeof pnode
        REPEATNZ

        mov   edi,ebx
        REPEAT
  OD

  push  eax
  mov   ah,lines-1
  mov   al,0
  kd____cursor
  kd____disp  <'L4KD: ',5>
  pop   eax
  push  eax
  IFAE  al,20h
        kd____outchar
  FI
  pop   eax

  pop   es
  pop   ds
  ret
  
  
  ENDIF



;----------------------------------------------------------------------------
;
;       display kernel data
;
;----------------------------------------------------------------------------


display_kernel_data:

  IFB_  esp,<offset tcb_space>
        ret
  FI

  push  ds
  push  es

  mov   eax,linear_kernel_space
  mov   ds,eax
  mov   es,eax

  kd____clear_page

  sub   esi,esi                       ; required for show macro !

  show  <6,2,1,'sys clock : '>,system_clock_high
  mov   eax,ds:[system_clock_low]
  kd____outhex32

  kd____disp  <6,7,40,'present root : '>
  mov   eax,offset present_root
  lno___thread eax,eax
  kd____outhex16
  
  IFDEF scheduled_tcb
        kd____disp <6,6,1,'scheduled tcb  : '>
        mov   eax,ds:[scheduled_tcb]
        lno___thread eax,eax
        kd____outhex16

  ELSE
        kd____disp <6,6,1,'ready actual   : '>
        mov   eax,ds:[ready_actual]
        lno___thread eax,eax
        kd____outhex16

        kd____disp  <6,8,1,'ready root     : '>
        mov   ecx,offset dispatcher_tcb+ready_link
        call  show_link
        kd____disp  <6,9,1,'intr root    : '>
        mov   ecx,offset dispatcher_tcb+interrupted_link
        call  show_link
  ENDIF

  kd____disp  <6,11,1, 'soon wakeup root :'>
  mov   ecx,offset dispatcher_tcb+soon_wakeup_link
  call  show_link
  kd____disp  <6,12,1, 'late wakeup root :'>
  mov   ecx,offset dispatcher_tcb+late_wakeup_link
  call  show_link

  kd____disp  <6,11,40, 'intrq link :'>
  sub   ebx,ebx
  DO
        mov   eax,ebx
        and   al,7
        imul  eax,5
        add   al,41
        mov   ah,bl
        shr   ah,3
        add   ah,12
        kd____cursor
        lea   ecx,[(ebx*4)+intrq_llink]
        call  show_llinks
        add   ebx,2
        cmp   ebx,lengthof intrq_llink
        REPEATB
  OD

  kd____disp  <6,18,61,' CR3 : '>
  mov   eax,cr3
  kd____outhex32

  kd____disp  <6,19,61,'ESP0 : '>
  mov   eax,ds:[cpu_esp0]
  kd____outhex32

  pop   es
  pop   ds
  ret




;----------------------------------------------------------------------------
;
;       page fault prot
;
;----------------------------------------------------------------------------


page_fault_prot:

  mov   eax,cr0
  bt    eax,31
  CORNC
  mov   eax,ss
  IFNZ  eax,linear_kernel_space

        mov   al,'-'
        kd____outchar
        ret
  FI


  kd____inchar
  mov   [page_fault_prot_state],al

  CORZ  al,'+'
  IFZ   al,'*'
        kd____outchar
        IFZ   [page_fault_handler],0
              mov   bl,page_fault
              call  get_exception_handler
              mov   [page_fault_handler],eax
        FI
        mov   eax,offset show_page_fault
        mov   bl,page_fault
        call  set_exception_handler
        ret
  FI
  IFZ   al,'-'
        kd____outchar
        sub   ecx,ecx
        mov   [page_fault_low],ecx
        dec   ecx
        mov   [page_fault_high],ecx
        sub   eax,eax
        xchg  eax,[page_fault_handler]
        test  eax,eax
        IFNZ
              mov   bl,page_fault
              call  set_exception_handler
        FI
        ret
  FI
  IFZ   al,'x'
        kd____disp  'x ['
        kd____inhex32
        mov   [page_fault_low],eax
        mov   al,','
        kd____outchar
        kd____inhex32
        mov   [page_fault_high],eax
        mov   al,']'
        kd____outchar
        ret
  FI

  mov   al,'?'
  kd____outchar

  ret



show_page_fault:

  ipre  ec_present

  mov   eax,cr2
  and   eax,-pagesize
  IFAE   eax,[page_fault_low+PM],long
  CANDBE eax,[page_fault_high+PM],long

        kd____disp  <13,10,'#PF: '>
        mov   eax,cr2
        kd____outhex32
        kd____disp  <', eip='>
        mov   eax,[esp+ip_eip]
        kd____outhex32
        kd____disp  <', thread='>
        mov   eax,esp
        and   eax,-sizeof tcb
        mov   eax,[eax+myself]
        lno___thread eax,eax
        kd____outhex16


        IFNZ  [page_fault_prot_state+PM],'*'
              call  open_debug_keyboard
              kd____inchar
              IFZ   al,'i'
                    ke    'page_fault'
              FI
              call  close_debug_keyboard
        FI
  FI

  IF kernel_type NE i486
        pop   ds
  ENDIF

  popad
  jmp   ds:[page_fault_handler+PM]



;----------------------------------------------------------------------------
;
;       IPC prot
;
;----------------------------------------------------------------------------


ipc_prot:

  mov   eax,cr0
  bt    eax,31
  CORNC
  mov   eax,ss
  IFNZ  eax,linear_kernel_space

        mov   al,'-'
        kd____outchar
        ret
  FI


  kd____inchar
  mov   [ipc_prot_state],al

  CORZ  al,'+'
  IFZ   al,'*'
        kd____outchar
        IFZ   [ipc_handler],0
              mov   bl,ipc
              call  get_exception_handler
              mov   [ipc_handler],eax
        FI
        mov   eax,offset show_ipc
        mov   bl,ipc
        call  set_exception_handler
        ret
  FI
  IFZ   al,'-'
        kd____outchar
        sub   eax,eax
        xchg  eax,[ipc_handler]
        test  eax,eax
        IFNZ
              mov   bl,ipc
              call  set_exception_handler
        FI
        ret
  FI

  mov   al,'?'
  kd____outchar

  ret



show_ipc:

  ipre  fault

	mov	ebp,esp

	kd____disp <13,10>

	mov	ebx,esp
	and	ebx,-sizeof tcb
	add	ebx,offset myself
	call	show_thread_id

	DO
		mov	edx,virtual_space_size
		IFAE	[ebp+ip_eax],edx
			test	byte ptr [ebp+ip_ebp],1
			IFZ
				kd____disp	<' <--- '>
			ELSE_
				kd____disp  <' <<-- '>
				EXIT
			FI

		ELIFB	[ebp+ip_ebp],edx
			test	byte ptr [ebp+ip_ebp],1
			IFZ
				kd____disp	<' <--> '>
			ELSE_
				kd____disp  <' <<-> '>
			FI
		ELSE_
	     		kd____disp	<' ---> '>
		FI	
		
		mov	eax,[ebp+ip_esi]
		mov	ebx,eax
		and	ebx,mask thread_no
		add	ebx,offset tcb_space+offset myself

		test__page_present ebx
		IFNC
			call	show_thread_id
		ELSE_
			kd____outhex32
		FI
	OD

	IFB_	[ebp+ip_eax],edx

		test	byte ptr [ebp+ip_eax],2
		IFZ
  		kd____disp	<'     ('>
		ELSE_
			kd____disp  <' map ('>
		FI
		mov	eax,[ebp+ip_edx]
		kd____outhex32
		mov	al,','
		kd____outchar
		mov	eax,[ebp+ip_ebx]
		kd____outhex32
		mov	al,')'
		kd____outchar
	FI

	

  IFNZ  [ipc_prot_state+PM],'*'
        call  open_debug_keyboard
        kd____inchar
        IFZ   al,'i'
              ke    'ipc'
        FI
        call  close_debug_keyboard
  FI

  IF kernel_type NE i486
        pop   ds
  ENDIF

  popad
	add	esp,4
  jmp   ds:[ipc_handler+PM]





;----------------------------------------------------------------------------
;
;       monit exception
;
;----------------------------------------------------------------------------


monit_exception:


  kd____inchar
  kd____outchar

  push  eax  
  CORZ  al,'+'  
  IFZ   al,'-'
  
        mov   al,false
        xchg  al,[exception_monitoring_flag]
        IFZ   al,true
              mov   eax,[monitored_exception_handler]
              mov   bl,[monitored_exception]
              call  set_exception_handler
        FI
  FI
  pop   eax
  
  
  IFZ   al,'+'
  
        kd____disp <' #'>
        kd____inhex8
        
        CORZ  al,debug_exception
        CORZ  al,breakpoint
        IFA   al,sizeof idt/8
              mov   al,'-'
              kd____outchar
              ret
        FI
        
        mov   [exception_monitoring_flag],true
        mov   [monitored_exception],al
        mov   bl,al
        
        IFAE  al,11
        CANDB al,15
        
              kd____disp <' ['>
              kd____inhex16
              mov   [monitored_ec_min],ax
              mov   al,','
              kd____outchar
              kd____inhex16
              mov   [monitored_ec_max],ax
              mov   al,']'
              kd____outchar
        FI
        
        call  get_exception_handler
        mov   [monitored_exception_handler],eax
        
        mov   eax,offset exception_monitor
        call  set_exception_handler
  FI
  
  ret
  
  



exception_monitor:

  ipre  ec_present
  
  call  $+5
  pop   edi
  and   edi,PM
  mov   al,cs:[edi+monitored_exception]
  mov   ebp,esp
  DO
        IF    kernel_type NE i486
        
              IFZ   al,general_protection
              CANDZ ss:[ebp+ip_cs],linear_space_exec
              bt    ss:[ebp+ip_eflags],vm_flag
              CANDNC
                    cmp   ss:[ebp+ip_ds],0
                    EXITZ
                    
                    mov   ebx,ss:[ebp+ip_eip]
                    mov   ecx,ebx
                    and   ecx,pagesize-1
                    IFBE  ecx,pagesize-4                       
                          push  ds
                          mov   ds,ss:[ebp+ip_cs]
                          mov   ebx,[ebx]
                          pop   ds
                          cmp   bx,010Fh       ; LIDT (emulated) etc.
                          EXITZ
                    FI
              FI
        ENDIF
              
        IFAE  al,11
        CANDB al,15
              movzx eax,word ptr ss:[ebp+ip_error_code]  
              movzx ebx,cs:[edi+monitored_ec_min]
              movzx ecx,cs:[edi+monitored_ec_max]
              IFBE  ebx,ecx
                    cmp   eax,ebx
                    EXITB
                    cmp   eax,ecx
                    EXITA
              ELSE_
                    IFBE  eax,ebx
                          cmp   eax,ecx
                          EXITAE
                    FI
              FI
        FI
        
        ke    'INTR'
        
  OD
                        

  sub   eax,eax
  CORB  cs:[edi+monitored_exception],8           
  IFA   cs:[edi+monitored_exception],14
        IFNZ  cs:[edi+monitored_exception],17           
              dec   eax      
        FI      
  FI
  
  test  edi,edi
  bt    eax,0
  
  IF kernel_type NE i486
        pop   ds
  ENDIF
  popad
  IFC
        lea   esp,[esp+4]
  FI
  
  IFZ
        jmp   cs:[monitored_exception_handler]
  FI
  jmp   cs:[monitored_exception_handler+PM]      




;----------------------------------------------------------------------------
;
;       remote kd intr
;
;----------------------------------------------------------------------------


remote_kd_intr:

  kd____inchar

  IFZ   al,'+'
  CANDAE esp,<offset tcb_space>
  CANDB  esp,<offset tcb_space+tcb_space_size>

        kd____outchar
        IFZ   [timer_intr_handler],0
              mov   bl,irq0_intr+8 
              call  get_exception_handler
              mov   [timer_intr_handler],eax
        FI
        mov   eax,offset kdebug_timer_intr_handler
  ELSE_
        mov   al,'-'
        kd____outchar
        sub   eax,eax
        xchg  eax,[timer_intr_handler]
  FI
  test  eax,eax
  IFNZ
        mov   bl,irq0_intr+8 
        call  set_exception_handler
  FI
  ret



kdebug_timer_intr_handler:

  ipre  fault,no_load_ds

  kd____incharety
  IFZ   al,27
        ke    'ESC'
  FI

  ko    T

  IF kernel_type NE i486
        pop   ds
  ENDIF

  popad
  add   esp,4
  jmp   cs:[timer_intr_handler+PM]



;----------------------------------------------------------------------------
;
;       tracing on/off
;
;----------------------------------------------------------------------------



tracing_on_off:

  kd____inchar
  mov   edi,[kdebug_esp]
  IFA   edi,<virtual_space_size>
        push  ds
        push  linear_kernel_space
        pop   ds
  FI

  IFZ   al,'+'
        bts   [edi+ip_eflags],t_flag
  else_
        btr   [edi+ip_eflags],t_flag
        mov   al,'-'
  FI

  IFA   edi,<virtual_space_size>
        pop   ds
  FI
  kd____outchar
  ret


;----------------------------------------------------------------------------
;
;       virtual address info
;
;----------------------------------------------------------------------------


virtual_address_info:

  kd____inhex32
  mov   ebx,eax
  kd____disp  <'  Task='>
  kd____inhex16
  test  eax,eax
  IFZ
        lno___task eax,esp
  FI
  xchg  eax,ebx
  call  page_phys_address
  IFZ
        kd____disp  <'  not mapped'>
  ELSE_
        push  eax
        kd____disp  <'  phys address = '>
        pop   eax
        kd____outhex32
  FI

  ret


;----------------------------------------------------------------------------
;
;       port io
;
;----------------------------------------------------------------------------


pic1_imr      equ 21h


port_io:

  mov   bh,al
  IFZ   al,'i'
        kd____disp <'n  '>
  ELSE_
        kd____disp <'ut '>
  FI            
  
  kd____inchar
  mov   bl,al
  kd____outchar
  IF    kernel_type EQ i486
        kd____disp <'-byte port '>                  
  ELSE
        IFZ   al,'a'
              kd____disp <'pic '>
        ELIFZ al,'i'
              kd____disp <'o apic '>
        ELSE_
              kd____disp <'-byte port '>                  
        FI
  ENDIF      
        
  kd____inhex16
  mov   edx,eax
  
  kd____disp <': '>
  IFZ   bh,'o'
        kd____inhex32
  FI      
        
  IFZ   bl,'1'
  
        IFZ   bh,'o'
              IFZ   dx,pic1_imr
                    mov   [old_pic1_imr],al
              ELSE_
                    out   dx,al
              FI
        ELSE_
              IFZ   dx,pic1_imr
                    mov   al,[old_pic1_imr]
              ELSE_
                    in    al,dx
              FI
              kd____outhex8
        FI
        ret
  FI

  IFZ   bl,'2'
  
        IFZ   bh,'o'
              out   dx,ax
        ELSE_
              in    ax,dx
              kd____outhex16
        FI
        ret
  FI
  
  IFZ   bl,'4'
  
        IFZ   bh,'o'
              out   dx,eax
        ELSE_
              in    eax,dx
              kd____outhex32
        FI
        ret
  FI
  
  
  
  


  IF    kernel_type NE i486
  
        IFB_  esp,virtual_space_size
              ret
        FI
        
        
        push  ds
        push  linear_kernel_space
        pop   ds
  
        
        IFZ   bl,'a'
        
              and   edx,00000FF0h
              IFZ   bh,'o'
                    mov   ds:[edx+local_apic],eax
              ELSE_
                    mov   eax,ds:[edx+local_apic]
                    kd____outhex32
              FI
        
        ELIFZ bl,'i'
        
              and   edx,000000FFh
              mov   byte ptr ds:[io_apic+0],dl
              IFZ   bh,'o'
                    mov   ds:[io_apic+10h],eax
              ELSE_
                    mov   eax,ds:[io_apic+10h]
                    kd____outhex32
              FI
        FI      
  
        pop   ds
  
  ENDIF
        
        
  ret            
              



;----------------------------------------------------------------------------
;
;       page phys address
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   linear address
;       EBX   task no
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       page present:
;
;             NZ
;             EAX   phys address (lower 12 bits unaffected)
;
;
;       page not present:
;
;             Z
;
;----------------------------------------------------------------------------


page_phys_address:

  push  eax
  mov   eax,cr0
  bt    eax,31
  pop   eax

  IFNC
        test  esp,esp     ; NZ !
        ret
  FI


  push  ds
  push  ecx
  push  edx

  mov   edx,linear_kernel_space
  mov   ds,edx

  shl   ebx,task_no
  add   ebx,offset tcb_space
  sub   edx,edx
  test__page_present ebx
  IFNC
        mov   edx,[ebx+task_pdir]
        IFAE  eax,shared_table_base
        CANDBE eax,shared_table_base+shared_table_size-1
              mov   edx,ds:[kernel_proot]
        FI

        xpdir ebx,eax
        xptab ecx,eax
        mov   ebx,dword ptr [(ebx*4)+edx+PM]
        mov   dl,bl
        and   ebx,-pagesize

        test  dl,page_present
        IFNZ
              IF    kernel_type EQ i486
                    mov   ecx,dword ptr [(ecx*4)+ebx+PM]
                    mov   dl,cl
                    and   ecx,-pagesize
              ELSE
                    test  dl,superpage
                    IFZ
                          mov   ecx,dword ptr [(ecx*4)+ebx+PM]
                          mov   dl,cl
                          and   ecx,-pagesize
                    ELSE_
                          and   ebx,-1 SHL 22
                          shl   ecx,12
                          add   ecx,ebx
                    FI
              ENDIF
              IFAE  ecx,<physical_kernel_mem_size>      ; no access beyond PM
                    mov   dl,0                          ; ( 0 ... 64 M )
              FI                                        ;
        FI
  FI      
  test  dl,page_present
  IFNZ
        and   eax,pagesize-1
        add   eax,ecx
        test  esp,esp     ; NZ !
  FI

  pop   edx
  pop   ecx
  pop   ds
  ret



;--------------------------------------------------------------------------
;
;       set / get exception handler
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX    offset
;       BL     interrupt number
;
;       SS     linear_kernel_space
;
;---------------------------------------------------------------------------


set_exception_handler:
 
  push  eax
  push  ebx

  IFA   esp,<offset tcb_space>
        add   eax,PM
  FI      

  call  address_idt

  mov   ss:[ebx],ax
  shr   eax,16
  mov   ss:[ebx+6],ax

  pop   ebx
  pop   eax
  ret



get_exception_handler:
 
  push  ebx

  call  address_idt

  mov   ax,ss:[ebx+6]
  shl   eax,16
  mov   ax,ss:[ebx]

  pop   ebx
  ret



address_idt:

  movzx ebx,bl
  shl   ebx,3
  sidt  [idt_descriptor]
  add   ebx,dword ptr [idt_descriptor+2]
  ret


idt_descriptor      df 0







;--------------------------------------------------------------------------
;
;       special PCI -> 21140 Throughput test
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX    offset
;       BL     interrupt number
;
;       SS     linear_kernel_space
;
;---------------------------------------------------------------------------


csr1    equ 08h
csr5    equ 28h
csr6    equ 30h



wait100 macro

  mov   eax,ecx
  DO
        dec   eax
        REPEATNZ
  OD
  endm
        


special_test:

  kd____disp <'MSR: '>
  kd____inhex32
  mov   ecx,eax
  
  kd____disp <'  r/w? '>
  kd____inchar
  
  IFZ   al,'w'
        kd____inhex32
        IFNZ  ecx,0
              mov   edx,eax
              kd____disp <':'>
              kd____inhex32
              wrmsr
        ELSE_
              mov   cr4,eax
        FI
  ELSE_
        IFNZ  ecx,0
              rdmsr
              xchg  eax,edx
              kd____outhex32
              kd____disp <':'>
              mov   eax,edx
              kd____outhex32
        ELSE_
              mov   eax,cr4
              kd____outhex32
        FI
  FI
  ret                                               
              

  kd____disp <'21140 base: '>
  kd____inhex16
  mov   ebx,eax

  kd____disp <' snoop interval: '>  
  kd____inhex8
  mov   ecx,eax
  
  kd____disp <' : '>
  
  lea   edx,[ebx+csr1]
  sub   eax,eax
  out   dx,eax
  
  lea   edx,[ebx+csr5]
  DO
        in    eax,dx
        and   eax,00700000h
        cmp   eax,00300000h
        REPEATNZ
  OD       

  rdtsc
  mov   edi,eax
  lea   edx,[ebx+csr5]
  DO
        wait100
        in    eax,dx
        and   eax,00700000h
        cmp   eax,00300000h
        REPEATZ
  OD       
  rdtsc
  sub   eax,edi
  sub   edx,edx
  mov   edi,6
  div   edi
  kd____outdec
  kd____disp <' PCI cycles'>
  
  ret        






;---------------------------------------------------------------------------




default_kdebug_end    equ $



  dcod  ends


  code  ends
  end