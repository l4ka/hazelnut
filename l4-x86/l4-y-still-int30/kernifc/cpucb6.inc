  


cpu_control_block struc

                     db offset cpu_cb dup (?)

  cpu_tss_area       dd 0           ; tss_backlink 
  
  cpu_esp0           dd 0           ; tss_esp0 
  cpu_ss0            dw 0,0         ; tss_ss0 
  cpu_cr3            dd 0           ; tss_esp1 
  tlb_invalidated    db 0,0,0,0     ; tss_ss1 
  
  actual_co1_tcb     dd 0           ; tss_esp2 
                     dw 0,0         ; tss_ss2 
                     dd 0           ; tss_cr3
                     dd 0           ; tss_eip
                      
  cpu_label          db 8 dup (0)   ; tss_eflags + tss_eax
  cpu_no             db 0           ; tss_ecx 
  cpu_type           db 0
  co1_type           db 0
  cpu_family         db 0
  cpu_feature_flags  dd 0           ; tss_edx 
  
                     dd 0           ; tss_ebx 
                     dd 0           ; tss_esp 
                     dd 0           ; tss_ebp 
                     dd 0           ; tss_esi 
                     
                     dd 0           ; tss_edi 
                     dw 0,0         ; tss_es 
                     dw 0,0         ; tss_cs 
                     dw 0,0         ; tss_ss 
                     
                     dw 0,0         ; tss_ds 
                     dw 0,0         ; tss_fs 
                     dw 0,0         ; tss_gs 
                     dw 0,0         ; tss_ldt 
                     
                     db 0,0         ; tss_tbit 
  cpu_iopbm          dw 0           ; io_map_base 

                     dd 0
         
cpu_control_block ends

cpu_tss_backlink   equ cpu_tss_area

.errnz          sizeof cpu_control_block GT (offset cpu_cb + sizeof cpu_cb)



no87            equ 0
i387            equ 3





;----------------------------------------------------------------------------
; 
;             switch thread
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       src_tcb  actual thread (tcb write addr)
;       EBP      destination thread (tcb write addr)
;
;       interrupts disabled
;
;       [ESP] continuation EIP of actual (source) thread
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EBX,ECX,EDX,EBP      values loaded by source thread
;       EDI                  might be set to src_tcb
;       ESI                  scratch
;
;       DS,ES,FS,GS,SS       unchanged
;
;----------------------------------------------------------------------------
; Remark: Direct context switch to dest thread's stack. Stored PL0 EIP of
;         dest thread ignored. The program execution only moves into the dest
;         thread environment.
;
; Remark: Semantics of 'ressources' is described at 'complex_context_switch'.
;
;----------------------------------------------------------------------------



switch_thread macro name,src_tcb

  cmp   [src_tcb+ressources],0
  jnz   deallocate_ressources_&name

  public switch_thread_&name&_ret
  switch_thread_&name&_ret:

  lea   esi,[ebp+sizeof tcb]
  mov   ds:[cpu_esp0],esi

  mov   [src_tcb+thread_esp],esp
  mov   esp,[ebp+thread_esp]

  endm







;----------------------------------------------------------------------------
; 
;             switch space
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   destination tcb
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX, EDX, EDI   scratch
;
;       DS,ES,FS,GS         linear space (related to dest task)
;
;----------------------------------------------------------------------------


switch_space macro

  mov   eax,[ebp+thread_segment]
  mov   edi,[ebp+thread_segment+4]

  mov   ds:[gdt+linear_space/8*8],eax
  mov   ds:[gdt+linear_space/8*8+4],edi
  add   edi,0000FB00h-0000F300h
  mov   ds:[gdt+linear_space_exec/8*8],eax
  mov   ds:[gdt+linear_space_exec/8*8+4],edi

  IFNS
  mov   edi,[ebp+thread_proot]
  CANDNZ edi,ds:[cpu_cr3]
        mov   ds:[cpu_cr3],edi
        mov   dword ptr ds:[tlb_invalidated],edi
        mov   cr3,edi
  FI
  mov   eax,linear_space
  mov   ds,eax
  mov   es,eax

  endm




;----------------------------------------------------------------------------
;
;       mark / unmark ressource
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       rsrc  resource
;
;       reg   tcb write addr
;
;       DS    linear space
;
;----------------------------------------------------------------------------

call_opc      equ 0E8h
cmp_opc       equ 03Dh



mark__ressource macro reg,rscr

  or    [reg+ressources],mask rscr
  endm



unmrk_ressource macro reg,rscr

  and   [reg+ressources],NOT mask rscr
  endm


unmrk_ressources macro reg,rscr1,rscr2

  and   [reg+ressources],NOT (mask rscr1 + mask rscr2)
  endm



;----------------------------------------------------------------------------
; de/re allocate INVARIANT:
;
;     thread has no cpu AND ressources <> 0  <==>  reallocate pushed on stack
;
;----------------------------------------------------------------------------


;----------------------------------------------------------------------------
;
;       lea esp  (of thread <> me)
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       tcb   tcb write addr <> me !
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reg   esp of thread (ignoring reallocate vec)
;
;----------------------------------------------------------------------------

lea___esp macro reg,tcb

  test  [tcb+ressources],0FFh
  mov   reg,[tcb+thread_esp]
  IFNZ
        add   reg,4*4
  FI
  endm


;----------------------------------------------------------------------------
;
;       lea processor number
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reg   prc number ( 0 / 1 )
;
;----------------------------------------------------------------------------


lno___prc macro reg

  IF   kernel_x2
       str  &reg&hl
       and  reg,11b
  ELSE
       sub  reg,reg
  ENDIF
  endm	




;----------------------------------------------------------------------------
;
;       machine specific registers
;
;----------------------------------------------------------------------------


sysenter_cs_msr             equ 174h
sysenter_esp_msr            equ 175h
sysenter_eip_msr            equ 176h




;----------------------------------------------------------------------------
;
;       cpu features
;
;----------------------------------------------------------------------------


fpu_on_chip_bit             equ 0
enhanced_v86_bit            equ 1
io_breakpoints_bit          equ 2
page_size_extensions_bit    equ 3
time_stamp_counter_bit      equ 4
pentium_style_msrs_bit      equ 5
phys_addr_extensions_bit    equ 6
machine_check_exception_bit equ 7
cmpxchg8b_instr_bit         equ 8
on_chip_apic_bit            equ 9
sysenter_present_bit        equ 11
mem_type_range_regs_bit     equ 12
global_pages_bit            equ 13
machine_check_arch_bit      equ 14
cmov_extensions_bit         equ 15
mmx_extensions_bit          equ 16


cr4_enable_io_breakpoints_bit   equ 3
cr4_enable_superpages_bit       equ 4
cr4_enable_MC_exception_bit     equ 6
cr4_enable_global_pages_bit     equ 7
