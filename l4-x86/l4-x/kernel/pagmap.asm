include l4pre.inc 


  Copyright IBM+UKA, L4.PAGMAP, 25,03,00, 46 
                                                   
 
;*********************************************************************
;******                                                         ******
;******         Page Mapper                                     ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 25.03.00    ******
;******                                                         ******
;*********************************************************************
 

  public alloc_initial_pagmap_pages
  public init_pagmap
  public map_ur_page_initially
  public pagmap_fault
  public grant_fpage
  public map_fpage
  public flush_address_space
  public translate_address


  extrn request_fresh_frame:near
  extrn map_fresh_ptab:near
  extrn alloc_kernel_pages:near
  extrn map_system_shared_page:near
  extrn define_idt_gate:near
  extrn phys_frames:dword
  extrn physical_kernel_info_page:dword
 


.nolist 
include l4const.inc
include uid.inc
include ktype.inc
include adrspace.inc
include tcb.inc
include cpucb.inc
include segs.inc
include intrifc.inc
include schedcb.inc
include syscalls.inc
.list
include pagconst.inc
include pagmac.inc
include pagcb.inc
include kpage.inc
.nolist
include msg.inc
.list





ok_for x86




include pnodes.inc
 


  assume ds:codseg





;****************************************************************************
;*****                                                                  *****
;*****                                                                  *****
;*****        PAGMAP      INITITIALIZATION                              *****
;*****                                                                  *****
;*****                                                                  *****
;****************************************************************************


  icode


;----------------------------------------------------------------------------
;
;       alloc pagmap pages
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       paging still disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       pages for chapter_map and pnode_space allocated
;
;       regs scratch
;
;----------------------------------------------------------------------------



alloc_initial_pagmap_pages:

  mov   esi,offset shadow_pdir
  mov   eax,sizeof shadow_pdir
  call  alloc_kernel_pages
  
  mov   esi,offset chapter_map
  mov   eax,[phys_frames]
  imul  eax,chapters_per_page
  mov   ecx,eax
  sub   eax,pagesize
  IFC
        sub   eax,eax
  FI
  and   eax,-pagesize
  add   esi,eax
  sub   ecx,eax

  mov   eax,ecx
  call  alloc_kernel_pages

  mov   esi,offset pnode_space
  mov   eax,[phys_frames]
  add   eax,max_M4_frames
  shl   eax,log2_size_pnode
  call  alloc_kernel_pages
        
  ret




;----------------------------------------------------------------------------
;
;       init pagmap
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       paging enabled
;
;----------------------------------------------------------------------------



init_pagmap:

  mov   edi,offset shadow_pdir
  mov   ecx,sizeof shadow_pdir/4
  sub   eax,eax
  cld
  rep   stosd
  
  mov   edi,offset chapter_map
  mov   eax,[phys_frames]
  imul  eax,chapters_per_page
  mov   ecx,eax
  sub   eax,pagesize
  IFC
        sub   eax,eax
  FI
  and   eax,-pagesize
  add   edi,eax
  sub   ecx,eax
  sub   eax,eax
  cld
  rep   stosb
  

  mov   eax,[phys_frames+PM]
  shl   eax,log2_size_pnode
  lea   esi,[eax+pnode_base]
  mov   ds:[free_pnode_root],esi
  movzx ecx,ds:[physical_kernel_info_page].pnodes_per_frame
  sub   ecx,1
  IFC
        mov   ecx,pnodes_per_frame_default-1
  FI      
  imul  eax,ecx

  mov   ecx,eax
  mov   edi,esi

  add   esi,pagesize-1
  and   esi,-pagesize
  sub   eax,esi
  add   eax,edi
  call  alloc_kernel_pages

  DO
        add   edi,sizeof pnode
        sub   ecx,sizeof pnode
        EXITBE

        mov   [edi-sizeof pnode].next_free,edi
        REPEAT
  OD
  mov   [edi-sizeof pnode].next_free,0


 
  mov   bh,3 SHL 5

  mov   bl,fpage_unmap
  mov   eax,offset unmap_fpage_sc
  call  define_idt_gate

  ret




  icod  ends


;----------------------------------------------------------------------------
;
;       pagmap fault
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       paging enabled
;
;----------------------------------------------------------------------------



pagmap_fault:

  mov   esi,eax
  call  request_fresh_frame
  IFNC
        call  map_system_shared_page
  ELSE_
        ke    'pmf'
  FI
  
  ipost        





;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
;  kcode




;****************************************************************************
;*****                                                                  *****
;*****                                                                  *****
;*****        flexpage handling                                         *****
;*****                                                                  *****
;*****                                                                  *****
;****************************************************************************



do_source_dest_data struc

  source_pdir         dd 0
  map_mask            dd 0
  operation           dd 0
  source_addr         dd 0

  dest_task           dd 0
  dest_addr           dd 0

do_source_dest_data ends


do_source_data struc

                      dd 0      ; source_pdir
                      dd 0      ; map_mask
                      dd 0      ; operation
                      dd 0      ; source_addr

  tlb_flush_indicator dd 0      ; 0: no tlb flush required, 2: required

do_source_data ends


                


  ;    align 16



;----------------------------------------------------------------------------
;
;       grant fpage
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   source fpage addr
;        CL   log2 (fpage size / pagesize)
;       EDX   dest tcb addr (determines dest address space only)
;       EDI   dest fpage addr
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,ESI,EDI,EBP scratch
;
;----------------------------------------------------------------------------



grant_fpage:

  mov   ch,0FFh                 ; map mask: all

  push  edi
  push  edx
  push  eax
  
  mov   ebp,cr3                 ; granting requires TLB flush on Pentium,
  mov   cr3,ebp                 ; because following addr space switch might
                                ; be executed *without* TLB flush

  push  offset grant_page_
  jmp   short do_fpage





;----------------------------------------------------------------------------
;
;       unmap fpage             special ext: set/reset PCD bits
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   fpage
;       ECX   map mask
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,ESI,EDI,EBP scratch
;
;----------------------------------------------------------------------------


unmap_fpage_sc:

  tpre  trap2,ds,es

  mov   ch,cl
  mov   cl,al
  shr   cl,2
  shr   eax,cl
  shl   eax,cl
  sub   cl,log2_pagesize
  IFNC

        push  offset unmap_fpage_ret

        sub   esp,sizeof do_source_dest_data-sizeof do_source_data

        sub   edx,edx
        push  edx
        push  eax

        test  ecx,ecx
        IFNS
              test  ch,page_write_permit
              IFNZ
                    push  offset unmap_page
                    jmp   short do_fpage
              FI
              push  offset unmap_write_page
              jmp   short do_fpage
        FI

        bt    ecx,30
        IFNC
              test  ch,page_write_permit
              IFNZ
                    push  offset flush_page
                    jmp   short do_fpage
              FI
              push  offset flush_write_page
              jmp   short do_fpage
        FI

              test  ch,page_cache_disable
              IFZ
                    push  offset set_page_cacheable
              ELSE_
                    push  offset set_page_uncacheable
              FI
              jmp   short do_fpage



      unmap_fpage_ret:

        IFNC
              mov   eax,cr3
              mov   cr3,eax
        FI
  FI          

  tpost eax,ds,es


;----------------------------------------------------------------------------
;
;       map fpage
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   source fpage addr
;        CL   log2 (fpage size / pagesize)
;        CH   map mask
;       EDX   dest tcb addr (determines dest address space only)
;       EDI   dest fpage addr
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,ESI,EDI,EBP scratch
;
;----------------------------------------------------------------------------



map_fpage:
        
  push  edi
  push  edx
  push  eax

  push  offset map_page


;----------------------------------------------------------------------------
;
;       do fpage operation
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   source fpage addr
;        CL   log2 (fpage size / pagesize)
;        CH   map mask
;       EDX   0 / dest tcb addr (determines dest address space only)
;       EDI   - / dest fpage addr
;
;        
;       interrupts disabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,ESI,EDI,EBP scratch
;
;       map/grant:
;
;             NC:   all mapping/granting was successful
;             C:    aborted due to requested ptab unavailable
;
;
;       flush/unmap:
;
;             NC:   at least one page unmapped, TLB flush required
;             C:    no page of current AS unmapped, no TLB flush needed
;
;----------------------------------------------------------------------------






do_fpage:

  mov   ebp,cr3
  IFAE  <byte ptr ds:[gdt+linear_space/8*8+7]>,<offset small_virtual_spaces SHR 24>
        lno___task ebp,esp
        load__proot ebp,ebp
  FI
                     
  
  
do_fpage_in_address_space:

  mov   ebx,1
  shl   ebx,cl

  mov   cl,ch
  or    ecx,NOT page_write_permit

  and   ebp,-pagesize

  push  ecx

  mov   ecx,PM
  add   ebp,ecx

  inc   ds:[do_fpage_counter]

  push  ebp


  DO

        cmp   eax,virtual_space_size
        jae   do_fpage_ret

        mov   ebp,esp

        push  eax
        push  edi

        mov   esi,eax
        and   eax,0FFC00000h
        mov   edx,[ebp+source_pdir]
        shr   eax,22-2
        and   esi,003FF000h
        mov   ecx,eax
        add   edx,eax
        shr   esi,log2_pagesize-2
        add   esi,PM

        mov   eax,[edx]


        test  al,superpage
        IFNZ
              test  al,page_present               ; not present 4M pages can only exist in sigma0 device
              xc    z,gen_emulated_4M_page,long   ; mem on machines that do not support 4M pages

              cmp   ebx,MB4/pagesize
              IF____xc ae,do_M4_operation,long
              ELSE__
                    test  ah,shadow_ptab SHR 8
                    mov   eax,[ecx+shadow_pdir]
                    xc    z,lock_shadow_ptab,long
              FI____
        FI             

        test  al,superpage+page_present
        IFPO  ,,long                       ; note: NOT present => NOT M4 rpage
              and   eax,-pagesize
              add   esi,eax

              mov   edx,[ebp+dest_task]
              IFA   edx,2

                    lno___task ecx,edx

                    cmp   edi,virtual_space_size
                    jae   do_fpage_pop_ret

                    load__proot ecx,ecx
                    mov   eax,edi
                    shr   eax,22
                    and   edi,003FF000h
                    lea   ecx,[eax*4+ecx+PM]

                    shr   edi,log2_pagesize-2
                    add   edi,PM

                    mov   eax,[ecx]
                    test  al,superpage+page_present
                    xc    pe,do_fpage_map_fresh_ptab,long
                    and   eax,-pagesize
                    add   edi,eax
              FI

              IFB_  ebx,ptes_per_chapter
  
                    sub   esi,4
                    sub   edi,4
                    DO
                          add   esi,4
                          add   edi,4

                          sub   ebx,1
                          EXITB

                          mov   eax,[esi]
                          test  eax,eax
                          REPEATZ

                          push  ebx
                          call  [ebp+operation]
                          pop   ebx

                          test  ebx,ebx
                          REPEATNZ
                    OD
                    cmp   [ebp+tlb_flush_indicator],1
                    lea   esp,[ebp+sizeof do_source_dest_data]
                    ret                                     
        
              FI              
              
              mov   ch,0

              sub   edi,esi
              shr   esi,log2_chaptersize
              DO
                    mov   cl,[esi+chapter_map-(PM SHR log2_chaptersize)]
                    test  cl,cl
                    IFZ
                          inc   esi
                          sub   ebx,ptes_per_chapter
                          test  ebx,pagesize/4-1
                          REPEATNZ

                          EXIT
                    FI

                    push  ebx
                    shl   esi,log2_chaptersize
                    DO
                          mov   eax,[esi]
                          add   esi,4
                          test  eax,eax
                          REPEATZ

                          sub   esi,4
                          add   edi,esi

                          call  [ebp+operation]

                          sub   edi,esi
                          add   esi,4
                          dec   cl
                          REPEATNZ
                    OD
                    pop   ebx

                    sub   esi,4
                    shr   esi,log2_chaptersize
                    inc   esi
                    sub   ebx,ptes_per_chapter

                    dec   ch
                    xc    z,permit_intr_in_do_fpage

                    test  ebx,pagesize/4-1
                    REPEATNZ
              OD

              add   ebx,MB4/pagesize
        FI

        pop   edi
        pop   eax

        add   edi,MB4
        add   eax,MB4
        sub   ebx,MB4/pagesize
        REPEATA

  OD



do_fpage_ret:

  cmp   [ebp+tlb_flush_indicator],1

  lea   esp,[ebp+sizeof do_source_dest_data]
  ret








XHEAD do_fpage_map_fresh_ptab

  jnz   short do_fpage_pop_ret

  call  map_fresh_ptab
  xret  nc,long



do_fpage_pop_ret:

  pop   edi
  pop   eax
  jmp   do_fpage_ret









XHEAD permit_intr_in_do_fpage

  mov   eax,ds:[do_fpage_counter]
  sti
  nop
  nop
  cli
  cmp   eax,ds:[do_fpage_counter]
  xret  z

  pop   edi
  pop   eax


  and   esi,(pagesize-1) SHR log2_chaptersize
  shl   esi,log2_chaptersize + log2_pagesize

  mov   eax,[ebp+source_addr]
  add   eax,esi
  mov   [ebp+source_addr],eax

  mov   edx,[ebp+dest_task]

  mov   edi,[ebp+dest_addr]
  add   edi,esi
  mov   [ebp+dest_addr],edi

  mov   ebp,[ebp+source_pdir]

  jmp   do_fpage






;  kcod  ends
;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||





XHEAD do_M4_operation


  mov   esi,edx
  mov   edx,[ebp+dest_task]
  IFA   edx,2

        lno___task edx,edx

        cmp   edi,virtual_space_size
        jae   do_fpage_pop_ret

        load__proot edx,edx
        shr   edi,22
        lea   edi,[edi*4+edx+PM]
  FI

  push  ebx
  
  test  ah,shadow_ptab SHR 8
  IFNZ
        pushad
        
        and   byte ptr [esi+1],NOT (shadow_ptab SHR 8)
        mov   esi,[ecx+shadow_pdir]
        and   esi,-pagesize
        mov   ecx,esi
        shr   ecx,log2_pagesize
        sub   eax,eax
        mov   ds:[ecx*4+ptab_backlink],eax
        DO
              mov   eax,dword ptr [esi+PM]
              test  eax,eax
              IFNZ
                    call  unmap_page
              FI
              add   esi,4
              test  esi,pagesize-1
              REPEATNZ
        OD                    
        popad
  FI
          
  call  [ebp+operation]
  
  sub   eax,eax

  pop   ebx
  xret  ,long







XHEAD lock_shadow_ptab

        
  push  ebx
  
  mov   ebx,eax      
  shr   ebx,log2_pagesize
  IFNZ
  CANDZ [ebx*4+ptab_backlink],0
  
        IFZ   [ebp+operation],<offset grant_page_>
              push  ecx
              shl   ecx,log2_size_pnode-2
              cmp   [ecx+M4_pnode_base].pte_ptr,edx
              pop   ecx
        CANDZ                                     ; transfer to K4 pages if
              sub   eax,eax                       ; ur pages granted
              xchg  eax,[ecx+shadow_pdir]         ; (typically sigma 0 to kernel)
              mov   [edx],eax
        
        ELSE_
              or    byte ptr [edx+1],shadow_ptab SHR 8
              mov   [ebx*4+ptab_backlink],edx
        FI      

  ELSE_
        sub   eax,eax                             ; inhibit any 4K operation if no
  FI                                              ; shadow ptab (i.e. device mem)
  
  pop   ebx                              
  xret  ,long                   
             
        



XHEAD gen_emulated_4M_page

  push  ebx
  push  ecx

  lea   ebx,[eax+page_present]

  DO
        mov   ecx,edx
        mov   edx,esp              ; denoting current task
        call  map_fresh_ptab 
        mov   edx,ecx
        IFC
              sub   eax,eax                 ; drop mem if no more ptabs available
              EXIT                          ;
        FI
        test  eax,(MB4-1) AND -pagesize     ; take another ptab if this one at 0 modulo 4M
        REPEATZ                             ; this enables to differentiate between real 4Ms
                                            ; and emulated 4Ms (bits 12..21 non zero)  
        push  eax
        push  ebx
        and   bl,NOT superpage
        DO
              mov   dword ptr ds:[eax+PM],ebx
              add   ebx,pagesize
              add   eax,4
              test  ebx,(MB4-1) AND -pagesize
              REPEATNZ
        OD
        pop   ebx
        pop   eax

        and   ebx,pagesize-1
        or    eax,ebx
  OD

  mov   dword ptr ds:[edx],eax
  
  pop   ecx
  pop   ebx
  xret  ,long





;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
;  kcode



;****************************************************************************
;*****                                                                  *****
;*****                                                                  *****
;*****        pnode handling                                            *****
;*****                                                                  *****
;*****                                                                  *****
;****************************************************************************





;----------------------------------------------------------------------------
;
;       alloc pnode
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       reg   new pnode
;       EBX   scratch
;
;----------------------------------------------------------------------------


alloc_pnode macro reg

  mov   reg,ds:[free_pnode_root]
  test  reg,reg
  jz    short free_pnode_unavailable
  mov   ebx,[reg].next_free
  mov   ds:[free_pnode_root],ebx

  endm




;----------------------------------------------------------------------------
;
;       release pnode
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       reg   allocated pnode to be released
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EDI   scratch
;
;----------------------------------------------------------------------------


release_pnode macro reg

  mov   edi,reg
  xchg  edi,ds:[free_pnode_root]
  mov   [reg].next_free,edi

  endm







;----------------------------------------------------------------------------
;
;       find pnode
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   page table entry
;       ESI   pointer to PTE (denoting a present page !)
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX   pointer to pnode associated to ESI-PTE
;       EBX   pointer to root pnode
;
;----------------------------------------------------------------------------


find_pnode macro

  DO
        test  al,superpage
        IFZ
              shr   eax,log2_pagesize-log2_size_pnode
              and   eax,-sizeof pnode
              add   eax,offset pnode_base
              mov   ebx,eax

              mov   eax,[eax+cache0]
              cmp   [eax].pte_ptr,esi
              EXITZ
              
              mov   eax,[ebx+cache1]
              cmp   [eax].pte_ptr,esi
              EXITZ
        FI      
        call  search_pnode
  OD

  endm



        align 16


search_pnode:

  test  al,page_present   ; = 1             ; means: EAX has superpage entry
  IFNZ  
        test  eax,(MB4-1) AND -pagesize
        IFNZ
              and   eax,-pagesize                 ; for emulated 4Ms, phys
              mov   eax,dword ptr ds:[eax+PM]     ; addr must be taken from ptab
        FI
        shr   eax,22-log2_size_pnode
        and   eax,-sizeof pnode
        add   eax,offset M4_pnode_base
        mov   ebx,eax
  FI      
              
  mov   eax,ebx
  DO
        cmp   [eax].pte_ptr,esi
        EXITZ
        mov   eax,[eax].child_pnode
        test  al,1
        REPEATZ

        DO
              mov   eax,[eax-1].succ_pnode
              test  al,1
              REPEATNZ
        OD
        REPEAT
  OD
  ret
    






;----------------------------------------------------------------------------
;
;       refind cached0 pnode
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   pointer to PTE (denoting a present page !)
;
;       cache0 of corresponding pnode tree holds ESI-related pnode
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX   pointer to pnode associated to ESI-PTE
;       EBX   pointer to root pnode
;
;----------------------------------------------------------------------------


refind_cached0_pnode macro

  mov   eax,[esi]
  mov   ebx,eax
  
  and   bl,superpage
  IFNZ
        shr   eax,22-log2_size_pnode
        and   eax,-sizeof pnode
        add   eax,offset M4_pnode_base
  ELSE_      
        shr   eax,log2_pagesize-log2_size_pnode
        and   eax,-sizeof pnode
        add   eax,offset pnode_base
  FI 
       
  mov   ebx,eax
  mov   eax,[eax+cache0]

  endm




;----------------------------------------------------------------------------
;
;       cache pnode
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBX   pointer to root pnode
;       reg0  entry to be cached in cache0 / nil
;       reg1  entry to be cached in cache1 / nil
;
;----------------------------------------------------------------------------

cache_pnode macro reg0,reg1

  mov   [ebx].cache0,reg0
  mov   [ebx].cache1,reg1

  endm







;----------------------------------------------------------------------------
;
;       grant page
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   pointer to first source PTE (present!)
;       EDI   pointer to first dest PTE
;       EBP   pointer to do... variables
;
;       dest PTE empty
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX   scratch
;
;       dest frame addr     = old source frame addr
;       dest access rights  = old source access rights AND [ebp+map_mask]
;
;       source PTE empty
;
;----------------------------------------------------------------------------


XHEAD flush_grant_dest_page

  push  eax
  push  ecx
  push  esi
  push  edi
  
  mov   esi,edi
  call  flush_page
  
  pop   edi
  pop   esi
  pop   ecx
  pop   eax
  
  xret
  





        align 16


grant_page_:


  IFB_  esi,<offset shadow_pdir>
  
        mov   ebx,[edi]
        test  ebx,ebx
        xc    nz,flush_grant_dest_page
  

        find_pnode

        mov   [eax].pte_ptr,edi
        mov   eax,[esi]
        mov   dword ptr [esi],0
     ;;   and   ebx,[ebp+map_mask]
        mov   [edi],eax

        mov   eax,esi
        mov   ebx,edi
        shr   eax,log2_chaptersize
        shr   ebx,log2_chaptersize
        dec   [eax+chapter_map-(PM SHR log2_chaptersize)]
        inc   [ebx+chapter_map-(PM SHR log2_chaptersize)]

  FI
  ret



  
  
  
void_or_access_attribute_widening:  
        
  and   eax,[ebp+map_mask]
  test  al,page_write_permit
  IFNZ
  xor   ebx,eax
  and   ebx,NOT (page_accessed+page_dirty)
  CANDZ ebx,page_write_permit

        mov   [edi],eax

  FI

  ret





;----------------------------------------------------------------------------
;
;       map page
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   pointer to source PTE (present!)
;       EDI   pointer to dest PTE
;       EBP   pointer to do... variables
;
;
;       dest PTE empty     OR     dest frame addr = source frame addr,
;                                 dest read only, source read/write, CL=FF
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,EDX     scratch
;
;       dest frame addr     = source frame addr
;       dest access rights  = source access rights AND CL
;
;----------------------------------------------------------------------------


        align 16


map_page:

  mov   ebx,[edi]
  test  ebx,ebx
  jnz   void_or_access_attribute_widening
  

  alloc_pnode edx

  find_pnode

  cache_pnode eax,edx

  mov   [edx].pte_ptr,edi
  mov   ebx,[esi]
  and   ebx,[ebp+map_mask]
  mov   [edi],ebx

  mov   ebx,edi
  shr   ebx,log2_chaptersize
  inc   [ebx+chapter_map-(PM SHR log2_chaptersize)]

  lea   ebx,[edx+1]
  mov   [edx].child_pnode,ebx

  mov   ebx,[eax].child_pnode
  mov   [eax].child_pnode,edx

  mov   [edx].succ_pnode,ebx
  test  bl,1
  IFZ
        mov   [ebx].pred_pnode,edx
  FI
  inc   eax
  mov   [edx].pred_pnode,eax

  ret







free_pnode_unavailable:

  ke    '-free_pnode_unav'




;----------------------------------------------------------------------------
;
;       unmap page
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;        CL   i
;        CH   j
;       ESI   pointer to dest PTE
;       EBP   pointer to do... variables
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;        CL   i - number of unmapped pages in same chapter beyond esi
;        CH   max (1, j - number of unmapped pages)
;
;       EAX,EBX,EDX,EDI scratch
;
;----------------------------------------------------------------------------


        align 16


unmap_page:

  find_pnode

  mov   edx,eax
  mov   eax,[eax].child_pnode
  test  al,1
  IFNZ
        cache_pnode edx,edx
        ret
  FI

  push  edi
  push  ebp

  mov   edi,[eax].pred_pnode
  and   edi,NOT 1
  cache_pnode edx,edi

  inc   edx
  mov   edi,ds:[free_pnode_root]
  mov   ebp,ds:[cpu_cr3]
  add   ebp,PM
  DO
        push  edx

        mov   edx,[eax].pte_ptr

        mov   [eax].next_free,edi

        mov   edi,edx
        mov   bl,[edx]
        
        mov   dword ptr [edi],0
        
        shr   edi,log2_chaptersize
        
        dec   [edi+chapter_map-(PM SHR (log2_chaptersize))]
        
        shr   edi,log2_pagesize-log2_chaptersize
        
        shr   bl,superpage_bit                 ; always flush TLB if 4M page unmapped,
        IFZ                                    ; avoid backlink access in this case
        
              mov   ebx,[(edi*4)+ptab_backlink-((PM SHR log2_pagesize)*4)]
        FI
        push  ebx
        shl   bl,8                             ; Z ! bit 0 -> cf
        IFNC
              xor   ebx,ebp
              test  ebx,-pagesize;;;;;;;;0;;;;;;;;;;;;;;;;;;;;;;;;-pagesize
        FI      
        pop   ebx
        xc    z,unmap_in_own_address_space

        pop   edx

        sub   ch,2
        adc   ch,1

        mov   edi,eax

        mov   eax,[eax].child_pnode
        test  al,1
        REPEATZ

        DO
              cmp   eax,edx
              OUTER_LOOP EXITZ
              mov   eax,[eax-1].succ_pnode
              test  al,1
              REPEATNZ
        OD
        REPEAT
  OD
  mov   ds:[free_pnode_root],edi
  mov   [eax-1].child_pnode,eax

  pop   ebp
  pop   edi

  ret







XHEAD unmap_in_own_address_space

  test  ebx,1
  IFNZ
        xchg  ebp,[esp]
        mov   byte ptr [ebp+tlb_flush_indicator],2
        xchg  ebp,[esp]
  
  ELSE_
        push  edx
        mov   ebx,edx
        and   edx,(pagesize-1) AND -4
        shr   ebx,log2_pagesize
        shl   edx,log2_pagesize-2
        mov   ebx,[(ebx*4)+ptab_backlink-((PM SHR log2_pagesize)*4)]
        and   ebx,(pagesize-1) AND -4
        shl   ebx,2*log2_pagesize-4
        add   edx,ebx
        invlpg [edx]
        pop   edx
  FI


  cmp   edx,esi
  xret  be

  lea   edi,[esi+chaptersize-1]
  and   edi,-chaptersize
  cmp   edx,edi
  xret  b

  dec   cl
  xret





unmap_write_page:

  find_pnode

  mov   edx,eax
  mov   eax,[eax].child_pnode
  test  al,1
  IFZ
        cache_pnode edx,eax

        push  ebp

        inc   edx
        mov   ebp,ds:[cpu_cr3]
        add   ebp,PM
        DO
              mov   ebx,[eax].pte_ptr
              
              and   byte ptr [ebx],NOT page_write_permit              
              mov   bl,[ebx]
                                                              ; flush TLB if 4 M page
              shr   bl,superpage_bit                          ; avoid backlink acc in this case
              IFZ
                    shr   ebx,log2_pagesize
                    mov   ebx,[(ebx*4)+ptab_backlink-((PM SHR log2_pagesize)*4)]
              FI      
              push  ebx
              shl   bl,8                             ; Z ! bit 0 -> cf
              IFNC
                    xor   ebx,ebp
                    test  ebx,-pagesize
              FI      
              pop   ebx
              xc    z,unmap_write_in_own_address_space

              mov   eax,[eax].child_pnode
              test  al,1
              REPEATZ

              DO
                    cmp   eax,edx
                    OUTER_LOOP EXITZ
                    mov   eax,[eax-1].succ_pnode
                    test  al,1
                    REPEATNZ
              OD
              REPEAT
        OD
        pop   ebp

  FI

  ret







XHEAD unmap_write_in_own_address_space



  test  ebx,1
  IFZ

        push  ecx
  
        mov   ebx,[eax].pte_ptr
        mov   ecx,ebx
        and   ebx,(pagesize-1) AND -4
        shr   ecx,log2_pagesize
        shl   ebx,log2_pagesize-2
        mov   ecx,[(ecx*4)+ptab_backlink-((PM SHR log2_pagesize)*4)]
        and   ecx,(pagesize-1) AND -4
        shl   ecx,2*log2_pagesize-4
        add   ebx,ecx
        invlpg [ebx]
  
        pop   ecx
        xret

  FI

  xchg  ebp,[esp]
  mov   byte ptr [ebp+tlb_flush_indicator],2
  xchg  ebp,[esp]
  xret






;----------------------------------------------------------------------------
;
;       flush page
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   pointer to dest PTE
;       EBP   pointer to do... variables
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,EDX,EDI scratch
;
;----------------------------------------------------------------------------


        align 16


flush_page:

  IFB_  esi,<offset shadow_pdir>
  
        call  unmap_page

        refind_cached0_pnode

        mov   edi,esi
        shr   edi,log2_chaptersize
        sub   edx,edx
        mov   dword ptr [esi],edx
        dec   [edi+chapter_map-(PM SHR log2_chaptersize)]

        release_pnode eax

        mov   edx,[eax].succ_pnode
        mov   eax,[eax].pred_pnode

        test  al,1
        IFZ
              mov   [eax].succ_pnode,edx
        ELSE_
              mov   [eax-1].child_pnode,edx
        FI
        test  dl,1
        IFZ
              mov   [edx].pred_pnode,eax
        FI

        and   dl,NOT 1
        and   al,NOT 1
        cache_pnode edx,eax

        mov   byte ptr [ebp+tlb_flush_indicator],2

  FI
  ret






flush_write_page:

  IFB_  esi,<offset shadow_pdir>
  
        call  unmap_write_page

        mov   eax,[ebp].cache0

        and   byte ptr [esi],NOT page_write_permit

        mov   byte ptr [ebp+tlb_flush_indicator],2

  FI
  ret





;----------------------------------------------------------------------------
;
;       set page cacheable/uncacheable
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   pointer to dest PTE
;       EBP   pointer to do... variables
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,EDX,EDI scratch
;
;----------------------------------------------------------------------------


set_page_cacheable:

  IFB_  esi,<offset shadow_pdir>
  
        mov   eax,[ebp].cache0

        and   byte ptr [esi],NOT (page_cache_disable+page_write_through)

        mov   byte ptr [ebp+tlb_flush_indicator],2

  FI
  ret



set_page_uncacheable:

  IFB_  esi,<offset shadow_pdir>
  
        mov   eax,[ebp].cache0

        or    byte ptr [esi],page_cache_disable+page_write_through

        mov   byte ptr [ebp+tlb_flush_indicator],2

  FI
  ret





;----------------------------------------------------------------------------
;
;       translate address
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   virtual address in source space
;       EBP   dest tcb
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX   virtual address in dest space / FFFFFFFF
;
;       EBX,ECX,EDX       scratch
;
;----------------------------------------------------------------------------






translate_address:  
  
  lno___task edx,ebp
  load__proot edx,edx
  add   edx,PM
  
  mov   ecx,cr3
  IFAE  <byte ptr ds:[gdt+linear_space/8*8+7]>,<offset small_virtual_spaces SHR 24>
        lno___task ecx,esp
        load__proot ecx,ecx
  FI
  and   ecx,-pagesize
  xpdir ebx,eax
  mov   ebx,dword ptr ds:[ebx*4+ecx+PM]
  test  bl,page_present
  jz    translate_to_nil
  and   ebx,-pagesize
  xptab eax,eax
  mov   eax,dword ptr ds:[eax*4+ebx+PM]
  test  al,page_present
  jz    translate_to_nil
  
  
  shr   eax,log2_pagesize-log2_size_pnode
  and   eax,-sizeof pnode
  add   eax,offset pnode_base
  lea   ebx,[eax+1]

  DO
        mov   ecx,[eax].pte_ptr
        shr   ecx,log2_pagesize
        mov   ecx,ds:[ecx*4+ptab_backlink-(PM SHR log2_pagesize)*4]
        sub   ecx,edx
        cmp   ecx,pagesize
        EXITB
        mov   eax,[eax].child_pnode
        test  eax,1
        REPEATZ

        DO
              cmp   eax,ebx
              jz    translate_to_nil
              mov   eax,[eax-1].succ_pnode
              test  eax,1
              REPEATNZ
        OD
        REPEAT
  OD

  and   ecx,-4
  shl   ecx,log2_pagesize+10-2
  mov   eax,[eax].pte_ptr
  and   eax,pagesize-1
  shl   eax,log2_pagesize-2
  add   eax,ecx
  
  ret
    

translate_to_nil:

  sub   eax,eax
  dec   eax
  ret







;  kcod ends
;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||





;----------------------------------------------------------------------------
;
;       flush address space
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EDI   phys pdir address (proot)
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,ESI,EBP scratch
;
;----------------------------------------------------------------------------


flush_address_space:

  push  edi
  push  offset flush_address_space_ret

  sub   esp,sizeof do_source_dest_data-sizeof do_source_data

  sub   edx,edx
  push  edx
  sub   eax,eax
  push  eax

  mov   cl,32-log2_pagesize

  mov   ebp,edi

  push  offset flush_page
  jmp   do_fpage_in_address_space


flush_address_space_ret:

  pop   edi
  ret





;----------------------------------------------------------------------------
;
;       map ur page initially
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX     physical address  (4K aligned)
;       DL      page attributes
;       ESI     linear address    (only bits 31...12 relevant)
;
;
;       DS,ES   phys mem / linear space & kernel task
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       mapped  (user, read/write, ur)
;       corresponding pnode_root initialized
;
;----------------------------------------------------------------------------


  icode



map_ur_page_initially:

  pushad

  mov   ebx,eax

  load__proot ecx,<((sigma0_task AND mask task_no) SHR task_no)>

  xpdir eax,esi
  lea   ecx,[(eax*4)+ecx+PM]
  
  test  dl,superpage
  IFZ
        mov   eax,[ecx]
        and   eax,-pagesize
        IFZ
              push  edx
              mov   edx,sigma0_task
              call  map_fresh_ptab
              pop   edx
        FI

        xptab ecx,esi
        lea   ecx,[(ecx*4)+eax+PM]
        
  ELIFB ebx,ds:[logical_info_page+main_mem].mem_end   ;;;; max_physical_memory_size
  mov   eax,ecx
  and   eax,pagesize-1
  test  byte ptr ds:[eax+shadow_pdir],page_present
  CANDZ
  
        pushad
        mov   eax,ebx
        and   dl,NOT superpage
        DO
              call  map_ur_page_initially
              add   eax,pagesize
              add   esi,pagesize
              test  eax,MB4-1
              REPEATNZ
        OD   
        mov   eax,[ecx]
        and   ecx,pagesize-1
        mov   ds:[ecx+shadow_pdir],eax
        shr   eax,log2_pagesize
        sub   ecx,ecx
        mov   ds:[eax*4+ptab_backlink],ecx
        popad  
        
  FI      

  test  dl,superpage
  IFNZ
  bt    ds:[cpu_feature_flags],page_size_extensions_bit ; on 486, no 4M support, device mem is
  CANDNC                                                ; initialized as 4M NOT present
        and   dl,NOT page_present                       ; ptabs (4K pages)) are generated on demand
  FI                                                    ; when sigma0 maps them to someone

  mov   bl,dl
  mov   [ecx],ebx

  test  dl,superpage
  IFZ
        shr   ebx,log2_pagesize
        shl   ebx,log2_size_pnode

        lea   eax,[ecx-PM]
        shr   eax,log2_chaptersize
        inc   [eax+chapter_map]

        add   ebx,offset pnode_base
  ELSE_
        shr   ebx,22
        shl   ebx,log2_size_pnode
        add   ebx,offset M4_pnode_base
  FI            

  mov   [ebx].pte_ptr,ecx
  mov   [ebx].cache0,ebx
  mov   [ebx].cache1,ebx
  inc   ebx
  mov   [ebx-1].child_pnode,ebx

  popad
  ret



  icod  ends








  code ends
  end
