include l4pre.inc 
   
    				   
  Copyright IBM, L4.ADRSMAN.5, 08, 08, 99, 9025
   
;*********************************************************************
;******                                                         ******
;******         Address Space Manager                           ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 08.08.99    ******
;******                                                         ******
;*********************************************************************
 

      
  public init_adrsman
  public init_sigma_1
  public sigma_1_installed
  public create_kernel_including_task
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

  IFZ   ebx,1                                           ;REDIR begin --------------------------
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
        lno___task eax,ebp                      ;              
        load__proot eax,eax                     ;
        mov   ds:[cpu_cr3],eax                  ;   If deleter has small address space,
        mov   dword ptr ds:[tlb_invalidated],eax;   it might execute inside to be deleted
        mov   cr3,eax                           ;   pdir. Avoided by explicitly switching 
                                                ;   to deleter's pdir.
                                                ;
        lno___task edi,ebx                      ;
                                                ;
        call  detach_associated_small_space     ;
                                                ;
        load__proot edi,edi                     ;  
                                                ;
        mov   ecx,lthreads                      ;
        DO                                      ;
              test__page_present ebx            ;
              IFNC                              ;
                    mov   ebp,ebx               ;
                    call  delete_thread         ;
              FI                                ;
              add   ebx,sizeof tcb              ;
              dec   ecx                         ;
              REPEATNZ                          ;
        OD                                      ;
                                                ;
        call  flush_address_space               ;

       	IFNZ  edi,ds:[empty_proot]		    ;
       	CANDNZ edi,ds:[kernel_proot]		    ;
								    ;
        	  add   edi,PM                    ;
        	  mov   ecx,virtual_space_size SHR 22;
        	  DO                              ;
              	    sub   eax,eax           ;
              	    cld                     ;
              	    repe  scasd             ;
              		EXITZ                 ;
                                                ;
              		mov   eax,[edi-4]     ;
              		call  insert_into_fresh_frame_pool;
              		REPEAT                ;
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
        mov   ecx,[(ecx*8)+task_proot].switch_ptr
        lno___task eax,ebp
        add   eax,ds:[empty_proot]
        cmp   eax,ecx

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
        store_inactive_proot eax,ebx

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
  store_proot esi,edi

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
  lno___task edi,eax                                    ;
  lea___tcb  ebp,eax                                    ;
                                                        ;
  IFZ   [edi*8+task_proot].proot_ptr,0                  ;
        mov   edi,[edi*8+task_proot].switch_ptr         ;
        sub   edi,ds:[empty_proot]                      ;
  ELSE_                                                 ;
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

        push  eax

        mov   ecx,[ebx].ktask_stack
        mov   edx,[ebx].ktask_start
        mov   eax,(255 SHL 16) + (16 SHL 8) + 10
        mov   esi,sigma0_task

        call  create_thread

        pop   eax

        lno___task ebp
        store_proot eax,ebp
  FI       

  ret



  icod  ends




;----------------------------------------------------------------------------
;
;       sigma_1
;
;----------------------------------------------------------------------------


sigma_1_installed   db false





init_sigma_1:

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
;       ESI   a tcb (!) address of dest task
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
  
          
  lno___task ebp,esi
  lea   edi,[ebp*8+task_proot]
    
  movzx eax,ah
  movzx ecx,[edi].small_as
  
  IFNZ  eax,ecx
  
        IFNZ  cl,0
              call  detach_small_space
        FI
        
        test  eax,eax
        IFNZ
              IFNZ  [(eax*2)+small_associated_task],0
                    mov   ecx,eax
                    call  detach_small_space
              FI        
              mov   [(eax*2)+small_associated_task],bp
        FI      
        mov   ecx,eax
        mov   [edi].small_as,al
  FI
     
  shl   ecx,22-16
  IFNZ
        add   ecx,(offset small_virtual_spaces SHR 16) + 0C0F30000h
        xchg  cl,ch
        ror   ecx,8
        mov   [edi].switch_ptr,ecx
  
        mov   eax,esp
        xor   eax,esi
        test  eax,mask task_no
        jz    short update_own_address_space_small
  FI
  ret
  
  



;----------------------------------------------------------------------------
;
;       detach small space
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ECX   0 < small no < small spaces
;
;       DS    linear kernel space
;
;----------------------------------------------------------------------------




detach_small_space:

  push  eax
  push  edi
  
  movzx edi,[(ecx*2)+small_associated_task]
  test  edi,edi
  IFNZ
        mov   word ptr [(ecx*2)+small_associated_task],0
        
        call  flush_small_pde_block_in_all_pdirs
        
        lea   ecx,[(edi*8)+task_proot]
        
        mov   eax,cr3
        mov   cr3,eax
        
        mov   eax,[ecx].proot_ptr
        mov   al,0
        mov   [ecx].switch_ptr,eax
        mov   [ecx].proot_ptr,eax
  FI
  
  lno___task  eax,esp
  IFZ   eax,edi
        call  make_own_address_space_large
  FI
  
  pop   edi
  pop   eax
  ret
  


;----------------------------------------------------------------------------
;
;       detach associated small space
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ECX   0 < small no < small spaces
;
;       DS    linear kernel space
;
;----------------------------------------------------------------------------




detach_associated_small_space:

  push  ecx
  
  movzx ecx,[edi*8+task_proot].small_as
  test  ecx,ecx
  IFNZ
        call  detach_small_space
  FI      
    
  pop   ecx
  ret  
  
  
  
  
  
;----------------------------------------------------------------------------
;
;       change own address space large <--> small
;
;----------------------------------------------------------------------------
; update..small PRECONDITION:
;
;       EDI   task_proot address of current task
;
;       DS    linear kernel space
;
;----------------------------------------------------------------------------
; make..large PRECONDITION:
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
  


update_own_address_space_small:

  mov   eax,[edi].switch_ptr
  mov   edi,ds:[kernel_proot]
  jmp   short update_cr3_and_seg_register
  
  
  
  
make_own_address_space_large:

  lno___task edi,esp
  
  lea   edi,[(edi*8)+task_proot]
  mov   eax,[edi].proot_ptr
  mov   al,0
  mov   [edi].switch_ptr,eax
  
  mov   edi,eax
  mov   eax,00CCF300h
  
  
  
update_cr3_and_seg_register:

  mov   ds:[cpu_cr3],edi
  mov   cr3,edi
  
  mov   ds:[gdt+linear_space/8*8+4],eax
  add   ah,0FBh-0F3h
  mov   ds:[gdt+linear_space_exec/8*8+4],eax
  
  mov   eax,linear_space
  mov   es,eax
  mov   fs,eax
  mov   gs,eax
  
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
      
  mov   edx,offset task_proot
  DO
        mov   ebp,[edx].proot_ptr
        add   edx,8
        and   ebp,0FFFFFF00h
        REPEATZ
        
        EXITS
  
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

  lno___task eax,esi
  mov   eax,[eax*8+task_proot].switch_ptr
  
  test  eax,eax
  IFNS
        mov   al,0
        ret
  FI
  
  rol   eax,8
  xchg  al,ah
  shr   eax,22-16
  sub   al,(offset small_virtual_spaces SHR 22) AND 0FFh
  add   al,al
  add   al,ds:[small_space_size_DIV_MB4]
  
  ret





  
  
  


        code ends
        end