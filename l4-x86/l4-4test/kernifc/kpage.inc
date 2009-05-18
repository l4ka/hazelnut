current_kpage_version     equ 1


kpage struc
 
  L4_magic_word     dd   0 
  L4_version_word   dd   0
  L4_label_link     db   0
  kpage_version     db   current_kpage_version
                    db   ?,? 
  next_kpage_link   dd   0

  init_kdebug       dd   ?
  kdebug_exception  dd   ?
                    dd   ?
  kdebug_end        dd   ?

  sigma0_ktask      dd   ?,?,?,?
  sigma1_ktask      dd   ?,?,?,?
  booter_ktask      dd   ?,?,?,?
  sigma2_ktask      dd   ?,?,?,?

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

