include l4pre.inc 
               
  scode                         

  Copyright GMD, L4.sigma0  , 20,04,97, 200

 
;*********************************************************************
;******                                                         ******
;******    Sigma 0   (Initial Address Space)                    ******
;******                                                         ******
;******    special RT version             Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  24.09.95    ******
;******                                   modified: 19.07.96    ******
;******                                                         ******
;*********************************************************************
 

include rtcache.inc
 
  public default_sigma0_start
  public default_sigma0_stack
  public default_sigma0_stack2
  public default_sigma0_begin
  public default_sigma0_end



.nolist 
include l4const.inc
include uid.inc
include adrspace.inc
include intrifc.inc
include tcb.inc
include msg.inc
include msgmac.inc
include cpucb.inc
include schedcb.inc
include lbmac.inc
include syscalls.inc
include pagconst.inc
include l4kd.inc
include kpage.inc
.list

 

 
 
        align 16







  align 16
  
default_sigma0_begin  equ $


                          dd    31 dup (0)
default_sigma0_stack      dd    0
                          dd    31 dup (0)
default_sigma0_stack2     dd    0


sigma0_data struc

  kernel_info_addr        dd    0
  recommended_kernel_pages dd   0

  memmap_descriptor       dd    0,0
  memmap_size             dd    0

  requestors              dd    4 dup (0)
  
sigma0_data ends  



free_mem            equ   0
dedicated_mem       equ   80h
reserved_mem        equ   0FFh





  

;----------------------------------------------------------------------------
;
;
;----------------------------------------------------------------------------

  assume ds:codseg



default_sigma0_start:

  pop   eax

  sub   esp,sizeof sigma0_data
  mov   ebp,esp

  mov   [ebp+kernel_info_addr],eax

  mov   eax,[eax+reserved_mem1].mem_begin
  IF    kernel_x2
        imul  eax,3
        shr   eax,1
  ENDIF      
  shr   eax,3+log2_pagesize
 ;; shr   eax,2+log2_pagesize  
  mov   [ebp+recommended_kernel_pages],eax


  call  init_mem_maps

  sub   eax,eax
  dec   eax
  
  

  sub   esi,esi
  sub   eax,eax

  DO
        push  ebp
        sub   ecx,ecx
        mov   edi,esi
        mov   esi,waiting_any
        int   ipc3
        pop   ebp
        
        cmp   eax,ipc_control_mask
        mov   eax,esi
        REPEATA
        
        and   eax,mask task_no
        and   edx,NOT 10b
        
        ror   eax,task_no-8
        
        cmp   eax,dedicated_mem SHL 8
        REPEATAE
        
        test  edx,01b
        IFZ
              IFNZ  edx,0FFFFFFFCh

                    call  grab_specific_page
                    REPEAT

              ELIFZ ah,kernel_task_no
              
                    call  grab_free_default_page
                    add   ebx,fpage_grant - fpage_map_read_write
                    REPEAT
                    
              ELSE_      
                    call  grab_free_default_page
                    REPEAT
              FI
        FI

        IFZ   bl,0
              mov   edx,[ebp+recommended_kernel_pages]
              REPEAT
        FI

        IFZ   bl,1
              mov   ebx,[ebp+kernel_info_addr]
              add   ebx,log2_pagesize*4 + fpage_map_read_only
           mov edx,ebx ;;;;;;;;;;;;
              mov   eax,map_msg
              REPEAT
        FI

        IFZ   bl,22*4

              call  grab_specific_4M_or_4K_page
              REPEAT
        FI

        IFZ   bl,2
              call  unmap_4K_page
              REPEAT
        FI
                      
        call  reply_nak
        REPEAT  
  OD



reply_nak:

  test  ebx,ebx
  IFNZ
        ke    <0E5h,'0_ill_rpc'>
  FI
  sub   edx,edx
  sub   eax,eax
  ret
        


;----------------------------------------------------------------------------
;
;       grab specific page
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EDX   page address
;       ESI   requester id low
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       Z:    EBX   grabbed fpage (map_read_write)
;
;       NZ:   page not available
;
;----------------------------------------------------------------------------


grab_specific_page:

  cmp   edx,GB1
  xc    ae,perhaps_special_device_4M_frame,long

  and   edx,-pagesize
  
  push  edx
 
  IFAE  esi,booter_task
  CANDB esi,<booter_task+(1 SHL task_no)>
  CANDA edx,KB64

        IF    partitioning_strategy EQ unpartitioned
        
              mov   edi,edx
              and   edi,-MB16
              and   edx,(KB128-1) AND -pagesize
              shr   edi,24-17
              lea   edx,[edi+edx+MB1]
              
        ELSE      
              
              mov   edi,edx
              shr   edi,24
              and   edi,L2_cache_colors/2-1
              add   edi,L2_cache_colors/2
              IFAE  esi,sigma2_task
                    add   edi,L2_cache_colors/4
              FI
              and   edx,(KB128-1) AND -pagesize
              imul  edx,L2_cache_colors
              shl   edi,log2_pagesize
              lea   edx,[edi+edx+MB1]
              
        ENDIF      
        
  ELIFAE esi,sigma2_task
  CANDB esi,<sigma2_task+(1 SHL task_no)>
  CANDA edx,KB64
                               
        IF partitioning_strategy EQ partitioned
              test  edx,(L2_cache_colors/2)*pagesize
              IFNZ
                    add   edx,KB512-(L2_cache_colors/2)*pagesize
              FI      
        ENDIF      
  FI      
                               
  
  
  
        
  
  mov   ebx,[ebp+memmap_descriptor].mem_begin
  mov   edi,edx
  
  shr   edi,log2_pagesize
  mov   ecx,[ebp+memmap_size]  
  
  IFB_  edi,ecx

        mov   al,[edi+ebx]
        
        CORZ  al,dedicated_mem
        CORZ  al,ah
        IFZ   al,free_mem
              
              mov   [edi+ebx],ah
              
              lea   ebx,[edx+log2_pagesize*4 + fpage_map_read_write]
              pop   edx
              
              mov   eax,map_msg
              ret
        FI
  FI
  
  pop   edx
  call  reply_nak
  ret




XHEAD perhaps_special_device_4M_frame
  
  cmp   edx,3*GB1
  xret  ae  ,long
        
  mov   ebx,edx
  and   ebx,-1 SHL 22
  add   ebx,22*4 + fpage_map_read_write

  mov   eax,map_msg
  ret



grab_specific_4M_or_4K_page:

  mov   edi,edx
  and   edi,-MB4
  shr   edi,log2_pagesize
  
  mov   ecx,[ebp+memmap_size]
  sub   ecx,edi
  
  cmp   ecx,MB4/pagesize
  jb    grab_specific_page
  

  add   edi,[ebp+memmap_descriptor].mem_begin

  add   ah,free_mem
  mov   ecx,MB4/pagesize
  DO
        mov   al,[edi]
        CORZ  al,dedicated_mem
        CORZ  al,ah
        IFZ   al,free_mem
              inc   edi
              dec   ecx
              REPEATNZ
        FI
  OD                    
  jnz   grab_specific_page
  
  
  mov   ecx,MB4/pagesize
  sub   edi,ecx
  mov   al,ah
  cld
  rep   stosb              
        
  mov   ebx,edx
  and   ebx,-MB4
  add   ebx,22*4 + fpage_map_read_write

  mov   eax,map_msg
  ret




;----------------------------------------------------------------------------
;
;       grab free default page
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   requester id low
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       Z:     EDX   grabbed page address
;              EBX   fpage (map_read_write)
;
;       NZ:    no more page available
;
;----------------------------------------------------------------------------



grab_free_default_page:

  mov   edi,[ebp+memmap_descriptor].mem_begin
  mov   ecx,[ebp+memmap_size]
  add   edi,ecx
  dec   edi

  DO
        mov   al,free_mem
        test  esp,esp
        std
        repne scasb

        IFZ
              lea   edx,[edi+1]
              sub   edx,[ebp+memmap_descriptor].mem_begin
              and   edx,L2_cache_colors-1
              cmp   edx,L2_cache_colors/2
              REPEATAE
              
              inc   edi
              mov   edx,edi
              or    [edi],ah
              sub   edx,[ebp+memmap_descriptor].mem_begin
              shl   edx,log2_pagesize

              mov   ebx,edx
              add   ebx,log2_pagesize*4 + fpage_map_read_write

              mov   eax,map_msg
              ret
        FI
  OD        

  call  reply_nak
  ret




;----------------------------------------------------------------------------
;
;       unmap 4K page
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EDX   page address
;       ESI   requester id low
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX   0
;
;----------------------------------------------------------------------------


unmap_4K_page:

  mov   ebx,[ebp+memmap_descriptor].mem_begin
  mov   edi,edx
  
  shr   edi,log2_pagesize
  mov   ecx,[ebp+memmap_size]  
  
  IFB_  edi,ecx

        and   edx,-pagesize
        mov   al,[edi+ebx]
        
        IFZ   al,ah
              
            ;  mov   byte ptr [edi+ebx],free_mem

              pushad              
              mov   eax,edx
              and   eax,-pagesize
              add   eax,12*4+2
              mov   cl,0FFh
              int   fpage_unmap
              popad
        FI
  FI            
              
  sub   eax,eax
  ret




;----------------------------------------------------------------------------
;
;       init mem maps
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       memmap initialized
;
;       REGs  scratch
;
;----------------------------------------------------------------------------



init_mem_maps:

  mov   edi,[ebp+kernel_info_addr]

  mov   eax,[edi+reserved_mem1].mem_begin
  mov   [ebp+memmap_descriptor].mem_end,eax

  mov   ecx,[edi+main_mem].mem_end
  shr   ecx,log2_pagesize
  mov   [ebp+memmap_size],ecx

  sub   eax,ecx
  and   eax,-pagesize
  mov   [ebp+memmap_descriptor].mem_begin,eax

  lea   esi,[edi+main_mem]
  mov   ebx,[esi].mem_begin
  mov   edx,[esi].mem_end
  mov   al,free_mem
  mov   ah,1
  call  fill_mem_map

  lea   esi,[ebp+memmap_descriptor]
  mov   al,reserved_mem
  mov   ah,1
  call  fill_mem_map

  lea   esi,[edi+reserved_mem0]
  mov   al,reserved_mem
  mov   ah,2
  call  fill_mem_map

  lea   esi,[edi+dedicated_mem0]
  mov   al,dedicated_mem
  mov   ah,5
  call  fill_mem_map

  ret




fill_mem_map:

  push  edi
  DO
        mov   ecx,[esi].mem_end
        IFA   ecx,edx
              mov   ecx,edx
        FI
        mov   edi,[esi].mem_begin
        IFB_  edi,ebx
              mov   edi,ebx
        FI      
        shr   ecx,log2_pagesize
        shr   edi,log2_pagesize
        sub   ecx,edi
        IFA
              add   edi,[ebp+memmap_descriptor].mem_begin
              cld
              rep   stosb
        FI
        add   esi,sizeof mem_descriptor
        dec   ah
        REPEATNZ
  OD
  pop   edi
  ret



;----------------------------------------------------------------------------
;
;       memory test
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBX   lower bound
;       ECX   upper bound
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       Z:    no memory failure detected
;
;       NZ:   EAX   address of detected failure
;
;----------------------------------------------------------------------------

  assume ds:codseg




check_pass_string   db sizeof check_pass_text
check_pass_text     db 6,10,10,0E5h,'0 memory test ','0'-1,': pass X-X'

check_no_offset     equ (sizeof check_pass_text-11+1)
pass_type_offset    equ (sizeof check_pass_text-3+1)
pass_no_offset      equ (sizeof check_pass_text-1+1)



memory_test:

  push  ecx
  push  edx
  push  esi
  push  edi

  sub   ecx,ebx
  IFA   ,,long

        inc   ds:[check_pass_string+check_no_offset]

        mov   eax,ecx
        sub   edx,edx
        mov   ecx,3*4*8
        div   ecx
        mov   ecx,eax
        mov   dl,'1'
        mov   dh,dl

        DO
              mov   eax,055555555h
              lea   edi,[ebx+1*4]
              call  memtest_wr

              mov   eax,055555555h
              lea   edi,[ebx+2*4]
              call  memtest_wr

              mov   eax,0AAAAAAAAh
              lea   edi,[ebx+0*4]
              call  memtest_wr

              mov   eax,055555555h
              lea   edi,[ebx+1*4]
              call  memtest_rd
              EXITNZ

              mov   eax,0AAAAAAAAh
              lea   edi,[ebx+1*4]
              call  memtest_wr

              mov   eax,055555555h
              lea   edi,[ebx+2*4]
              call  memtest_rd
              EXITNZ

              mov   eax,0AAAAAAAAh
              lea   edi,[ebx+0*4]
              call  memtest_rd
              EXITNZ

              mov   eax,0AAAAAAAAh
              lea   edi,[ebx+1*4]
              call  memtest_rd
              EXITNZ

              mov   eax,055555555h
              lea   edi,[ebx+0*4]
              call  memtest_wr

              mov   eax,055555555h
              lea   edi,[ebx+0*4]
              call  memtest_rd
              EXITNZ

              mov   eax,0AAAAAAAAh
              lea   edi,[ebx+2*4]
              call  memtest_wr

              mov   eax,0AAAAAAAAh
              lea   edi,[ebx+2*4]
              call  memtest_rd
        OD
  FI

  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  ret


memtest_wr:

  pushad
  mov   eax,offset check_pass_string
  mov   byte ptr [eax+pass_type_offset],'W'
  mov   [eax++pass_no_offset],dl
  kd____outstring
  popad
  inc   dl

  mov   esi,ecx
  clign 16
  DO
        mov   [edi],eax
        mov   [edi+1*3*4],eax
        mov   [edi+2*3*4],eax
        mov   [edi+3*3*4],eax
        mov   [edi+4*3*4],eax
        mov   [edi+5*3*4],eax
        mov   [edi+6*3*4],eax
        mov   [edi+7*3*4],eax

        add   edi,8*3*4
        dec   esi
        REPEATNZ
  OD
  ret



memtest_rd:

  pushad                                                        ;Ž90-09-15
  mov   eax,offset check_pass_string
  mov   byte ptr [eax+pass_type_offset],'R'
  mov   [eax++pass_no_offset],dh
  kd____outstring
  popad                                                         ;...Ž
  inc   dh

  mov   esi,ecx
  clign 16
  DO
        cmp   [edi],eax
        EXITNZ
        cmp   [edi+1*3*4],eax
        EXITNZ
        cmp   [edi+2*3*4],eax
        EXITNZ
        cmp   [edi+3*3*4],eax
        EXITNZ
        cmp   [edi+4*3*4],eax
        EXITNZ
        cmp   [edi+5*3*4],eax
        EXITNZ
        cmp   [edi+6*3*4],eax
        EXITNZ
        cmp   [edi+7*3*4],eax
        EXITNZ

        add   edi,8*3*4
        dec   esi
        REPEATNZ

        ret
  OD

  mov   eax,edi
  ret


default_sigma0_end  equ $


  scod ends
  code ends
  end
