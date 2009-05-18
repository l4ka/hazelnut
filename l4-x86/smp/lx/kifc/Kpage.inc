current_kpage_version     equ 3


kpage struc
 
  LN_magic_word     dd   0 
  LN_version_word   dd   0
  LN_label_link     db   0
  kpage_version     db   current_kpage_version
                    db   0,0
  next_kpage_link   dd   0

  init_kdebug       dd   ?
  kdebug_exception  dd   ?
                    dd   ?
  kdebug_end        dd   ?

  sigma0_ktask      dd   ?,?,?,?
  sigma1_ktask      dd   ?,?,?,?
  booter_ktask      dd   ?,?,?,?
  
  ptabs_per_4M      db   ?
  pnodes_per_frame  db   ? 
                    db   ?,?
                    
                    dd   ?
                    
  kdebug_pages      db   ?
  kdebug_startflags db   ?
  kdebug_start_port dw   ?
  
  kdebug_max_task   db   ?
  kdebug_permissions db  ?
                    db   ?,?

  main_mem          dd   ?,? 
  reserved_mem0     dd   ?,? 
  reserved_mem1     dd   ?,? 

  dedicated_mem0    dd   ?,? 
  dedicated_mem1    dd   ?,? 
  dedicated_mem2    dd   ?,? 
  dedicated_mem3    dd   ?,? 
  dedicated_mem4    dd   ?,? 

  user_clock        dd   0,0
                    dd   0,0
  cpu_clock_freq    dd   0
  bus_clock_freq    dd   0     
                    dd   0,0
  aliased_boot_mem  dd   0,0
  alias_base        dd   0
  start_ebx         dd   0
                                 
  mpfloatstruc	    dd 	 0
  mpconftbl	    dd	 0
		    dd   0
  proc_data	    dd	 0
  
  numb_proc	    db   0
  prc_sema	    db   0
  prc_release	    db   0
		    db   0
  		    dd   0,0,0
  
kpage ends
 
 
 
kpage_mem_regions = (offset dedicated_mem4-offset reserved_mem0)/sizeof mem_descriptor+1

 




ktask_descriptor  struc

  ktask_stack       dd 0
  ktask_start       dd 0
  ktask_begin       dd 0
  ktask_end         dd 0

ktask_descriptor  ends
 
 
mem_descriptor  struc

  mem_begin         dd 0
  mem_end           dd 0

mem_descriptor  ends




kdebug_startup_flags_bits record ksf_free:7,skdebug:1

kdebug_permission_bits record kp_free:2,kp_p:1,kp_i:1,kp_w:1,kp_d:1,kp_r:1,kp_m:1



startup_kdebug            equ (mask skdebug)

kdebug_dump_map_enabled   equ (mask kp_m)
kdebug_dump_regs_enabled  equ (mask kp_r)
kdebug_dump_mem_enabled   equ (mask kp_d)
kdebug_write_enabled      equ (mask kp_w)
kdebug_io_enabled         equ (mask kp_i)
kdebug_protocol_enabled   equ (mask kp_p)


