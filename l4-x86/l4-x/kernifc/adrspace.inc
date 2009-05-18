                    
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
  physical_kernel_mem_size equ  ((64+64)*MB)
  pnode_space_size         equ  ( 64*MB)

  reserved_space_1_size    equ  ((88-64-4)*MB)

  kdebug_cached_alias_size equ  (  4*MB)

  pdir_space_size          equ  (  4*MB)
  kernel_resident_tab_size equ  ( 12*MB)

     vint_size             equ     32  
     iopbm_size            equ  ( 8*KB-32)  
     ldt_size              equ  (64*KB)  

     reserved_space_2_size equ  (4*MB-(64+8)*KB)

     ptab_space_size       equ  ( 4*MB)
     com_space_size        equ  ( 8*MB)




    

linear_address_space       struc

     virtual_space         db virtual_space_size  dup (?)

  small_virtual_spaces     db small_virtual_spaces_size dup (?)
   
  tcb_space                db tcb_space_size      dup (?)
  physical_kernel_memory   db physical_kernel_mem_size dup (?) ; dyn allocated !
  pnode_space              db pnode_space_size    dup (?) ; dyn allocated !

  reserved_space_1         db reserved_space_1_size dup (?)

  kdebug_cached_alias_mem  db kdebug_cached_alias_size dup (?)

  pdir_space               db pdir_space_size     dup (?)
  kernel_resident_tables   db kernel_resident_tab_size dup (?) ; dyn allocated !

                           dd vint_size/4         dup (?)
     iopbm                 dd iopbm_size/4        dup (?)
     ldt                   dd ldt_size/4          dup (?)

     reserved_space_2      db reserved_space_2_size dup (?)

     ptab_space            db ptab_space_size     dup (?)
     com0_space            db com_space_size      dup (?)
     com1_space            db com_space_size      dup (?)



linear_address_space       ends


linear_address_space_size   equ  (offset com1_space+com_space_size)    ;  masm611 bug

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

KR                         equ 0

max_ptabs                  equ (physical_kernel_mem_size / pagesize)




log2_chaptersize           equ (5+2)                   ; 32 PTEs per chapter
chaptersize                equ (1 SHL log2_chaptersize);
ptes_per_chapter           equ (chaptersize / 4)       ;
chapters_per_page          equ (pagesize / chaptersize);





com0_base            equ offset com0_space         ; due to pharlap asm bug
com1_base            equ offset com1_space         ; [pdir+offset cs/MB4] generates

shared_table_base    equ offset small_virtual_spaces
shared_table_size    equ (offset kernel_resident_tables+kernel_resident_tab_size - shared_table_base)



dwords_per_task_proot equ 2







kernel_r_tables struc

                    db offset kernel_resident_tables             dup (?)

  accessed_bitmap   dd   max_physical_memory_size/pagesize/32    dup (?)
  ptab_backlink     dd   max_ptabs                               dup (?)
  chapter_map       db   max_ptabs*chapters_per_page             dup (?)

  reserved_space_3  db   00BF3000h-(max_ptabs*(chapters_per_page+4))-4*KB4-KB256 dup (?) ;REDIR: -KB256

  shadow_pdir       dd   KB4/4                                   dup (?)
  
  io_apic           dd   KB4/4                                   dup (?)
  local_apic        dd   KB4/4                                   dup (?)

  logical_info_page db   KB4                                     dup (?)

                    db   100h-8                                  dup (?)
  gdt               dd   8*2*4/4                                 dup (?)
                    db   4                                       dup (?)
  cpu_cb            db   128+4                                   dup (?)
  sched_cb          db   16*8+64                                 dup (?)
  intr_cb           db   16*4                                    dup (?)
  pag_cb            db   32                                      dup (?)
  idt               dd   40h*8/4                                 dup (?)
  task_proot        dd   tasks*dwords_per_task_proot             dup (?)  ;     |
  proot_end_marker  dd   dwords_per_task_proot                   dup (?)  ;     |
                                                                          ;     |
  redirection_table dd   tasks*tasks 				         dup (?)  ;     |  ;REDIR
                                                                          ;     |
  small_associated_task  dw max_small_spaces                     dup (?)  ;     |																		  ;     |
                                                                          ;     |
kernel_r_tables ends                                                      ;     |
                                                                          ;     V
kernel_r_tables_size equ (offset small_associated_task+2)                 ;  masm611 bug 

 dd    (kernel_r_tables_size - offset kernel_resident_tables) 

.erre         (kernel_r_tables_size - offset kernel_resident_tables) LE kernel_resident_tab_size





special_proots struc

                    db offset task_proot                         dup (?)

  empty_proot       dd 0,0
  kernel_proot      dd 0,0
  sigma0_proot      dd 0,0

special_proots ends

             
.erre         offset pnode_space GE (offset physical_kernel_memory + physical_kernel_mem_size)
