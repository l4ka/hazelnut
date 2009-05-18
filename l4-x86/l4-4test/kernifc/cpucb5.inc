   


cpu_control_block struc

                     db offset cpu_cb dup (?)

  cpu_tss_area       dd 0           ; tss_backlink          == IDT 3B  (upper 4 bytes)
  
  cpu_esp0           dd 0           ; tss_esp0              == IDT 3C
  cpu_ss0            dw 0,0         ; tss_ss0 
  cpu_cr3            dd 0           ; tss_esp1              == IDT 3D
  tlb_invalidated    db 0,0,0,0     ; tss_ss1 
  
  actual_co1_tcb     dd 0           ; tss_esp2              == IDT 3E
                     dw 0,0         ; tss_ss2 
                     dd 0           ; tss_cr3               == IDT 3F    ipc3  !
                     dd 0           ; tss_eip
                      
  cpu_label          db 8 dup (0)   ; tss_eflags + tss_eax
  cpu_no             db 0           ; tss_ecx 
  cpu_type           db 0
  co1_type           db 0
                     db 0
  cpu_feature_flags  dd 0           ; tss_edx 
  
  amd_feature_flags  dd 0           ; tss_ebx 
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



mark__ressource macro reg,rscr

  or    [reg+resources],mask rscr
  endm



unmrk_ressource macro reg,rscr

  and   [reg+resources],NOT mask rscr
  endm


;----------------------------------------------------------------------------
; de/re allocate INVARIANT:
;
;     thread has no cpu AND resources <> 0  <==>  reallocate pushed on stack
;
;----------------------------------------------------------------------------





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
phys_addr_extensions_bit    equ 6
machine_check_exception_bit equ 7
cmpxchg8b_instr_bit         equ 8
on_chip_apic_bit            equ 9
mem_type_range_regs_bit     equ 12
global_pages_bit            equ 13
machine_check_arch_bit      equ 14
cmov_extensions_bit         equ 15
mmx_extensions_bit          equ 16


cr4_enable_io_breakpoints_bit   equ 3
cr4_enable_superpages_bit       equ 4
cr4_enable_MC_exception_bit     equ 6
cr4_enable_global_pages_bit     equ 7



amd_syscall_ret_bit         equ 10
amd_int_cmov_bit            equ 15
amd_fp_cmov_bit             equ 16
