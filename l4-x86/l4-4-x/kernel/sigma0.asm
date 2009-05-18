include l4pre.inc 

  scode

  Copyright IBM, L4.sigma0  , 12,12,97, 12

 
;*********************************************************************
;******                                                         ******
;******    Sigma 0   (Initial Address Space)                    ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 12.12.97    ******
;******                                                         ******
;*********************************************************************
 

 
  public default_sigma0_start
  public default_sigma0_stack
  public default_sigma0_stack2
  public default_sigma0_begin
  public default_sigma0_end



.nolist 
include l4const.inc
include uid.inc
include adrspace.inc
include tcb.inc
include msg.inc
include intrifc.inc
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

  device_mem_begin        dd    0
  
  memmap_descriptor       dd    0,0
  memmap_size             dd    0

  requestors              dw    6 dup (0)
  
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

  pop   ebx

  sub   esp,sizeof sigma0_data
  mov   ebp,esp

  mov   [ebp+kernel_info_addr],ebx
  
  mov   ecx,[ebx+main_mem].mem_end 
  add   ecx,MB4-1
  and   ecx,-MB4
  mov   [ebp+device_mem_begin],ecx

  mov   eax,[ebx+reserved_mem1].mem_begin
  IF    kernel_x2
        imul  eax,3
        shr   eax,1
  ENDIF      
  shr   eax,log2_pagesize
  add   eax,MB4/pagesize-1
  shr   eax,22-log2_pagesize
  movzx ecx,[ebx].ptabs_per_4M
  test  ecx,ecx
  IFZ   
        mov   ecx,128
  FI      
  imul  eax,ecx
  
 ;; shr   eax,7+log2_pagesize
 ;; shr   eax,3+log2_pagesize
 ;; shr   eax,2+log2_pagesize  
  mov   [ebp+recommended_kernel_pages],eax


  call  init_mem_maps


;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
;
;  pushad
;  
;  extrn ide_start:near
;  extrn ide_stack:dword
;  
;  mov   eax,(sigma0_disk_driver AND mask lthread_no) SHR lthread_no
;  mov   ecx,offset ide_stack
;  mov   edx,offset ide_start
;  sub   ebx,ebx
;  sub   ebp,ebp
;  sub   esi,esi
;  sub   edi,edi
;  int   lthread_ex_regs
;  
;  popad
;
;
;
;>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  



  sub   eax,eax
  dec   eax


  DO
	      push  ebp
        mov   ecx,10h
        mov   ebp,open_receive
        int   ipc
	      pop   ebp
        
        push ds
        pop  es   

        test  al,ipc_error_mask
        mov   eax,-1
        REPEATNZ

        sub   eax,eax

        and   dl,NOT 10b
        test  dl,01b
        IFZ
              IFNZ  edx,0FFFFFFFCh

                    call  grab_specific_page
                    jnz   short reply_nak

                    add___eax map_msg
                    REPEAT

              ELSE_
                    call  grab_free_default_page
                    jnz   short reply_nak

                    add___eax map_msg

                    lno___task ecx,esi
                    cmp   ecx,kernel_task_no
                    REPEATNZ

                    add   ebx,fpage_grant - fpage_map_read_write
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
              add___eax map_msg
              REPEAT
        FI

        IFZ   bl,22*4

              call  grab_specific_4M_or_4K_page
              jnz   short reply_nak

              add___eax map_msg
              REPEAT
        FI
        

  reply_nak:

        test  ebx,ebx
        IFNZ
              ke    <0E5h,'0_ill_rpc'>
        FI
        sub   edx,edx
        sub   eax,eax
        REPEAT
  OD


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

  push  eax
  push  ecx
  push  edi

  call  identify_requestor

  IFZ
        mov   edi,[ebp+memmap_descriptor].mem_begin
        mov   ecx,[ebp+memmap_size]
        add   edi,ecx
        dec   edi

        mov   al,free_mem
        test  esp,esp
        std
        repne scasb

        IFZ
              inc   edi
              mov   edx,edi
              or    [edi],ah
              sub   edx,[ebp+memmap_descriptor].mem_begin
              shl   edx,log2_pagesize

              mov   ebx,edx
              mov   bl,log2_pagesize*4 + fpage_map_read_write

              cmp   eax,eax
        FI
  FI

  pop   edi
  pop   ecx
  pop   eax
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


grab_specific_4M_or_4K_page:

  push  eax
  push  ecx
  push  edi
  
  mov   edi,edx
  and   edi,-MB4
  shr   edi,log2_pagesize
  
  mov   ecx,[ebp+memmap_size]
  sub   ecx,edi
  
  IFAE  ecx,MB4/pagesize

  call  identify_requestor

  CANDZ

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
  
  CANDZ
  
        mov   ecx,MB4/pagesize
        sub   edi,ecx
        mov   al,ah
        cld
        rep   stosb              
              
        mov   ebx,edx
        and   ebx,-MB4
        mov   bl,22*4 + fpage_map_read_write

        cmp   eax,eax     ; Z !
        pop   edi
        pop   ecx
        pop   eax
        ret
  FI

  pop   edi
  pop   ecx
  pop   eax






grab_specific_page:


  IFAE  edx,[ebp+device_mem_begin]                     ;;;;;;   GB1
  CANDB edx,3*GB1

        mov   ebx,edx
        and   ebx,-1 SHL 22
        mov   bl,22*4 + fpage_map_read_write

        cmp   eax,eax
        ret
  FI


  push  eax
  push  edi

  mov   edi,edx
  shr   edi,log2_pagesize

  IFB_  edi,[ebp+memmap_size]

  call  identify_requestor

  CANDZ

        add   edi,[ebp+memmap_descriptor].mem_begin

        add   ah,free_mem
        mov   al,[edi]
        CORZ  al,dedicated_mem
        CORZ  al,ah
        IFZ   al,free_mem
              mov   [edi],ah

              mov   ebx,edx
              and   ebx,-pagesize
              mov   bl,log2_pagesize*4 + fpage_map_read_write

              cmp   eax,eax     ; Z !
              pop   edi
              pop   eax
              ret
        FI
  FI

  test  esp,esp     ; NZ !
  pop   edi
  pop   eax
  ret



;----------------------------------------------------------------------------
;
;       identify_requestor
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   requester id low
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       Z:    AH    requestor no
;       NZ:   too many requestors
;
;----------------------------------------------------------------------------



identify_requestor:

  push  ecx
  push  esi
  push  edi

  lno___task esi

  lea   edi,[ebp+requestors]
  mov   ah,1
  DO
        movzx ecx,word ptr [edi]
        cmp   ecx,esi
        EXITZ
        test  ecx,ecx
        EXITZ

        add   edi,2
        inc   ah
        cmp   ah,sizeof requestors/2
        REPEATBE
                          ; NZ !
  OD
  IFZ
        mov   [edi],si
  FI

  pop   edi
  pop   esi
  pop   ecx
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

  pushad                                                       
  mov   eax,offset check_pass_string
  mov   byte ptr [eax+pass_type_offset],'R'
  mov   [eax++pass_no_offset],dh
  kd____outstring
  popad                                                        
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
