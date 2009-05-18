include l4pre.inc 

  Copyright GMD, L4.MEMCTR, 30,05,96, 20
 
 
;*********************************************************************
;******                                                         ******
;******         Memory Controller                               ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  05.03.91    ******
;******                                   modified: 30.05.96    ******
;******                                                         ******
;*********************************************************************
 
  public init_memctr
  public init_sigma0_1
  public grab_frame
  public initial_grab_mem
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
include intrifc.inc
include tcb.inc
include pagconst.inc
include pagmac.inc
include syscalls.inc
include msg.inc
include kpage.inc
.list


ok_for i486,pentium,ppro,k6


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

  pushad

  sub   edx,edx
  IFAE  esp,<offset tcb_space>
  CANDB esp,<offset tcb_space+tcb_space_size>
        add   edx,PM
  FI
  IF    kernel_x2
        lno___prc eax
        lea   edx,[eax*4+edx]
  ENDIF      
  mov   eax,[edx+lowest_allocated_frame]

  test  eax,eax
  jnz   initial_grab_frame

  sub   eax,eax
  mov   ecx,eax
  lea   edx,[eax-2]              ; w0 = FFFFFFFE

  log2  <%physical_kernel_mem_size>

  mov   ebp,log2_*4+map_msg

  mov   esi,sigma0_task
  sub   edi,edi

  int   ipc

  test  al,ipc_error_mask
  CORNZ
  test  al,map_msg
  CORZ
  test  bl,fpage_grant
  CORZ
  shr   bl,2
  IFNZ  bl,log2_pagesize

        ke    <'-',0E5h,'0_err'>
  FI

  and   edx,-pagesize
  
  mov   [esp+7*4],edx
  popad
  ret



  icode

;include rtcache.inc
;
;


initial_grab_frame:

  popad
  mov   eax,pagesize


; DO
;  mov   eax,pagesize
;  call   initial_grab_mem
;  test   eax,L2_cache_colors/2*pagesize
;  REPEATNZ
; OD 
; ret 

 
  
initial_grab_mem:

  pushad

  sub   edx,edx
  IFAE  esp,<offset tcb_space>
  CANDB esp,<offset tcb_space+tcb_space_size>
        add   edx,PM
  FI
  IF    kernel_x2
        lno___prc ecx
        lea   edx,[ecx*4+edx]
  ENDIF      
  mov   ecx,[edx+lowest_allocated_frame]
  
  add   eax,pagesize-1
  and   eax,-pagesize
  
  sub   ecx,eax
  CORC
  IFB_  ecx,MB1
        ke    '-memory_underflow'
  FI

  mov   [edx+lowest_allocated_frame],ecx

  mov   [esp+7*4],ecx
  popad
 
  
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
  
  lno___prc edx
  IF    kernel_x2
        shl   edx,2
        test  edx,edx
        IFNZ
              call  generate_x2_info_page
        FI      
  ENDIF      

  mov   eax,offset dcod_start
  mov   ecx,ds:[edi+kdebug_end]
  IFZ   ecx,<offset default_kdebug_end>
  CANDA ecx,eax
        mov   eax,ecx
  FI
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
  mov   eax,offset physical_kernel_info_page
  test  ecx,ecx
  CORZ
  IFB_  eax,ds:[edi+reserved_mem0].mem_begin      
        mov   ds:[edi+reserved_mem0].mem_begin,eax
  FI      


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

  mov   ds:[edi+reserved_mem1].mem_begin,eax
  IFB_  ds:[edi+reserved_mem1].mem_end,eax
        mov   ds:[edi+reserved_mem1].mem_end,eax
  FI
  
  mov   [edx+lowest_allocated_frame],eax
  
  

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

  sub   ecx,4
  mov   [ecx],reg

  endm






init_sigma0_1:

  mov   ecx,ds:[logical_info_page+sigma0_ktask].ktask_stack

  mov   edi,offset physical_kernel_info_page
  xpush edi

  lea   ebx,ds:[logical_info_page+sigma0_ktask]
  mov   [ebx].ktask_stack,ecx

  mov   eax,sigma0_task
  call  create_kernel_including_task
  
  call  init_sigma0_space

  mov   eax,sigma1_task
  lea   ebx,ds:[logical_info_page+offset sigma1_ktask]
  call  create_kernel_including_task

  ret





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

  mov   ebx,ds:[((sigma0_task AND mask task_no) + offset tcb_space)].task_pdir
  mov   ebx,dword ptr [ebx+PM]
  and   ebx,-pagesize
  mov   ecx,MB4/pagesize
  DO
        mov   eax,dword ptr [ebx+PM]
        test  al,page_present
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
  IFNZ  eax,edi
        call  map_ur_pages
  FI      

  mov   eax,offset physical_kernel_info_page
  mov   esi,eax
  IF    kernel_x2
        lno___prc edi
        test  edi,edi
        IFNZ
              mov   eax,ds:[eax+next_kpage_link]
        FI
  ENDIF            
  mov   dl,page_user_permit+page_present
  call  map_ur_page_initially

  lno___prc eax
    
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
              
              mov   eax,[ebx+reserved_mem0].mem_end
           ;;;;;;;;;;;;;   mov   eax,KB64
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
        IF    kernel_type NE i486
              test  esi,MB4-1
              IFZ
              test  eax,MB4-1
              CANDZ
              mov   ecx,edi
              sub   ecx,esi
              CANDAE ecx,MB4
                    or    dl,superpage
                    mov   ebx,MB4
              FI
        ENDIF                      

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
