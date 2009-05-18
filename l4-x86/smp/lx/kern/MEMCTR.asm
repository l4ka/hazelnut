include l4pre.inc 
     
  Copyright IBM, L4.MEMCTR, 30,09,97, 20
 
 
;*********************************************************************
;******                                                         ******
;******         Memory Controller                               ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 30.09.97    ******
;******                                                         ******
;*********************************************************************
 
  public init_memctr
  public init_sigma0_1
  public grab_frame
  public phys_frames
 
  extrn create_kernel_including_task:near
  extrn map_ur_page_initially:near
  extrn physical_kernel_info_page:dword
  extrn dcod_start:byte
  extrn default_kdebug_end:byte
  extrn default_sigma0_end:byte
  extrn ktest_end:byte
  



.nolist 
include l4const.inc
include uid.inc
.list
include adrspace.inc
.nolist
include cpucb.inc
include tcb.inc
include pagconst.inc
include pagmac.inc
include syscalls.inc
include msg.inc
include kpage.inc
.list


ok_for x86


  IF    kernel_x2
        extrn generate_x2_info_page:near
  ENDIF      



                    align 4


phys_frames               dd    0

lowest_allocated_frame    dd    0,0



                    align 4


;----------------------------------------------------------------------------
;
;       grab frame / grab mem
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   size of grabbed area / -
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       phys_mem:EAX  begin of grabbed mem (4096 bytes / n*4096 bytes)
;
;----------------------------------------------------------------------------

  assume ds:codseg


grab_frame:

  push  ebx
  push  ecx
  push  edx
  push  esi
  push  edi
  push  ebp

  ;<cd> edx = 0
  ;<cd> if stack pointer within tcb space (tcb in kernel mode)
  sub   edx,edx
  IFAE  esp,<offset tcb_space>
  CANDB esp,<offset tcb_space+tcb_space_size>
	;<cd> edx = Physical Memory
        add   edx,PM
  FI

  ;<cd> load lowest allocated frame
  mov   eax,[edx+lowest_allocated_frame]

  ;<cd> if not 0 grab frame initially
  test  eax,eax
  jnz   initial_grab_frame

  ;<cd> setup ipc to sigma 0
  sub   eax,eax		; send register only 
  mov   ecx,eax		; timeout 0
  lea   edx,[eax-2]     ; w0 = FFFFFFFE (request grant of free page)

  log2  <%physical_kernel_mem_size>

  mov   ebp,log2_*4+map_msg ; receive mapping in physical kernel memory area 

  mov   esi,sigma0_task	; dest sigma0 
  sub   edi,edi		; open receive

  ;<cd> send ipc 
  int	ipc		

  ;<cd> if error occured or no map message or page not granted or not exactly one page granted
  test  al,ipc_error_mask
  CORNZ
  test  al,map_msg
  CORZ
  test  bl,fpage_grant
  CORZ
  shr   bl,2
  IFNZ  bl,log2_pagesize
	;<cd> kernel exception due to false mapping from sigma0
        ke    <'-',0E5h,'0_err'>
  FI

  ;<cd> return offset of physical page
  mov   eax,edx
  and   eax,-pagesize


  pop   ebp
  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
  ret



  icode


initial_grab_frame:

  ;<cd> decrease pointer to lowest allocated frame by one page
  sub   eax,pagesize

  ;<cd> memory underflow if pointer below 1 MB
  IFB_  eax,MB1
        ke    '-memory_underflow'
  FI

  ;<cd> store current pointer to lowest allocated frame
  mov   [edx+lowest_allocated_frame],eax

  pop   ebp
  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
  ret



  icod  ends



;-----------------------------------------------------------------------
;
;       init memory controller
;
;-----------------------------------------------------------------------
; PRECONDITION:
;
;       protected mode, paging not yet enabled
;
;       disable interrupt
;
;       DS : R/W 0..4GB
;       CS : X/R 0..4GB, USE32
;
;-----------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX...EBP scratch
;
;----------------------------------------------------------------------------
 

 assume ds:codseg
 
  icode


 
init_memctr:

  mov   edi,offset physical_kernel_info_page

  ;<cd> set end main memory to minimum of current and kernel memory size
  IFA   [edi+main_mem].mem_end,physical_kernel_mem_size
        mov   [edi+main_mem].mem_end,physical_kernel_mem_size
  FI

  ;<cd> x2 info page for second kernel
  sub edx,edx			
  ;lno___prc edx
  IF    kernel_x2		
        shl   edx,2
        test  edx,edx
        IFNZ
              call  generate_x2_info_page
        FI      
  ENDIF      

  ;<cd> set resevered memory area to correct size including:
  ;<cd> kernel debugger
  mov   eax,offset dcod_start
  mov   ecx,ds:[edi+kdebug_end]
  IFZ   ecx,<offset default_kdebug_end>
  CANDA ecx,eax
        mov   eax,ecx
  FI

  ;<cd> sigma 0
  mov   ecx,ds:[edi+sigma0_ktask].ktask_end
  IFZ   ecx,<offset default_sigma0_end>
  CANDA ecx,eax
        mov   eax,ecx
  FI

;  mov   ecx,ds:[edi+sigma1_ktask].ktask_end
;  IFZ   ecx,<offset default_sigma1_end>
;  CANDA ecx,eax
;        mov   eax,ecx
;  FI

  ;<cd> booter task
  mov   ecx,ds:[edi+booter_ktask].ktask_end
  IFZ   ecx,<offset ktest_end>
  CANDA ecx,eax
        mov   eax,ecx
  FI
  add   eax,pagesize-1
  and   eax,-pagesize
  
  mov   ecx,ds:[edi+reserved_mem0].mem_end
  test  ecx,ecx
  CORZ
  IFA   eax,ecx
        mov   ds:[edi+reserved_mem0].mem_end,eax
  FI

  ;<cd> set reserved memory 0 information in physical kernel info page
  mov   eax,offset physical_kernel_info_page
  test  ecx,ecx
  CORZ
  IFB_  eax,ds:[edi+reserved_mem0].mem_begin      
        mov   ds:[edi+reserved_mem0].mem_begin,eax
  FI      

  ;<cd> set physical regions to correct size
  mov   eax,ds:[edi+main_mem].mem_end

  mov   ch,kpage_mem_regions
  DO

        lea   esi,[edi+reserved_mem0]
        mov   cl,kpage_mem_regions
        DO
              IFAE  [esi].mem_end,eax
              CANDB [esi].mem_begin,eax
                    mov   eax,[esi].mem_begin
              FI
              add   esi,sizeof mem_descriptor
              dec   cl
              REPEATNZ
        OD

        dec   ch
        REPEATNZ
  OD                            

	
  ;<cd> set reserved memory 1 region
  mov   ds:[edi+reserved_mem1].mem_begin,eax
  IFB_  ds:[edi+reserved_mem1].mem_end,eax
        mov   ds:[edi+reserved_mem1].mem_end,eax
  FI

  ;<cd> set lowest allocated frame
  mov   [edx+lowest_allocated_frame],eax
  
  ;<cd>set physical frame number
  mov   eax,ds:[edi+main_mem].mem_end
  shr   eax,log2_pagesize
  mov   [phys_frames],eax

  ret

 




;-----------------------------------------------------------------------
;
;       init sigma 0 and 1
;
;
; PRECONDITION:
;
;       interrupts disabled
;
;-----------------------------------------------------------------------


 


xpush   macro reg

  ;<cd> push macro using ecx instead of esp
  sub   ecx,4
  mov   [ecx],reg

  endm






init_sigma0_1:

  ;<cd> read kernel task stack pointer of sigma0
  mov   ecx,ds:[logical_info_page+sigma0_ktask].ktask_stack

  ;<cd> push physical kernel info page address on stack
  mov   edi,offset physical_kernel_info_page
  xpush edi

  ;<cd> set new stack pointer of sigma0
  lea   ebx,ds:[logical_info_page+sigma0_ktask]
  mov   [ebx].ktask_stack,ecx

  ;<cd> create new kernel including task for sigma0
	;task id: sigma0 task in eax, pointer to logical info page task information in ebx
  mov   eax,sigma0_task
  call  create_kernel_including_task

  ;<cd> initialise sigma 0 space
  call  init_sigma0_space

  ;<cd> create task for sigma1
  mov   eax,sigma1_task
  lea   ebx,ds:[logical_info_page+offset sigma1_ktask]
  call  create_kernel_including_task

  ret



;############################################################################################

;-----------------------------------------------------------------------
;
;       init sigma 0 address space ptabs
;
;-----------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   addr of first available page
;
;----------------------------------------------------------------------------
 

init_sigma0_space:

  load__proot ebx,<((sigma0_task AND mask task_no) SHR task_no)>
  mov   ebx,dword ptr [ebx+PM]
  and   ebx,-pagesize
  mov   ecx,MB4/pagesize
  DO
        mov   eax,dword ptr [ebx+PM]
        test  eax,page_present
        IFNZ
              and   eax,-pagesize
              mov   esi,eax
              mov   dl,page_present+page_write_permit+page_user_permit
              call  map_ur_page_initially
        FI
        add   ebx,4
        dec   ecx
        REPEATNZ
  OD              
              
  sub   eax,eax
  mov   edi,ds:[logical_info_page+reserved_mem0].mem_begin
  call  map_ur_pages

  mov   eax,offset physical_kernel_info_page
  mov   esi,eax
  mov   dl,page_user_permit+page_present
  call  map_ur_page_initially

	;;   lno___prc eax
	sub eax,eax
    
  sub   edi,edi
  xchg  [eax*4+lowest_allocated_frame+PM],edi            ; turn off simple grabbing
  mov   ds:[logical_info_page+reserved_mem1].mem_begin,edi

  
  pushad
  mov   ebx,offset logical_info_page+dedicated_mem0
  DO
        mov   esi,[ebx].mem_begin
        mov   edi,[ebx].mem_end
        test  edi,edi
        IFNZ
              push  eax
              push  ebx
              mov   ebx,offset logical_info_page
              
           ;;   mov   eax,[ebx+reserved_mem0].mem_end
              mov   eax,KB64
              IFB_  esi,eax
                    mov   esi,eax
              FI      
              mov   eax,[ebx+reserved_mem1].mem_begin
              IFBE  esi,eax
              CANDA edi,eax
                    mov   edi,eax
              FI
              mov   eax,[ebx+main_mem].mem_end
              IFA   esi,eax
                    mov   esi,eax
              FI
              IFA   edi,eax
                    mov   edi,eax
              FI
              
              IFB_  esi,edi                  
                    mov   eax,esi
                    call  map_ur_pages
              FI
                    
              pop   ebx
              pop   eax
        FI
        add   ebx,sizeof mem_descriptor
        cmp   ebx,offset logical_info_page+dedicated_mem4
        REPEATBE
  OD
  popad                  

  mov   eax,ds:[logical_info_page+reserved_mem0].mem_end
  
  call  map_ur_pages


  mov   eax,ds:[logical_info_page+main_mem].mem_end
  add   eax,MB4-1
  and   eax,-MB4  
  DO                                           ; explicitly map free physical
        cmp   eax,2*GB1                        ;    mem beyond main mem 4M
        EXITAE                                 ;    aligned up to 2G
        mov   esi,eax
        mov   dl,superpage+page_user_permit+page_write_permit+page_present
        call  map_ur_page_initially
        add   eax,MB4
        REPEAT
  OD


                                               ; explicitly map 2...3 GB
  mov   esi,2*GB1                              ;    to physical 3...4 GB
  DO                                           ;    for devices
        lea   eax,[esi+GB1]                    
        mov   dl,superpage+page_user_permit+page_write_permit+page_present
        call  map_ur_page_initially
        add   esi,MB4
        cmp   esi,3*GB1
        REPEATB
  OD
        
  ret



map_ur_pages:
  mov   esi,eax
  DO
        mov   ebx,pagesize
        mov   dl,page_user_permit+page_write_permit+page_present
        bt    dword ptr ds:[cpu_feature_flags],page_size_extensions_bit
        IFC
        test  esi,MB4-1
        CANDZ
        test  eax,MB4-1
        CANDZ
        mov   ecx,edi
        sub   ecx,esi
        CANDAE ecx,MB4
              or    dl,superpage
              mov   ebx,MB4
        FI

        call  map_ur_page_initially
        add   eax,ebx
        add   esi,ebx
        cmp   esi,edi
        REPEATB
  OD

  ret


  icod  ends



  code ends
  end
