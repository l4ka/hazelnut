
intr_sources     equ 16 
 
.erre (intr_sources * 4) LE sizeof intr_cb 

 
scheduler_control_block struc

                           db offset sched_cb dup (?)
 
  intrq_llink              dd intr_sources*2 dup (0)

  highest_active_prio      dd 0
  pulse_counter            dd 0
  if    V4_clock_features
        next_timer_int                dd 0
        next_clock_tick               dd 0
        next_random_sampling_event    dd 0
        next_wakeup_event             dd 0
        next_wakeup_precision         db 0
  else
        system_clock                  dd 0,0
  endif
  soon_wakeup_count        db 0
  late_wakeup_count        db 0 
  late_late_wakeup_count   db 0 
                                                                    ;   |
scheduler_control_block ends                                        ;   |
                                                                    ;   V
scheduler_control_block_size  equ (offset late_late_wakeup_count+1) ;   masm611 bug


 
.erre        scheduler_control_block_size LE (offset sched_cb + sizeof sched_cb)
 
present_root               equ (offset dispatcher_tcb)


ticklength                 equ 2000

clock_tick_precision       equ 5
random_sampling_precision  equ 5 
 
soon_wakeup_interval       equ 1 
late_wakeup_interval       equ 50
late_late_wakeup_interval  equ 1000


