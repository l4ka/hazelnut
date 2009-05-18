include l4pre.inc 
   
    				   
  Copyright IBM, L4.ADRSMAN.5, 10,04,00, 9026
   
;*********************************************************************
;******                                                         ******
;******         Address Space Manager                           ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 10.10.00    ******
;******                                                         ******
;*********************************************************************
 

      
  public init_adrsman
  public create_kernel_including_task
  public set_address_space_data_for_all_threads
  public set_proot_for_all_threads
  public init_small_address_spaces
  public attach_small_space
  public get_small_space
  public make_own_address_space_large
  public set_small_pde_block_in_pdir


  extrn create_thread:near
  extrn delete_thread:near
  extrn insert_into_fresh_frame_pool:near
  extrn flush_address_space:near
  extrn gen_kernel_including_address_space:near
  extrn define_idt_gate:near
  extrn ipc_update_small_space_size:near


.nolist
include l4const.inc
include uid.inc
include adrspace.inc
include tcb.inc
include cpucb.inc
include intrifc.inc
include pagconst.inc
include pagmac.inc
include schedcb.inc
include syscalls.inc
include kpage.inc
include pagcb.inc
.list



ok_for x86,pIII



  assume ds:codseg






;----------------------------------------------------------------------------
;
;       init address space manager
;
;----------------------------------------------------------------------------


  icode



init_adrsman:

  mov   eax,kernel_task         ; ensuring that first ptab for pdir space
  lea___pdir eax,eax            ; becomes allocated before task creation
  mov   eax,[eax]               ;
  
  mov   bh,3 SHL 5

  mov   bl,task_new
  mov   eax,offset task_new_sc+KR
  call  define_idt_gate

  mov   edi,offset redirection_table              ;REDIR
  mov   ecx,tasks*tasks                           ;
  movi  eax,ipc_transparent                       ;
  cld                                             ;
  rep   stosd                                     ;
	

  ret


  icod  ends




;----------------------------------------------------------------------------
;
;       task new sc
;
;       delete/create task (incl. creation of lthread 0 of new task
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX      new chief / mcp
;       ECX      initial ESP                             of lthread 0
;       EDX      initial EIP                             of lthread 0
;       EBX      pager
;       ESI      task id
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       ESI      new task id  /   0
;
;       ECX,EDX,ESI,EDI,EBP scratch
;
;       task created,
;       lthread 0 created and started at PL3 with:
;
;                   EAX...EBP            0
;                   ESP                  initial ESP
;                   EIP                  initial EIP
;                   DS...GS              linear space
;                   CS                   linear space exec
;
;----------------------------------------------------------------------------


task_new_failed:

  ke    'tfail'

  sub   esi,esi
  sub   edi,edi

  add   esp,3*4
  iretd
  
  



task_new_sc:

  tpre  trap2,ds,es

  and   esi,NOT mask lthread_no

  mov   ebp,esp
  and   ebp,-sizeof tcb

  IFZ   ebx,1,long                                      ;REDIR begin --------------------------
                                                        ;
        mov   ebx,[ebp+myself]                          ;
        lno___task ebx                                  ;
                                                        ;
        mov   eax,esi                                   ;
        call  is_ruled_by                               ;
        IFZ                                             ;
        mov   eax,edx                                   ;
        call  is_ruled_by_or_is_myself                  ;
        CANDZ                                           ;
              CORZ  ecx,ipc_transparent                 ;
              CORZ  ecx,ipc_inhibited                   ;
              mov   eax,ecx                             ;
              call  is_ruled_by_or_is_myself            ;
              IFZ                                       ;
                    mov   ebp,esi                       ;
                    and   ebp,mask task_no              ;
                    shr   ebp,task_no-log2_tasks-2      ;
                    lno___task edx                      ;
                    mov   [edx*4+ebp+redirection_table],ecx;
                                                        ;
                    tpost ,ds,es                        ;
              FI                                        ;
        FI                                              ;
        sub   esi,esi                                   ;
        tpost ,ds,es                                    ;
                                                        ;
  FI                                                    ;REDIR ends --------------------------


  push  esi
  push  ebx

  mov   ebx,[ebp+myself]
  shl	ebx,chief_no-task_no
  xor	ebx,esi
  test  ebx,mask chief_no
  CORNZ
  IFA   [ebp+clan_depth],max_clan_depth
  	jmp   task_new_failed
  FI

  lea___tcb ebx,esi
  test__page_present ebx
  IFNC  ,,long
  CANDNZ [ebx+coarse_state],unused_tcb,long

        xor   esi,[ebx+myself]
        test  esi,NOT mask ver
        jnz   task_new_failed


        pushad                                  ;-------------------------
                                                ;
                                                ; delete task
                                                ;
        mov   edi,ebp                           ; ;REDIR begin ---------------------------
        and   edi,mask task_no                  ; ;
        shr   edi,task_no-log2_tasks-2          ; ;
        add   edi,offset redirection_table      ; ;
        movi  eax,ipc_transparent               ; ;
        mov   ecx,tasks                         ; ;
        cld                                     ; ;
        rep   stosd                             ; ;REDIR ends ----------------------------
                                                ;
                                                ;
        mov   eax,[ebp+thread_proot]            ;
        mov   ds:[cpu_cr3],eax                  ;   If deleter has small address space,
        mov   dword ptr ds:[tlb_invalidated],eax;   it might execute inside to be deleted
        mov   cr3,eax                           ;   pdir. Avoided by explicitly switching 
                                                ;   to deleter's pdir.
                                                ;
        sub   edi,edi                           ;
        mov   ecx,lthreads                      ;
        DO                                      ;
              test__page_present ebx            ;
              IFNC                              ;
                    mov   ebp,ebx               ;
                    test  edi,edi               ;
                    IFZ                         ;
                          mov   edi,[ebp+thread_proot]
                          call  detach_associated_small_space
                    FI                          ;
                    call  delete_thread         ;
              FI                                ;
              add   ebx,sizeof tcb              ;
              dec   ecx                         ;
              REPEATNZ                          ;
        OD                                      ;
                                                ;
        call  flush_address_space               ;
                                                ;
       	IFNZ  edi,ds:[empty_proot]		    ;
       	CANDNZ edi,ds:[kernel_proot]		    ;
								    ;
        	  add   edi,PM                    ;
        	  mov   ecx,virtual_space_size SHR 22;
        	  DO                              ;
                      sub eax,eax               ;
                      cld                       ;
                      repe  scasd               ;
                      EXITZ                     ;
                                                ;
                      mov   eax,[edi-4]         ;
                      call  insert_into_fresh_frame_pool;
                      REPEAT                    ;
        	  OD                              ;
                                                ;
                lea   eax,[edi-PM]              ;
                call  insert_into_fresh_frame_pool;
                                                ;
	      FI						    ;
        popad                                   ;--------------------------


  ELSE_

        push  eax
        push  ecx

        lno___task ecx,ebx
        lno___task eax,ebp
        add   eax,ds:[empty_proot]
        cmp   eax,[(ecx*4)+task_root]

        pop   ecx
        pop   eax
        jnz   task_new_failed
  FI

 
  IFZ   <dword ptr [esp]>,0

        and   eax,mask task_no
        shl	eax,chief_no-task_no
        and   esi,NOT mask chief_no
        or    esi,eax

        lno___task ebx
        shr   eax,chief_no
        add   eax,ds:[empty_proot]
        store_inactive_root eax,ebx

        add   esp,3*4
        push  linear_space
        pop   ds
        push  linear_space
        pop   es
        iretd
  FI


  IFA   al,[ebp+max_controlled_prio]
        mov   al,[ebp+max_controlled_prio]
  FI
  shl   eax,16
  mov   ah,[ebp+prio]
  mov   al,[ebp+timeslice]

  lno___task edi,ebx
  mov   esi,ds:[empty_proot]
  store_root esi,edi

  pop   esi

  xchg  ebp,ebx
  push  ebx
  call  create_thread
  pop   ebx

  pop   esi

	mov	eax,[ebx+myself]
  and	eax,mask task_no
  shl	eax,chief_no-task_no
  or	esi,eax
  mov	[ebp+myself],esi
  
  IFNZ  eax,root_chief_no SHL chief_no
        inc   [ebp+clan_depth]
  FI

  tpost eax,ds,es







is_ruled_by_or_is_myself:                               ;REDIR begin --------------------------------
                                                        ;
  lno___task edi,eax                                    ;
  IFZ   edi,ebx                                         ;
        ret                                             ;
  FI                                                    ;
                                                        ;
                                                        ;
                                                        ;
                                                        ;
is_ruled_by:                                            ;
                                ; EAX checked task      ;
                                ; EBX mytask no         ;
                                ; Z:  yes               ;
                                ; EAX,EDI,EBP scratch   ;
                                                        ;
  and   eax,NOT mask lthread_no ; always check lthread 0;
                                                        ;
  lno___task edi,eax
  lea___tcb  ebp,eax                                    ;

  mov   edi,[edi*4+task_root]
  sub   edi,ds:[empty_proot]
  CORC
  IFAE  edi,KB4
        test__page_present ebp                          ;
        IFNC                                            ;
              mov   edi,[ebp+myself]                    ;
              shr   edi,chief_no                        ;
        FI                                              ;
  FI                                                    ;
                                                        ;
  cmp   edi,ebx                                         ;
  ret                                                   ;
                                                        ;REDIR ends -------------------------------
                                                        






  icode




create_kernel_including_task:

	IFNZ	[ebx].ktask_stack,0

        lea___tcb ebp,eax

        call  gen_kernel_including_address_space

        lno___task ecx,ebp
        store_root eax,ecx 

        mov   ecx,[ebx].ktask_stack
        mov   edx,[ebx].ktask_start
        mov   eax,(255 SHL 16) + (16 SHL 8) + 10
        mov   esi,sigma0_task

        call  create_thread

  FI       

  ret



  icod  ends







;----------------------------------------------------------------------------
;
;       set address space / small space data for all threads
;
;----------------------------------------------------------------------------
; set address space PRECONDITION:
;
;       EAX   task pdir
;       EBX   as base
;       ECX   as size
;       EBP   tcb of task
;
;       DS    linear kernel space
;
;       paging enabled
;
;----------------------------------------------------------------------------
; set small space PRECONDITION:
;
;       EAX   as number
;       EBP   tcb of task
;
;       DS    linear kernel space
;
;       paging enabled
;
;----------------------------------------------------------------------------
; set proot PRECONDITION:
;
;       EAX   proot address
;       EBP   tcb of task
;
;       DS    linear kernel space
;
;       paging enabled
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       thread_proot set in all threads of task
;
;       CR2, CPU_CR3, and linear_space segements updated if own task
;
;----------------------------------------------------------------------------


set_address_space_data_for_all_threads:

  push  edx
  push  esi
  push  edi
  push  ebp

  lno___task edi,ebp
  mov   ds:[(edi*4)+task_root],eax

  mov   edi,ebx
  rol   edi,8
  bswap edi
  and   edi,0FF0000FFh
  or    edi,000C0F300h

  mov   edx,ecx
  shr   edx,log2_pagesize
  dec   edx
  mov   esi,edx
  and   edx,000F0000h
  and   esi,0000FFFFh
  or    edi,edx

  mov   edx,ebx
  shl   edx,16
  or    esi,edx


  and   ebp,-(lthreads*sizeof tcb)
  mov   edx,lthreads
  DO
        test__page_writable ebp
        IFNC
              mov   [ebp+thread_proot],eax
              mov   [ebp+thread_segment],esi
              mov   [ebp+thread_segment+4],edi
              mov   [ebp+as_base],ebx
              mov   [ebp+as_size],ecx

              add   ebp,sizeof tcb
              dec   edx
              REPEATNZ
        ELSE_
              and   ebp,-pagesize
              add   ebp,pagesize
              sub   edx,pagesize/sizeof tcb
              REPEATA
        FI
  OD
    
  pop   ebp

  mov   edx,ebp
  xor   edx,esp
  test  edx,mask task_no
  IFZ
        mov   ds:[gdt+linear_space/8*8],esi
        mov   ds:[gdt+linear_space/8*8+4],edi
        add   edi,0000FB00h-0000F300h
        mov   ds:[gdt+linear_space_exec/8*8],esi
        mov   ds:[gdt+linear_space_exec/8*8+4],edi
        mov   ds:[cpu_cr3],eax
        mov   cr3,eax
        mov   edx,linear_space
        mov   es,edx
        mov   fs,edx
        mov   gs,edx
  FI

  pop   edi
  pop   esi
  pop   edx
  ret        





set_small_space_data_for_all_threads:

  push  ebx
  push  ecx
  mov   ebx,eax

  mov   eax,[ebp+thread_proot]
  mov   ecx,virtual_space_size
  shl   ebx,22
  IFNZ
        add   ebx,offset small_virtual_spaces
        movzx ecx,ds:[small_space_size_div_MB4]
        shl   ecx,22
  FI
  call  set_address_space_data_for_all_threads

  mov   eax,ebx
  pop   ecx
  pop   ebx
  ret



set_proot_for_all_threads:

  push  ebx
  push  ecx

  mov   ebx,[ebp+as_base]
  mov   ecx,[ebp+as_size]
  call  set_address_space_data_for_all_threads

  pop   ecx
  pop   ebx
  ret






;*********************************************************************
;******                                                         ******
;******                                                         ******
;******         Small Address Space Handler                     ******
;******                                                         ******
;******                                                         ******
;*********************************************************************
 






 
;----------------------------------------------------------------------------
;
;       init small address spaces
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       DS     linear kernel space
;
;       paging enabled
;
;----------------------------------------------------------------------------


  icode



init_small_address_spaces:

  mov   ds:[log2_small_space_size_DIV_MB4],-22
  mov   ds:[small_space_size_DIV_MB4],1
  
  sub   ebx,ebx
  DO
        mov   ds:[ebx*2+small_associated_task],0
        inc   ebx
        cmp   ebx,max_small_spaces
        REPEATB
  OD
  
  mov   cl,3                                   ; 32  MB
  call  change_small_space_size

  ret


 


  icod ends



;----------------------------------------------------------------------------
;
;       attach small space
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       AH    0 < small as no < small spaces     (attach small space)
;             0 = small as no                    (detach small space)
;
;       EBP   a tcb (!) address of dest task
;
;       DS    linear kernel space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,ECX,EDI  scratch
;
;       ES,FS,GS     undefined
;
;----------------------------------------------------------------------------




attach_small_space:


  CORZ  ah,2
  IFZ   ah,1
        shl   ah,3+1
        add   ah,32/4
  FI
        

  mov   cl,-1
  DO
        inc   cl
        shr   ah,1
        REPEATA           ; NZ , NC
  OD
  IFNZ
        shl   ah,cl
        IFNZ  cl,ds:[log2_small_space_size_DIV_MB4]
              call  change_small_space_size
        FI
  FI                    
  movzx eax,ah  

  mov   ecx,[ebp+as_base]
  sub   ecx,offset small_virtual_spaces
  IFNC
  shr   ecx,22
  CANDNZ eax,ecx
  
        call  detach_associated_small_space
        
        test  eax,eax
        IFNZ
              movzx ecx,[(eax*2)+small_associated_task]
              test  ecx,ecx
              IFNZ
                    push  ebp
                    mov   ebp,ecx
                    shl   ebp,task_no
                    lea___tcb ebp,ebp
                    call  detach_small_space
                    pop   ebp
              FI        
        FI      
  FI

  lno___task ecx,ebp
  mov   [(eax*2)+small_associated_task],cx
     
  call  set_small_space_data_for_all_threads
 
  ret
  
  



;----------------------------------------------------------------------------
;
;       detach small space / detach associated small space
;
;----------------------------------------------------------------------------
; detach associated small space PRECONDITION:
;
;       EBP   tcb of task
;
;       DS    linear kernel space
;
;----------------------------------------------------------------------------
; detach small space PRECONDITION:
;
;       EBP   tcb of task
;       ECX   small space no
;
;       DS    linear kernel space
;
;----------------------------------------------------------------------------




detach_associated_small_space:

  push  eax
  push  ecx

  mov   ecx,[ebp+as_base]
  sub   ecx,offset small_virtual_spaces
  IFNC
        shr   ecx,22
        lno___task eax,ebp
        call  detach_small_space
  FI

  pop   ecx
  pop   eax
  ret



detach_small_space:

  push  eax
        
  IFZ   ax,word ptr [(ecx*2)+small_associated_task]

        mov   word ptr [(ecx*2)+small_associated_task],0
                
        sub   eax,eax
        call  set_small_space_data_for_all_threads

        call  flush_small_pde_block_in_all_pdirs
        
        mov   eax,cr3
        mov   cr3,eax

  ELSE_
        ke    'detach_err'
  FI
    
  pop   eax
  ret
  


  
  
  
  
;----------------------------------------------------------------------------
;
;       make own address space large
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       DS    linear kernel space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       linar_space / exec      segment descriptor updated
;       ES,FS,GS                reloaded
;       DS                      unchanged
;
;       TLB flushed
;            
;       EAX,EDI scratch
;
;----------------------------------------------------------------------------
  


  
  
  
make_own_address_space_large:

  push  ebp
  mov   ebp,esp
  and   ebp,-sizeof tcb

  sub   eax,eax
  call  set_small_space_data_for_all_threads

  pop   ebp
  ret
  

  
  
  
;----------------------------------------------------------------------------
;
;       flush small page directory block in ALL pdirs
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ECX   0 < small no < small spaces
;
;       DS    linear kernel space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX   scratch
;
;----------------------------------------------------------------------------
  
  
flush_small_pde_block_in_all_pdirs:

  push  ebx
  push  edx
  push  ebp
      
  sub   edx,edx
  DO
        cmp   edx,tasks
        EXITAE

        mov   ebp,[(edx*4)+task_root]
        inc   edx
        test  ebp,pagesize-1
        REPEATNZ
        
        push  ecx
        mov   bl,ds:[small_space_size_DIV_MB4]
        DO
              sub   eax,eax
              xchg  dword ptr [(ecx*4)+ebp+(offset small_virtual_spaces SHR 22)*4+PM],eax
              inc   ecx
              test  al,superpage
              IFZ
              shr   eax,log2_pagesize
              CANDNZ
                    and   byte ptr [(eax*4)+ptab_backlink],NOT 01h
              FI      
              dec   bl
              REPEATNZ
        OD                    
        pop   ecx
        REPEAT                
  OD
        
  pop   ebp
  pop   edx
  pop   ebx
  ret



        




;----------------------------------------------------------------------------
;
;       set small page directory entry in ONE pdir
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   source pde block addr
;       EDI   dest pde block addr
;
;       DS    linear kernel space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX,EBX,ECX,EDX,ESI,EDI,EBP   scratch
;
;----------------------------------------------------------------------------


  
  
set_small_pde_block_in_pdir:

  mov   cl,ds:[small_space_size_DIV_MB4]
  DO
        mov   eax,[esi]
        add   esi,4
        mov   dword ptr [edi],eax
        add   edi,4
        test  al,superpage
        IFZ
        shr   eax,log2_pagesize
        CANDNZ
              or    byte ptr [(eax*4)+ptab_backlink],01h
        FI      
        dec   cl
        REPEATNZ
  OD                    
  
  ret


  
;----------------------------------------------------------------------------
;
;       change small space size
;
;----------------------------------------------------------------------------


change_small_space_size:

  pushad
 
  mov   ch,1
  shl   ch,cl
  mov   ds:[small_space_size_DIV_MB4],ch
  
  shl   ch,2
  dec   ch
  mov   byte ptr ds:[gdt+linear_space/8*8+1],ch               ; recall: 256 MB is max small_space_size            
  mov   byte ptr ds:[gdt+linear_space_exec/8*8+1],ch            
    
  mov   ch,cl
  xchg  cl,ds:[log2_small_space_size_DIV_MB4]
  add   cl,22
  add   ch,22
  call  ipc_update_small_space_size
  
  popad
  ret
  



  
  
;----------------------------------------------------------------------------
;
;       get small space
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   a tcb (!) address of dest task
;
;       DS    linear kernel space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       AL    small space / 0
;
;----------------------------------------------------------------------------




get_small_space:

  mov   eax,[esi+as_base]
  test  eax,eax
  IFNZ
        sub   eax,offset small_virtual_spaces
        shr   eax,22
        add   al,al
        add   al,ds:[small_space_size_DIV_MB4]
  FI
  
  ret





  
  
  


        code ends
        end