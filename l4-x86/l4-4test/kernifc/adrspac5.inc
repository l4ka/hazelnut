                     
;----------------------------------------------------------------------------
;
;       address space description
;
;----------------------------------------------------------------------------


;small_space_size          equ   (32*MB)




; shared by all tasks
; I
; I  per task
; I  I
; I  I


     virtual_space_size    equ  (3*GB)
     
                                
  small_virtual_spaces_size equ (512*MB)               

  tcb_space_size           equ  (256*MB)
  physical_kernel_mem_size equ  (128*MB)

  reserved_space_1_size    equ  ( 92*MB)

  kernel_resident_tab_size equ  ( 12*MB)

     vint_size             equ     32  
     iopbm_size            equ  ( 8*KB-32)  

     reserved_space_2_size equ  (4*MB-(8)*KB)

     ptab_space_size       equ  ( 4*MB)
     com0_space_size       equ  ( 8*MB)
     com1_space_size       equ  ( 8*MB)




    

linear_address_space       struc

     virtual_space         db virtual_space_size  dup (?)

  small_virtual_spaces     db small_virtual_spaces_size dup (?)
   
  tcb_space                db tcb_space_size      dup (?)
  physical_kernel_memory   db physical_kernel_mem_size dup (?) ; dyn allocated !

  reserved_space_1         db reserved_space_1_size dup (?)

  kernel_resident_tables   db kernel_resident_tab_size dup (?) ; dyn allocated !

                           dd vint_size/4         dup (?)
     iopbm                 dd iopbm_size/4        dup (?)

     reserved_space_2      db reserved_space_2_size dup (?)

     ptab_space            db ptab_space_size     dup (?)
     com0_space            db com0_space_size     dup (?)
     com1_space            db com1_space_size     dup (?)



linear_address_space       ends


linear_address_space_size   equ  (offset com1_space+com1_space_size)    ;  masm611 bug

         ;  MOD 2**32  is ok , since only used for segment descriptor construction.
         


max_small_spaces   equ (small_virtual_spaces_size/MB4)



kernel_firewall            equ small_virtual_spaces             ; is small space 0 (never used !)
kernel_firewall_size       equ MB4



;.errnz virtual_space_size AND (small_space_size-1)




auxiliary_address_space    struc

           db   offset ptab_space dup (?); -offset tcb_space dup (?)

           dd   offset ptab_space SHR 12  dup (?)
     pdir  dd                                  ? 
     
auxiliary_address_space    ends



max_physical_memory_size   equ (1*GB)

max_phys_memory struc

  max_physical_memory      db  max_physical_memory_size  dup (?)

max_phys_memory ends


log2_pagesize              equ 12                      ; 4 K pages
pagesize                   equ (1 SHL log2_pagesize)   ;



PM                         equ offset physical_kernel_memory

max_ptabs                  equ (physical_kernel_mem_size / pagesize)


com0_base            equ offset com0_space         ; due to pharlap asm bug
com1_base            equ offset com1_space         ; [pdir+offset cs/MB4] generates

shared_table_base    equ offset small_virtual_spaces
shared_table_size    equ (offset kernel_resident_tables+kernel_resident_tab_size - shared_table_base)



dwords_per_task_proot equ 2


kernel_r_tables struc

                    db offset kernel_resident_tables             dup (?)

  accessed_bitmap   dd   max_physical_memory_size/pagesize/32    dup (?)

  reserved_space_3  db   00BF3000h-4*KB4                         dup (?)

  shadow_pdir       dd   KB4/4                                   dup (?)
  
  io_apic           dd   KB4/4                                   dup (?)
  local_apic        dd   KB4/4                                   dup (?)

  logical_info_page db   KB4                                     dup (?)

                    db   100h                                    dup (?)
  gdt               dd   8*2*4/4                                 dup (?)
                    dd   2*2*4/4                                 dup (?)
                    db   8                                       dup (?)
  idt               dd   (40h-1)*8/4                             dup (?)
                    db   4                                       dup (?)
  cpu_cb            db   128+4                                   dup (?)
  sched_cb          db   16*8+64                                 dup (?)
  intr_cb           db   16*4                                    dup (?)
  pag_cb            db   32                                      dup (?)
                                                                          ;     |
  empty_proot       dd   2                                       dup (?)
  kernel_proot      dd   2                                       dup (?)
  sigma0_proot      dd   2                                       dup (?)
  highest_task      dd   1                                       dup (?)
                    dd   1                                       dup (?) 
  
  small_associated_task  dd max_small_spaces                     dup (?)  ;     |
                                                                          ;     |
kernel_r_tables ends                                                      ;     |
                                                                          ;     V
kernel_r_tables_size equ (offset small_associated_task+2)                 ;  masm611 bug 

.erre         kernel_r_tables_size LE (offset kernel_resident_tables+kernel_resident_tab_size)



             
