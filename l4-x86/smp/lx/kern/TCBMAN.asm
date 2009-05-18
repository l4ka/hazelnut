include l4pre.inc 


  Copyright IBM, L4.TCBMAN, 16,03,99, 9141
 
;*********************************************************************
;******                                                         ******
;******         Thread Control Block Manager                    ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 16.03.99    ******
;******                                                         ******
;*********************************************************************
 

 
  public init_tcbman
  public tcb_fault
  public flush_tcb
  public create_thread
  public delete_thread
  public shutdown_thread
  public create_kernel_thread

  extrn ipcman_open_tcb:near
  extrn ipcman_close_tcb:near
  extrn cancel_if_within_ipc:near
  extrn dispatcher_open_tcb:near
  extrn dispatcher_close_tcb:near
  extrn insert_into_ready_list:near
  extrn insert_into_fresh_frame_pool:near
  extrn detach_coprocessor:near
  extrn dispatch:near
	extrn refresh_reallocate:near
  extrn map_system_shared_page:near
  extrn flush_system_shared_page:near
  extrn exception:near
  extrn init_sndq:near
  extrn define_idt_gate:near
  extrn request_fresh_frame:near
  extrn sigma_1_installed:byte


.nolist 
include l4const.inc
.list
include uid.inc
.nolist
include adrspace.inc
.list
include tcb.inc
.nolist
include cpucb.inc
include intrifc.inc
include pagconst.inc
include pagmac.inc
include schedcb.inc
include syscalls.inc
.list


ok_for x86


  assume ds:codseg




;----------------------------------------------------------------------------
;
;       init tcb manager
;
;----------------------------------------------------------------------------

  icode


init_tcbman:

  ;<cd> Set Interruptgate for l_thread_ex_regs 
  lno___prc eax
  IFZ eax,1
	mov   bh,3 SHL 5
	mov   bl,lthread_ex_regs
	mov   eax,offset lthread_ex_regs_sc
	call  define_idt_gate
  FI

  ;<cd> create dispatcher thread	<pr> dispatcher thread per cpu
  mov   ebp,offset dispatcher_tcb
  call  create_kernel_thread

  ;<cd> create and start kernel booter thread
  mov   ebp,kbooter_tcb
  call  create_kernel_thread
	
  pop   ebx
  mov   esp,[ebp+thread_esp]

  jmp   ebx


  icod  ends


;######################################################################################
	
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

  lno___prc ebx
  dec   ebx
  shl   ebx,task_no

  add   ebx,ebp
	
  CORB  ebx,dispatcher_table				; (ebx = ebp) 
  IFAE  ebx,dispatcher_table+dispatcher_table_size
  
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
              and   [ebp+coarse_state],NOT restarting
        FI


        call  open_tcb
        
  FI        

  ipost

;#####################################################################################

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
;----------------------------------------------------------------------------




open_tcb:

  ;<cd> if thread is not dead and is present
  test  [ebp+coarse_state],ndead                 ; concurrent mapping must
  IFNZ                                           ; not lead to multiple open,
  test  [ebp+list_state],is_present              ;
  CANDZ                                          ; else polling threads would
	;<cd> load task number			 ;
        lno___task edx,ebp                       ; be mult inserted into sndq

	;<cd> open dispatcher tcb
        call  dispatcher_open_tcb
	;<cd> open ipcmanager tcb
        call  ipcman_open_tcb
  FI

  ret


;###############################################################################

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

;##########################################################################




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
;       EBX   preempter	   / FFFFFFFF 
;       ESI   pager          / FFFFFFFF 
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;
;       EAX   EFLAGS
;       ECX   ESP
;       EDX   EIP
;       EBX   preempter
;       ESI   pager
;
;----------------------------------------------------------------------------


lthread_ex_regs_sc:

  tpre  trap2,ds,es

  ;<cd> load tcb of current thread
  mov   ebp,esp
  and   ebp,-sizeof tcb

  push  eax	
  mov   edi,eax

  ;<cd> set dest thread to ebp
  shl   eax,lthread_no
  and   eax,mask lthread_no
  set___lthread ebp,eax

  ;<cd> if (actual processor == 0 (thread to be created)) or (on current processor)
  push  eax
  lno___prc eax
  CORZ [ebp+proc_id],0
  IFZ [ebp+proc_id],eax,long
	pop eax

	;<cd> if thread is dead
	test  [ebp+coarse_state],ndead
	IFZ
		pushad
		;<cd> load tcb address
		mov   ebx,esp
		and   ebx,-sizeof tcb

		;<cd> read creators thread mcp, prio and timeslice information
		mov   al,[ebx+max_controlled_prio]
	        shl   eax,16
		mov   ah,[ebx+prio]
	        mov   al,[ebx+timeslice]

		;<cd> create new thread with creators mcp, prio and timeslice
		call  create_thread

	        popad
	FI

	;<cd> test if autopropagation information should be updated
	bt    edi,ex_regs_update_flag
	IFC
		;<cd> if auto_propagation flag is set
		bt    edi,ex_regs_auto_propagating_flag
		IFC
		;<cd> cand lthread not last in the task
		CANDL eax,max_lthread_no
			;<cd> enable autopropagation
			or    [ebp+coarse_state],auto_propagating
		ELSE_
			;<cd> disable autopropagation
			and   [ebp+coarse_state],NOT auto_propagating
		FI
	FI            

	;<cd> set new stack pointer
	IFNZ  ecx,-1
		xchg  ecx,[ebp+sizeof tcb-sizeof int_pm_stack].ip_esp
	ELSE_
		;<cd> load current stack pointer
		mov   ecx,[ebp+sizeof tcb-sizeof int_pm_stack].ip_esp
	FI

	;<cd> set new eip 
	IFNZ  edx,-1
 
		xchg  edx,[ebp+sizeof tcb-sizeof int_pm_stack].ip_eip
 
		pushad
		;<cd> cancel ongoing ipc
		call  cancel_if_within_ipc
		IFZ   al,running
			;<cd> reset thread if running
			call  reset_running_thread 
		FI 
		popad

	ELSE_
		;<cd> load instruction pointer
		mov   edx,[ebp+sizeof tcb-sizeof int_pm_stack].ip_eip
	FI

	;<cd> set internal preempter
	cmp   ebx,-1
	IFNZ
		xchg  ebx,[ebp+int_preempter]
	ELSE_
		;<cd> load internal preempter
		mov   ebx,[ebp+int_preempter]
	FI

	;<cd> set pager
	IFNZ  esi,-1
		xchg  esi,[ebp+pager]
	ELSE_
		;<cd> load current pager
		mov   esi,[ebp+pager]
	FI

	pop eax
	;<cd> load eflags 
	mov   eax,[ebp+sizeof tcb-sizeof int_pm_stack].ip_eflags
  ELSE_
	pop eax
	pop eax
	
	ke 'lthread ex regs cross processor borders'
	; send ipc with information to proxy thread
	; lthread ex regs from proxy thread


	
  FI
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

  ;<cd> return if thread is not running 
  IFZ   [ebp+fine_state],running
        ret
  FI      
  
  
  pop   ecx

  ;<cd> load offset of interrupt stack on current thread's stack
  lea   esi,[ebp+sizeof tcb-sizeof int_pm_stack-4] 

  ;<cd> store return address in ip_eip
  mov   dword ptr [esi],offset reset_running_thread_ret

  ;<cd> store linear user segment in ip_ds
  mov	dword ptr [esi+4],linear_space

  ;<cd> set thread state to running
  mov   [ebp+fine_state],running
  ;<cd> mark thread ready
  mov   ebx,ebp
  mark__ready ebx

  ;<cd> store threads new stack pointer
  mov   [ebp+thread_esp],esi
;  xor   esi,esp
;  test  esi,mask thread_no
;  xc	z,reset_own_thread

  push  ecx
	;<cd> if resources allocated refresh allocate
	cmp	[ebp+ressources],0
	jnz	refresh_reallocate
	ret




;XHEAD reset_own_thread
;
; xor   esp,esi
; xret





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
;       ESI   pager
;
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       ESI   new thread id
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
  ;<cd> if thread is not on current processor 
  lno___prc edi
  IFNZ [ebp+proc_id],edi
  CANDNZ [ebp+proc_id],0
	;<cd> return with kernel error message
	ke 'killing a thread cross processor boundaries'
	popad
	ret
  FI

  ;<cd> if tcb used and present
  IFNZ  [ebp+coarse_state],unused_tcb
  test  [ebp+list_state],is_present
  CANDNZ
	;<cd> detach coprocessor
        mov   ebp,ebp
        call  detach_coprocessor
	;<cd> ipcman close tcb
        call  ipcman_close_tcb
	;<cd> dispatcher close tcb
        call  dispatcher_close_tcb
  FI

  ;<cd> clear first quater of tcb 
  mov   edi,ebp
  mov   ecx,sizeof tcb/4
  sub   eax,eax
  cld
  rep   stosd

  popad

  ;<cd> save tcb id
  mov   dword ptr [ebp+tcb_id],'BCT'+(new_tcb_version SHL 24)

  ;<cd> set thread state to not dead and not blocked
  mov   [ebp+coarse_state],ndead+nblocked

  ;<cd> initialise send Queue
  call  init_sndq                               ; must be done before (!)
                                                ; myself initiated

  push  eax
  ;<cd> load creators tcb address
  mov   eax,esp
  and   eax,-sizeof tcb

; test  esi,esi
; IFZ
;       mov   esi,[eax+pager]
; FI
	
  ;<cd> set pager
  mov   [ebp+pager],esi

  ;<cd> set clan_depth and compute thread id
  test  ebp,mask lthread_no
  IFNZ
  CANDA ebp,max_kernel_tcb
        mov   bl,[eax+clan_depth]
        mov	[ebp+clan_depth],bl
        mov   esi,[eax+myself]
        and   esi,NOT mask thread_no
  ELSE_
        mov   esi,initial_version+root_chief_no SHL chief_no
  FI
  mov   ebx,ebp
  and   ebx,mask thread_no
  add   esi,ebx
  ;<cd> set thread id
  mov   [ebp+myself],esi

  pop   eax

  ;<cd> set new threads stack pointer
  lea   ebx,[ebp+sizeof pl0_stack-sizeof int_pm_stack-4]
  mov   [ebp+thread_esp],ebx

  ;<cd> set eip, code segment and data segment of new thread
  mov   dword ptr [ebx],offset reset_running_thread_ret
  mov   [ebx+ip_error_code+4],fault
  mov   [ebx+ip_eip+4],edx
  mov   [ebx+ip_cs+4],linear_space_exec
  mov   dword ptr [ebx+ip_ds+4],linear_space

  ;<cd> set priority and timeslice of new thread
  mov   [ebp+prio],ah
  mov   [ebp+timeslice],al
  IFDEF ready_llink
        mov   [ebp+rem_timeslice],al
  ENDIF

  ;<cd> set maximum controlled priority
  shr   eax,16
  mov   [ebp+max_controlled_prio],al

  ;<cd> set flags to pl0 and i_flag
  mov   [ebx+ip_eflags+4],(0 SHL iopl_field)+(1 SHL i_flag)

  ;<cd> if tcb address < -1 (max root tcb) (for all threads)
  IFB_  ebp,-1  ;;; max_root_tcb
	;<cd> set minimal priviledge level for accessing the io address space to pl3 
        or    [ebp+coarse_state],iopl3_right
        or    byte ptr [ebx+ip_eflags+4+1],3 SHL (iopl_field-8)
  FI

  ;<cd> set user level stack pointer and stack segment
  mov   [ebx+ip_esp+4],ecx
  mov   [ebx+ip_ss+4],linear_space

  ;<cd> set threads current processor id
  push  eax
  lno___prc eax
  mov   [ebp+proc_id],eax
  pop   eax

  ;<cd> set thread to running
  mov   [ebp+fine_state],running

  ;<cd> open thread control block
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

  ;<cd> enter kdebug if thread is on different processor 
  push eax
  lno___prc eax
  IFNZ [ebp+proc_id],eax
  CANDNZ [ebp+proc_id],0
	ke 'killing a thread cross processor boundaries'
	pop eax
	popad
	popfd
  FI
  pop eax

  ;<cd> cancel ongoing ipc
  call  cancel_if_within_ipc
  popad

  ;<cd> detach coprocessor
  call  detach_coprocessor

  ;<cd> close tcb (dispatcher)
  call  dispatcher_close_tcb

  push  eax
  push  ebp
  push  ecx
	
  ;<cd> set thread state to unused tcb
  mov   [ebp+coarse_state],unused_tcb
  ;<cd> clear id
  sub   eax,eax
  mov   [ebp+myself],eax

  ;<cd> check if all tcb's of that page are unused
  and   ebp,-pagesize
  mov   al,pagesize/sizeof tcb
  lno___prc ecx
  DO
	;<cd> for all threads of that page check if tcb is unused
	CORZ [ebp+proc_id],0
	IFZ [ebp+proc_id],ecx
		cmp   [ebp+coarse_state],unused_tcb
	        EXITNZ
	ELSE_
		EXIT
	FI
        add   ebp,sizeof tcb
        dec   al
        REPEATNZ
  OD
  ;<cd> if no thread of that page is in use
  IFZ
	;<cd> flush page
        lea   eax,[ebp-1]
        call  flush_system_shared_page
        IFNZ
	      ;<cd> insert into fresh frame pool
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

  ;<cd> load tcb address of current thread
  mov   ebp,esp
  and   ebp,-sizeof tcb

  ;<cd> set thread state to dead
  and   [ebp+coarse_state],NOT ndead

  ;<cd> repeat forever
  DO
	;<cd> setup ipc
        sub   ecx,ecx		; timeout = 0
        mov   esi,ecx		; send to nilthread
        mov   edi,ecx		; receive from anyone
        lea   eax,[ecx-1]	; no send phase
        mov   ebp,ecx		; open receive register messages only
	;<cd> send ipc
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
  mov   eax,(255 SHL 16) + (16 SHL 8) + 10 ;mcp = 255, priority = 16, timeslice 10 
  sub   ecx,ecx
  sub   edx,edx
  sub   esi,esi
  sub   edi,edi
  ;<cd> create Thread 
  call  create_thread
  pop   ecx

  ;<cd> if current thread == dispatcher 
  IFZ   ebp,dispatcher_tcb
	;<cd> set invalid thread id (so no one is able to send to this thread)
        mov   [ebp+myself],-1
  ELSE_
	;<cd> else mark thread ready
        mark__ready ebp
  FI
  ;<cd> set new thread's eip
  mov   ebx,[ebp+thread_esp]
  mov   [ebx],ecx

  ret










        code ends
        end