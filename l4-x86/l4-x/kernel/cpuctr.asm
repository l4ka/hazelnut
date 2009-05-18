include l4pre.inc 
  

  Copyright IBM+UKA, L4.CPUCTR, 22,11,00, 73, K
 
 
;*********************************************************************
;******                                                         ******
;******            CPU Controller                               ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 22.11.00    ******
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
include ktype.inc
include adrspace.inc
include tcb.inc
include pagmac.inc
.list
include cpucb.inc
.nolist
include segs.inc
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

  pushfd
  pop   eax
  mov   ebx,eax
  xor   eax,(1 SHL ac_flag) + (1 SHL id_flag)
  push  eax
  popfd
  pushfd
  pop   eax
  xor   eax,ebx

  test  eax,1 SHL ac_flag
  IFZ
        mov   ds:[pre_paging_cpu_label],'3'
        mov   ds:[pre_paging_cpu_type],i386

  ELIFZ eax,<(1 SHL ac_flag)>

        mov   ds:[pre_paging_cpu_label],'4'
        mov   ds:[pre_paging_cpu_type],i486

  ELSE_
 
        mov   eax,1
        cpuid

        mov   cl,ah
        shl   cl,4
        mov   ch,al
        shr   ch,4
        or    cl,ch
       
        mov   ds:[pre_paging_cpu_type],cl

        and   ah,0Fh
        add   ah,'0'
        mov   ds:[pre_paging_cpu_label],ah
        mov   ah,al
        and   ah,0Fh
        add   ah,'0'
        shr   al,4
        add   al,'A'-1
        mov   word ptr ds:[pre_paging_cpu_label+6],ax

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

  shr   al,4
  CORA  al,p6_family
  IFB_  al,p5_family
        mov   al,other_family
  FI
  mov   ds:[cpu_family],al

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

  add   eax,cpu0_tss
  ltr   ax

  mov   bl,debug_exception
  mov   bh,3 SHL 5
  mov   eax,offset debug_exception_handler
  call  define_idt_gate

  IFAE  ds:[cpu_family],p6_family

        mov   eax,cr4
        bts   eax,cr4_enable_rdpmc
        mov   cr4,eax
  FI

  bt    ds:[cpu_feature_flags],io_breakpoints_bit
  IFC
        mov   eax,cr4
        bts   eax,cr4_enable_io_breakpoints_bit
        mov   cr4,eax
  FI      

  bt    ds:[cpu_feature_flags],machine_check_exception_bit
  IFC
        mov   bl,machine_check
        mov   bh,0 SHL 5
        mov   eax,offset machine_check_exception
        call  define_idt_gate

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


  mov   al,ds:[cpu_type]
  IFB_  al,i486
        ke    '-at least 486 required'
  FI

  mov   eax,cr0
  bts   eax,wp_bit
  mov   cr0,eax
  
  
  lno___prc eax
  test  eax,eax
  IFZ
        call  wait_for_one_second_tick
        rdtsc
        mov   ebx,eax
        call  wait_for_one_second_tick
        rdtsc
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

  mov   bl,irq8_intr
  mov   bh,0 SHL 5
  mov   eax,offset apic_timer_int
  call  define_idt_gate

  IFAE  ds:[cpu_type],ppro
  
        mov   ecx,27      ; apicbase for PentiumPro
        rdmsr
        and   eax,KB4-1
        add   eax,0FEE00000h
        wrmsr
  FI
        
  
  
  mov   ds:[local_apic+apic_timer_divide],1011b               ; divide by 1
  
  lno___prc eax
  test  eax,eax
  IFZ
  
        mov   edi,1000000
        mov   ds:[local_apic+apic_timer_init],edi
        mov   ds:[local_apic+apic_LINT_timer],(1 SHL 17) + irq8_intr

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
    
  
  mov   eax,ds:[logical_info_page+bus_clock_freq]      
  add   eax,500
  mov   ebx,1000
  sub   edx,edx
  div   ebx
  mov   ds:[local_apic+apic_timer_init],eax
  
  
  mov   eax,offset apic_error_handler
  mov   bl,apic_error
  mov   bh,0 SHL 5
  call  define_idt_gate
  
  sub   eax,eax
  mov   ds:[local_apic+apic_error_mask],eax
  add   eax,apic_error
  mov   ds:[local_apic+apic_error],eax

  
  mov   eax,ds:[local_apic+apic_svr]
  or    ah,1
  mov   ds:[local_apic+apic_svr],eax
  
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


  switch_thread con,ebx

  shr   ebp,task_no

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





tunnel_to:

  pop   ecx

  switch_thread tunnel,edi

  or    [edi+fine_state],nready
  and   [ebp+fine_state],NOT nready

  push  ebp

  shr   ebp,task_no

  mov   esi,[(ebp*8)+task_proot-(offset tcb_space SHR (task_no-3))].switch_ptr
  mov   ebp,[(ebp*8)+task_proot-(offset tcb_space SHR (task_no-3))].proot_ptr
  and   ebp,0FFFFFF00h

  IFNZ  ds:[cpu_cr3],ebp

        mov   ds:[cpu_cr3],ebp
        mov   dword ptr ds:[tlb_invalidated],ebp
        mov   cr3,ebp
  FI

  test  esi,esi
  IFNS
        mov   esi,00CCF300h
  FI
  mov   ds:[gdt+linear_space/8*8+4],esi
  add   esi,0000FB00h-0000F300h
  mov   ds:[gdt+linear_space_exec/8*8+4],esi

  pop   ebp
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
  push  offset switch_thread_tunnel_ret
  jmp   short deallocate_ressources


deallocate_ressources_con:
  mov   edi,ebx
  push  offset switch_thread_con_ret
  jmp   short deallocate_ressources


deallocate_ressources_int:
  mov   edi,ebx
  push  offset switch_thread_int_ret
  jmp   short deallocate_ressources


deallocate_ressources_ipc:
  push  offset switch_thread_ipc_ret


deallocate_ressources:

  push  eax

  mov   al,[edi+ressources]

.errnz  mask ts_prot-80h
  test  al,mask ts_prot+mask x87_used
  xc    s,thread_switch_prot,long

  IFNZ
        mov   eax,cr0
        or    al,1 SHL ts_bit
        mov   cr0,eax

        and   [edi+ressources],NOT mask x87_used
        IFZ
              pop   eax
              ret
        FI
  FI

  test  [edi+ressources],mask com_used 
  IFNZ
        sub   eax,eax
        mov   ds:[pdir+(com0_base SHR 20)],eax
        mov   ds:[pdir+(com0_base SHR 20)+4],eax
        mov   ds:[pdir+(com1_base SHR 20)],eax
        mov   ds:[pdir+(com1_base SHR 20)+4],eax

        mov   eax,ebp
        shr   eax,task_no
        mov   eax,[(eax*8)+task_proot-(offset tcb_space SHR (task_no-3))]
        CORA  eax,<physical_kernel_mem_size>
        IFZ   ds:[cpu_cr3],eax

              mov   eax,cr3
              mov   cr3,eax
        FI

        and   [edi+ressources],NOT mask com_used 
        IFZ
              pop   eax
              ret
        FI
  FI

  test  [edi+ressources],mask dr_used+mask in_partner_space
  CORZ
  mov   eax,dr7
  test  al,10101010b
  IFNZ

        pop   eax
        ret

  FI

  mov   eax,dr6
  and   ax,0F00Fh
  or    al,ah
  mov   [edi+thread_dr6],al
  sub   eax,eax
  mov   dr7,eax

  pop   eax
  pop   edi

  pushfd
  push  cs
  push  offset reallocate_ressources_by_popebp_iretd
  push  offset reallocate_ressources_by_ret

  push  edi
  mov   edi,esp
  and   edi,-sizeof tcb
  ret






XHEAD thread_switch_prot

  or    [ebp+ressources],mask ts_prot

  lno___thread eax,ebp
  ke    '/-cs->'

  mov   al,[edi+ressources]
  and   al,NOT mask ts_prot
  xret  nz,long

  pop   eax
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

  call  reallocate_ressources

  pop   ebp
  iretd





reallocate_ressources_by_ret:

  add   esp,3*4



reallocate_ressources:

  push  eax
  push  ebx
  mov   ebp,esp
  and   ebp,-sizeof tcb

  mov   al,[ebp+ressources]

  test  al,mask dr_used
  IFNZ
        push  eax
        call  reload_debug_registers
        mov   al,[ebp+thread_dr6]
        mov   ah,al
        mov   dr6,eax
        pop   eax
  FI

  test  al,mask in_partner_space
  IFNZ
        mov   eax,[ebp+com_partner]
        lno___task eax,eax
        mov   eax,ss:[(eax*8)+task_proot+4]
        and   eax,0FFFFFF00h

        IFNZ  eax,ss:[cpu_cr3]
              mov   ss:[cpu_cr3],eax
              mov   cr3,eax
        FI
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

	mov	eax,esp
	sub	eax,ebp
	IFAE	eax,<sizeof tcb>
	
	test	[ebp+ressources],mask dr_used
	CANDNZ

	mov	eax,[ebp+thread_esp]
	CANDNZ <dword ptr ds:[eax]>,<offset reallocate_ressources_by_ret>
	
		sub	eax,4*4
		mov	dword ptr ds:[eax],offset reallocate_ressources_by_ret
		mov	dword ptr ds:[eax+4],offset reallocate_ressources_by_popebp_iretd
		mov	dword ptr ds:[eax+8],cs
		mov	dword ptr ds:[eax+12],0

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

  mov   eax,dr7
  test  al,10b
  IFZ
  CANDZ [esp+ip_cs],phys_mem_exec
  test  byte ptr ss:[esp+ip_eflags+2],(1 SHL (vm_flag-16)) 
  CANDZ
        bts   [esp+ip_eflags],r_flag         ; ignore DB exc if in kernel
        ipost                                ; and no kernel (global)
  FI                                         ; breakpoint
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

  mov   eax,dr7
  test  al,10101010b
  IFZ 
        mov   eax,ss:[ebp+thread_dr0+7*4]
        and   al,01010101b
        IFNZ
              mark__ressource ebp,dr_used
           
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
              unmrk_ressource ebp,dr_used
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

  mov   cl,ah
  xor   cl,7
  and   ecx,7
  mov   ecx,ss:[edi+(ecx*4)]

  shr   eax,19-8
  and   eax,7

  CORZ  al,7
  IFBE  al,3
  CANDB ecx,<virtual_space_size>
        mov   ss:[(eax*4)+ebp+thread_dr0],ecx
        call  reload_debug_registers

  ELIFZ al,6
        mov   dr6,ecx
  FI

  mov   eax,3

  pop   ecx
  ret



emu_store_dr:

  push  ecx

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

  mov   eax,3

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

  mov   ds:[actual_co1_tcb],0

  mov   al,ds:[co1_type]

  IFZ   al,no87

        mov   eax,cr0
        bts   eax,em_bit
        btr   eax,mp_bit
        mov   cr0,eax

        ret
  FI

  mov   bh,0 SHL 5
  mov   bl,co_not_available
  mov   eax,offset co_not_available_handler 
  call  define_idt_gate

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

  mov   ebp,esp
  and   ebp,-sizeof tcb

  clts
  cmp   ds:[co1_type],no87
  mov   al,co_not_available
  jz    exception

  mov   eax,ds:[actual_co1_tcb]
  IFNZ  eax,ebp

        test  eax,eax
        IFNZ
              fnsave [eax+reg_387]
              fwait
        FI

        IFZ   [ebp+reg_387+8],0        ; word 8 (16 bit) or 16 (32 bit) contains
        CANDZ [ebp+reg_387+16],0       ; either opcode (V86) or CS <> 0 !
              finit
        ELSE_
              frstor [ebp+reg_387]
        FI
        mov   ds:[actual_co1_tcb],ebp
  FI

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

  IFZ   ds:[actual_co1_tcb],ebp

        push  eax

        clts                             ; clts prevents from INT 7 at fnsave
        fnsave [ebp+reg_387]
        fwait
        sub   eax,eax
        mov   ds:[actual_co1_tcb],eax

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

  mov   eax,cr4
  and   al,NOT (1 SHL cr4_enable_MC_exception_bit)    ; disable machine check
  mov   cr4,eax
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