include l4pre.inc 


  Copyright GMD, L4.TCBMAN, 31,12,96, 200
 
;*********************************************************************
;******                                                         ******
;******         Thread Control Block Manager                    ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  07.03.90    ******
;******                                   modified: 03.07.96    ******
;******                                                         ******
;*********************************************************************
 

 
  public init_tcbman
  public tcb_fault
  public flush_tcb
  public create_kernel_thread
  public create_thread
  public delete_thread
  public shutdown_thread


  extrn ipcman_open_tcb:near
  extrn ipcman_close_tcb:near
  extrn cancel_if_within_ipc:near
  extrn dispatcher_open_tcb:near
  extrn dispatcher_close_tcb:near
  extrn insert_into_ready_list:near
  extrn insert_into_fresh_frame_pool:near
  extrn detach_coprocessor:near
  extrn map_system_shared_page:near
  extrn flush_system_shared_page:near
  extrn refresh_reallocate:near
  extrn exception:near
  extrn init_sndq:near
  extrn define_idt_gate:near
  extrn request_fresh_frame:near


.nolist 
include l4const.inc
.list
include uid.inc
.nolist
include adrspace.inc
.list
include intrifc.inc
include tcb.inc
.nolist
include cpucb.inc
include pagconst.inc
include pagmac.inc
include schedcb.inc
include syscalls.inc
.list


ok_for i486,pentium,ppro,k6


  assume ds:codseg





;----------------------------------------------------------------------------
;
;       init tcb manager
;
;----------------------------------------------------------------------------

  icode


init_tcbman:

  mov   bh,3 SHL 5
  mov   bl,lthread_ex_regs
  mov   eax,offset lthread_ex_regs_sc+PM
  call  define_idt_gate

  mov   ebp,offset dispatcher_tcb
  call  create_kernel_thread

  mov   ebp,kbooter_tcb
  call  create_kernel_thread

  pop   ebx
  mov   esp,[ebp+thread_esp]
  jmp   ebx


  icod  ends


;----------------------------------------------------------------------------
;
;       tcb page fault handler
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   faulting address
;       EDX   = EAX
;
;----------------------------------------------------------------------------


tcb_fault:


  call  request_fresh_frame

  IFC
        ke    'tcb_fail'
  FI

  mov   esi,edx
  call  map_system_shared_page


  mov   ebp,esi
  and   ebp,-sizeof tcb

  mov   ebx,dword ptr [ebp+tcb_id]
  test  ebx,ebx
  IFNZ
        xor   ebx,'BCT'
        ror   ebx,24
        CORB  ebx,40h
        IFA   ebx,new_tcb_version
              ke    'inv_tcb'
        FI
  FI

  mov   al,[ebp+coarse_state]
  test  al,restarting
  IFNZ
  test  al,ndead
  CANDNZ
        call  rerun_thread
        and   [ebp+coarse_state],NOT restarting
  FI


  call  open_tcb


  ipost



;----------------------------------------------------------------------------
;
;       open thread control block
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP     tcb write address
;
;       DS,ES   linear space
;
;       tcb must have valid structure
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       thread opened     (known to prc list, all physical refs updated)
;
;       EAX,EBX,ECX,EDX     scratch
;
;----------------------------------------------------------------------------




open_tcb:

  test  [ebp+coarse_state],ndead                 ; concurrent mapping must
  IFNZ                                           ; not lead to multiple open,
  test  [ebp+list_state],is_present              ;
  CANDZ                                          ; else polling threads would
        mov   edx,ebp                            ; be mult inserted into sndq
        and   edx,-(lthreads*sizeof tcb)
        test__page_writable edx
        IFNC
        mov   ebx,[edx+task_pdir]
        test  ebx,ebx
        CANDNZ
              mov   ecx,[edx+thread_proot]
              mov   al,[edx+small_as]
        ELSE_      
              mov   ebx,ds:[empty_proot]
              mov   ecx,ebx
              mov   al,0
        FI      
        mov   [ebp+task_pdir],ebx
        mov   [ebp+thread_proot],ecx
        mov   [ebp+small_as],al
        
        call  dispatcher_open_tcb
        call  ipcman_open_tcb
  FI

  ret




;----------------------------------------------------------------------------
;
;       flush thread control block
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX      tcb addr (lower bits ignored)
;       DS       linear space
;
;       spv locked
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       thread closed      (removed from prc list, all physical refs invalid)
;                          (numeric coprocessor detached, if necessary)
;
;----------------------------------------------------------------------------



flush_tcb:

  pushad
  pushfd
  cli

  and   eax,-sizeof tcb
  test__page_writable eax
  IFNC
        IFNZ  [eax+coarse_state],unused_tcb
              mov   edx,esp
              xor   edx,eax
              and   edx,-sizeof tcb
              IFZ
                    ke    'tcb_flush_err'
              FI
              test  [eax+list_state],is_present
              IFNZ
                    mov   ebp,eax
                    call  detach_coprocessor
                    call  ipcman_close_tcb
                    call  dispatcher_close_tcb
                    and   [eax+list_state],NOT is_present
              FI
        FI

        call  flush_system_shared_page

  FI

  popfd
  popad
  ret






;----------------------------------------------------------------------------
;
;       lthread exchange registers system call
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;
;       EAX   lthread no
;       ECX   ESP            / FFFFFFFF 
;       EDX   EIP            / FFFFFFFF 
;       EBX   preempter.low
;       EBP   preempter.high / FFFFFFFF 
;       ESI   pager.low
;       EDI   pager.high     / FFFFFFFF 
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;
;       EAX   EFLAGS
;       ECX   ESP
;       EDX   EIP
;       EBX   preempter.low
;       EBP   preempter.high
;       ESI   pager.low
;       EDI   pager.high
;
;----------------------------------------------------------------------------


lthread_ex_regs_sc:

  tpre  trap2,ds,es

  push  ebp

  mov   ebp,esp
  and   ebp,-sizeof tcb

  shl   eax,lthread_no
  and   eax,mask lthread_no
  set___lthread ebp,eax

  test  [ebp+coarse_state],ndead
  IFZ
        pushad

        mov   ebx,esp
        and   ebx,-sizeof tcb

        mov   al,[ebx+max_controlled_prio]
        shl   eax,16
        mov   ah,[ebx+prio]
        mov   al,[ebx+timeslice]

        call  create_thread

        popad
  FI


  IFNZ  ecx,-1
        xchg  ecx,[ebp+kernel_stack_bottom-sizeof int_pm_stack].ip_esp
  ELSE_
        mov   ecx,[ebp+kernel_stack_bottom-sizeof int_pm_stack].ip_esp
  FI

  IFNZ  edx,-1
 
        xchg  edx,[ebp+kernel_stack_bottom-sizeof int_pm_stack].ip_eip
 
        pushad
        call  cancel_if_within_ipc
        IFZ   eax,ready
              call  reset_running_thread 
        FI 
        popad

  ELSE_
        mov   edx,[ebp+kernel_stack_bottom-sizeof int_pm_stack].ip_eip
  FI

  pop   eax

  cmp___eax -1
  IFNZ
        xchg  ebx,[ebp+int_preempter]
        xchg  eax,[ebp+int_preempter+4]
  ELSE_
        mov   ebx,[ebp+int_preempter]
        mov   eax,[ebp+int_preempter+4]
  FI

  IFNZ  edi,-1
        xchg  esi,[ebp+pager]
        xchg  edi,[ebp+pager+4]
  ELSE_
        mov   esi,[ebp+pager]
        mov   edi,[ebp+pager+4]
  FI

  mov   ebp,[ebp+kernel_stack_bottom-sizeof int_pm_stack].ip_eflags

  xchg  eax,ebp


  tpost ,ds,es






;----------------------------------------------------------------------------
;
;       reset running thread 
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb write addr
;
;       bottom state = running 
;
;       interrupts disabled !
;
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       REGs  unchanged
;
;       kernel activities cancelled 
;
;----------------------------------------------------------------------------



reset_running_thread: 


  pop   ecx

  lea   esi,[ebp+kernel_stack_bottom-sizeof int_pm_stack-4] 

  mov   dword ptr [esi],offset reset_running_thread_ret+PM

	IF	kernel_type NE i486

		mov	dword ptr [esi+4],linear_space

	ENDIF

  movi  eax,ready
  mov   [ebp+thread_state],eax
  mark__ready ebp

  mov   [ebp+thread_esp],esi
  xor   esi,esp
  test  esi,mask thread_no
  xc	z,reset_own_thread

  push  ecx
  test  [ebp+resources],NOT is_polled
  jnz   refresh_reallocate
  ret




XHEAD reset_own_thread

	xor	esp,esi
	xret





reset_running_thread_ret:
 
  ipost 






;----------------------------------------------------------------------------
;
;       create (and start) thread
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX      mcp SHL 16 + prio SHL 8 + timeslice
;
;       ECX   initial ESP
;       EDX   initial EIP
;       EBP   tcb address
;       ESI   pager (low)
;       EDI   pager (high)
;
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       ESI   new thread id (low)
;       EDI   new thread id (high)
;
;       thread created and started at PL3 with:
;
;                   EAX...EBP            0
;                   ESP                  initial ESP
;                   EIP                  initial EIP
;                   DS...GS              linear space
;                   CS                   linear space exec
;
;----------------------------------------------------------------------------



create_thread:

  pushad

  IFNZ  [ebp+coarse_state],unused_tcb
  test  [ebp+list_state],is_present
  CANDNZ
        mov   ebp,ebp
        call  detach_coprocessor
        call  ipcman_close_tcb
        call  dispatcher_close_tcb
  FI

  mov   edi,ebp
  mov   ecx,sizeof tcb/4
  sub   eax,eax
  cld
  rep   stosd

  popad

  mov   dword ptr [ebp+tcb_id],'BCT'+(new_tcb_version SHL 24)

  mov   [ebp+coarse_state],ndead


  call  init_sndq                               ; must be done before (!)
                                                ; myself initiated


  push  eax
  mov   eax,esp
  and   eax,-sizeof tcb

; test  esi,esi
; IFZ
;       mov   esi,[eax+pager]
;       mov   edi,[eax+pager+4]
; FI
  mov   [ebp+pager],esi
  mov   [ebp+pager+4],edi

  test  ebp,mask lthread_no
  IFNZ
  CANDA ebp,max_kernel_tcb
        mov   esi,[eax+myself]
        and   esi,mask ver1 + mask ver0
        mov   edi,[eax+chief]
  ELSE_
        movi  esi,initial_version
        movi  edi,root_chief
  FI
  mov   ebx,ebp
  and   ebx,mask task_no+mask lthread_no
  add   esi,ebx
  mov   [ebp+myself],esi
  mov   [ebp+chief],edi

  pop   eax

  lea   ebx,[ebp+kernel_stack_bottom-sizeof int_pm_stack-4]
  mov   [ebp+thread_esp],ebx

  mov   dword ptr [ebx],offset reset_running_thread_ret+PM
  IF    kernel_type NE i486
        mov   [ebx+ip_ds+4],linear_space
  ENDIF      
  mov   [ebx+ip_error_code+4],fault
  mov   [ebx+ip_eip+4],edx
  mov   [ebx+ip_cs+4],linear_space_exec

  mov   [ebp+prio],ah
  mov   [ebp+timeslice],al
  IFDEF ready_llink
        mov   [ebp+rem_timeslice],al
  ENDIF

  shr   eax,16
  mov   [ebp+max_controlled_prio],al

  mov   [ebx+ip_eflags+4],(0 SHL iopl_field)+(1 SHL i_flag)

  IFB_  ebp,-1  ;;; max_root_tcb

        or    [ebp+coarse_state],iopl3_right
        or    byte ptr [ebx+ip_eflags+4+1],3 SHL (iopl_field-8)
  FI
  mov   [ebx+ip_esp+4],ecx
  mov   [ebx+ip_ss+4],linear_space

  mov   [ebp+thread_state],ready

  call  open_tcb

  ret










;----------------------------------------------------------------------------
;
;       delete thread
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb address
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       thread deleted, frame inserted into free frame pool if no tcb left
;
;----------------------------------------------------------------------------


delete_thread:

  pushfd
  cli
 
  pushad
  call  cancel_if_within_ipc
  popad
  
  call  detach_coprocessor
  
  call  dispatcher_close_tcb

  push  eax
  push  ebp

  mov   [ebp+coarse_state],unused_tcb
  sub   eax,eax
  mov   [ebp+myself],eax
  mov   [ebp+chief],eax

  and   ebp,-pagesize
  mov   al,pagesize/sizeof tcb
  DO
        cmp   [ebp+coarse_state],unused_tcb
        EXITNZ
        add   ebp,sizeof tcb
        dec   al
        REPEATNZ
  OD
  IFZ
        lea   eax,[ebp-1]
        call  flush_system_shared_page
        IFNZ
              call  insert_into_fresh_frame_pool
        FI
  FI
 
  pop   eax
  pop   ebp
  popfd
  ret





;----------------------------------------------------------------------------
;
;       shutdown_thread
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESP   kernel stack
;
;----------------------------------------------------------------------------


shutdown_thread:

  mov   ebp,esp
  and   ebp,-sizeof tcb

  and   [ebp+coarse_state],NOT ndead

  DO
        sub   ecx,ecx
        mov   esi,ecx
        mov   edi,ecx
        lea   eax,[ecx-1]
        mov   ebp,ecx
        int   ipc
        REPEAT
  OD



;----------------------------------------------------------------------------
;
;       create kernel thread
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP      tcb address    lthread > 0   (!)
;       ECX      initial EIP
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,ESI,EDI scratch
;
;       thread (thread of kernel task) created and started at kernel_cs:EDI
;
;----------------------------------------------------------------------------




create_kernel_thread:

  push  ecx
  mov   eax,(255 SHL 16) + (16 SHL 8) + 10
  sub   ecx,ecx
  sub   edx,edx
  sub   esi,esi
  sub   edi,edi
  call  create_thread
  pop   ecx

  IFZ   ebp,dispatcher_tcb
        mov   [ebp+myself],-1
  ELSE_
        mark__ready ebp
  FI
  mov   ebx,[ebp+thread_esp]
  mov   [ebx],ecx

  ret










;----------------------------------------------------------------------------
;
;       rerun thread
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb addr
;
;       DS,ES linear space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       thread restarted
;
;----------------------------------------------------------------------------
;
;       align 4
;
;
;
rerun_thread:

        ret

;
; pushad
;
; mov   eax,[ebp+thread_esp]
; test  al,11b
; CORNZ
; sub   eax,edx
; CORB  eax,<sizeof thread_control_block+4*4+16*4>
; IFA   eax,<sizeof pl0_stack>
;       ke    'inv_tcb_rerun'
; FI
;
; lea___ip_bottom ebx,ebp
;
; mov   eax,[ebx+ip_error_code]
;
; IFB_  eax,<hardware_ec SHL 24>
;       lea   ecx,[ebx+ip_error_code]
;       call  ipcman_rerun_thread
; ELSE_
;       mov   ecx,ebx
;       IFZ   eax,trap1
;             sub   [ebx+ip_eip],1
;       ELIFZ eax,trap2
;             sub   [ebx+ip_eip],2
;       ELIFZ eax,trap6
;             sub   [ebx+ip_eip],6
;       ELIFZ eax,trap8
;             sub   [ebx+ip_eip],8
;       FI
;       lea   ecx,[ebx-4]
;       mov   dword ptr [ebx-4],offset rerun_iret
; FI
; mov   eax,ecx
; and   eax,sizeof tcb-1
; add   eax,edx
; mov   [ebp+thread_esp],eax
;
; call  cpuctr_rerun_thread
;
; mov   [esp+2*4],ebp
; popad
; ret
;
;
;
;
;rerun_iret:
;
; ipost
;










        code ends
        end