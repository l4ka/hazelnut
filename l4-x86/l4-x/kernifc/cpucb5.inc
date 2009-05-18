  


cpu_control_block struc

                     db offset cpu_cb dup (?)

  if    random_sampling OR fast_myself

        sampled_total       dd    0

        sampled             dd    6 dup (0)

        sampl_flags         dd    0

        sampl_block         equ   sampled_total
        sampl_block_len     equ   (8*4+16)

  endif


  cpu_tss_area       dd 0           ; tss_backlink 
  
  cpu_esp0           dd 0           ; tss_esp0 
  cpu_ss0            dw 0,0         ; tss_ss0 
  cpu_cr3            dd 0           ; tss_esp1 
  tlb_invalidated    db 0,0,0,0     ; tss_ss1 
  
  actual_co1_tcb     dd 0           ; tss_esp2 
  if precise_cycles
  last_rdtsc_value   dd 0           ; tss_ss2 
  else
                     dd 0           ; tss_ss2
  endif  
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





cpu___cycles macro old_tcb
 local xx
  if    precise_cycles
  push  eax
  push  edx
  rdtsc
  mov   edx,ds:[last_rdtsc_value]
  mov   ds:[last_rdtsc_value],eax
  sub   eax,edx
  add   [&old_tcb&+cpu_cycles],eax
  jnc   xx
  inc   [&old_tcb&+cpu_cycles+4]
  xx:
  pop   edx
  pop   eax
  endif
endm





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

  cpu___cycles &src_tcb

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

  mov   ebp,[(ebp*8)+task_proot-(offset tcb_space SHR (task_no-3))]
  test  ebp,ebp
  IFNS
        IFNZ  ds:[cpu_cr3],ebp

              mov   ds:[cpu_cr3],ebp
              mov   dword ptr ds:[tlb_invalidated],ebp
              mov   cr3,ebp
        FI
        mov   ebp,00CCF300h
  FI
  mov   ds:[gdt+linear_space/8*8+4],ebp
  add   ebp,0000FB00h-0000F300h
  mov   ds:[gdt+linear_space_exec/8*8+4],ebp

  mov   ebp,linear_space
  mov   ds,ebp
  mov   es,ebp
  mov   fs,ebp
  if    random_sampling OR fast_myself
        mov   ebp,sampling_space
  endif
  mov   gs,ebp
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



