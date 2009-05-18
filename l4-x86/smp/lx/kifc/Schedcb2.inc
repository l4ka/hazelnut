
intr_sources     equ 16 
 
.erre (intr_sources * 4) LE sizeof intr_cb 

 
scheduler_control_block struc

                           db offset sched_cb dup (?)
 
  intrq_llink              dd intr_sources*2 dup (0)

  highest_active_prio      dd 0
  pulse_counter            dd 0
  system_clock_low         dd 0
  system_clock_high        db 0 
  soon_wakeup_count        db 0
  late_wakeup_count        db 0 
  late_late_wakeup_count   db 0 
                                                                    ;   |
scheduler_control_block ends                                        ;   |
                                                                    ;   V
scheduler_control_block_size  equ (offset late_late_wakeup_count+1) ;   masm611 bug


 
.erre        scheduler_control_block_size LE (offset sched_cb + sizeof sched_cb)
 
present_root               equ (offset dispatcher_tcb)	;xxx
 
 
soon_wakeup_interval       equ 1 
late_wakeup_interval       equ 50
late_late_wakeup_interval  equ 1000














