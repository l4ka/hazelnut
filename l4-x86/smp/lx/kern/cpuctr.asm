include l4pre.inc 
 

  Copyright IBM+UKA, L4.CPUCTR, 24,08,99, 68, K
 
 
;*********************************************************************
;******                                                         ******
;******            CPU Controller                               ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 24.08.99    ******
;******                                                         ******
;*********************************************************************
 


  public determine_processor_type
  public init_cpuctr
  public switch_context
  public tunnel_to
  public deallocate_ressources_int
  public deallocate_ressources_ipc
  public refresh_reallocate
  public debug_exception_handler
  public detach_coprocessor
  public emu_load_dr
  public emu_store_dr
  public cpuctr_rerun_thread
  public machine_check_exception
  public init_apic
  public apic_millis_per_pulse
  public apic_micros_per_pulse
  public pre_paging_cpu_feature_flags


  extrn switch_thread_ipc_ret:near
  extrn switch_thread_int_ret:near
  extrn define_idt_gate:near
  extrn exception:near
  extrn apic_timer_int:near
  extrn wait_for_one_second_tick:near
  extrn irq8_intr:abs  



.nolist
include l4const.inc
include uid.inc
include adrspace.inc
include tcb.inc
.list
include cpucb.inc
.nolist
include apic.inc
include intrifc.inc
include schedcb.inc
include kpage.inc
.list


ok_for x86


;----------------------------------------------------------------------------
;
;       data
;
;----------------------------------------------------------------------------


pe_bit        equ 0
mp_bit        equ 1
em_bit        equ 2
ts_bit        equ 3

ne_bit        equ 5
wp_bit        equ 16
am_bit        equ 18
nw_bit        equ 29
cd_bit        equ 30
pg_bit        equ 31



;----------------------------------------------------------------------------
;
;       determine processor type
;
;----------------------------------------------------------------------------

  assume ds:codseg

  icode


pre_paging_cpu_label            db 8 dup (0)
pre_paging_cpu_type             db 0
                                db 0,0,0
pre_paging_cpu_feature_flags    dd 0        






determine_processor_type:

  mov   dword ptr ds:[pre_paging_cpu_label],' 68 '
  mov   dword ptr ds:[pre_paging_cpu_label+4],'    '

  mov   ds:[pre_paging_cpu_feature_flags],0

  ;<cd> xor alignment check flag and id flag in Eflags register 
  pushfd
  pop   eax
  mov   ebx,eax
  xor   eax,(1 SHL ac_flag) + (1 SHL id_flag)
  push  eax
  popfd
  pushfd
  pop   eax
  xor   eax,ebx

  ;<cd> if alignment check flag not set
  test  eax,1 SHL ac_flag
  IFZ
	;<cd> set cpu label to 386
        mov   ds:[pre_paging_cpu_label],'3'
        mov   ds:[pre_paging_cpu_type],i386

  ;<cd> else if ****
  ELIFZ eax,<(1 SHL ac_flag)>

        mov   ds:[pre_paging_cpu_label],'4'
        mov   ds:[pre_paging_cpu_type],i486

  ELSE_
  ;<cd> else
	;<cd> read model type with cpuid
        mov   eax,1
        cpuid

	;<cd> set cpu type 
        mov   cl,ah
        shl   cl,4
        mov   ch,al
        shr   ch,4
        or    cl,ch
       
        mov   ds:[pre_paging_cpu_type],cl

	;<cd> set cpu label
        and   ah,0Fh
        add   ah,'0'
        mov   ds:[pre_paging_cpu_label],ah
        mov   ah,al
        and   ah,0Fh
        add   ah,'0'
        shr   al,4
        add   al,'A'-1
        mov   word ptr ds:[pre_paging_cpu_label+6],ax

	;<cd> set cpu feature flags
        IFB_  cl,pentium
              btr   edx,enhanced_v86_bit
        FI
        mov   ds:[pre_paging_cpu_feature_flags],edx

  FI

  ret


  icod ends



;----------------------------------------------------------------------------
;
;       init cpu controller
;
;----------------------------------------------------------------------------

  assume ds:codseg

  icode


init_cpuctr:
		
  ;<cd> clear cpu control block
  mov   edi,offset cpu_cb
  mov   ecx,sizeof cpu_cb
  mov   al,0
  cld
  rep   stosb



;----------------------------------------------------------------------------
;
;       get processor type
;
;----------------------------------------------------------------------------


  mov   eax,dword ptr ds:[pre_paging_cpu_label+PM]
  mov   dword ptr ds:[cpu_label],eax
  mov   eax,dword ptr ds:[pre_paging_cpu_label+4+PM]
  mov   dword ptr ds:[cpu_label+4],eax

  mov   al,ds:[pre_paging_cpu_type+PM]
  mov   ds:[cpu_type],al

  mov   eax,ds:[pre_paging_cpu_feature_flags]
  mov   ds:[cpu_feature_flags],eax



  mov   eax,cr0
  btr   eax,am_bit
  btr   eax,nw_bit
  btr   eax,cd_bit
  mov   cr0,eax


  mov   cl,no87

  fninit
  push  -1
  fnstsw word ptr ss:[esp]
  pop   eax
  IFZ   al,0
  push  eax
  fnstcw word ptr ss:[esp]
  pop   eax
  and   eax,103Fh
  CANDZ eax,3Fh

        mov   cl,i387

  FI
  mov   ds:[co1_type],cl


  lno___prc eax
  
  mov   ds:[cpu_no],al

  mov   ds:[cpu_iopbm],offset iopbm - offset cpu_tss_area

  mov   ds:[cpu_ss0],linear_kernel_space
	
;  add   eax,cpu0_tss		
;  ltr   ax ;;;

  ;<cd> define debug exception handler
  lno___prc eax
  IFZ eax,1		
	mov   bl,debug_exception
	mov   bh,3 SHL 5
	mov   eax,offset debug_exception_handler
	call  define_idt_gate
  FI

  ;<cd> enable io breakpoint 
  bt    ds:[cpu_feature_flags],io_breakpoints_bit
  IFC
        mov   eax,cr4
        bts   eax,cr4_enable_io_breakpoints_bit
        mov   cr4,eax
  FI      

  
  bt    ds:[cpu_feature_flags],machine_check_exception_bit
  IFC
	;<cd> set idt gate for machine check exception
	lno___prc eax
	IFZ eax,1
		mov   bl,machine_check
		mov   bh,0 SHL 5	
		mov   eax,offset machine_check_exception
        
		call  define_idt_gate
	FI

        DO
              mov   ecx,1
              rdmsr                    ; resets machine check type
              test  al,1
              REPEATNZ              
        OD                                 
        
        mov   eax,cr4
        bts   eax,cr4_enable_MC_exception_bit
        ;;;;;;      Thinkpad (755?) bug: HW coninuously raises MC exception 
        ;;;;;;      mov   cr4,eax
  FI


  call  init_numctr

  ;<cd> if cpu type is less than 486 enter kernel debugger with error message
  mov   al,ds:[cpu_type]
  IFB_  al,i486
        ke    '-at least 486 required'
  FI

  ;<cd> set write protect bit (allow pl0 to write to pl3 pages)
  mov   eax,cr0
  bts   eax,wp_bit
  mov   cr0,eax
  
  ;<cd> if bootstrap
  lno___prc eax
  IFZ eax,1
	;<cd> wait for two second ticks and measure time in between
        call  wait_for_one_second_tick
        rdtsc
        mov   ecx,eax
	mov   edi,edx
        call  wait_for_one_second_tick
        rdtsc

	;<cd> compute cpu frequency and store it in kernel info page
        sub   eax,ebx
        mov   ds:[logical_info_page+cpu_clock_freq],eax
  FI      


  ret





;----------------------------------------------------------------------------
;
;       APIC initialization
;
;----------------------------------------------------------------------------


apic_millis_per_pulse     equ 1
apic_micros_per_pulse     equ apic_millis_per_pulse * 1000



init_apic:
  ;<cd> define idt gate for apic timer interrupt
  lno___prc eax
  IFZ eax,1
	mov   bl,irq8_intr
	mov   bh,0 SHL 5
	mov   eax,offset apic_timer_int
	call  define_idt_gate
  FI

  ;<cd> reallocate apicbase on PPros
  IFAE  ds:[cpu_type],ppro
  
        mov   ecx,27      ; apicbase for PentiumPro
        rdmsr
        and   eax,KB4-1
        add   eax,0FEE00000h
        wrmsr
  FI

  ;<cd> reset apic id to processor id
  mov   eax,ds:[local_apic+apic_id]
  and   eax,0F000000h
  lno___prc ebx
  shl   ebx,24
  add   eax,ebx
  
	ke 'eax apic id'
  mov   ds:[local_apic+apic_id],eax
  
  
  ;<cd> set apic timer devide to by 1
  mov   ds:[local_apic+apic_timer_divide],1011b               ; divide by 1

  ;<cd> initialise apic timer 
  mov   edi,1000000
  mov   ds:[local_apic+apic_timer_init],edi
  mov   ds:[local_apic+apic_LINT_timer],(1 SHL 17) + irq8_intr

  lno___prc eax
  IFZ eax,1

	;<cd> compute bus clock frequency and update logical kernel info page
        rdtsc
        mov   ebx,eax

        mov   ecx,10000
        DO
              dec   ecx
              REPEATNZ
        OD

        mov   esi,ds:[local_apic+apic_timer_curr]      
        sub   edi,esi
        rdtsc
        sub   eax,ebx
  
        imul  eax,10
        mov   ebx,edi
        shr   ebx,5
        add   eax,ebx
        sub   edx,edx
        div   edi
  
        mov   ebx,eax
        mov   eax,ds:[logical_info_page+cpu_clock_freq]
        imul  eax,10
        sub   edx,edx
        div   ebx
        mov   ds:[logical_info_page+bus_clock_freq],eax
    
  FI
    
  ;<cd> initialise apic timer interrupt
  mov   eax,ds:[logical_info_page+bus_clock_freq]      
  add   eax,500
  mov   ebx,1000
  sub   edx,edx
  div   ebx
  mov   ds:[local_apic+apic_timer_init],eax

  ;<cd> define IDT gate for apic error handler
  lno___prc eax
  IFZ eax,1
	mov   eax,offset apic_error_handler
	mov   bl,apic_error
	mov   bh,0 SHL 5
	call  define_idt_gate
  FI

  ;<cd> enable any epic errors 
  sub   eax,eax
  mov   ds:[local_apic+apic_error_mask],eax
  add   eax,apic_error
  mov   ds:[local_apic+apic_error],eax

  ;<cd> enable apic
  mov   eax,ds:[local_apic+apic_svr]
  or    ah,1
  mov   ds:[local_apic+apic_svr],eax

  ;<cd> enable timer interrupt
  mov   ds:[local_apic+apic_LINT_timer],(1 SHL 17) + irq8_intr
      
  ret
  


  icod  ends




;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  kcode



;----------------------------------------------------------------------------
;
;       switch context
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   destination thread (tcb write addr)
;       EBX   actual thread (tcb write addr)
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EBX,ECX,ESI         values loaded by source thread
;       EAX,EDX,EDI         scratch
;
;       DS,ES,FS,GS,SS      unchanged
;
;----------------------------------------------------------------------------




  klign 16



switch_context:

  ;<cd> switch thread context
  switch_thread con,ebx

  ;<cd> load task number (tcb shr task_no)
  shr   ebp,task_no

  ;<cd> switch address space
  switch_space

  ret



  kcod  ends

;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||



;----------------------------------------------------------------------------
;
;       tunnel to
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   destination thread (tcb write addr) (must be locked_running!)
;       EDI   actual thread (tcb write addr)      (must be locked_waiting!)
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,EDX         values loaded by source thread
;       ECX,ESI             scratch
;
;----------------------------------------------------------------------------



;#############################################################################

tunnel_to:

  pop   ecx

  switch_thread tunnel,edi

  push  eax

  or    [edi+fine_state],nready
  and   [ebp+fine_state],NOT nready

  pop   eax

  mov   esi,ebp
  shr   ebp,task_no

  push  ds
  switch_space            ; switch space may load user_linear space into ds
  pop   ds
  
  mov   ebp,esi

  jmp   ecx



;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  kcode


;----------------------------------------------------------------------------
;
;       deallocate ressources
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       [EBX+ressources]       ressources used by actual thread
;
;       DS    linear space
;       SS    linear space PL0
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       ressources switched and updated
;
;----------------------------------------------------------------------------
; Semantics of ressources:
;
;       Ressources are:       comX_space extensions (next 4MB areas)
;                             numeric coprocessor
;                             debug register
;                             M4 lock
;                             M4 exec lock
;
;----------------------------------------------------------------------------



  align 16

deallocate_ressources_tunnel:
  ;<cd> push return address and jump to deallocate resources
  push  offset switch_thread_tunnel_ret
  jmp   short deallocate_ressources


deallocate_ressources_con:
  ;<cd> edi = ebx
  mov   edi,ebx
  ;<cd> push return address and jump to deallocate resources	
  push  offset switch_thread_con_ret
  jmp   short deallocate_ressources


deallocate_ressources_int:
  ;<cd> edi = ebx
  mov   edi,ebx
  ;<cd> push return address and jump to deallocate resources	
  push  offset switch_thread_int_ret
  jmp   short deallocate_ressources


deallocate_ressources_ipc:
  ;<cd> push return address and jump to deallocate resources		
  push  offset switch_thread_ipc_ret


deallocate_ressources:

  push  eax

  ;<cd> edi = tcb address of thread

  ;<cd> if numeric coprocessor is used
  test  [edi+ressources],mask x87_used
  IFNZ
	;<cd> set task switch bit to delay save of FPU state until next usage
        mov   eax,cr0
        or    al,1 SHL ts_bit
        mov   cr0,eax
	
	;<cd> set numeric coprocessor to unused
        and   [edi+ressources],NOT mask x87_used
        IFZ
	      ;<cd> return if no other resources are currently in usage
              pop   eax
              ret
        FI
  FI

  ;<cd> if com space used
  test  [edi+ressources],mask com_used 
  IFNZ
	;<cd> clear com_0 and com_1 base in page dir 
        sub   eax,eax
        mov   ds:[pdir+(com0_base SHR 20)],eax
        mov   ds:[pdir+(com0_base SHR 20)+4],eax
        mov   ds:[pdir+(com1_base SHR 20)],eax
        mov   ds:[pdir+(com1_base SHR 20)+4],eax

	;<cd> load task number (tcb shr task_no)
        mov   eax,ebp
        shr   eax,task_no

	; mov   eax,[(eax*8)+task_proot-(offset tcb_space SHR (task_no-3))]
	;<cd> load task_page root entry
	and   eax,0FFh ;taskno in lower bytes
	shl   eax, max_cpu
	push edi
	lno___prc edi
	add   eax, edi
	pop edi
	mov   eax,[(eax*8)+task_proot-8]

	;<cd> (if current page directory above physical kernel memory size) or equal to cpu_cr3
        CORA  eax,<physical_kernel_mem_size>
        IFZ   ds:[cpu_cr3],eax
	      ;<cd> flush tlb
              mov   eax,cr3
              mov   cr3,eax
        FI
	
	;<cd> set resources to com unused
        and   [edi+ressources],NOT mask com_used 
        IFZ
	      ;<cd> return if no other resources are in use
              pop   eax
              ret
        FI
  FI

  ;<cd> if debug registers are unused and no breakpoints enabled
  test  [edi+ressources],mask dr_used
  CORZ
  mov   eax,dr7
  test  al,10101010b
  IFNZ
	;<cd> return
        pop   eax
        ret

  FI

  ;<cd> reset and store dr6
  mov   eax,dr6
  and   ax,0F00Fh
  or    al,ah
  mov   [edi+thread_dr6],al
  ;<cd> clear any user level breakpoints
  sub   eax,eax
  mov   dr7,eax

  pop   eax
  pop   edi

  ;<cd> push return information
  pushfd
  push  cs
  push  offset reallocate_ressources_by_popebp_iretd
  push  offset reallocate_ressources_by_ret

  push  edi
  ;<cd> load tcb address
  mov   edi,esp
  and   edi,-sizeof tcb
  ret






;----------------------------------------------------------------------------
;
;       reallocate ressources
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       REGs scratch
;
;       ressources reestablished
;
;----------------------------------------------------------------------------



reallocate_ressources_by_popebp_iretd:

  ;<cd> reallocate ressources
  call  reallocate_ressources
	
  ;<cd> return from interrupt
  pop   ebp
  iretd





reallocate_ressources_by_ret:

  add   esp,3*4



reallocate_ressources:

  push  eax
  push  ebx
	
  ;<cd> load current tcb
  mov   ebp,esp
  and   ebp,-sizeof tcb

  ;<cd> load resources mask
  mov   al,[ebp+ressources]

  ;<cd> if debg register are used
  test  al,mask dr_used
  IFNZ
	;<cd> reload debug registers
        call  reload_debug_registers
	;<cd> reload dr6
        mov   al,[ebp+thread_dr6]
        mov   ah,al
        mov   dr6,eax

  FI

  pop   ebp
  pop   eax
  ret





  kcod ends

;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||




;----------------------------------------------------------------------------
;
;       refresh reallocate
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb addr	     (thread must be existent)
;       DS    linear space
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reallocate vec reestablished if necessary        
;
;----------------------------------------------------------------------------


refresh_reallocate:

	push	eax

	;<cd> if thread is in kernel mode (esp points inside tcb)
	mov	eax,esp
	sub	eax,ebp
	IFAE	eax,<sizeof tcb>
	
	;<cd> cand debug reisters are used
	test	[ebp+ressources],mask dr_used
	CANDNZ
	;<cd> cand user thread pointer != offset reallocate_ressouces_by_ret
	mov	eax,[ebp+thread_esp]
	CANDNZ <dword ptr ds:[eax]>,<offset reallocate_ressources_by_ret>

		;<cd> push reallocate resources by ret on stack
		sub	eax,4*4
		mov	dword ptr ds:[eax],offset reallocate_ressources_by_ret
		mov	dword ptr ds:[eax+4],offset reallocate_ressources_by_popebp_iretd
		mov	dword ptr ds:[eax+8],cs
		mov	dword ptr ds:[eax+12],0

		;<cd> reload stack pointer
		mov	[ebp+thread_esp],eax
	FI

	
	pop	eax
	ret







;----------------------------------------------------------------------------
;
;       cpuctr rerun thread  (called when rerunning a thread <> me)
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBX   tcb addr
;       DS    linear space
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reallocate vec reestablished if necessary
;
;----------------------------------------------------------------------------


cpuctr_rerun_thread:

  ret



 
;*********************************************************************
;******                                                         ******
;******       Debug Register Handling                           ******
;******                                                         ******
;*********************************************************************


;----------------------------------------------------------------------------
;
;       debug exception handler
;
;----------------------------------------------------------------------------
 
 
debug_exception_handler:
 
  ipre  debug_ec,no_load_ds

  ;<cd> if bit 2 of dr7 is clear and comes from kernel code and vm flag is clear
  mov   eax,dr7
  test  al,10b
  IFZ
  CANDZ [esp+ip_cs],phys_mem_exec
  test  byte ptr ss:[esp+ip_eflags+2],(1 SHL (vm_flag-16)) 
  CANDZ
	;<cd> ignore debug exception 
        bts   [esp+ip_eflags],r_flag         ; ignore DB exc if in kernel
        ipost                                ; and no kernel (global)
  FI                                         ; breakpoint
  ;<cd> else enter kernel debugger
  mov   al,debug_exception
  jmp   exception




;----------------------------------------------------------------------------
;
;       reload debug register from tcb
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb write addr
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       DR0..3, DR7 reloaded
;
;       EAX,ECX     scratch
;
;----------------------------------------------------------------------------


reload_debug_registers:

  push  eax
	
  ;<cd> if debug register 7 is not used
  mov   eax,dr7
  test  al,10101010b
  IFZ
	;<cd> if any debug register were used previously by this thread
        mov   eax,ss:[ebp+thread_dr0+7*4]
        and   al,01010101b
        IFNZ
	      ;<cd> mark ressource debug register used
              mark__ressource ebp,dr_used

	      ;<cd> load all debug registers
              mov   dr7,eax
              mov   eax,ss:[ebp+thread_dr0+0*4]
              mov   dr0,eax
              mov   eax,ss:[ebp+thread_dr0+1*4]
              mov   dr1,eax
              mov   eax,ss:[ebp+thread_dr0+2*4]
              mov   dr2,eax
              mov   eax,ss:[ebp+thread_dr0+3*4]
              mov   dr3,eax
        ELSE_
	      ;<cd> unmark ressource debug register used
              unmrk_ressource ebp,dr_used
	      ;<cd> clear dr7
              sub   eax,eax
              mov   dr7,eax
        FI
  FI


  pop   eax
  ret






;----------------------------------------------------------------------------
;
;       emulate load/store debug register
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   instruction SHR 8
;       EBP   tcb write addr
;       EDI   REG addr
;
;       DS    linear space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       AL    instruction length
;
;----------------------------------------------------------------------------


emu_load_dr:

  push  ecx

  ;<cd> compute register address in tcb
  mov   cl,ah
  xor   cl,7
  and   ecx,7
  mov   ecx,ss:[edi+(ecx*4)]

  shr   eax,19-8
  and   eax,7

  ;<cd> load debug register
  CORZ  al,7
  IFBE  al,3
  CANDB ecx,<virtual_space_size>
        mov   ss:[(eax*4)+ebp+thread_dr0],ecx
        call  reload_debug_registers

  ELIFZ al,6
        mov   dr6,ecx
  FI

  mov   al,3

  pop   ecx
  ret



emu_store_dr:

  push  ecx

  ;<cd> store debug register
  mov   ecx,eax
  shr   ecx,19-8
  and   ecx,7

  IFZ   cl,6
        mov   ecx,dr6
  ELSE_
        mov   ecx,ss:[ebp+(ecx*4)+thread_dr0]

  FI

  mov   al,ah
  xor   al,7
  and   eax,7
  mov   ss:[edi+(eax*4)],ecx

  mov   al,3

  pop   ecx
  ret







;*********************************************************************
;******                                                         ******
;******       Floating Point Unit Handling                      ******
;******                                                         ******
;*********************************************************************




;----------------------------------------------------------------------------
;
;       init numeric devices and controller
;
;----------------------------------------------------------------------------


  icode



init_numctr:
	
  ;<cd> set actual occupying tcb to 0 	
  mov   ds:[actual_co1_tcb],0

  ;<cd> if no coprocessor available
  mov   al,ds:[co1_type]
  IFZ   al,no87

	;<cd> set emulate coprosessor flag and reset monitor coprocessor flag and return
        mov   eax,cr0
        bts   eax,em_bit
        btr   eax,mp_bit
        mov   cr0,eax

        ret
  FI

  ;<cd> define idt gate for numeric coprocessor not available handler
  lno___prc eax
  IFZ eax,1
	mov   bh,0 SHL 5
	mov   bl,co_not_available
	mov   eax,offset co_not_available_handler 
	call  define_idt_gate
  FI

  ;<cd> reset emulate coprocessor and set monitor coprocessor and task switch flag 
  mov   eax,cr0
  btr   eax,em_bit                ; 387 present
  bts   eax,mp_bit
  bts   eax,ts_bit
  mov   cr0,eax
  ret


  icod  ends



;----------------------------------------------------------------------------
;
;       coprocessor not available handler
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ipre
;
;----------------------------------------------------------------------------
;
;       PROC coprocessor not available
;
;         IF emulator flag set
;           THEN emulate coprocessor instruction
;           ELSE schedule coprocessor
;         FI .
;
;       schedule coprocessor:
;         IF actual coprocessor owner <> me
;           THEN detach coprocessor ;
;                IF first time to use coprocessor by this process
;                  THEN init coprocessor
;                  ELSE attach coprocessor
;                FI
;         FI ;
;         clear task switch .
;
;       ENDPROC coprocessor not available ;
;
;----------------------------------------------------------------------------



co_not_available_handler:

  ipre  fault

  ;<cd> load current tcb
  mov   ebp,esp
  and   ebp,-sizeof tcb

  ;<cd> clear task switch flag (coprocessor will be available for next task)
  clts
  ;<cd> jump to exception if no coprocessor on board
  cmp   ds:[co1_type],no87
  mov   al,co_not_available
  jz    exception

  ;<cd> if last user of coprocessor != current
  mov   eax,ds:[actual_co1_tcb]
  IFNZ  eax,ebp
	;<cd> if last user is valid thread
        test  eax,eax
        IFNZ
	      ;<cd> save floating point registers to last user's tcb
              fnsave [eax+reg_387]
	      ;<cd> wait for floating point unit to finish
              fwait
        FI

	;<cd> if floating point unit was not in use before by this thread
        IFZ   [ebp+reg_387+8],0        ; word 8 (16 bit) or 16 (32 bit) contains
        CANDZ [ebp+reg_387+16],0       ; either opcode (V86) or CS <> 0 !
	      ;<cd> initialise floating point unit
              finit
        ELSE_
	      ;<cd> else reload register contents from tcb
              frstor [ebp+reg_387]
        FI
	;<cd> set current tcb to be the actual user of the floating point unit
        mov   ds:[actual_co1_tcb],ebp
  FI
  ;<cd> mark the floating point unit to be a used ressource of this thread
  mark__ressource ebp,x87_used

  ipost 




;----------------------------------------------------------------------------
;
;       detach numeric devices if necessary
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   tcb write addr
;
;       DS    linear space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       no more atachement of numeric devices to this process
;
;----------------------------------------------------------------------------


detach_coprocessor:

  ;<cd> if current tcb is last user of the FPU
  IFZ   ds:[actual_co1_tcb],ebp

        push  eax

	;<cd> clear task switch flag
        clts                             ; clts prevents from INT 7 at fnsave
	;<cd> save FPU registers to tcb
        fnsave [ebp+reg_387]
	;<cd> wait for FPU to complete
        fwait
	;<cd> set actual FPU user tcb to nil
        sub   eax,eax
        mov   ds:[actual_co1_tcb],eax

	;<cd> set task switch bit
        mov   eax,cr0
        or    al,1 SHL ts_bit
        mov   cr0,eax

        pop   eax
  FI

  ret



;*********************************************************************
;******                                                         ******
;******       APIC Error Handling                               ******
;******                                                         ******
;*********************************************************************



apic_error_handler:

  ;; ke    'apic_error'
  
  iretd





;*********************************************************************
;******                                                         ******
;******       Machine Check Exception                           ******
;******                                                         ******
;*********************************************************************




machine_check_exception:

  ;<cd> disable machine check
  mov   eax,cr4
  and   al,NOT (1 SHL cr4_enable_MC_exception_bit)    ; disable machine check
  mov   cr4,eax
  ;<cd> load model specific register 0 and 1 and enter kernel debugger
  sub   ecx,ecx
  rdmsr
  mov   esi,eax
  mov   edi,edx
  inc   ecx
  rdmsr

  DO
        ke    '#MC'
        REPEAT
  OD




  code ends
  end