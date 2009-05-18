include l4pre.inc 


  Copyright GMD, L4.CPUCTR.4, 18,07,96, 62, K
   
 
;*********************************************************************
;******                                                         ******
;******            CPU Controller                               ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  12.03.90    ******
;******                                   modified: 18.07.96    ******
;******                                                         ******
;*********************************************************************
 
								

  public init_cpuctr
  public switch_context
  public tunnel_to
  public deallocate_resources_int
  public deallocate_resources_ipc
	public refresh_reallocate
  public debug_exception_handler
  public detach_num
  public emu_load_dr
  public emu_store_dr
  public cpuctr_rerun_thread
  public machine_check_exception


  extrn switch_thread_ipc_ret:near
  extrn switch_thread_int_ret:near
  extrn define_idt_gate:near
  extrn exception:near



.nolist
include l4const.inc
include uid.inc
include adrspace.inc
include intrifc.inc
include tcb.inc
.list
include cpucb.inc
.nolist
include schedcb.inc
.list


ok_for i486


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
;       init cpu controller
;
;----------------------------------------------------------------------------

  assume ds:codseg

  icode



init_cpuctr:

  IF    uniprocessor

        mov   eax,offset deallocate_resources
        sub   eax,offset fast_switch+5
        mov   dword ptr ds:[fast_switch+1+PM],eax

  ENDIF

  mov   edi,offset cpu_cb
  mov   ecx,sizeof cpu_cb
  mov   al,0
  cld
  rep   stosb



;----------------------------------------------------------------------------
;
;       determine processor type
;
;----------------------------------------------------------------------------



  mov   dword ptr ds:[cpu_label],' 68 '
  mov   dword ptr ds:[cpu_label+4],'    '

  mov   ds:[cpu_feature_flags],0


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
        mov   ds:[cpu_label],'3'
        mov   ds:[cpu_type],i386
  .listmacroall
  ELIFZ eax,<(1 SHL ac_flag)>
  .nolistmacro
        mov   ds:[cpu_label],'4'
        mov   ds:[cpu_type],i486

  ELSE_
 
        mov   eax,1
        cpuid

        mov   cl,ah
        mov   ds:[cpu_type],cl

        and   ah,0Fh
        add   ah,'0'
        mov   ds:[cpu_label],ah
        mov   ah,al
        and   ah,0Fh
        add   ah,'0'
        shr   al,4
        add   al,'A'-1
        mov   word ptr ds:[cpu_label+6],ax

        IFB_  cl,pentium
              btr   edx,enhanced_v86_bit
        FI
        mov   ds:[cpu_feature_flags],edx

  FI




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




  mov   ds:[cpu_no],0

  mov   ds:[cpu_iopbm],offset iopbm - offset cpu_tss_area

  mov   ds:[cpu_ss0],linear_kernel_space

  mov   ax,cpu0_tss
  ltr   ax

  mov   bl,debug_exception
  mov   bh,3 SHL 5
  mov   eax,offset debug_exception_handler+PM
  call  define_idt_gate

  bt    ds:[cpu_feature_flags],machine_check_exception_bit
 cmp eax,eax
  IFC
        mov   bl,machine_check
        mov   bh,0 SHL 5
        mov   eax,offset machine_check_exception+PM
        call  define_idt_gate

        mov   ecx,1
        rdmsr                    ; resets machine check type

                                 db    0Fh,20h,0E0h
      ; mov   eax,cr4
        bts   eax,6              ; enable machine check
      ; mov   cr4,eax
                                 db    0Fh,22h,0E0h
  FI

  call  init_numctr


  mov   al,ds:[cpu_type]
  IFB_  al,i486
        ke    '-486_required'
  FI

  mov   eax,cr0
  bts   eax,wp_bit
  mov   cr0,eax

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

  xor   ebx,ebp
  shr   ebp,task_no
  test  ebx,mask task_no

  IFNZ
        switch_space
  FI

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

  push  eax

  or    [edi+fine_state],nready
  and   [ebp+fine_state],NOT nready

  pop   eax

  mov   esi,ebp
  shr   ebp,task_no

  switch_space

  mov   ebp,esi

  jmp   ecx





;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  kcode


;----------------------------------------------------------------------------
;
;       deallocate resources
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EDI   source tcb
;       EBP   dest tcb
;
;       [EDI+resources]       resources used by actual thread
;
;       DS    linear space
;       SS    linear space PL0
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       resources switched and updated
;
;----------------------------------------------------------------------------
; Semantics of resources:
;
;       Ressources are:       com_space
;                             numeric coprocessor
;                             debug register
;
;----------------------------------------------------------------------------



  align 16

deallocate_resources_tunnel:
  push  offset switch_thread_tunnel_ret+PM
  jmp   short deallocate_resources


deallocate_resources_con:
  mov   edi,ebx
  push  offset switch_thread_con_ret+PM
  jmp   short deallocate_resources


deallocate_resources_int:
  mov   edi,ebx
  push  offset switch_thread_int_ret+PM
  jmp   short deallocate_resources


deallocate_resources_ipc:
  push  offset switch_thread_ipc_ret+PM


deallocate_resources:

  push  eax
  mov   eax,cr0
  or    al,1 SHL ts_bit
  mov   cr0,eax

  and   [edi+resources],NOT mask x87_used
  IFZ
        pop   eax
        ret
  FI

  test  [edi+resources],mask com_used 
  IFNZ
        and   [edi+resources],NOT mask com_used 

        sub   eax,eax
        mov   ds:[pdir+(com0_base SHR 20)],eax
        mov   ds:[pdir+(com0_base SHR 20)+4],eax
        mov   ds:[pdir+(com1_base SHR 20)],eax
        mov   ds:[pdir+(com1_base SHR 20)+4],eax

        mov   eax,ebp
        xor   eax,edi
        test  eax,mask task_no
        IFZ
              mov   eax,cr3
              mov   cr3,eax
        FI
  FI

  test  [edi+resources],mask dr_used
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
  push  offset reallocate_resources_by_popebp_iretd+PM
  push  offset reallocate_resources_by_ret+PM

  push  edi
  mov   edi,esp
  and   edi,-sizeof tcb
  ret






;----------------------------------------------------------------------------
;
;       reallocate resources
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
;       resources reestablished
;
;----------------------------------------------------------------------------



reallocate_resources_by_popebp_iretd:

  call  reallocate_resources

  pop   ebp
  iretd





reallocate_resources_by_ret:

  add   esp,3*4



reallocate_resources:

  push  eax
  push  ebp
  mov   ebp,esp
  and   ebp,-sizeof tcb

  mov   al,[ebp+resources]

  test  al,mask dr_used
  IFNZ
        call  reload_debug_registers
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
;       EBP   tcb addr		    (thread must be existent)
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
	
	test	[ebp+resources],mask dr_used
	CANDNZ

	mov	eax,[ebp+thread_esp]
	CANDNZ <dword ptr ds:[eax]>,<offset reallocate_resources_by_ret>
	
		sub	eax,4*4
		mov	dword ptr ds:[eax],offset reallocate_resources_by_ret
		mov	dword ptr ds:[eax+4],offset reallocate_resources_by_popebp_iretd
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
 
  ipre  debug_ec

  mov   eax,dr7
  test  al,10b
  IFZ
  CANDZ ss:[esp+ip_cs],linear_kernel_space_exec
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
;----------------------------------------------------------------------------


reload_debug_registers:

  push  eax

  mov   eax,dr7
  test  al,10101010b
  IFZ 
        mov   eax,[ebp+thread_dr0+7*4]
        and   al,01010101b
        IFNZ
              mark__ressource ebp,dr_used

              mov   dr7,eax
              mov   eax,[ebp+thread_dr0+0*4]
              mov   dr0,eax
              mov   eax,[ebp+thread_dr0+1*4]
              mov   dr1,eax
              mov   eax,[ebp+thread_dr0+2*4]
              mov   dr2,eax
              mov   eax,[ebp+thread_dr0+3*4]
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
  mov   ecx,[edi+(ecx*4)]

  shr   eax,19-8
  and   eax,7

  CORZ  al,7
  IFBE  al,3
  CANDB ecx,<virtual_space_size>
        mov   [(eax*4)+ebp+thread_dr0],ecx
        call  reload_debug_registers

  ELIFZ al,6
        mov   dr6,ecx
  FI

  mov   al,3

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
        mov   ecx,[ebp+(ecx*4)+thread_dr0]

  FI

  mov   al,ah
  xor   al,7
  and   eax,7
  mov   [edi+(eax*4)],ecx

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
  mov   eax,offset co_not_available_handler+PM 
  call  define_idt_gate

  mov   eax,cr0
  btr   eax,em_bit                ; 387 present
  bts   eax,mp_bit
  mov   cr0,eax
  fninit
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
;       EDX   tcb write addr
;
;       DS    linear space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       no more atachement of numeric devices to this process
;
;----------------------------------------------------------------------------


detach_num:

  IFZ   ds:[actual_co1_tcb],edx

        push  eax

        clts                             ; clts prevents from INT 7 at fnsave
        fnsave [edx+reg_387]
        fwait
        sub   eax,eax
        mov   ds:[actual_co1_tcb],eax

        mov   eax,cr0
        or    al,1 SHL ts_bit
        mov   cr0,eax

        pop   eax
  FI

  ret






;----------------------------------------------------------------------------
;
;       Machine Check Exception
;
;----------------------------------------------------------------------------


machine_check_exception:

  sub   ecx,ecx
  rdmsr
  mov   esi,eax
  mov   edi,edx
  inc   ecx
  rdmsr
  mov   ebx,eax

  DO
        ke    '#MC'
        REPEAT
  OD




  code ends
  end
