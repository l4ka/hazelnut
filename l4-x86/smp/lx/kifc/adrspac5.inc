           
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
  physical_kernel_mem_size equ  ( 64*MB)
  pnode_space_size         equ  ( 64*MB)

  reserved_space_1_size    equ  ( 88*MB)

  pdir_space_size          equ  (  4*MB)
  kernel_resident_tab_size equ  ( 12*MB)

     vint_size             equ     32  		
     iopbm_size            equ  ( 8*KB-32)  	
     ldt_size              equ  (64*KB)  	; = 16 Pages

     reserved_space_2_size equ  (4*MB-(64+8)*KB)

     ptab_space_size       equ  ( 4*MB)
     com0_space_size       equ  ( 8*MB)
     com1_space_size       equ  ( 8*MB)



     cachelines_per_mailbox	equ 1
     mailbox_size		equ ((32*cachelines_per_mailbox)-4) 
     mailbox_space_size     	equ ((mailbox_size+4) * (1 SHL max_cpu) * (1 SHL max_cpu))	
     proxies			equ 3

		




linear_address_space       struc

  virtual_space            db virtual_space_size  dup (?)				;        0 .. C0000000 (3GB)

  small_virtual_spaces     db small_virtual_spaces_size dup (?)				; C0000000 .. E0000000 (512MB)
   											; C0000000 .. C0400000 Firewall
  tcb_space                db tcb_space_size      dup (?)				; E0000000 .. F0000000 (256MB)
; first 4 tcbs (1st page) is local and not accessible cross processor boundaries

  physical_kernel_memory   db physical_kernel_mem_size dup (?) ; dyn allocated !	; F0000000 .. F4000000 (64MB)
  pnode_space              db pnode_space_size    dup (?) ; dyn allocated !		; F4000000 .. F8000000 (64MB)

  reserved_space_1         db reserved_space_1_size dup (?)				; F8000000 .. FD800000 (88MB)

  pdir_space               db pdir_space_size     dup (?)				; FD800000 .. FDC00000 (4MB)
  kernel_resident_tables   db kernel_resident_tab_size dup (?) ; dyn allocated !	; FDC00000 .. FE800000 (12MB)

                           dd vint_size/4         dup (?)				; FE800000 .. FE800020 (32B)
     iopbm                 dd iopbm_size/4        dup (?)				; FE800020 .. FE802000 (8KB)
     ldt                   dd ldt_size/4          dup (?)				; FE802000 .. FE812000 (64KB)

     reserved_space_2      db reserved_space_2_size dup (?)				; FE812000 .. FEC00000 

     ptab_space            db ptab_space_size     dup (?)				; FEC00000 .. FF000000 (4MB)
     com0_space            db com0_space_size     dup (?)				; FF000000 .. FF800000 (8MB)
     com1_space            db com1_space_size     dup (?)				; FF800000 .. FFFFFFFF (8MB)



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


log2_chaptersize           equ (5+2)                   ; 32 PTEs per chapter
chaptersize                equ (1 SHL log2_chaptersize);
ptes_per_chapter           equ (chaptersize / 4)       ;
chapters_per_page          equ (pagesize / chaptersize);




PM                         equ offset physical_kernel_memory

max_ptabs                  equ (physical_kernel_mem_size / pagesize)


com0_base            equ offset com0_space         ; due to pharlap asm bug
com1_base            equ offset com1_space         ; [pdir+offset cs/MB4] generates

shared_table_base    equ offset small_virtual_spaces
shared_table_size    equ (offset kernel_resident_tables+kernel_resident_tab_size - shared_table_base)



dwords_per_task_proot equ 2

; MP structures:
  gdt_size equ 8*4*2	; 8 entries times 2 double words per entry   ;;;



kernel_r_tables struc

                    db offset kernel_resident_tables             dup (?)	;FDC00000 .. FE000000

  accessed_bitmap   dd   max_physical_memory_size/pagesize/32    dup (?)	; global
  ptab_backlink     dd   max_ptabs                               dup (?)	; sync needed if new ptab is build
  chapter_map       db   max_ptabs*chapters_per_page             dup (?)	; sync needed if new ptab is build

  reserved_space_3  db   00B63000h-6*KB4-KB256                   dup (?) ;REDIR: -KB256 ;

  mailbox	    dd   (mailbox_space_size*proxies)/4		 dup (?)

  reserved_space_3a dd   (KB4 - (mailbox_space_size*proxies))/4	 dup (?)

  shadow_pdir       dd   KB4/4                                   dup (?)	
  
  io_apic           dd   KB4/4                                   dup (?)	; global done
  local_apic        dd   KB4/4                                   dup (?)	; local done

  logical_info_page db   KB4                                     dup (?)	; ?

; local page start
                    db   100h-8                                  dup (?)

  gdt               dd   8*2*4/4         	                 dup (?) ; local: gdt = offset gdt + gdtsize*pnr-1
                    db   4                                       dup (?)

  cpu_cb            db   (128+4)	                         dup (?) ; local
  sched_cb          db   (16*8+64)      	                 dup (?) ; local

  intr_cb           db   16*4                                    dup (?) ; local
 
  reserved_space_3b dd   (3*KB+320)/4			 	 dup (?) 

  reserved_space_3c dd   (704)/4				 dup (?)
; local page end 

  pag_cb            db   32                                      dup (?)
  idt               dd   40h*8/4                                 dup (?)
  task_proot        dd   tasks*dwords_per_task_proot*(1 SHL max_cpu)     dup (?)  ;     |
  proot_end_marker  dd   dwords_per_task_proot*(1 SHL max_cpu)           dup (?)  ;     |
                                                                          ;     |
  redirection_table dd   tasks*tasks 			         dup (?)  ;     |  ;REDIR
                                                                          ;     |
  small_associated_task  dw max_small_spaces                     dup (?)  ;     |																		  ;     |
                                                                          ;     |
kernel_r_tables ends                                                      ;     |
                                                                          ;     V
kernel_r_tables_size equ (offset small_associated_task+2)                 ;  masm611 bug 

.erre         (kernel_r_tables_size - offset kernel_resident_tables) LE kernel_resident_tab_size




special_proots struc

                    db offset task_proot                         dup (?)

  empty_proot       dd (2*(1 SHL max_cpu))				 dup (?)
  kernel_proot      dd (2*(1 SHL max_cpu))				 dup (?)
  sigma0_proot      dd (2*(1 SHL max_cpu))				 dup (?)

special_proots ends


             
.erre         offset pnode_space GE (offset physical_kernel_memory + physical_kernel_mem_size)






