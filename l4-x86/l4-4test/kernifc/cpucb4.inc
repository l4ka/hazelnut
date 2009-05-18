   
uniprocessor     equ off



cpu_control_block struc

                     db offset cpu_cb dup (?)

  cpu_tss_area       dd 0           ; tss_backlink 
  
  cpu_esp0           dd 0           ; tss_esp0 
  cpu_ss0            dw 0,0         ; tss_ss0 
  cpu_cr3            dd 0           ; tss_esp1 
  actual_co1_tcb     dd 0           ; tss_ss1 
  
                     dd 0           ; tss_esp2 
                     dw 0,0         ; tss_ss2 
                     dd 0           ; tss_cr3
                     dd 0           ; tss_eip
                      
  cpu_label          db 8 dup (0)   ; tss_eflags + tss_eax
  cpu_no             db 0           ; tss_ecx 
  cpu_type           db 0
  co1_type           db 0
                     db 0
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

.erre              (offset cpu_iopbm+2+4)  LE  (offset cpu_cb + sizeof cpu_cb)



no87            equ 0
i387            equ 3



;----------------------------------------------------------------------------


  IF    uniprocessor
        extrn fast_switch:near
  ENDIF



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
; Remark: Semantics of 'resources' is described at 'complex_context_switch'.
;
;----------------------------------------------------------------------------



switch_thread macro name,src_tcb

  cmp   [src_tcb+resources],0
  jnz   deallocate_resources_&name

  public switch_thread_&name&_ret
  switch_thread_&name&_ret:

  lea   esi,[ebp+sizeof tcb]
  mov   ds:[cpu_esp0],esi

  mov   [src_tcb+thread_esp],esp
  mov   esp,[ebp+thread_esp]

  endm




switch_thread_fast macro src_tcb

  IF    uniprocessor

        IFDIF <src_tcb>,<edi>
              mov   edi,src_tcb
        ENDIF

        public fast_switch
        fast_switch:
        call  deallocate_resources

        lea   esi,[ebp+sizeof tcb]
        mov   ds:[cpu_esp0],esi

        mov   [edi+thread_esp],esp
        mov   esp,[ebp+thread_esp]

  ELSE
        switch_thread
  ENDIF

  endm


.errnz   sizeof pl0_stack AND 0FFh


;----------------------------------------------------------------------------
; 
;             switch space
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBP   destination task no
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EBP   scratch
;
;       DS,ES,FS,GS         linear space (related to dest task)
;
;----------------------------------------------------------------------------


switch_space macro

  mov   ebp,[(ebp*4)+task_proot-(offset tcb_space SHR (task_no-2))]
  mov   cr3,ebp

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

  or    [reg+resources],mask rscr
  IF    uniprocessor
        mov   byte ptr ss:[fast_switch+PM],call_opc
  ENDIF
  endm



unmrk_ressource macro reg,rscr

  and   [reg+resources],NOT mask rscr
  IF    uniprocessor
        IFZ
              mov   byte ptr ss:[fast_switch+PM],cmp_opc
        FI
  ENDIF
  endm


;----------------------------------------------------------------------------
; de/re allocate INVARIANT:
;
;     thread has no cpu AND resources <> 0  <==>  reallocate pushed on stack
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

  test  [tcb+resources],0FFh
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


lno___prc macro reg,mode

  sub  reg,reg
  endm	


 
 
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
machine_check_exception_bit equ 7
cmpxchg8b_instr_bit         equ 8