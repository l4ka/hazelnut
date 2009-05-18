include l4pre.inc 


  Copyright IBM+UKA, L4.PAGCTR, 14,04,00, 516
 
 
;*********************************************************************
;******                                                         ******
;******         Paging Controller                               ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 14.04.00    ******
;******                                                         ******
;*********************************************************************
 
  public enable_paging_mode
  public init_fresh_frame_pool
  public map_page_initially
  public alloc_kernel_pages
  public ptabman_init
  public ptabman_start
  public insert_into_fresh_frame_pool
  public request_fresh_frame
  public map_fresh_ptab
  public map_system_shared_page
  public flush_system_shared_page
  public gen_kernel_including_address_space
  


  extrn alloc_initial_pagmap_pages:near
  extrn define_idt_gate:near
  extrn kernel_ipc:near
  extrn set_proot_for_all_threads:near
  extrn grab_frame:near
  extrn phys_frames:dword
  extrn max_kernel_end:near
  extrn physical_kernel_info_page:dword
  extrn pre_paging_cpu_feature_flags:dword


.nolist 
include l4const.inc
include uid.inc
.list
include adrspace.inc
.nolist
include tcb.inc
include cpucb.inc
include schedcb.inc
include intrifc.inc
include pagconst.inc
include pagmac.inc
include pagcb.inc
include msg.inc
include syscalls.inc
include kpage.inc
.list


ok_for x86,pIII



  assume ds:codseg




;****************************************************************************
;*****                                                                  *****
;*****                                                                  *****
;*****        PAGCTR      INITITIALIZATION                              *****
;*****                                                                  *****
;*****                                                                  *****
;****************************************************************************


 


;----------------------------------------------------------------------------
;
;       enable paging mode
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       DS     phys mem
;       ES     phys mem
;
;       paging disabled
;
;----------------------------------------------------------------------------


  icode


 
enable_paging_mode:

  pushad

  call  grab_frame
  mov   edx,eax

  mov   cr3,eax
  mov   ebx,eax

  mov   edi,eax
  mov   ecx,pagesize/4
  sub   eax,eax
  cld
  rep   stosd

  lea   eax,[ebx+page_present+page_write_permit]
  mov   [ebx+offset ptab_space SHR 20],eax

  sub   eax,eax
  sub   esi,esi
  mov   edi,offset max_kernel_end+pagesize-1
  shr   edi,log2_pagesize
  DO
        mov   cl,page_present+page_write_permit+page_user_permit
        call  map_page_initially
        add   eax,pagesize
        add   esi,pagesize
        dec   edi
        REPEATNZ
  OD
  
  bt    ds:[pre_paging_cpu_feature_flags],page_size_extensions_bit

  IFC        
        mov   eax,cr4
        bts   eax,cr4_enable_superpages_bit
        mov   cr4,eax
  
        mov   edi,[phys_frames]
        add   edi,1024-1
        shr   edi,10
        mov   esi,PM
        sub   eax,eax
        DO
              mov   cl,page_present+page_write_permit+page_user_permit
              call  map_superpage_initially
              add   eax,1024*pagesize
              add   esi,1024*pagesize
              dec   edi
              REPEATNZ
        OD
  
  ELSE_      
       
        mov   edi,[phys_frames]
        mov   esi,PM
        sub   eax,eax
        DO
              mov   cl,page_present+page_write_permit+page_user_permit
              call  map_page_initially
              add   eax,pagesize
              add   esi,pagesize
              dec   edi
              REPEATNZ
        OD
  FI      


  call  alloc_initial_pagmap_pages
  
  mov   esi,offset gdt+first_kernel_sgm
  mov   eax,kernel_r_tables_size-(offset gdt+first_kernel_sgm)
  call  alloc_kernel_pages
  
 ;-------- special try: PWT on gdt page set ------
 ;pushad
 ;mov   edi,cr3
 ;mov   esi,offset gdt+first_kernel_sgm
 ;xpdir ebx,esi
 ;xptab esi,esi
 ;mov   edi,dword ptr [(ebx*4)+edi+PM]
 ;and   edi,-pagesize
 ;or    byte ptr [(esi*4)+edi+PM],page_write_through
 ;popad 
 ;------------------------------------------------  
  

  mov   eax,0FEE00000h
  mov   esi,offset local_apic
  mov   cl,page_present+page_write_permit+page_write_through+page_cache_disable
  call  map_page_initially
  
  mov   eax,0FEC00000h
  mov   esi,offset io_apic
  mov   cl,page_present+page_write_permit+page_write_through+page_cache_disable
  call  map_page_initially        
  
  
  mov   eax,offset physical_kernel_info_page
  IF    kernel_x2
        lno___prc ecx
        test  cl,cl
        IFNZ
              mov   eax,[eax+next_kpage_link]
        FI
  ENDIF            
  mov   esi,offset logical_info_page
  mov   cl,page_present+page_write_permit
  call  map_page_initially

  mov   esi,offset ptab_backlink
  mov   eax,[phys_frames]
  lea   eax,[eax*4]
  call  alloc_kernel_pages


  call  grab_frame

  mov   ebx,eax

  mov   edi,eax
  mov   ecx,pagesize/4
  sub   eax,eax
  cld
  rep   stosd

  lea   eax,[ebx+page_present+page_write_permit]
  mov   [ebx+offset ptab_space SHR 20],eax


  lea   esi,[edx+shared_table_base SHR 20]
  lea   edi,[ebx+shared_table_base SHR 20]
  mov   ecx,shared_table_size SHR 22
  cld
  rep   movsd

  mov   eax,cr0
  bts   eax,31
  mov   cr0,eax

  jmp   $+2

  mov   edi,offset task_root
  lea   eax,[ebx+root_chief_no]
  sub   ecx,ecx
  DO
        mov   [edi],eax
        add   edi,4
        cmp   edi,offset task_root+tasks*4
        REPEATB
  OD
    

  mov   ds:[kernel_proot],edx
  mov   ds:[empty_proot],ebx

  popad
  ret



     
  icod  ends




;----------------------------------------------------------------------------
;
;       alloc kernel pages
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX     area size         (will be rounded upwards to multiple of 4K)
;       ESI     linear address    (only bits 31...12 relevant)
;
;       CR3     physical address of kernel page directory
;
;       DS,ES   phys mem / linear space & kernel task
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       frames grabbed and mapped S/W
;
;----------------------------------------------------------------------------


  icode


alloc_kernel_pages:

  pushad

  mov   edx,cr3

  add   eax,pagesize-1
  shr   eax,12
  DO
        push  eax
        mov   cl,page_present+page_write_permit
        call  grab_frame
        call  map_page_initially
        pop   eax
        add   esi,pagesize
        sub   eax,1
        REPEATA
  OD

  popad
  ret


  icod  ends



;----------------------------------------------------------------------------
;
;       map page initially
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX     physical address  (4K aligned)
;       CL      access attributes (U/S, R/W, P-bit)
;       EDX     kernel proot OR sigma0 proot
;       ESI     linear address    (only bits 31...12 relevant)
;
;
;       DS,ES   phys mem / linear space & kernel task
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EBX     PTE address
;
;       mapped
;
;----------------------------------------------------------------------------


  icode



map_page_initially:

  push  edi
  push  ebp

  sub   ebp,ebp
  IFAE  esp,<offset tcb_space>
        mov   ebp,PM
  FI
  add   edx,ebp

  xpdir edi,esi
  shl   edi,2

  mov   ebx,[edx+edi]
  test  bl,page_present
  IFZ
        push  eax
        push  ecx
        push  edi
        call  grab_frame
        mov   ebx,eax
        lea   edi,[eax+ebp]
        mov   ecx,pagesize/4
        sub   eax,eax
        cld
        rep   stosd
        pop   edi
        pop   ecx
        pop   eax

        mov   bl,cl
        or    bl,page_present+page_write_permit
        mov   [edx+edi],ebx

  FI
  and   ebx,-pagesize

  xptab edi,esi
  lea   ebx,[(edi*4)+ebx]
  add   ebx,ebp

  mov   [ebx],eax
  mov   [ebx],cl

  sub   edx,ebp
  pop   ebp
  pop   edi
  ret





;----------------------------------------------------------------------------
;
;       map superpage (4M) initially
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX     physical address  (4M aligned)
;       CL      access attributes (U/S, R/W, P-bit)
;       EDX     kernel proot
;       ESI     linear address    (only bits 31...22 relevant)
;
;
;       DS,ES   phys mem / linear space & kernel task
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       mapped (always resident)
;
;----------------------------------------------------------------------------


map_superpage_initially:

  push  eax
  push  edi
  
  xpdir edi,esi
  shl   edi,2
  add   edi,edx

  mov   al,cl
  or    al,superpage

  mov   [edi],eax
        
  pop   edi
  pop   eax
  ret








  icod  ends


;****************************************************************************
;*****                                                                  *****
;*****                                                                  *****
;*****        Fresh Frame Pool and PTAB Management                      *****
;*****                                                                  *****
;*****                                                                  *****
;****************************************************************************



;----------------------------------------------------------------------------
;
;       init fresh frame pool
;
;----------------------------------------------------------------------------
;
;       NOTE: fresh frames are always (!) 0-filled
;
;----------------------------------------------------------------------------



initial_fresh_frames equ 48 


  icode


init_fresh_frame_pool:

  sub   eax,eax
  mov   ds:[first_free_fresh_frame],eax
  mov   ds:[free_fresh_frames],eax
 
  mov   ecx,initial_fresh_frames
  DO
        call  grab_frame
        call  insert_into_fresh_frame_pool
        dec   ecx
        REPEATNZ
  OD
  ret


  icod  ends


;----------------------------------------------------------------------------
;
;       insert into fresh frame pool
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   physcial frame address  (bits 0..11 ignored)
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       inserted into ptab pool
;
;       initialized to 0  (all entries except first one)
;       offset 0: link to next frame in pool / 0
;
;----------------------------------------------------------------------------


insert_into_fresh_frame_pool:

  push  eax
  push  ecx
  push  edi
  pushfd

  cli

  and   eax,-pagesize
  lea   edx,[eax+PM]

  xchg  ds:[first_free_fresh_frame],eax
  mov   [edx],eax

  inc   ds:[free_fresh_frames]

  lea   edi,[edx+4]
  mov   ecx,pagesize/4-1
  sub   eax,eax
  cld
  rep   stosd

  popfd
  pop   edi
  pop   ecx
  pop   eax
  ret




;----------------------------------------------------------------------------
;
;       request fresh frame
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       NC:   EAX   fresh frame's physical address
;
;             fresh frame is all 0
;
;       C:    EAX   scratch
;
;             no fresh frame available
;
;----------------------------------------------------------------------------


request_fresh_frame:

  sub   ds:[free_fresh_frames],1
  IFNC
        push  edi

        mov   eax,ds:[first_free_fresh_frame]
        sub   edi,edi
        xchg  edi,dword ptr [eax+PM]
        mov   ds:[first_free_fresh_frame],edi

        pop   edi
        ret

  FI

  inc   ds:[free_fresh_frames]

  ke    '-fframe_underflow'

  stc
  ret




;----------------------------------------------------------------------------
;
;       request fresh ptab
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       NC:   EAX   fresh ptab's physical address
;
;             fresh ptab is all 0
;             corresponding chapter entries are 0
;
;       C:    EAX   scratch
;
;             no fresh ptab available
;
;----------------------------------------------------------------------------


request_fresh_ptab:

  call  request_fresh_frame
  IFNC
        push  esi

        mov   esi,eax
        shr   esi,log2_chaptersize
        add   esi,offset chapter_map

        test__page_present esi
        IFC
              push  eax
              call  request_fresh_frame
              IFNC
                    call  map_system_shared_page
                    IFC
                          call  insert_into_fresh_frame_pool
                          stc
                    FI
              FI
              pop   eax
        FI
        pop   esi
  FI

  ret





;----------------------------------------------------------------------------
;
;       map fresh ptab
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EDX   dest task (kernel if system shared space)
;       ECX   pointer to pdir entry
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       NC:   EAX   fresh ptab's physical address
;
;             fresh ptab is all 0
;             corresponding chapter entries are 0
;             pdir link set
;             ptab marked present, write permitted
;             ptab marked user permitted iff pdir entry corresponds to user space
;
;
;       C:    EAX   scratch
;
;             no fresh ptab available
;
;
;----------------------------------------------------------------------------




map_fresh_ptab:


  push  ebx
  
  push  linear_kernel_space
  pop   es

  mov   ebx,ecx
  and   ebx,pagesize-1

  CORB  ebx,<shared_table_base SHR 20>
  IFAE  ebx,<(shared_table_base+shared_table_size) SHR 20>

        lea   eax,[ecx-PM]
        sub   eax,ebx
        cmp   eax,ds:[empty_proot]
        xc    z,generate_own_pdir
  FI

  IFB_  ebx,<virtual_space_size SHR 20>

        call  request_fresh_ptab
        jc    short map_ptab_exit
        mov   al,page_present+page_write_permit+page_user_permit

  ELSE_

        call  request_fresh_frame           ; kernel ptabs don't (!) get
        jc    short map_ptab_exit           ; associated chapter maps !!
        mov   al,page_present+page_write_permit

        IFAE  ebx,<shared_table_base SHR 20>
        CANDB ebx,<(shared_table_base+shared_table_size) SHR 20>

              add   ebx,ds:[kernel_proot]   ; ptab inserted into kernel
              mov   dword ptr [ebx+PM],eax  ; *and empty* proot !
              and   ebx,pagesize-1          ; Sharing ptabs ensures that later
              add   ebx,ds:[empty_proot]    ; mapped pages (tcbs) are shared
              mov   dword ptr [ebx+PM],eax  ; automatically. This is required
        FI                                  ; to permit switching to empty space !!
  FI

  mov   [ecx],eax

  shr   eax,log2_pagesize
  IFAE  esp,<offset tcb_space>
  CANDB esp,<offset tcb_space+tcb_space_size>
        mov   [(eax*4)+ptab_backlink],ecx
  FI
  shl   eax,log2_pagesize
                          ; NC !


map_ptab_exit:

  pop   ebx
  ret




XHEAD generate_own_pdir

  call  request_fresh_ptab      ; new pdir for task, copy of empty
  jc    map_ptab_exit
  
  and   ecx,pagesize-1
  lea   ecx,[eax+ecx+PM]

  push  ecx
  call  init_pdir

  push  ebp
  mov   ebp,edx
  call  set_proot_for_all_threads
  pop   ebp

  lea___pdir eax,edx
  call  flush_system_shared_page
  pop   ecx
  
  xret  ,long







;----------------------------------------------------------------------------
;
;       init pdir
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   phys addr of pdir, must be all 0 !
;
;----------------------------------------------------------------------------



init_pdir:

  push  ecx
  push  esi
  push  edi

  mov   esi,ds:[empty_proot]                     ; 1. shared tables taken from nil proot
  lea   esi,[esi+PM+(shared_table_base SHR 20)]  ; 2. small ptab link reset
  lea   edi,[eax+PM+(shared_table_base SHR 20)]  ; 
  mov   ecx,(pagesize-(shared_table_base SHR 20))/4   ; ATTENTION:
  cld                                            ;   chapters not marked !!
  rep   movsd                                    ;   (not necessary, better efficiency)

;;sub   ecx,ecx                             ; Remember: even nil proot may have
;;mov   [eax+(com0_base SHR 20)+PM],ecx     ;           temporal com ptab links
;;mov   [eax+(com1_base SHR 20)+PM],ecx     ;

                    ; Attention: pdir mapped as page into itself for fast access.
  mov   ecx,eax
  mov   cl,page_present+page_write_permit
  mov   dword ptr [eax+(offset ptab_space SHR 20)+PM],ecx

  pop   edi
  pop   esi
  pop   ecx
  ret




;****************************************************************************
;*****                                                                  *****
;*****                                                                  *****
;*****        PTAB Manager Thread                                       *****
;*****                                                                  *****
;*****                                                                  *****
;****************************************************************************


  log2  <%physical_kernel_mem_size>


;----------------------------------------------------------------------------
;
;       ptabman init          called before (!) booter is started
;
;----------------------------------------------------------------------------


ptabman_init:

  push  ds
  push  es
  
  DO
        sub   ecx,ecx
        mov   eax,ecx
        mov   ebp,ecx
        lea   edx,[ecx+1]                   ; w0 = 00000001
        mov   ebx,ecx                       ; w1 = 00000000
        mov   esi,sigma0_task

        call  kernel_ipc

        test  al,ipc_error_mask
        CORNZ
        test  al,map_msg
        IFNZ
              sub   esi,esi
              int   thread_switch
              REPEAT
        FI
  OD
  
  pop   es
  pop   ds


  DO
        push  edx

        sub   ecx,ecx
        mov   eax,ecx
        lea   ebp,[ecx+(log2_)*4+map_msg]
        lea   edx,[ecx-2]                   ; w0 = FFFFFFFE
        mov   esi,sigma0_task
        
        push  ds
        push  es
      
        call  kernel_ipc

        pop   es
        pop   ds

        IFZ   al,map_msg
        CANDZ bl,(log2_pagesize*4+fpage_grant)
        xor   ebx,edx
        and   ebx,-pagesize
        CANDZ
              mov   eax,edx
              call  insert_into_fresh_frame_pool
        ELSE_
              ke    'ill_s0_msg'
        FI

        pop   edx
        dec   edx
        REPEATNZ
  OD

  ret




;----------------------------------------------------------------------------
;
;       ptabman thread         continued after (!) booter started
;
;----------------------------------------------------------------------------



ptabman_start:

  DO
        sub   ecx,ecx
        lea   eax,[ecx-1]
        mov   ebp,ecx
        sub   esi,esi
        sub   edi,edi

        call  kernel_ipc

        REPEAT
  OD






;****************************************************************************
;*****                                                                  *****
;*****                                                                  *****
;*****        map/flush special pages                                   *****
;*****                                                                  *****
;*****                                                                  *****
;****************************************************************************





;----------------------------------------------------------------------------
;
;       flush system shared page
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   virtual addr
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       NZ:   was present
;
;             EAX   phys addr + access attributes
;
;       Z:    was not present
;
;             EAX   scratch
;
;
;       flushed in all tasks
;
;----------------------------------------------------------------------------
; Remark: Since the ptabs of all system shared areas are shared itself,
;         flushing in kernel address space (reached by kernel_proot) is
;         sufficient.
;
;----------------------------------------------------------------------------




flush_system_shared_page:


  push  ebx
  push  ecx

  mov   ebx,ds:[kernel_proot]
  xpdir ecx,eax
  mov   ebx,dword ptr [(ecx*4)+ebx+PM]
  test  bl,page_present
  IFNZ
        and   ebx,-pagesize                 ; Note: Since ptab is shared
        xptab ecx,eax                       ; between all pdirs (even empty),
                                            ; page is flushed in the universe
        invlpg [eax]

        sub   eax,eax
        xchg  eax,dword ptr [(ecx*4)+ebx+PM]

        test  eax,eax
  FI

  pop   ecx
  pop   ebx
  ret







;----------------------------------------------------------------------------
;
;       map system shared page
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   physical addr         (only bits 12...31 relevant)
;       ESI   virtual addr          within system shared area !
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       NC:   mapped (present, read/write, supervisor) in kernel space
;
;       C:    required ptab unavailable, not mapped
;
;----------------------------------------------------------------------------
; Remark: Since the ptabs of all system shared areas are shared itself,
;         mapping in kernel address space (reached by kernel_proot) is
;         sufficient.
;
;----------------------------------------------------------------------------




map_system_shared_page:

  push  eax
  push  ecx
  push  edx

  mov   edx,eax

  mov   ecx,ds:[kernel_proot]
  xpdir eax,esi
  lea   ecx,[(eax*4)+ecx+PM]
  mov   eax,[ecx]
  and   eax,-pagesize
  IFZ
        push  edx
        mov   edx,kernel_task
        call  map_fresh_ptab                     ; Note: new ptab with system
        pop   edx
        IFC                                      ; shared area will be shared
              ke    'syspt_unav'                 ; between *all* address spaces

              pop   edx
              pop   ecx
              pop   eax
              ret               ; C !
        FI
  FI

  xptab ecx,esi
  lea   ecx,[(ecx*4)+eax+PM]

  mov   dl,page_present+page_write_permit
  mov   [ecx],edx

  pop   edx
  pop   ecx
  pop   eax
  clc                           ; NC !
  ret









;----------------------------------------------------------------------------
;
;       gen kernel including new address space
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       <EBX+8>     begin of data+code area
;       <EBX+12>    end of data+code area
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX   physical address of new pdir
;
;             new pdir is a copy (!) of empty pdir, complemented
;             by a new ptab (0..4M), which is a copy (!) of the kernel's
;             0..4M ptab.
;
;----------------------------------------------------------------------------

  icode


gen_kernel_including_address_space:

  push  ecx
  push  edx
  push  esi
  push  edi

  call  request_fresh_ptab

  mov   edx,eax

  mov   edi,PM
  mov   esi,ds:[kernel_proot]
  and   esi,-pagesize
  mov   esi,[esi+edi]
  and   esi,-pagesize
  add   esi,edi
  add   edi,eax

  mov   eax,[ebx].ktask_begin
  shr   eax,log2_pagesize
  lea   edi,[eax*4+edi]
  lea   esi,[eax*4+esi]

  mov   ecx,[ebx].ktask_end
  IFA   ecx,<offset max_kernel_end>
        mov   ecx,offset max_kernel_end
  FI
  add   ecx,pagesize-1
  shr   ecx,log2_pagesize
  sub   ecx,eax

  IFA
        DO
              mov   eax,[esi]
              mov   [edi],eax
              add   esi,4
              add   edi,4
              dec   ecx
              REPEATNZ
        OD
  FI

  call  request_fresh_ptab

  call  init_pdir

  lea   ecx,[edx+page_present+page_write_permit+page_user_permit]
  lea   edi,[eax+PM]
  mov   [edi],ecx
  
  shr   ecx,log2_pagesize
  mov   [ecx*4+ptab_backlink],edi

  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  ret


        



  icod ends





  code ends
  end