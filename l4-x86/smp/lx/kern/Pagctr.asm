include l4pre.inc 


  Copyright IBM+UKA, L4.PAGCTR, 24,08,99, 116
 
 
;*********************************************************************
;******                                                         ******
;******         Paging Controller                               ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 24.08.99    ******
;******                                                         ******
;*********************************************************************
 
  public enable_paging_mode
  public init_fresh_frame_pool
  public map_page_initially
  public alloc_kernel_pages
  public alloc_shared_kernel_pages
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
  extrn grab_frame:near
  extrn phys_frames:dword
  extrn max_kernel_end:near
  extrn physical_kernel_info_page:dword
  extrn pre_paging_cpu_feature_flags:dword

  extrn ltr_pnr:near

	
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


ok_for x86



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


 
enable_paging_mode:				; alloc shared pages 	 
  ;<cd> establish initial Address space 
  pushad

  ;<cd> Grab Frame for new Page Directory
  call  grab_frame
  mov   edx,eax

  ;<cd> load Directory Address in CR3
  mov   cr3,eax
  mov   ebx,eax

  ;<cd> clear Page for Page Directory
  mov   edi,eax
  mov   ecx,pagesize/4
  sub   eax,eax
  cld
  rep   stosd

  ;<cd> set page directory entry 
  lea   eax,[ebx+page_present+page_write_permit]
  ;<cd> ebx = physical address of pdir + ptab space
  mov   [ebx+offset ptab_space SHR 20],eax


  ;<cd> map kernel pages to the beginning of the address space
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
	
  ;<cd> Map Physical Memory (with 4MB Pages if present)  
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
		mov   cl,page_present+page_write_permit
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
	        mov   cl,page_present+page_write_permit
		call  map_page_initially
	        add   eax,pagesize
		add   esi,pagesize
	        dec   edi
		REPEATNZ
	OD
  FI
	

  ;<cd> allocate page nodes and chapter map
  call  alloc_initial_pagmap_pages

  ;<cd> allocate kernel pages	;first page after gdt is local
  mov   esi,offset gdt+first_kernel_sgm
  mov   eax,pagesize
  call  alloc_kernel_pages
	
  lno___prc eax
  IFZ eax,1
 	mov   esi,offset gdt+first_kernel_sgm+pagesize
 	mov   eax,kernel_r_tables_size-(offset gdt+first_kernel_sgm)-pagesize
   	call  alloc_kernel_pages
  ELSE_
 	mov   esi,offset gdt+first_kernel_sgm+pagesize
 	mov   eax,kernel_r_tables_size-(offset gdt+first_kernel_sgm)-pagesize
	call  alloc_shared_kernel_pages
  FI
	
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

  ;<cd> map local apic
  mov   eax,0FEE00000h
  mov   esi,offset local_apic
  mov   cl,page_present+page_write_permit+page_write_through+page_cache_disable
  call  map_page_initially

  ;<cd> map io apic  
  mov   eax,0FEC00000h
  mov   esi,offset io_apic
  mov   cl,page_present+page_write_permit+page_write_through+page_cache_disable
  call  map_page_initially

  ;<cd> map logical kernel info page
  mov   eax,offset physical_kernel_info_page
  mov   esi,offset logical_info_page
  mov   cl,page_present+page_write_permit
  call  map_page_initially

  ;<cd> allocate frames for page table backlink	
  mov   esi,offset ptab_backlink
  mov   eax,[phys_frames]
  lea   eax,[eax*4]
  lno___prc ebx
  IFZ ebx,1
	call  alloc_kernel_pages
  ELSE_
	call  alloc_shared_kernel_pages
  FI

  ;<cd> allocate frame for processor local tcbs: dispatcher + 3
  mov esi,offset dispatcher_tcb
  mov eax,pagesize
  call alloc_kernel_pages
		
  ;<cd> grab frame and clear page for empty task page root
  call  grab_frame

  mov   ebx,eax

  mov   edi,eax
  mov   ecx,pagesize/4
  sub   eax,eax
  cld	
  rep   stosd

  lea   eax,[ebx+page_present+page_write_permit]
  mov   [ebx+offset ptab_space SHR 20],eax

  ;<cd> copy kernel pages to free page
  lea   esi,[edx+shared_table_base SHR 20]
  lea   edi,[ebx+shared_table_base SHR 20]
  mov   ecx,shared_table_size SHR 22
  cld
  rep   movsd
  

	;;   ke 'first done before paging'
	;;   call ltr_pnr
	;;   jmp enable_paging_mode

  ;<cd> enable paging 
  mov   eax,cr0
  bts   eax,31
  mov   cr0,eax


	
  ;<cd> set all task page roots to the clear page  
  lno___prc eax
  IFZ eax,1
	mov   edi,offset task_proot       ; ??????????????????????????
	lea   eax,[ebx+root_chief_no]
	sub   ecx,ecx
	DO
		mov   [edi],eax
	        mov   [edi+4],ecx
		add   edi,8
	        cmp   edi,offset proot_end_marker
		REPEATB
	OD
	dec   ecx
	mov   [edi],ecx
	mov   [edi+4],ecx

	mov   [bootstrap_page_root],edx
  FI

  ;<cd> set kernel page root	;get processor number
  lno___prc edi

  lea   ecx,[empty_proot+8*edi-8]	
  lea   edi,[kernel_proot+8*edi-8]
  mov   [ecx],ebx
  mov   [ecx+4],ebx
  mov   [edi],edx
  mov   [edi+4],edx

	
  popad
  ret

  bootstrap_page_root dd 0

     
  icod  ends




;----------------------------------------------------------------------------
;
;       alloc kernel pages
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX     area size         (will be rounded upwards to multiple of 4K)
;	CL	page attributes   (U/S, RW, Cacheability, Global)
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
  ;<cd> while pages to be mapped > 0
  DO
        push  eax
 	mov   cl, page_present+page_write_permit
	;<cd> grab physical frame
        call  grab_frame
	;<cd> map page to grabbed frame
        call  map_page_initially
        pop   eax
	;<cd> get offset of next page
        add   esi,pagesize
	;<cd> decrease pages to be mapped
        sub   eax,1
        REPEATA
  OD

  popad
  ret

	
alloc_shared_kernel_pages:
  pushad

  mov   edx,cr3
	
  add   eax,pagesize-1
  shr   eax,12
  ;<cd> while pages to be mapped > 0
  DO
	push  eax
 	mov   cl, page_present+page_write_permit
	;<cd> get physical address of kernel_proot 1
	  xpdir edi,esi

	
 	  mov eax,ds:[bootstrap_page_root]
	;; mov eax,ds:[kernel_proot]	; be sure address is value of bootstraps cr3
	
	  lea edi,[edi*4]
	  add eax,edi
	  IFAE esp,<tcb_space>
		add eax,PM
	  FI
	  mov eax,[eax]
	  and eax,-pagesize	
	
	  xptab edi,esi
	  lea   edi,[edi*4]
	  add   edi,eax
	  IFAE esp,<tcb_space>
		add edi,PM
	  FI
	  mov   eax,ds:[edi]
	  and eax,-pagesize

	;<cd> map page to same location as in bootstrap kernel
	call map_page_initially
	pop eax
	;<cd> get offset of next page	
	add esi,pagesize
	;<cd> decrease pages to be mapped
	sub eax,1
	REPEATA
  OD
	
  popad	
  ret
	

  icod  ends

;; ################################################################################

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



;##################################################################################

	
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



initial_fresh_frames equ 32


  icode


init_fresh_frame_pool:

  ;<cd> clear free_fresh_frames structure
  sub   eax,eax
  mov   ds:[first_free_fresh_frame],eax
  mov   ds:[free_fresh_frames],eax

  ;<cd> grab 32 frames and insert them into fresh frame pool
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

  ;<cd> clear interrupts 
  cli

  ;<cd> aquire lock
  lock bts ds:[fresh_frame_pool_lock],0
  IFC	
	ins_fresh_frame_pool_lock_aquired:
				
	;<cd> mask out bits 0..11 and load address of frame
	and   eax,-pagesize
	lea   edx,[eax+PM]

	;<cd> set new first free fresh frame
	xchg  ds:[first_free_fresh_frame],eax
	;<cd> first double links to next free fresh frame
	mov   [edx],eax

	;<cd> increase number of free fresh frames
	inc   ds:[free_fresh_frames]
  ELSE_
	;<cd> spin for lock
	DO
		DO
			bt ds:[fresh_frame_pool_lock],0
			REPEATS
		OD
		lock bts ds:[fresh_frame_pool_lock],0
		jc ins_fresh_frame_pool_lock_aquired
		REPEAT
	OD
  FI
  ;<cd> release lock
  lock btr ds:[fresh_frame_pool_lock],0		
		
  ;<cd> clear remaining part of the page
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
	
  ;<cd> aquire lock
  lock bts ds:[fresh_frame_pool_lock],0
  IFC	
	request_fresh_frame_lock_aquired:
	
	sub   ds:[free_fresh_frames],1
	IFNC
		push  edi

		mov   eax,ds:[first_free_fresh_frame]
		sub   edi,edi
		xchg  edi,dword ptr [eax+PM]
		mov   ds:[first_free_fresh_frame],edi

		;<cd> release lock
		lock btr ds:[fresh_frame_pool_lock],0		
	
		pop   edi
		ret

	FI
   ELSE_
	;<cd> spin for lock
	DO
		DO
			bt ds:[fresh_frame_pool_lock],0
			REPEATS
		OD
		lock bts ds:[fresh_frame_pool_lock],0
		jc request_fresh_frame_lock_aquired
		REPEAT
	OD
  FI

  inc   ds:[free_fresh_frames]

  ;<cd> release lock
  lock btr ds:[fresh_frame_pool_lock],0		

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

  ;<cd> request fresh frame
  call  request_fresh_frame
  IFNC
        push  esi

	;<cd> test location in chapter map
        mov   esi,eax
        shr   esi,log2_chaptersize
        add   esi,offset chapter_map

        test__page_present esi
        IFC
              push  eax
	      ;<cd> get page for chapter map
              call  request_fresh_frame
              IFNC
		    ;<cd> map page to chapter map area
                    call  map_system_shared_page
                    IFC
			  ;<cd> if mapping failed, reinsert page into pool
                          call  insert_into_fresh_frame_pool
			  ;<cd> no free pagetable available
                          stc
                    FI
              FI
              pop   eax
        FI
        pop   esi
  FI

  ret



;#####################################################################################################

	

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

  push  edx
  lno___prc edx
	
	
  mov   ebx,ecx
  and   ebx,pagesize-1

  CORB  ebx,<shared_table_base SHR 20>
  IFAE  ebx,<(shared_table_base+shared_table_size) SHR 20>

        lea   eax,[ecx-PM]
        sub   eax,ebx
        cmp   eax,ds:[empty_proot+8*edx-8]
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
	
              add   ebx,ds:[kernel_proot+8*edx-8]   ; ptab inserted into kernel
              mov   dword ptr [ebx+PM],eax          ; *and empty* proot !
              and   ebx,pagesize-1		    ; Sharing ptabs ensures that later
              add   ebx,ds:[empty_proot+8*edx-8]    ; mapped pages (tcbs) are shared
              mov   dword ptr [ebx+PM],eax          ; automatically. This is required
        FI                                          ; to permit switching to empty space !!
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

  pop   edx
  pop   ebx
  ret




XHEAD generate_own_pdir

  call  request_fresh_ptab      ; new pdir for task, copy of empty
  jc    map_ptab_exit
  
  and   ecx,pagesize-1
  lea   ecx,[eax+ecx+PM]

  push  ecx
  call  init_pdir
  push  edx
  lno___task edx
  chnge_proot eax,edx
  pop   edx
  ;<cd> load page directory address of task
  lea___pdir eax,edx
  lno___prc ecx
  dec   ecx
  shl   ecx,log2_pagesize
  add   eax,ecx
	
  call  flush_system_shared_page
  pop   ecx
  
  xret  ,long




;###############################################################################################
	


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

  lno___prc esi
  mov   esi,ds:[empty_proot+8*esi-8]             ; 1. shared tables taken from nil proot
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
;       ptabman int          called before (!) booter is started
;
;----------------------------------------------------------------------------


ptabman_init:

  ;<cd> store current segments (data, extra)
  push  ds
  push  es

  ;<cd> while s0 does not answer 
  DO
	;<cd> call sigma0 for pages for kernel use
        sub   ecx,ecx		            ; timeouts 0 
        mov   eax,ecx			    ; register send
        mov   ebp,ecx			    ; register receive
        lea   edx,[ecx+1]                   ; w0 = 00000001 
        mov   ebx,ecx                       ; w1 = 00000000
        mov   esi,sigma0_task		    ; dest = sigma0 

	ke 'before ipc to s0'
	
        int   ipc
	;<cd> repeat the loop if error occured in IPC or Sigma 0 answered with mapping (wrong answer to protocol)
        test  al,ipc_error_mask
        CORNZ
        test  al,map_msg
        IFNZ
              sub   esi,esi

	      ;<cd> switch to any thread and repeat the loop next time
              int   thread_switch
              REPEAT
        FI
  OD
  
  pop   es
  pop   ds

  ;<cd> sigma 0 answered with available kernel pages = k in edx
  ;<cd> for all available kernel pages k
  DO
        push  edx

	;<cd> send ipc to sigma0
	sub   ecx,ecx		            ; timeout 0
        mov   eax,ecx		            ; register IPC only
        lea   ebp,[ecx+(log2_)*4+map_msg]   ; rcv mapping to 0 of size ???
        lea   edx,[ecx-2]                   ; w0 = FFFFFFFE request a grant of any page
        mov   esi,sigma0_task		    ; dest = sigma0 
        
        push  ds
        push  es
        int   ipc	
        pop   es
        pop   ds

	;<cd> if map message received and 1 page was granted 
        IFZ   al,map_msg
        CANDZ bl,(log2_pagesize*4+fpage_grant)
	;<cd> and fpage_base == send_fpage
        xor   ebx,edx	
        and   ebx,-pagesize
        CANDZ
		;<cd> insert fpage into fresh frame pool
                mov   eax,edx
                call  insert_into_fresh_frame_pool
        ELSE_
		;<cd> sigma0 message failed.
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
	;<cd> wait, open receiving for any ipc, but do not reply
        sub   ecx,ecx		; timeout 0
        lea   eax,[ecx-1]	; no send
        mov   ebp,ecx		; wait for register receive
        sub   esi,esi		; dest = nil thread
        sub   edi,edi		; receive from anyone
        int   ipc		; send ipc
        REPEAT
  OD




;################################################################################################


	
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

  lno___prc ebx	
  mov   ebx,ds:[kernel_proot+8*ebx-8]
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
	
  lno___prc ecx
  mov   ecx,ds:[kernel_proot+8*ecx-8]
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
  lno___prc esi
  lea   esi,ds:[kernel_proot+8*esi-8] 
  mov   esi,[esi]
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