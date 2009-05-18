include l4pre.inc

 
  dcode

  Copyright IBM+UKA, L4.KDEBUG, 24,08,99, 75


;*********************************************************************
;******                                                         ******
;******                LN Kernel Debug                          ******
;******                                                         ******
;******                                                         ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 27.08.99    ******
;******                                                         ******
;*********************************************************************
 

.nolist
include l4const.inc
include uid.inc
include adrspace.inc
include tcb.inc
include cpucb.inc
include schedcb.inc
include intrifc.inc
include pagconst.inc
include syscalls.inc
IFDEF   task_proot
include pagmac.inc
include pnodes.inc
ENDIF
include kpage.inc
include l4kd.inc
.list


ok_for x86






  public init_default_kdebug
  public default_kdebug_exception
  public default_kdebug_end


  extrn grab_frame:near
  extrn init_kdio:near
  extrn open_debug_keyboard:near
  extrn close_debug_keyboard:near
  extrn set_remote_info_mode:near
  extrn open_debug_screen:near
  extrn kd_outchar:near
  extrn kd_incharety:near
  extrn kd_inchar:near
  extrn kd_kout:near
  extrn old_pic1_imr:byte
  extrn irq0_intr:abs
  extrn physical_kernel_info_page:dword

  extrn first_lab:byte
  extrn kcod_start:byte
  extrn cod_start:byte
  extrn dcod_start:byte
  extrn scod_start:byte
  extrn kernelstring:byte
  extrn reset:near

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





kd_xsave_area struc

  kd_es       dw    0
  kd_dr7      db    0,0
  kd_ds       dw    0,0

kd_xsave_area ends

kd_save_area struc

              dw    0     ; kd_es
              db    0,0   ; kd_dr7

kd_save_area ends



	


kdpre  macro

  push  es

  push  eax
  mov   eax,dr7
  mov   ss:[esp+kd_dr7+4],al
  mov   al,0
  mov   dr7,eax
  pop   eax

  endm



kdpost macro

  push  eax
  mov   eax,dr7
  mov   al,ss:[esp+kd_dr7+4]
  mov   dr7,eax
  pop   eax

  pop   es

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

debug_breakpoint_counter_value  dd 0
debug_breakpoint_counter        dd 0

bx_low              dd 0
bx_high             dd 0
bx_addr             dd 0
bx_size             db 0

debug_exception_active_flag db false

                    align 4

debug_exception_handler dd 0



page_fault_prot_state db 0

ipc_prot_state	db 0
ipc_prot_mask       db 0FFh
                    align 4
ipc_prot_thread     dd 0
ipc_prot_non_thread dd 0
                    


niltext             db 0

                    align 4

page_fault_low      dd 0
page_fault_high     dd 0FFFFFFFFh


page_fault_handler  dd 0

ipc_handler		dd 0


timer_intr_handler  dd 0

kdebug_timer_intr_counter       db 0,0,0,0

monitored_exception_handler     dd 0
monitored_ec_min                dw 0
monitored_ec_max                dw 0
monitored_exception             db 0
exception_monitoring_flag       db false
                                db 0,0


kdebug_buffer             db 32 dup (0)



;----------------------------------------------------------------------------
;
;       kdebug trace buffer
;
;----------------------------------------------------------------------------

              align 4
              

debug_trace_buffer_size   equ KB64


trace_buffer_entry  struc

  trace_entry_type        dd    0
  trace_entry_string      dd    0,0,0,0,0
  trace_entry_timestamp   dd    0,0
  trace_entry_perf_count0 dd    0
  trace_entry_perf_count1 dd    0
                          dd    0,0,0,0,0,0
  
trace_buffer_entry  ends



get___timestamp macro

  IFA   esp,virtual_space_size
        rdtsc
  FI
  
  endm


    
 
 

trace_buffer_begin              dd 0
trace_buffer_end                dd 0
trace_buffer_in_pointer         dd 0

trace_buffer_active_stamp       dd 0,0

trace_display_mask              dd 0,0


no_references                   equ 0
forward_references              equ 1
backward_references             equ 2
performance_counters            equ 3

display_trace_index_mode        equ 0
display_trace_delta_time_mode   equ 1
display_trace_offset_time_mode  equ 2

no_perf_mon                     equ 0
kernel_perf_mon                 equ 1
user_perf_mon                   equ 2
kernel_user_perf_mon            equ 3

trace_link_presentation         db display_trace_index_mode
trace_reference_mode            db no_references
trace_perf_monitoring_mode      db no_perf_mon

				db 0
kd_proc_dest			dd 0
kd_proc_mask			dd 0
kd_proc_eax			dd 0

	
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
  movzx eax,[physical_kernel_info_page].kdebug_start_port
  IFA   eax,16
        call  set_remote_info_mode
  FI      
  call  init_trace_buffer
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
        call  [ebx*4+kdebug_io_call_tab]
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

  mov   cl,cs:[eax]
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

  sub   ecx,ecx
  
outdec_:  

  push  eax
  push  edx

  sub   edx,edx
  push  ebx
  mov   ebx,10
  div   ebx
  pop   ebx
  test  eax,eax
  IFNZ
        inc   ecx
        
        call  outdec_
        
        CORZ  ecx,9
        CORZ  ecx,6
        IFZ   ecx,3
  ;            mov   al,','
  ;            call  kd_outchar
        FI
        dec   ecx
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

  ;<cd> check if kdebug is locked by another processor

  push ds
  push ebx
  push eax

  push phys_mem
  pop  ds

  sub eax,eax  
  str ax
  shr ax,3
  and eax,0ffh

  lock bts [kd_proc_mask],eax
  ;<cd> if there is a lock, simply wait until it is released

  DO
     DO
       mov ebx,ds:[kd_proc_dest]
       cmp ebx,eax
       EXITZ
       cmp ebx, 0
       REPEATNZ
     OD
     mov ebx,eax   
     lock xchg ds:[kd_proc_dest],ebx
     cmp ebx,eax
     EXITZ
     cmp ebx,0
     EXITZ
     lock xchg ds:[kd_proc_dest],ebx
     REPEAT
  OD
kd_nosynch:	
	
  ;<cd> postcondition: kd_proc_dest is set to our PID, which prevents
  ;<cd> other processors from entering kdebug

  pop  eax
  mov ds:[kd_proc_eax],eax
 
  pop  ebx
  pop  ds

  kdpre

  lea   ebp,[esp+sizeof kd_save_area]


  IFAE  ebp,<physical_kernel_mem_size>
        push  phys_mem
        pop   ds

        push  eax 
        movzx eax,[physical_kernel_info_page].kdebug_start_port
        IFA   eax,16
              call  set_remote_info_mode
        FI      
        pop   eax
  FI
 
  
  IF    kernel_x2
        call  enter_single_processor_mode
  ENDIF      

  movzx eax,al
  lea   esi,[(eax*2)+id_table]

  IFZ   al,3
        mov   ebx,[ebp+ip_eip]

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
        
        mov   al,[ebx+1]
        IFNZ  al,'*'
              cmp   al,'#'
        FI      
        jz    trace_event 
              

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

  call  show_active_trace_buffer_tail

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

	cmp   bl,'s'
	je   pseudo_go	
	
        cmp   bl,'g'
        REPEATNZ

  OD

  call  flush_active_trace_buffer

  pop   [kdebug_text]
  pop   [kdebug_esp]

  mov   [kdebug_sema],0

  IFZ   [ebp+ip_error_code],debug_ec
  mov   eax,dr7
  mov   al,[esp+kd_dr7]
  test  al,10b
  CANDNZ
  shr   eax,16
  test  al,11b
  CANDZ
        bts   [ebp+ip_eflags],r_flag
  FI

  ;<cd> release the kdebug flag

  push ds
  push eax

  push phys_mem
  pop  ds

  sub eax,eax  
  str ax
  shr ax,3
  and eax,0ffh

  lock btr ds:[kd_proc_mask],eax
  mov ds:[kd_proc_dest],0

  pop  eax
  pop  ds

ret_from_kdebug:

  IF    kernel_x2
        call  exit_single_processor_mode
  ENDIF      

  kdpost

  ipost


pseudo_go:

  ;<cd> inform user that we are about to release the debugger

  kd____disp  <'switching to next processor',13,10>

  ;<cd> do some cleanup 

  call  flush_active_trace_buffer

  pop   [kdebug_text]
  pop   [kdebug_esp]

  mov   [kdebug_sema],0

  IFZ   [ebp+ip_error_code],debug_ec
  mov   eax,dr7
  mov   al,[esp+kd_dr7]
  test  al,10b
  CANDNZ
  shr   eax,16
  test  al,11b
  CANDZ
        bts   [ebp+ip_eflags],r_flag
  FI

  ;<cd> kd_proc_dest is reset. it is now safe to enter kdebug

  mov [kd_proc_dest],0

  ;<cd> restore old status code to EAX

  mov eax,[kd_proc_eax]

  kdpost

  ;<cd> jump back to the top. if any other processor was waiting for
  ;<cd> kdebug, it has entered by now and stored its PID in kd_proc_dest.
  ;<cd> if this is not the case, we simply enter again.

  jmp default_kdebug_exception




id_table db 'DVDBNM03OVBNUD07DF09TSNPSFGPPF15FPAC'

exception_string        db 14,'LN Kernel: #'
exception_id            db 'xx'

ec_exception_string     db 21,'LN Kernel: #'
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
              cmp   al,'g'
              OUTER_LOOP   EXITZ long

              cmp   al,'s'
              OUTER_LOOP   EXITZ long

              mov   ah,[physical_kernel_info_page].kdebug_permissions
              
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
              IFDEF task_proot      
                    ELIFZ al,'p',long
                          call  display_ptabs
                          cmp   al,0
                          REPEATNZ
                    ELIFZ al,'m',long
                          call  display_mappings
                          cmp   al,0
                          REPEATNZ
                    ELIFZ al,'P',long
                          call  page_fault_prot
                    ELIFZ al,'v',long
                          call  virtual_address_info
              ENDIF            
              ELIFZ al,'k',long
                    call  display_kernel_data
              ELIFZ al,'X',long
                    call  monit_exception
              ELIFZ al,'I',long
                    call  ipc_prot
              ELIFZ al,'R'
                    call  remote_kd_intr
              ELIFZ al,'i'
                    call  port_io
              ELIFZ al,'o'
                    call  port_io      
              ELIFZ al,'H'
                    call  halt_current_thread
              ELIFZ al,'K'
                    call  ke_disable_reenable      
              ELIFZ al,' '
                    call  out_id_text
              ELIFZ al,'T'
                    call  dump_trace_buffer
                    cmp   al,0
                    REPEATNZ
              ELIFZ al,'V'
                    call  set_video_mode
              ELIFZ al,'y'
                    call  special_test
              ELIFZ al,'^'
                    call  reset_system      
              ELSE_
                    call  out_help
              FI
        OD
        REPEAT
  OD

  call  close_debug_keyboard
  mov   bl,al
  kd____disp  <13,10>

  pop   ebp

  ret



get_kdebug_cmd:

  kd____disp  <6,lines-1,0,'LNKD'>

  ;<cd> display processor number on command prompt

  str   cx
  shr   cx,3
  and   ecx,0Fh
  mov   al,030h
  add   al,cl
  kd____outchar

  ;<cd> display numbers of processors who are waiting to enter kdebug

;  mov ebx,[kd_proc_mask]
mov ebx,0   ;;;
  mov edx,1
  shl edx,cl

  xor ebx,edx
  test ebx,0FFh
  jz nobody_active

  mov al,'[';
  kd____outchar
  mov ecx,16
  mov eax,030h
see_active:
  test ebx,1
  jz is_not_active
  push eax
  push ebx
  push ecx
  kd____outchar
  pop ecx
  pop ebx
  pop eax
is_not_active:
  shr ebx,1
  inc eax
  loop see_active
  mov al,']'
  kd____outchar

nobody_active:

  mov   al,':'
  kd____outchar
  mov   al,' '
  kd____outchar

  kd____inchar
  push  eax
  IFAE  al,20h
        kd____outchar
  FI
  
  pop   eax

  ret




is_main_level_command_key:

  IFNZ   al,'a'
  CANDNZ al,'b'
  CANDNZ al,'t'
  CANDNZ al,'d'
  CANDNZ al,'p'
  CANDNZ al,'m'
  CANDNZ al,'k'
  CANDNZ al,'m'
  CANDNZ al,'P'
  CANDNZ al,'I'
  CANDNZ al,'X'
  CANDNZ al,'T'
  CANDNZ al,'R'
  CANDNZ al,'i'
  CANDNZ al,'o'
  CANDNZ al,'H'
  CANDNZ al,'K'
  CANDNZ al,'V'
  CANDNZ al,'g'
  CANDNZ al,'s'
        IFZ   al,'q'
              mov   al,0
        FI      
  FI
  ret
  
  
  
;----------------------------------------------------------------------------
;
;       reset system
;
;----------------------------------------------------------------------------

reset_system:

  push  ds
  push  phys_mem
  pop   ds
  
  kd____disp <' RESET ? (y/n)'>
  kd____inchar
  mov   ecx,esp
  cmp   al,'y'
  jz    reset
  
  pop   ds
  ret
  


;----------------------------------------------------------------------------
;
;       out id text
;
;----------------------------------------------------------------------------


out_id_text:

  mov   al,'"'
  kd____outchar
  mov   eax,cs:[kdebug_text]
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

  mov   al,ah
  
  kd____disp  <13,10,'a  :  modules,        xxxx       : find module and rel addr'>
  kd____disp  <13,10,'t  :  current tcb,    xxxxx      : tcb of thread xxxx'>
  kd____disp  <13,10,'                      xxx.yy     : task xxx, lthread yy'>
  test  al,kdebug_dump_mem_enabled
  IFNZ
  kd____disp  <13,10,'d  :  dump mem,       xxxxxxxx   : dump memory'>
  FI
  test  al,kdebug_dump_map_enabled
  IFNZ  ,,long
  kd____disp  <13,10,'p  :  dump ptab,      xxx        : ptabs (pdir) of task xxxx'>
  kd____disp  <13,10,'                      xxxxx000   : ptab at addr xxxxx000'>
  kd____disp  <13,10,'m  :  dump mappings   xxxx       : mappings of frame xxxx'>
  FI
  kd____disp  <13,10,'k  :  kernel data'>              
  kd____disp  <13,10,'b  :  bkpnt,          i/w/a/p    : set instr/wr/rdwr/ioport bkpnt'>
  kd____disp  <13,10,'                      -/b/r      : reset/base/restrict'>
  test  al,kdebug_protocol_enabled
  IFNZ  ,,long
  kd____disp  <13,10,'P  :  monit PF        +/-/*/r    : on/off/trace/restrict'>
  kd____disp  <13,10,'I  :  monit ipc       +/-/*/r    : on/off/trace/restrict'>
  kd____disp  <13,10,'X  :  monit exc       +xx/-/*xx  : on/off/trace'>
  FI
  IFNZ  [physical_kernel_info_page].kdebug_pages,0
  kd____disp  <13,10,'T  :  dump trace'>          
  FI
  kd____disp  <13,10,'R  :  remote kd intr  +/-        : on/off'>
  test  al,kdebug_io_enabled
  IFNZ  ,,long
  kd____disp  <13,10,'i  :  in port         1/2/4xxxx  : byte/word/dword'>
  kd____disp  <13,10,'         apic/PCIconf a/i/pxxxx  : apic/ioapic/PCIconf-dword'>
  kd____disp  <13,10,'o  :  out port/apic...'>
  FI
  kd____disp  <13,10,'H  :  halt current thread'>
  kd____disp  <13,10,'^  :  reset system'>
  kd____disp  <13,10,'K  :  ke              -/+xxxxxxxx: disable/reenable'>
  kd____disp  <13,10,'V  :  video mode      a/c/m/h    : auto/cga/mono/hercules'>
  kd____disp  <13,10,'                      1/2/-      : com1/com2/no-com'>
  kd____disp  <13,10,'   :  id text         [s: switch processor]'>

  ret


;----------------------------------------------------------------------------
;
;       set video mode
;
;----------------------------------------------------------------------------

set_video_mode:

  kd____inchar
  kd____outchar
  CORZ  al,'2'
  IFZ   al,'1'
        IFZ   al,'1'
              mov   ebx,3F8h SHL 4 
        ELSE_ 
              mov   ebx,2F8h SHL 4 
        FI
        mov   al,byte ptr [physical_kernel_info_page].kdebug_start_port
        and   eax,0Fh
        or    eax,ebx
        mov   [physical_kernel_info_page].kdebug_start_port,ax
        call  set_remote_info_mode
  
  ELIFZ al,'p',long
        kd____disp <'ort: '>
        kd____inhex16  
        
        push  eax      
        kd____disp <' ok? (y/n) '>
        kd____inchar 
        IFNZ  al,'y'
        CANDNZ al,'z'
              pop   eax
              ret
        FI
        pop   eax
  
        shl   eax,4
        mov   bl,byte ptr [physical_kernel_info_page].kdebug_start_port
        and   bl,0Fh
        or    al,bl
        mov   [physical_kernel_info_page].kdebug_start_port,ax
        call  set_remote_info_mode
        
  ELIFZ al,'b',long
        kd____disp <'aud rate divisor: '>
        kd____inhex8
        CORZ  al,0
        IFA   al,15
              mov   al,1     
        FI
        and   byte ptr [physical_kernel_info_page].kdebug_start_port,0F0h
        or    byte ptr [physical_kernel_info_page].kdebug_start_port,al
        IFZ   al,12
              kd____disp <' 9600'>
        ELIFZ al,6
              kd____disp <' 19200'>
        ELIFZ al,3
              kd____disp <' 38400'>
        ELIFZ al,2
              kd____disp <' 57600'>      
        ELIFZ al,1
              kd____disp <' 115200'>      
        FI            

  ELSE_
        kd____outchar
        call  init_kdio
  FI

  ret



;----------------------------------------------------------------------------
;
;       ke disable / reenable
;
;----------------------------------------------------------------------------


ke_disable_reenable:

  IFA   esp,max_physical_memory_size
  
        kd____inchar
        mov   bl,al
        kd____outchar
        kd____inhex32
        
        mov   ebp,[kdebug_esp]

        test  eax,eax
        IFZ
              mov   eax,ss:[ebp+ip_eip]
              test  byte ptr ss:[ebp+ip_cs],11b
              IFZ
                    add   eax,PM
              FI
        FI
        
        push  ds
        push  linear_kernel_space
        pop   ds
        
        dec   eax
        test__page_writable eax
        IFNC
              IFZ   bl,'-'
              CANDZ <byte ptr ds:[eax]>,0CCh
                    mov   byte ptr ds:[eax],90h
              ELIFZ bl,'+'
              CANDZ <byte ptr ds:[eax]>,90h
                    mov   byte ptr ds:[eax],0CCh
              FI      
        FI
        pop   ds                    
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
        kd____disp <'y',13,10>
        call  close_debug_keyboard
        
        sub   eax,eax
        mov   ss:[kdebug_sema+PM],eax

        mov   ebp,esp
        and   ebp,-sizeof tcb

        mov   ss:[ebp+coarse_state],unused_tcb
        mov   ss:[ebp+fine_state],aborted

        DO
              ke 'H'
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
                    call  is_module_header
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
                    call  is_module_header
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
              call  is_module_header
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



is_module_header:
  
  mov   ecx,32
  push  esi
  DO
        cmp   word ptr [esi+8],'C('
        EXITZ
        inc   esi
        RLOOP
  OD
  pop   esi
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
  kd____disp <', '>

  lea   ebx,[esi+8]
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

  kd____disp <', '>

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

  mov   ebp,[kdebug_esp]

  kd____inchar
  IFZ   al,13

        mov   ah,lines-1
        mov   al,20
        kd____cursor

        mov   eax,dr7
        mov   al,[ebp-sizeof kd_save_area+kd_dr7]
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
        mov   [ebp-sizeof kd_save_area+kd_dr7],al
        mov   dr6,eax
        mov   [bx_size],al
        sub   eax,eax
        mov   [debug_breakpoint_counter_value],eax
        mov   [debug_breakpoint_counter],eax

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

  CORZ  al,'p'
  CORZ  al,'i'
  CORZ  al,'w'
  IFZ   al,'a'
        sub   ecx,ecx
        IFNZ  al,'i'
              IFZ   al,'w'
                    mov   cl,01b
              FI
              IFZ   al,'p'
                    mov   cl,10b
              FI            
              IFZ   al,'a'
                    mov   cl,11b
              FI      
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
        mov   [ebp-sizeof kd_save_area+kd_dr7],cl
        sub   eax,eax
        mov   dr6,eax

        ret
  FI

  IFZ   al,'r',long
        kd____disp <'r',6,lines-1,columns-58,'t/T/124/e/-: thread/non-thread/monit124/reg/reset restrictions',6,lines-1,8>
        kd____inchar
        kd____disp <5>
        kd____outchar
        
        IFZ   al,'-'
              sub   eax,eax
              mov   [bx_size],al
              mov   [breakpoint_thread],eax
              mov   [no_breakpoint_thread],eax
              ret
        FI


        CORZ  al,'e'
        CORZ  al,'1'
        CORZ  al,'2'
        IFZ   al,'4',long
              sub   al,'0'
              mov   [bx_size],al
              IFZ   al,'e'-'0',long
                    kd____disp <8,' E'>
                    sub   ebx,ebx
                    kd____inchar
                    and   al,NOT ('a'-'A')
                    mov   ah,'X'
                    IFZ   al,'A'
                          mov   bl,7*4
                    ELIFZ al,'B'
                          kd____outchar
                          kd____inchar
                          and   al,NOT ('a'-'A')
                          IFZ   al,'P'
                                mov     bl,2*4
                                mov     ah,al
                          ELSE_            
                                mov     bl,4*4
                                mov     ah,'X'
                          FI
                          mov   al,0
                    ELIFZ al,'C'
                          mov   bl,6*4
                    ELIFZ al,'D'
                          kd____outchar
                          kd____inchar
                          and   al,NOT ('a'-'A')
                          IFZ   al,'I'
                                mov     bl,0*4
                                mov     ah,al
                          ELSE_            
                                mov     bl,5*4
                                mov     ah,'X'
                          FI
                          mov   al,0
                    ELIFZ al,'S'
                          mov   bl,1*4
                          mov   ah,'I'
                    ELIFZ al,'I'
                          mov   bl,8*4+iret_eip+4
                          mov   ah,'P'      
                    FI
                    IFNZ  al,0
                          push  eax
                          kd____outchar
                          pop   eax
                    FI
                    mov   al,ah
                    kd____outchar            
                    mov   eax,ebx
                                      
              ELSE_                                
                    
                    kd____disp  <' at ',>
                    kd____inhex32
                    
              FI      
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
              kd____inhex16
              mov   [breakpoint_thread],eax
              ret
        FI

        IFZ   al,'T'
              kd____inhex16
              mov   [no_breakpoint_thread],eax
              ret
        FI
        
        mov   al,'?'
        kd____outchar
        ret
  FI       

  IFZ   al,'#'
        kd____outchar
        kd____inhex32
        mov   [debug_breakpoint_counter_value],eax
        mov   [debug_breakpoint_counter],eax
  FI
        
  ret




kdebug_debug_exception_handler:


  push  eax
  mov   eax,dr6
  and   al,NOT 1b
  mov   dr6,eax

  lno___thread eax,esp

  IFZ   cs:[no_breakpoint_thread],eax
        pop   eax
        bts   [esp+iret_eflags],r_flag
        iretd
  FI

  IFNZ  cs:[breakpoint_thread],0
  cmp   cs:[breakpoint_thread],eax
  CANDNZ
        pop   eax
        bts   [esp+iret_eflags],r_flag
        iretd
  FI
  pop   eax

  
  call  check_monitored_data
  IFNZ
        bts   [esp+iret_eflags],r_flag
        iretd
  FI      
  
  IFA   esp,max_physical_memory_size
  CANDA ss:[debug_breakpoint_counter_value+PM],0
        dec   ss:[debug_breakpoint_counter+PM]
        IFNZ 

              bts   [esp+iret_eflags],r_flag
              iretd

        FI      
        push  eax
        mov   eax,ss:[debug_breakpoint_counter_value+PM]     
        mov   ss:[debug_breakpoint_counter+PM],eax
        pop   eax
  FI                 
        

  jmp   cs:[debug_exception_handler]



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

  IFNZ   cs:[bx_size],0,long
  CANDAE esp,<offset tcb_space>,long
  CANDB  esp,<offset tcb_space+tcb_space_size>,long
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
        ELIFZ al,4      
              mov   eax,ss:[ebx]
        ELSE_
              mov   eax,ss:[esp+ebx]
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
  mov   eax,[esi+myself]
  lno___thread eax,eax
  push  eax
  kd____outhex16
  kd____disp <' ('>
  pop   eax
  push  eax
  shr   eax,width lthread_no
  kd____outhex12
  mov   al,'.'
  kd____outchar
  pop   eax
  and   al,lthreads-1
  kd____outhex8

  show  <') ',60>,myself
  mov   al,62
  kd____outchar

  IFNZ  [esi+ressources],0
        kd____disp  <6,0,45,'resrc: '>
        mov   al,[esi+ressources]
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
  mov   bl,[esi+fine_state]
  test  bl,nwait
  IFZ
        kd____disp  <'wait '>
  FI
  test  bl,nclos
  IFZ
        kd____disp  <'clos '>
  FI
  test  bl,nlock
  IFZ
        kd____disp  <'lock '>
  FI
  test  bl,npoll
  IFZ
        kd____disp  <'poll '>
  FI
  test  bl,nready
  IFZ
        kd____disp  <'ready '>
  FI
  test  bl,nwake
  IFZ
        show  <',  wakeup: '>,wakeup_low
        show  <'+'>,wakeup_high
  FI
  show  <6,1,45,'lists: '>,list_state

  show  <6,0,72,'prio: '>,prio
  IFNZ  [esi+max_controlled_prio],0
        show  <6,1,73,'mcp: '>,max_controlled_prio
  FI

  IFDEF state_sp
        movzx eax,[esi+state_sp]
        shl   eax,2
        IFNZ
              push  eax
              kd____disp <6,2,42,'state_sp: '>
              pop   eax
              kd____outhex12
        FI
  ENDIF
  

  kd____disp <6,3,0, 'wait for: '>
  lea   ebx,[esi+waiting_for]
  call  show_thread_id

  kd____disp  <6,4,0, 'sndq    : '>
  lea   ecx,[esi+sndq_root]
  call  show_llinks
  mov   al,' '
  kd____outchar
  lea   ecx,[esi+sndq_llink]
  call  show_llinks

  show  <6,3,40,' rcv descr: '>,rcv_descriptor
  show  <6,4,40,'  timeouts: '>,timeouts

  show  <6,3,60,'   partner: '>,com_partner
  IFDEF waddr
        kd____disp  <6,4,60,'   waddr0/1: '>
        mov   eax,[esi+waddr]
        shr   eax,22
        kd____outhex12
        mov   al,'/'
        kd____outchar
        movzx eax,word ptr [esi+waddr]
        shr   eax,22-16
        kd____outhex12
  ENDIF

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

  IFDEF pager
        kd____disp <6,7,0, 'pager   : '>
        lea   ebx,[esi+pager]
        call  show_thread_id
  ENDIF      

  kd____disp <6,8,0, 'ipreempt: '>
  lea   ebx,[esi+int_preempter]
  call  show_thread_id

  kd____disp <6,9,0, 'xpreempt: '>
  lea   ebx,[esi+ext_preempter]
  call  show_thread_id

  kd____disp  <6, 7,40, 'prsent lnk: '>
  test  [esi+list_state],is_present
  IFNZ
        lea   ecx,[esi+present_llink]
        call  show_llinks
  FI
  kd____disp  <6, 8,40, 'ready link : '>
  IFDEF ready_llink
        test  [esi+list_state],is_ready
        IFNZ
              lea   ecx,[esi+ready_llink]
              call  show_llinks
        FI
  ELSE
        lea   ecx,[esi+ready_link]
        call  show_link
        kd____disp  <6,9,40, 'intr link : '>
        lea   ecx,[esi+interrupted_link]
        call  show_link
  ENDIF

  kd____disp  <6,10,40, 'soon wakeup lnk: '>
  test  [esi+list_state],is_soon_wakeup
  IFNZ
        lea   ecx,[esi+soon_wakeup_link]
        call  show_link
  FI
  kd____disp  <6,11,40, 'late wakeup lnk: '>
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
        kd____outhex16
        kd____disp  <'  ',60>
        mov   eax,[ebx]
        kd____outhex32
        mov   al,' '
        kd____outchar
        mov   eax,[ebx+4]
        kd____outhex32
        mov   al,62
        kd____outchar
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

  test  cs:[physical_kernel_info_page].kdebug_permissions,kdebug_dump_regs_enabled
  IFZ
        mov   al,0
        ret
  FI
        

  IFZ   esi,ebp
  mov   eax,cs:[kdebug_esp]
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
        call  is_main_level_command_key
        EXITZ

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

        IFZ   ebp,cs:[kdebug_esp]

              kd____disp <6,11,28,'DS='>
              mov   ax,[ebp-sizeof kd_save_area].kd_ds
              kd____outhex16
              kd____disp <6,12,28,'ES='>
              mov   ax,[ebp-sizeof kd_save_area].kd_es
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

  test  ah,kdebug_dump_mem_enabled
  IFZ
        mov   al,0
        ret
  FI


  mov   [dump_area_base],0
  mov   [dump_area_size],linear_address_space_size

  kd____inhex32
  test  eax,eax
  IFZ   ,,long
  mov   eax,ss
  CANDZ eax,linear_kernel_space,long
        kd____disp <' Gdt/Idt/Task/Sigma0/Redir ? '>    ;REDIR ----
        kd____inchar
        IFZ   al,'g'
              mov   eax,offset gdt
        ELIFZ al,'i'
              mov   eax,offset idt
        IFDEF task_proot
              ELIFZ al,'t'
                    mov   eax,offset task_proot
        ENDIF            
        ELIFZ al,'s'
              mov   edi,offset logical_info_page
              mov   eax,ss:[edi+reserved_mem1].mem_begin
              mov   ecx,ss:[edi+main_mem].mem_end
              shr   ecx,log2_pagesize
              sub   eax,ecx
              and   eax,-pagesize
              add   eax,PM
        ELIFZ al,'r'                                    ;REDIR begin ----------------
              kd____disp <'task: '>                     ;
              kd____inhex16                             ;
              and   eax,tasks-1                         ;
              shl   eax,log2_tasks+2                    ;
              add   eax,offset redirection_table        ;
             ; mov   [dump_area_size],tasks*4            ;REDIR ends -----------------
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
        call  is_main_level_command_key
        REPEATNZ
  OD
  pop   esi
  pop   edi

  push  eax
  mov   ah,lines-1
  mov   al,0
  kd____cursor
  kd____disp  <'LNKD: ',5>
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


  IFDEF task_proot


display_ptabs:

  test  ah,kdebug_dump_map_enabled
  IFZ
        mov   al,0
        ret
  FI


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
        load__proot eax,eax
        pop   ds
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
                          push  esi
                          push  edi
                          push  ebp
                          mov   eax,edx
                          call  display_mappings_of
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

                    test  [physical_kernel_info_page].kdebug_permissions,kdebug_dump_mem_enabled
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
              
                    call  is_main_level_command_key
                    REPEATNZ

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
  kd____disp  <'LNKD: ',5>
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
  
  
  
  ENDIF


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
        kd____disp <6,lines-1,0,'LNKD: '>
        mov   al,[dump_type]
        kd____outchar
        mov   al,'<'
        kd____outchar
        mov   eax,ebx
        kd____outhex32
        mov   al,'>'
        kd____outchar
        kd____disp <6,lines-1,columns-35,'++KEYS: ',24,' ',25,' ',26,' ',27,' Pg',24,' Pg',25,' CR Home    '>
        IFDEF task_proot
              IFZ   ebp,<offset dump_ptab>
                    kd____disp <6,lines-1,columns-3,3Ch,'m',3Eh>
              FI
        ENDIF      
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
        CORZ  al,'+'
        IFZ   al,11h
              add   esi,dumplines*8*4 AND -100h
              add   edi,dumplines*8*4 AND -100h
              mov   al,0
        FI
        CORZ  al,'-'
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
              test  dl,page_present
              ;; IFZ
              ;;      mov   eax,edx
              ;;      kd____outhex32
              ;;      mov   al,' '
              ;;      kd____outchar
              ;;      ret
              ;; FI
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
  CORZ
  test  edx,(MB4-1) AND -pagesize
  IFNZ
        shr   eax,12
        kd____outhex16
  ELSE_
        shr   eax,22
        shl   eax,2
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
  test  dl,page_present
  IFNZ
        mov   al,'r'
        test  dl,page_write_permit
        IFNZ
              mov   al,'w'
        FI
  ELSE_
        mov   al,'y'
        test  dl,page_write_permit
        IFNZ
              mov   al,'z'
        FI
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

  
  IFDEF task_proot
  
  

display_mappings:

  IFB_  esp,<offset tcb_space>
        ret
  FI

  kd____inhex32
  shl   eax,log2_pagesize



display_mappings_of:

  push  ds
  push  es

  push  linear_kernel_space
  pop   ds
  push  linear_kernel_space
  pop   es


  mov   esi,eax

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
              call  is_main_level_command_key
              EXITZ
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
  kd____disp  <'LNKD: ',5>
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
  
  IFDEF highest_active_prio
        kd____disp <6,6,1,'highest prio  : '>
        mov   eax,ds:[highest_active_prio]
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


  IFDEF task_proot
  

page_fault_prot:

  test  ah,kdebug_protocol_enabled
  IFZ
        mov   al,0
        ret
  FI


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

  CORZ  al,'+'
  IFZ   al,'*'
        mov   [page_fault_prot_state],al
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
        mov   [page_fault_prot_state],al
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
        mov   [page_fault_prot_state],al
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
  IFNZ  eax,<offset ldt>,long
  CANDAE eax,[page_fault_low+PM],long
  CANDBE eax,[page_fault_high+PM],long

        mov   ebx,cr2
        mov   ecx,[esp+ip_eip]
        lno___thread eax,esp
        shl   eax,8
        
        IFZ   [page_fault_prot_state+PM],'*'
        
              call  put_into_trace_buffer
              
        ELSE_      
              kd____disp <13,10>
              call  display_page_fault
              call  event_ack
        FI
  FI

  pop   ds

  popad
  jmp   cs:[page_fault_handler]





display_page_fault:                            ; EBX  fault address
                                               ; ECX  fault EIP
                                               ; EAX  thread no SHL 8 + 00
                                               
                                               ; --> EAX scratch
  mov   edx,eax
  shr   edx,8                                             
  kd____disp  <'#PF: '>
  mov   eax,ebx
  kd____outhex32
  kd____disp  <', eip='>
  mov   eax,ecx
  kd____outhex32
  kd____disp  <', thread='>
  mov   eax,edx
  kd____outhex16
  
  ret
  
  
  ENDIF



event_ack:

 call  open_debug_keyboard
 kd____inchar
 IFZ   al,'i'
       ke    'kdebug'
 FI
 call  close_debug_keyboard

 ret
 
 
 
;----------------------------------------------------------------------------
;
;       IPC prot
;
;----------------------------------------------------------------------------


ipc_prot:

  test  ah,kdebug_protocol_enabled
  IFZ
        mov   al,0
        ret
  FI


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
  kd____outchar

  CORZ  al,'+'
  IFZ   al,'*'
        mov   [ipc_prot_state],al
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
        sub   eax,eax
        xchg  eax,[ipc_handler]
        test  eax,eax
        IFNZ
              mov   bl,ipc
              call  set_exception_handler
        FI
        ret
  FI

  IFZ   al,'r',long
        kd____disp <6,lines-1,columns-58,'t/T/s/-    : thread/non-thread/send-only/reset restrictions',6,lines-1,8>
        kd____inchar
        kd____disp <5>
        kd____outchar
        
        IFZ   al,'-'
              sub   eax,eax
              mov   [ipc_prot_thread],eax
              mov   [ipc_prot_non_thread],eax
              mov   [ipc_prot_mask],0FFh
              ret
        FI

        IFZ   al,'t'
              kd____inhex16
              mov   [ipc_prot_thread],eax
              ret
        FI
        IFZ   al,'T'
              kd____inhex16
              mov   [ipc_prot_non_thread],eax
              ret
        FI
        
        IFZ   al,'s'
              mov   [ipc_prot_mask],08h
              ret
        FI      
  FI       

  mov   al,'?'
  kd____outchar

  ret



show_ipc:

  ipre  fault
  
  mov   ecx,ebp
  and   cl,11b
  IFB_  ebp,virtual_space_size
        or   cl,100b
  FI
  and   al,11b
  IFB_  eax,virtual_space_size
        or   al,100b
  FI
  shl   cl,3
  add   cl,al
  add   cl,40h
  lno___thread eax,esp
  
  IFNZ  [ipc_prot_thread+PM],0
        cmp   [ipc_prot_thread+PM],eax
  FI
  IFZ
  CANDNZ eax,[ipc_prot_non_thread+PM]
  test  cl,[ipc_prot_mask+PM]
  CANDNZ      
         
        shl   eax,8
        mov   al,cl
        mov   ecx,esi
  
        IFZ   [ipc_prot_state+PM],'*'
  
              call  put_into_trace_buffer
        
        ELSE_      
              kd____disp <13,10>
              call  display_ipc
              call  event_ack
        FI
  FI       

  pop   ds

  popad
	add	esp,4
  jmp   cs:[ipc_handler]




display_ipc:                                   ; EAX  :  src SHL 8 + ipc type
                                               ; EBX  :  msg w1
                                               ; ECX  :  dest 
                                               ; EDX  :  msg w0
                                               
                                               ; --> EAX scratch
  kd____disp <'ipc: '>                                             
  push  eax
  shr   eax,8
  kd____outhex16
  pop   eax
  
  mov   ah,al
  and   al,00101100b
  push  eax
  
  IFZ   al,00100000b
		kd____disp	<' waits for '>
	ELIFZ al,00101000b
	      kd____disp  <' waits '>
  ELIFZ al,00000100b
	      kd____disp  <' -sends--> '>
  ELIFZ al,00100100b
        kd____disp  <' -calls--> '>
  ELIFZ al,00101100b
	      kd____disp  <' replies-> '>
	FI	
	IFNZ  al,00101000b	
        lno___thread eax,ecx
        test  eax,eax
        IFZ
              kd____disp <' - '>
        ELSE_      
              kd____outhex16
        FI      
  FI      
  pop   eax
         
  push  eax
  test  al,00000100b
	IFNZ
        test  ah,00000010b
		IFZ
  		kd____disp	<'     ('>
		ELSE_
			kd____disp  <' map ('>
		FI
		mov	eax,edx
		kd____outhex32
		mov	al,','
		kd____outchar
		mov	eax,ebx
		kd____outhex32
		mov	al,')'
		kd____outchar
	FI       
  pop   eax
  
  IFZ   al,00101100b
        kd____disp  <' and waits'>
  FI      

  ret
  
  

;----------------------------------------------------------------------------
;
;       monit exception
;
;----------------------------------------------------------------------------


monit_exception:

  test  ah,kdebug_protocol_enabled
  IFZ
        mov   al,0
        ret
  FI


  kd____inchar
  kd____outchar

  push  eax  
  CORZ  al,'*'
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
  
  
  CORZ  al,'*'
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
  
  mov   al,cs:[monitored_exception]
  mov   ebp,esp
  DO
        
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
              
        IFAE  al,11
        CANDB al,15
              movzx eax,word ptr ss:[ebp+ip_error_code]        
              movzx ebx,cs:[monitored_ec_min]
              movzx ecx,cs:[monitored_ec_max]
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
                        

  pop   ds

  popad
  
  CORB  cs:[monitored_exception],8           
  IFA   cs:[monitored_exception],14
        IFNZ  cs:[monitored_exception],17           
              add   esp,4
        FI      
  FI
  
  jmp   cs:[monitored_exception_handler]




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

  dec   byte ptr ss:[kdebug_timer_intr_counter+PM]
  IFZ
  
        ipre  fault,no_load_ds

        kd____incharety
        IFZ   al,27
              ke    'ESC'
        FI

        ko    T

        pop   ds

        popad
        add   esp,4
  FI
                
  jmp   cs:[timer_intr_handler]



;----------------------------------------------------------------------------
;
;       single stepping on/off
;
;----------------------------------------------------------------------------
;
;
;
;single_stepping_on_off:
;
;  kd____inchar
;  mov   edi,[kdebug_esp]
;  IFA   edi,<virtual_space_size>
;        push  ds
;        push  linear_kernel_space
;        pop   ds
;  FI
;
;  IFZ   al,'+'
;        bts   [edi+ip_eflags],t_flag
;  else_
;        btr   [edi+ip_eflags],t_flag
;        mov   al,'-'
;  FI
;
;  IFA   edi,<virtual_space_size>
;        pop   ds
;  FI
;  kd____outchar
;  ret



;----------------------------------------------------------------------------
;
;       virtual address info
;
;----------------------------------------------------------------------------


  IFDEF task_proot
  

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


  ENDIF
  
;----------------------------------------------------------------------------
;
;       port io
;
;----------------------------------------------------------------------------


pic1_imr            equ 21h


pci_address_port    equ 0CF8h
pci_data_port       equ 0CFCh


port_io:

  test  ah,kdebug_io_enabled
  IFZ
        mov   al,0
        ret
  FI



  mov   bh,al
  IFZ   al,'i'
        kd____disp <'n  '>
  ELSE_
        kd____disp <'ut '>
  FI            
  
  kd____inchar
  mov   bl,al
  kd____outchar
  IFZ   al,'a'
        kd____disp <'pic '>
  ELIFZ al,'i'
        kd____disp <'o apic '>
  ELIFZ al,'p'
        kd____disp <'ci conf dword '>      
  ELSE_
        kd____disp <'-byte port '>                  
  FI
        
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
  
  
  IFZ   bl,'p'
  
        push  eax
        mov   eax,edx
        or    eax,8000000h
        mov   dx,pci_address_port
        out   dx,eax
        pop   eax
        
        mov   dx,pci_data_port
        IFZ   bh,'o'
              out   dx,eax
        ELSE_
              in    eax,dx
              kd____outhex32
        FI
        ret
  FI
        
        
  
  
  


  
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


  IFNDEF task_proot
  
        test  esp,esp
        ret
        
  ELSE
        

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

  load__proot edx,ebx
  IFAE  eax,shared_table_base
  CANDBE eax,shared_table_base+shared_table_size-1
	lno___prc edx
        mov   edx,ds:[kernel_proot+8*edx-8]
  FI

  xpdir ebx,eax
  xptab ecx,eax
  mov   ebx,dword ptr [(ebx*4)+edx+PM]
  mov   dl,bl
  and   ebx,-pagesize

  test  dl,page_present
  IFNZ
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
        IFAE  ecx,<physical_kernel_mem_size>      ; no access beyond PM
              mov   dl,0                          ; ( 0 ... 64 M )
        FI                                        ;
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


  ENDIF


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




;--------------------------------------------------------------------------
; 
;       trace events
;
;--------------------------------------------------------------------------


trace_event:

  IFAE  esp,virtual_space_size
  
        mov   esi,ebx
        mov   cl,al
        lno___thread eax,esp
        shl   eax,8

        push  ds
        push  linear_kernel_space
        pop   ds

        add   esi,PM
        
        IFZ   cl,'*'
              mov   al,80h
              mov   ebx,[esi+13]
              mov   ecx,[esi+17]
              call  put_words_4_to_5_into_trace_buffer
              mov   ebx,[esi+1]
              mov   bl,[esi]
              dec   bl
              IFA   bl,19
                    mov   bl,19
              FI      
              mov   ecx,[esi+5]
              mov   edx,[esi+9]
        ELSE_
              mov   al,81h
              mov   ebx,[esi+9]
              mov   ebx,[esi+13]
              call  put_words_4_to_5_into_trace_buffer
              mov   ebx,[ebp+ip_eax]
              mov   ecx,[esi+1]
              mov   cl,[esi]
              dec   cl
              IFA   cl,15
                    mov   cl,15
              FI      
              mov   edx,[esi+5]
        FI
        call  put_into_trace_buffer
        DO
        
        pop   ds
  FI        

  jmp   ret_from_kdebug
  



display_event:

  push  eax
  IFZ   al,80h
        kd____disp <'ke : *'>
        lea   eax,[esi+trace_entry_string]
        kd____outstring
  ELSE_
        kd____disp <'ke : #'>
        lea   eax,[esi+trace_entry_string+4]
        kd____outstring
        kd____disp <' ('>
        mov   eax,ebx
        kd____outhex32
        kd____disp <')'>
  FI
  kd____disp <', thread='>
  pop   eax
  shr   eax,8
  kd____outhex16
  
  ret
  
                    


;--------------------------------------------------------------------------
;
;       init trace buffer
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       DS    phys mem
;
;---------------------------------------------------------------------------


init_trace_buffer:

  pushad
  
  IFNZ  [physical_kernel_info_page].kdebug_pages,0

        call  grab_frame
        mov   ebx,eax
        mov   ecx,KB4
        DO
              call  grab_frame
              sub   ebx,eax
              IFNC
              CANDZ ebx,KB4
                    add   ecx,ebx
                    mov   ebx,eax
                    movzx edx,[physical_kernel_info_page].kdebug_pages
                    shl   edx,log2_pagesize
                    cmp   ecx,edx
                    REPEATB
              ELSE_
                    lea   eax,[eax+ebx]
              FI      
        OD

        mov   [trace_buffer_begin],eax
        mov   [trace_buffer_in_pointer],eax
        mov   edi,eax
        add   eax,ecx
        mov   [trace_buffer_end],eax
  
        call  flush_active_trace_buffer
  
        shr   ecx,2
        sub   eax,eax
        cld
        rep   stosd
  FI      
  
  popad
  ret
  
        


;--------------------------------------------------------------------------
;
;       put into trace buffer
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX         0                          src/ipc type
;       EBX         fault addr                 msg w1
;       ECX         fault EIP                  dest
;       EDX         thread                     msg w0
;
;       DS    linear kernel space   
;
;---------------------------------------------------------------------------



put_into_trace_buffer:
  
  mov   edi,[trace_buffer_in_pointer+PM]
  test  edi,edi
  IFNZ
        add   edi,PM
        
        mov   [edi+trace_entry_type],eax
        mov   [edi+trace_entry_string],ebx
        mov   [edi+trace_entry_string+4],ecx
        mov   [edi+trace_entry_string+8],edx
        
        get___timestamp
        mov   [edi+trace_entry_timestamp],eax
        mov   [edi+trace_entry_timestamp+4],edx
        
        IFNZ  [trace_perf_monitoring_mode+PM],no_perf_mon
              mov   ecx,event_select_msr
              rdmsr
              mov   ebx,eax
              and   eax,NOT ((11b SHL 22)+(11b SHL 6))
              wrmsr
              mov   ecx,event_counter0_msr
              rdmsr
              mov   [edi+trace_entry_perf_count0],eax
              mov   ecx,event_counter1_msr
              rdmsr
              mov   [edi+trace_entry_perf_count1],eax
              mov   ecx,event_select_msr
              mov   eax,ebx
              wrmsr
        FI
  
        add   edi,sizeof trace_buffer_entry-PM
        IFAE  edi,[trace_buffer_end+PM]
              mov   edi,[trace_buffer_begin+PM]
        FI
        mov   [trace_buffer_in_pointer+PM],edi  
  FI      
        
  ret
  
  
  
put_words_4_to_5_into_trace_buffer:

  mov   edi,[trace_buffer_in_pointer+PM]
  test  edi,edi
  IFNZ
        add   edi,PM
        
        mov   [edi+trace_entry_string+12],ebx
        mov   [edi+trace_entry_string+16],ecx
  FI
  ret
        
        
        
        
flush_active_trace_buffer:

  get___timestamp
  mov   [trace_buffer_active_stamp],eax
  mov   [trace_buffer_active_stamp+4],edx
  
  ret
  
          


open_trace_buffer:

  mov   ebx,[trace_buffer_begin]
  test  ebx,ebx
  IFNZ
  mov   eax,[ebx+trace_entry_timestamp]
  or    eax,[ebx+trace_entry_timestamp+4]
  CANDNZ
        DO
              mov   esi,ebx
              mov   eax,[ebx+trace_entry_timestamp]
              mov   edx,[ebx+trace_entry_timestamp+4]
              add   ebx,sizeof trace_buffer_entry
              IFAE  esi,[trace_buffer_end]
                    mov   ebx,[trace_buffer_begin]
              FI
              sub   eax,[ebx+trace_entry_timestamp]
              sbb   edx,[ebx+trace_entry_timestamp+4]
              REPEATC
        OD         
              ret               ; NC!
  FI
  
  stc
  ret
      
  
  
  
  
forward_trace_buffer:

  push  ebx
  
  mov   ebx,esi
  DO
        mov   eax,[ebx+trace_entry_timestamp]
        mov   edx,[ebx+trace_entry_timestamp+4]
        add   ebx,sizeof trace_buffer_entry
        IFAE  ebx,[trace_buffer_end]
              mov   ebx,[trace_buffer_begin]
        FI
        sub   eax,[ebx+trace_entry_timestamp]
        sbb   edx,[ebx+trace_entry_timestamp+4]
        EXITNC
        
        IFNZ  [trace_display_mask],0
              mov   eax,[ebx+trace_entry_type]
              cmp   eax,[trace_display_mask]
              REPEATNZ
              IFNZ  [trace_display_mask+4],0
                    mov   eax,[ebx+trace_entry_string]
                    cmp   eax,[trace_display_mask+4]
                    REPEATNZ
              FI         
        FI      
        mov   esi,ebx
        sub   cl,1              ; NC!
        REPEATNZ
        
        pop   ebx
        ret
        
  OD      
  stc
  
  pop   ebx
  ret
  
  
backward_trace_buffer:

  push  ebx
  
  mov   ebx,esi
  DO
        mov   eax,[ebx+trace_entry_timestamp]
        mov   edx,[ebx+trace_entry_timestamp+4]
        sub   ebx,sizeof trace_buffer_entry
        IFB_  ebx,[trace_buffer_begin]
              mov   ebx,[trace_buffer_end]
              sub   ebx,sizeof trace_buffer_entry
        FI
        sub   eax,[ebx+trace_entry_timestamp]
        sbb   edx,[ebx+trace_entry_timestamp+4]
        EXITC
        mov   eax,[ebx+trace_entry_timestamp]
        or    eax,[ebx+trace_entry_timestamp+4]
        EXITZ
        
        IFNZ  [trace_display_mask],0
              mov   eax,[ebx+trace_entry_type]
              cmp   eax,[trace_display_mask]
              REPEATNZ
              IFNZ  [trace_display_mask+4],0
                    mov   eax,[ebx+trace_entry_string]
                    cmp   eax,[trace_display_mask+4]
                    REPEATNZ
              FI         
        FI     
        mov   esi,ebx 
        sub   cl,1
        REPEATNZ          ; NC!
        
        pop   ebx
        ret
  OD       
  stc 
  
  pop   ebx
  ret
  
                              
                      
        
;--------------------------------------------------------------------------
;
;       show active trace buffer tail
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       DS    phys mem
;
;--------------------------------------------------------------------------


show_active_trace_buffer_tail:


  IFAE  esp,virtual_space_size
  call  open_trace_buffer
  CANDNC
        sub   eax,eax
        mov   [trace_display_mask],eax
        mov   cl,lines-3
        call  backward_trace_buffer
        
        DO
              mov   eax,[esi+trace_entry_timestamp]
              mov   edx,[esi+trace_entry_timestamp+4]
              sub   eax,[trace_buffer_active_stamp]
              sbb   edx,[trace_buffer_active_stamp+4]
              IFAE
                    kd____disp <13,10>
                    mov   eax,[esi+trace_entry_type]
                    mov   ebx,[esi+trace_entry_string]
                    mov   ecx,[esi+trace_entry_string+4]
                    mov   edx,[esi+trace_entry_string+8]
                    IFB_  al,40h
                          IFDEF task_proot
                                call  display_page_fault
                          ENDIF      
                    ELIFB al,80h
                          call  display_ipc
                    ELSE_
                          call  display_event      
                    FI
              FI      
              mov   cl,1
              call  forward_trace_buffer
              REPEATNC
        OD
        
  FI
  ret                   
  
              

;--------------------------------------------------------------------------
;
;       dump trace buffer
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       DS    phys mem
;
;--------------------------------------------------------------------------


dump_trace_buffer:

  mov   al,0
  
  CORB  esp,virtual_space_size
  call  open_trace_buffer
  IFC
        ret
  FI      
  
  mov   [trace_link_presentation],display_trace_index_mode
  mov   [trace_reference_mode],no_references
  sub   eax,eax
  mov   [trace_display_mask],eax
  
  mov   cl,lines-2
  call  backward_trace_buffer
  
  DO
  
        kd____clear_page
        mov   al,1
        kd____outchar
        sub   ecx,ecx
        
        sub   ecx,ecx
        mov   ebp,esi
        mov   edi,esi
        DO           
              push  ecx
              mov   eax,[esi+trace_entry_type]
              mov   ebx,[esi+trace_entry_string]
              mov   ecx,[esi+trace_entry_string+4]
              mov   edx,[esi+trace_entry_string+8]
              IFB_  al,40h
                    IFDEF task_proot
                          call  display_page_fault
                    ENDIF
              ELIFB al,80h
                    call  display_ipc
              ELSE_
                    call  display_event      
              FI
              mov   al,5
              kd____outchar                
              pop   ecx

              IFNZ  [trace_reference_mode],no_references
                    mov   al,columns-40
                    mov   ah,cl
                    kd____cursor
                    kd____disp <5,' '> 
                    
                    IFZ   [trace_reference_mode],performance_counters
                          call   display_trace_performance_counters
                    ELSE_
                          push  ebp
                          mov   ebx,offset backward_trace_buffer
                          IFZ   [trace_reference_mode],forward_references
                                mov   ebx,offset forward_trace_buffer
                          FI      
                          call  display_trace_reference
                          pop   ebp
                    FI      
              FI      
                                  
              push  ecx
              mov   al,columns-19
              mov   ah,cl
              kd____cursor
              mov   al,[trace_link_presentation]
              IFZ   al,display_trace_index_mode
                    mov   ch,' '
                    call  display_trace_index
              ELIFZ al,display_trace_delta_time_mode
                    mov   ch,' '
                    call  display_trace_timestamp
              ELIFZ al,display_trace_offset_time_mode
                    mov   ch,'t'
                    xchg  ebp,edi
                    call  display_trace_timestamp
                    xchg  ebp,edi
              FI      
              kd____disp <13,10>
              mov   ebp,esi
              mov   cl,1
              call  forward_trace_buffer
              pop   ecx
              EXITC
              inc   ecx
              cmp   ecx,lines-1
              REPEATB
        OD
        
        call  backward_trace_buffer
        
        call  get_kdebug_cmd
        
        mov   cl,0
        IFZ   al,10
              mov   cl,1
        FI      
        IFZ   al,3
              mov   cl,-1
        FI    
        CORZ  al,'+'  
        IFZ   al,11h
              mov   cl,lines-1
        FI    
        CORZ  al,'-'  
        IFZ   al,10h
              mov   cl,-(lines-1)
        FI
        
        IFZ   cl,0,long
              
              IFZ   al,8
                    mov   ebx,offset trace_reference_mode
                    IFZ   <byte ptr [ebx]>,forward_references
                          mov   byte ptr [ebx],no_references
                    ELIFZ <byte ptr [ebx]>,performance_counters
                          mov  byte ptr [ebx],forward_references
                    ELSE_      
                          mov   byte ptr [ebx],backward_references
                    FI
              
              ELIFZ al,2,long
                    mov   ebx,offset trace_reference_mode
                    IFZ   <byte ptr [ebx]>,backward_references
                          mov   byte ptr [ebx],no_references
                    ELIFZ <byte ptr [ebx]>,forward_references
                    CANDNZ [trace_perf_monitoring_mode],no_perf_mon
                          mov  byte ptr [ebx],performance_counters
                    ELSE_      
                          mov   byte ptr [ebx],forward_references
                    FI
                      
              ELIFZ al,13
                    mov   ebx,offset trace_display_mask
                    sub   eax,eax
                    IFZ   [ebx],eax
                          mov   [ebx+4],eax
                          mov   eax,[esi]
                          mov   [ebx],eax
                    ELSE_
                          mov   eax,[esi+4]
                          mov   [ebx+4],eax
                    FI            
              
              ELIFZ al,1
                    mov   ebx,offset trace_display_mask
                    sub   eax,eax
                    IFNZ  [ebx+4],eax
                          mov   [ebx+4],eax
                    ELSE_      
                          mov   [ebx],eax
                    FI      
                    
                    
              ELIFZ al,' '
                    mov   al,[trace_link_presentation]
                    IFZ   al,display_trace_index_mode
                          mov   al,display_trace_delta_time_mode
                    ELIFZ al,display_trace_delta_time_mode
                          mov   al,display_trace_offset_time_mode
                    ELSE_
                          mov   al,display_trace_index_mode
                    FI
                    mov   [trace_link_presentation],al       
              
              ELIFZ al,'P'
              bt    ss:[cpu_feature_flags],pentium_style_msrs_bit      
              CANDC
                    call  set_performance_tracing              
                    
              ELSE_
                    call  is_main_level_command_key
                    EXITZ
              FI
        FI
                          
        IFG   cl,0  
              mov   ch,cl
              call  forward_trace_buffer
              push  esi
              mov   cl,lines-2
              call  forward_trace_buffer
              pop   esi
              IFNZ  cl,0
                    IFB_  ch,cl
                          mov   cl,ch
                    FI      
                    call  backward_trace_buffer
              FI      
        ELIFL cl,0
              neg   cl
              call  backward_trace_buffer
        FI            
        
        REPEAT      
        
  OD                 
              
  
  ret                   
  
              


;--------------------------------------------------------------------------
;
;       display trace index
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   pointer to current trace entry
;       CH    prefix char
;
;       DS    phys mem
;
;--------------------------------------------------------------------------


display_trace_index:

  push  eax
  
  mov   al,ch
  kd____outchar
  mov   eax,esi
  sub   eax,[trace_buffer_in_pointer]
  IFC
        add   eax,[trace_buffer_end]
        sub   eax,[trace_buffer_begin]
  FI      
  log2  <(sizeof trace_buffer_entry)>
  shr   eax,log2_
  kd____outhex12
  
  pop   eax
  ret
  
  
  
              
;--------------------------------------------------------------------------
;
;       display trace timestamp
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   pointer to current trace entry
;       EBP   pointer to reference trace entry
;       CH    prefix char
;
;       DS    phys mem
;
;--------------------------------------------------------------------------


display_trace_timestamp:

  push  eax
  push  ebx
  push  ecx
  push  edx
  
  
  mov   eax,[esi+trace_entry_timestamp]
  mov   edx,[esi+trace_entry_timestamp+4]
  
  IFNZ  esi,ebp      
        mov   cl,'+'
        sub   eax,ds:[ebp+trace_entry_timestamp]
        sbb   edx,ds:[ebp+trace_entry_timestamp+4]
        IFC
              mov   cl,'-'
              neg   eax
              adc   edx,0
              neg   edx
        FI            
  FI 
  
  push  ecx
  
  mov   ebx,eax
  mov   ecx,edx
  
  mov   eax,2000000000
  sub   edx,edx
  div   dword ptr ds:[physical_kernel_info_page+cpu_clock_freq]
  shr   eax,1
  adc   eax,0
  
  imul  ecx,eax
  mul   ebx
  add   edx,ecx                    ; eax,edx : time in nanoseconds
  
  pop   ecx
  
  IFZ   esi,ebp
        IFZ   ch,'t'
              kd____disp <' t=.'>
        ELSE_      
              kd____disp <'   .'>
        FI      
        mov   cl,'.'
        mov   ch,'.'
        mov   ebx,1000/200
        call  outdec2
        kd____disp <' us'>
        
  ELSE_
        CORA  edx,0
        IFAE  eax,1000000000
              mov   ebx,1000000000/200
              call  outdec2
              kd____disp <' s'>
        
        ELIFAE eax,1000000
              kd____disp <'    '>
              mov   ebx,1000000/200
              call  outdec2
              kd____disp <' ms'>
        ELSE_           
              kd____disp <'        '>
              mov   ebx,1000/200
              call  outdec2
              kd____disp <' us'>
        FI
  FI            
          
  
  pop   edx
  pop   ecx
  pop   ebx
  pop   eax
  ret
  
  
  
outdec2:

  sub   edx,edx
  div   ebx
  shr   eax,1
  adc   eax,0
    
  mov   ebx,100
  sub   edx,edx
  div   ebx
  
  IFB_  eax,10
        kd____disp <'  '>
  ELIFB eax,100
        kd____disp <' '>
  FI                  
  
  push  eax
  mov   al,ch
  kd____outchar
  mov   al,cl
  kd____outchar
  pop   eax
    
  kd____outdec
  kd____disp <'.'>
  mov   eax,edx
  IFB_  eax,10
        kd____disp <'0'>
  FI
  kd____outdec
  
  ret                                




;--------------------------------------------------------------------------
;
;       display reference
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   pointer to current trace entry
;
;       DS    phys mem
;
;--------------------------------------------------------------------------


display_trace_reference:

  push  eax
  push  ecx
  push  ebp
  
  mov   ebp,esi
  
  DO
        mov   cl,1
        call  ebx
        EXITC
        
        mov   eax,[esi+trace_entry_type]
        cmp   eax,ds:[ebp+trace_entry_type]
        REPEATNZ
        mov   eax,[esi+trace_entry_string]
        cmp   eax,ds:[ebp+trace_entry_string]
        REPEATNZ
        
        mov   ch,'@'
        IFZ   [trace_link_presentation],display_trace_index_mode
              call  display_trace_index
        ELSE_
              call  display_trace_timestamp      
        FI
  OD
  
  mov   esi,ebp
  pop   ebp
  pop   ecx
  pop   eax
  ret            
  


;--------------------------------------------------------------------------
;
;       set performance tracing
;
;--------------------------------------------------------------------------



event_select_msr    equ 11h
event_counter0_msr  equ 12h
event_counter1_msr  equ 13h



rd_miss       equ    000011b
wr_miss       equ    000100b
rw_miss       equ    101001b
ex_miss       equ    001110b

d_wback       equ    000110b

rw_tlb        equ    000010b
ex_tlb        equ    001101b

a_stall       equ  00011111b
w_stall       equ  00011001b
r_stall       equ  00011010b
x_stall       equ  00011011b
agi_stall     equ  00011111b

bus_util      equ  00011000b

pipline_flush equ    010101b

non_cache_rd  equ    011110b
ncache_refs   equ    011110b
locked_bus    equ    011100b

mem2pipe      equ    001001b
bank_conf     equ    001010b


instrs_ex     equ    010110b
instrs_ex_V   equ    010111b






set_performance_tracing:

  kd____clear_page
  call  show_trace_perf_monitoring_mode
  kd____disp <' Performance Monitoring',13,10,10,'- : off, + : kernel+user, k : kernel, u : user',13,10,10>
  kd____disp <'i : Instructions   (total/V-pipe)',13,10>
  kd____disp <'c : Cache Misses  (DCache/ICache)',13,10>
  kd____disp <'t : TLB Misses      (DTLB/ITLB)',13,10>
  kd____disp <'m : Memory Stalls   (read/write)',13,10>
  kd____disp <'a : Interlocks       (AGI/Bank Conflict)',13,10>
  kd____disp <'b : Bus Utilization  (Bus/Instructions)',13,10>
  
  DO
        kd____inchar
        kd____outchar
  
  
        IFZ   al,'-'
              mov   [trace_perf_monitoring_mode],no_perf_mon
              sub   eax,eax
              mov   ecx,event_select_msr
              wrmsr
              ret
        FI
         
        CORZ  al,'+'
        CORZ  al,'k'
        IFZ   al,'u'      
              IFZ   al,'+'
                    mov   al,kernel_user_perf_mon
              ELIFZ al,'k'
                    mov   al,kernel_perf_mon
              ELIFZ al,'u'
                    mov   al,user_perf_mon
              FI
              mov   [trace_perf_monitoring_mode],al
              call  show_trace_perf_monitoring_mode
              REPEAT      
        FI
  OD        
        
  sub   ebx,ebx
  IFZ   al,'i'
        mov   ebx,instrs_ex + (instrs_ex_V SHL 16)
  FI
  IFZ   al,'c'
        mov   ebx,rw_miss + (ex_miss SHL 16)
  FI
  IFZ   al,'t'
        mov   ebx,rw_tlb + (ex_tlb SHL 16)
  FI
  IFZ   al,'m'
        mov   ebx,r_stall + (w_stall SHL 16)
  FI
  IFZ   al,'a'
        mov   ebx,agi_stall + (bank_conf SHL 16)
  FI
  IFZ   al,'b'
        mov   ebx,bus_util + (instrs_ex SHL 16)
  FI            
  
  test  ebx,ebx
  IFNZ          
        sub   eax,eax
        mov   ecx,event_select_msr
        wrmsr
        mov   ecx,event_counter0_msr
        wrmsr
        mov   ecx,event_counter1_msr
        wrmsr
        mov   al,[trace_perf_monitoring_mode]
        IFZ   al,kernel_perf_mon
              mov   al,01b
        ELIFZ al,user_perf_mon
              mov   al,10b
        ELSE_
              mov   al,11b
        FI
        shl   eax,6
        or    ebx,eax
        shl   eax,22-6
        or    eax,ebx
        mov   ecx,event_select_msr
        wrmsr
  FI
  
  ret                            
  
        
                    
  


show_trace_perf_monitoring_mode:

  mov   al,1
  mov   ah,1
  kd____cursor
  
  mov   al,[trace_perf_monitoring_mode]
  
  IFZ   al,no_perf_mon
        kd____disp <'           '>
  ELIFZ al,kernel_user_perf_mon
        kd____disp <'Kernel+User'>
  ELIFZ al,kernel_perf_mon
        kd____disp <'     Kernel'>
  ELSE_
        kd____disp <'       User'>
  FI                  
              
  ret



;--------------------------------------------------------------------------
;
;       display trace performance counters
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   pointer to current trace entry
;       EBP   pointer to reference trace entry
;
;       DS    phys mem
;
;--------------------------------------------------------------------------

  
  

display_trace_performance_counters:

  push  eax
  
  IFNZ  esi,ebp
  
        kd____disp <'P: '>
  
        mov   eax,[esi+trace_entry_perf_count0]
        sub   eax,ds:[ebp+trace_entry_perf_count0]
        kd____outdec
        
        kd____disp <' / '>
        
        mov   eax,[esi+trace_entry_perf_count1]
        sub   eax,ds:[ebp+trace_entry_perf_count1]
        kd____outdec
  
  FI
  
  pop   eax
  ret
  
  
  




;---------------------------------------------------------------------------

default_kdebug_end    equ $



  dcod  ends


  code  ends
  end