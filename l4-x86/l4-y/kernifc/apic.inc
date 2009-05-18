  
  apic_id           equ 0020h
  apic_version      equ 0030h
  
  apic_task_prio    equ 0080h
  apic_arb_prio     equ 0090h
  apic_proc_prio    equ 00A0h
  apic_eoi          equ 00B0h
  apic_remote_read  equ 00C0h
  apic_logical_dest equ 00D0h
  apic_dest_format  equ 00E0h
  apic_svr          equ 00F0h
  
  apic_isr          equ 0100h
  apic_tmr          equ 0180h
  apic_irr          equ 0200h
  
  apic_error_mask   equ 0280h
  
  apic_icr          equ 0300h

  apic_LINT_timer   equ 0320h
  apic_LINT0        equ 0350h
  apic_LINT1        equ 0360h
  apic_LINT_err     equ 0370h
    
  apic_timer_init   equ 0380h
  apic_timer_curr   equ 0390h
  apic_timer_divide equ 03E0h
  
  
  apic_enabled_bit              equ 8
  focus_processor_checking_bit  equ 9
  
  
  
  icr_dest_specified            equ  00b SHL 18
  icr_dest_self                 equ  01b SHL 18
  icr_dest_all_incl_self        equ  10b SHL 18
  icr_dest_all_excl_self        equ  11b SHL 18
  
  icr_trigger_edge              equ   0b SHL 15
  icr_trigger_level             equ   1b SHL 15
  
  icr_level_deassert            equ   0b SHL 14
  icr_level_assert              equ   1b SHL 14
  
  icr_dest_mode_physical        equ   0b SHL 11
  icr_dest_mode_logical         equ   1b SHL 11
  
  icr_delivery_mode_fixed       equ 000b SHL 8
  icr_delivery_mode_lowest_prio equ 001b SHL 8
  icr_delivery_mode_SMI         equ 010b SHL 8
  icr_delivery_mode_remote_read equ 011b SHL 8
  icr_delivery_mode_NMI         equ 100b SHL 8
  icr_delivery_mode_init        equ 101b SHL 8
  icr_delivery_mode_startup     equ 110b SHL 8
  
  
  apic_startup_msg              equ icr_dest_all_excl_self + icr_delivery_mode_startup + icr_level_assert



  io_apic_select_reg            equ 0
  io_apic_window                equ 10h
  
  
  io_apic_redir_table           equ 10h
  
  