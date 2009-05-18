include l4pre.inc 


  Copyright GMD, L4.PAGMAP.4, 17,07,96, 40, K
 
 
;*********************************************************************
;******                                                         ******
;******         Page Mapper                                     ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  24.02.90    ******
;******                                   modified: 17.07.96    ******
;******                                                         ******
;*********************************************************************
 

  public alloc_initial_pagmap_pages
  public init_pagmap
  public map_ur_page_initially
  public pagmap_fault
  public grant_fpage
  public map_fpage
  public flush_address_space


  extrn request_fresh_frame:near
  extrn map_fresh_ptab:near
  extrn alloc_kernel_pages:near
  extrn map_page_initially:near
  extrn map_system_shared_page:near
  extrn gen_4M_page:near
  extrn define_idt_gate:near
  extrn phys_frames:dword
 


.nolist 
include l4const.inc
include adrspace.inc
include intrifc.inc
include uid.inc
include tcb.inc
include cpucb.inc
include schedcb.inc
include syscalls.inc
.list
include pagconst.inc
include pagmac.inc
include pagcb.inc
.nolist
include msg.inc
.list





ok_for i486,ppro




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
  mov   cl,page_present+page_write_permit
  call  alloc_kernel_pages
  mov   ecx,eax
  mov   edi,esi
  mov   al,0
  cld
  rep   stosb

  mov   esi,offset pnode_space
  mov   eax,[phys_frames]
  add   eax,high4M_pages
  shl   eax,log2_size_pnode
  mov   cl,page_present+page_write_permit
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


  mov   eax,[phys_frames+PM]
  shl   eax,log2_size_pnode
  lea   esi,[eax+pnode_base]
  mov   ds:[free_pnode_root],esi
  imul  eax,pnodes_per_frame-1

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



  mov   ebx,M4_pnode_base
  DO
        mov   [ebx].pte_ptr,0
        mov   [ebx].cache0,ebx
        mov   [ebx].cache1,ebx
        inc   ebx
        mov   [ebx-1].child_pnode,ebx

        add   ebx,sizeof pnode-1
        cmp   ebx,M4_pnode_base + high4M_pages * sizeof pnode
        REPEATB
  OD


 
  mov   bh,3 SHL 5

  mov   bl,fpage_unmap
  mov   eax,offset unmap_fpage_sc+PM
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
  kcode




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





      klign 32



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

  push  offset grant_page_+PM
  jmp   short do_fpage





;----------------------------------------------------------------------------
;
;       unmap fpage
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

        push  offset unmap_fpage_ret+PM

        sub   esp,sizeof do_source_dest_data-sizeof do_source_data

        sub   edx,edx
        push  edx
        push  eax

        test  ecx,ecx
        IFNS
              test  ch,page_write_permit
              IFNZ
                    push  offset unmap_page+PM
                    jmp   short do_fpage
              FI
              push  offset unmap_write_page+PM
              jmp   short do_fpage
        FI

              test  ch,page_write_permit
              IFNZ
                    push  offset flush_page+PM
                    jmp   short do_fpage
              FI
              push  offset flush_write_page+PM
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

  push  offset map_page+PM


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
  IF    kernel_type NE i486
        IFAE  <byte ptr ds:[ldt+linear_space/8*8+7]>,<offset small_virtual_spaces SHR 24>
              lno___task ebp,esp
              load__proot ebp,ebp
        FI
  ENDIF            
                     
  
  
do_fpage_in_address_space:

  mov   ebx,1
  shl   ebx,cl

  mov   cl,ch
  and   ch,NOT (ur_page SHR 8)
  or    ecx,NOT (page_write_permit + (1 SHL no_autoflush_bit) + ur_page)

  and   ebp,-pagesize

  push  ecx

  mov   ecx,PM
  add   ebp,ecx

  inc   ds:[do_fpage_counter]

  push  ebp

  cmp   ebx,ptes_per_chapter
  jae   do_large_fpage

  mov   esi,eax
  shr   eax,22
  and   esi,003FF000h

  cmp   eax,virtual_space_size SHR 22
  jae   do_fpage_ret

  shr   esi,log2_pagesize-2
  mov   eax,[eax*4+ebp]
  add   esi,ecx


  test  eax,M4_page+page_present
  IFPO                               ; note: NOT present => NOT M4 page
        and   eax,-pagesize
        add   esi,eax

        test  edx,edx
        IFNZ
              lno___task ecx,edx

              cmp   edi,virtual_space_size
              jae   short do_fpage_ret

              load__proot ecx,ecx
              mov   ebp,edi
              shr   ebp,22

              and   edi,003FF000h
              shr   edi,log2_pagesize-2
              add   edi,PM

              mov   eax,dword ptr [ebp*4+ecx+PM]

              test  eax,(M4_page SHL 8)+page_present
              xc    pe,do_fpage_map_fresh_ptab
              and   eax,-pagesize
              add   edi,eax
        FI


        mov   ebp,esp

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
  FI




do_fpage_ret:

  cmp   [esp+tlb_flush_indicator],1

  lea   esp,[esp+sizeof do_source_dest_data]
  ret









XHEAD do_fpage_map_fresh_ptab

  jnz   do_fpage_ret

  lea   ecx,[ebp*4+ecx+PM]
  call  map_fresh_ptab
  xret  nc

  jmp   do_fpage_ret





  kcod  ends
;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||






do_large_fpage:

  DO

        cmp   eax,virtual_space_size
        jae   do_large_fpage_ret

        mov   ebp,esp

        push  eax
        push  edi

        mov   esi,eax
        and   eax,0FFC00000h
        shr   eax,22-2
        mov   edx,[ebp+source_pdir]
        add   edx,eax
        and   esi,003FF000h
        shr   esi,log2_pagesize-2
        add   esi,PM

        mov   eax,[edx]

        cmp   ebx,MB4/pagesize
        xc    ae,perhaps_high4M_page,long

        test  eax,M4_page+page_present
        IFPO  ,,long                       ; note: NOT present => NOT M4 rpage
              and   eax,-pagesize
              add   esi,eax

              mov   edx,[ebp+dest_task]
              IFA   edx,2

                    lno___task ecx,edx

                    cmp   edi,virtual_space_size
                    jae   do_large_fpage_pop_ret

                    load__proot ecx,ecx
                    mov   eax,edi
                    shr   eax,22
                    and   edi,003FF000h
                    lea   ecx,[eax*4+ecx+PM]

                    shr   edi,log2_pagesize-2
                    add   edi,PM

                    mov   eax,[ecx]
                    test  eax,M4_page+page_present
                    xc    pe,do_large_fpage_map_fresh_ptab,long
                    and   eax,-pagesize
                    add   edi,eax
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
                    xc    z,permit_intr_in_do_large_fpage

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



do_large_fpage_ret:

  cmp   [esp+tlb_flush_indicator],1

  lea   esp,[esp+sizeof do_source_dest_data]
  ret








XHEAD do_large_fpage_map_fresh_ptab

  jnz   short do_large_fpage_pop_ret

  call  map_fresh_ptab
  xret  nc,long



do_large_fpage_pop_ret:

  pop   edi
  pop   eax
  jmp   do_large_fpage_ret







XHEAD permit_intr_in_do_large_fpage

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

  jmp   do_large_fpage











XHEAD perhaps_high4M_page

  test  ah,M4_page SHR 8
  IFZ
        test  eax,eax
        xret  nz,long

        mov   ecx,[ebp+source_pdir]
        sub   ecx,PM
        cmp   ecx,ds:[sigma0_proot]
        xret  nz,long

        call  gen_high4M_ur_page
        xret  z,long
  FI


  mov   esi,edx
  mov   edx,[ebp+dest_task]
  IFA   edx,2

        lno___task ecx,edx

        cmp   edi,virtual_space_size
        jae   do_large_fpage_pop_ret

        load__proot ecx,ecx
        shr   edi,22
        lea   edi,[edi*4+ecx+PM]
  FI

  push  ebx
  call  [ebp+operation]
  pop   ebx

  sub   eax,eax

  xret  ,long






;||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
  kcode



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

  shr   eax,log2_pagesize-log2_size_pnode
  and   eax,-sizeof pnode
  add   eax,offset pnode_base
  mov   ebx,eax

  mov   eax,[eax+cache0]
  IFNZ  [eax].pte_ptr,esi
  mov   eax,[ebx+cache1]
  CANDNZ [eax].pte_ptr,esi
        call  search_pnode
  FI

  endm



        align 16


search_pnode:

  push  ecx
  mov   ecx,100
  mov   eax,ebx
  DO
        dec   ecx
        IFZ
              ke    's00'
        FI

        cmp   [eax].pte_ptr,esi
        EXITZ
        mov   eax,[eax].child_pnode
        test  al,1
        REPEATZ

        DO
              dec   ecx
              IFZ
                    ke    's01'
              FI

              mov   eax,[eax-1].succ_pnode
              test  al,1
              REPEATNZ
        OD
        REPEAT
  OD
  pop   ecx
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
  shr   eax,12-log2_size_pnode
  and   eax,-sizeof pnode
  add   eax,offset pnode_base
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


        align 16


grant_page_:


  mov   ebx,[edi]
  test  ebx,ebx
  jnz   short void_or_access_attribute_widening
  

  find_pnode

  mov   [eax].pte_ptr,edi
  mov   eax,[esi]
  mov   dword ptr [esi],0
  mov   [edi],eax

  mov   eax,esi
  mov   ebx,edi
  shr   eax,log2_chaptersize
  shr   ebx,log2_chaptersize
  dec   [eax+chapter_map-(PM SHR log2_chaptersize)]
  inc   [ebx+chapter_map-(PM SHR log2_chaptersize)]

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
  mov   ebp,cr3
  add   ebp,PM
  DO
        mov   ebx,[eax].pte_ptr

        mov   [eax].next_free,edi

        mov   edi,ebx
        mov   dword ptr [ebx],0

        shr   ebx,log2_pagesize
        shr   edi,log2_chaptersize

        mov   ebx,[(ebx*4)+ptab_backlink-((PM SHR log2_pagesize)*4)]

        dec   [edi+chapter_map-(PM SHR (log2_chaptersize))]

        xor   ebx,ebp
        test  ebx,-pagesize
        xc    z,unmap_in_own_address_space

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

  xchg  ebp,[esp]
  mov   byte ptr [ebp+tlb_flush_indicator],2
  xchg  ebp,[esp]

  mov   ebx,[eax].pte_ptr
  cmp   ebx,esi
  xret  be

  lea   edi,[esi+chaptersize-1]
  and   edi,-chaptersize
  cmp   ebx,edi
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
        mov   ebp,cr3
        add   ebp,PM
        DO
              mov   ebx,[eax].pte_ptr

              and   byte ptr [ebx],NOT page_write_permit

              shr   ebx,log2_pagesize
              mov   ebx,[(ebx*4)+ptab_backlink-((PM SHR log2_pagesize)*4)]
              xor   ebx,ebp
              test  ebx,-pagesize
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

  ret






flush_write_page:

  call  unmap_write_page

  mov   eax,[ebp].cache0

  and   byte ptr [edi],NOT page_write_permit

  mov   byte ptr [ebp+tlb_flush_indicator],2

  ret









  kcod ends
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
  push  offset flush_address_space_ret+PM

  sub   esp,sizeof do_source_dest_data-sizeof do_source_data

  sub   edx,edx
  push  edx
  sub   eax,eax
  push  eax

  mov   cl,32-log2_pagesize

  mov   ebp,edi

  push  offset flush_page+PM
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

  push  edx

  mov   edx,sigma0_task
  load__proot ecx,<((sigma0_task AND mask task_no) SHR task_no)>

  xpdir eax,esi
  lea   ecx,[(eax*4)+ecx+PM]
  mov   eax,[ecx]
  and   eax,-pagesize
  IFZ
        call  map_fresh_ptab
  FI

  xptab ecx,esi
  lea   ecx,[(ecx*4)+eax+PM]

  pop   edx

  mov   bl,dl
  bts   ebx,ur_page_bit
  mov   [ecx],ebx

  lea   eax,[ecx-PM]
  shr   eax,log2_chaptersize
  inc   [eax+chapter_map]

  shr   ebx,log2_pagesize
  shl   ebx,log2_size_pnode
  add   ebx,offset pnode_base

  mov   [ebx].pte_ptr,ecx
  mov   [ebx].cache0,ebx
  mov   [ebx].cache1,ebx
  inc   ebx
  mov   [ebx-1].child_pnode,ebx

  popad
  ret



  icod  ends




;----------------------------------------------------------------------------
;
;       gen high 4M page
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EDX   PDE address
;
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       NZ    succeeded
;
;       Z     failed
;
;
;----------------------------------------------------------------------------


gen_high4M_ur_page:

  push  ebx

 ke '4M'
  mov   ebx,M4_pnode_base
  DO
        cmp   [ebx].pte_ptr,0
        IFNZ
              add   ebx,sizeof pnode
              cmp   ebx,M4_pnode_base + high4M_pages * sizeof pnode
              REPEATB

              EXIT
        FI

        mov   eax,edx
        and   eax,pagesize-1
        shl   eax,22-2
        add   eax,GB1
        EXITNS

        call  gen_4M_page
        EXITC

        mov   [ebx].pte_ptr,edx

        test  al,superpage
        IFZ
              mov   ebx,eax
              shr   ebx,log2_pagesize-log2_size_pnode
              add   ebx,pnode_base

              mov   [ebx].pte_ptr,edx
              mov   [ebx].cache0,ebx
              mov   [ebx].cache1,ebx
              inc   ebx
              mov   [ebx-1].child_pnode,ebx
        FI
        or    eax,M4_page+ur_page+page_user_permit+page_write_permit+page_present
        mov   [edx],eax
                                ; NZ !
        pop   ebx
        ret
  OD

  pop   ebx
  sub   eax,eax                 ; Z
  ret






  code ends
  end
