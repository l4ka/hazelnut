include l4pre.inc 
 


  Copyright GMD, L4.START, 19,07,96, 28
 
;*********************************************************************
;******                                                         ******
;******         System Start                                    ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  04.03.91    ******
;******                                   modified: 19.07.96    ******
;******                                                         ******
;*********************************************************************
 
 
  public kernel_start
  public kernel_start_x2

  extrn  init_default_kdebug:near,default_kdebug_exception:near
  extrn  default_kdebug_end:byte
  extrn  default_sigma0_stack:dword,default_sigma0_start:near
  extrn  default_sigma0_begin:byte,default_sigma0_end:byte
  extrn  ktest0_stack:dword,ktest0_start:near
  extrn  ktest1_stack:dword,ktest1_start:near
  extrn  ktest_begin:byte,ktest_end:byte
  extrn  labseg_start:byte
  extrn  kernelstring:byte
  extrn  kernelver:abs
  extrn  physical_kernel_info_page:dword


  extrn init_memctr:near
  extrn enable_paging_mode:near
  extrn init_sgmctr:near
  extrn init_intctr:near
  extrn init_pagmap:near
  extrn init_fresh_frame_pool:near
  extrn init_pagfault:near
  extrn init_schedcb:near
  extrn init_cpuctr:near
  extrn init_tcbman:near
  extrn init_dispatcher:near
  extrn init_ipcman:near
  extrn init_adrsman:near
  extrn init_emuctr:near
  extrn init_basic_hw_interrupts:near
  extrn init_rtc_timer:near
  extrn init_sigma0_1:near
  extrn start_dispatch:near
  extrn ptabman_init:near
  extrn ptabman_start:near
  extrn create_kernel_including_task:near
  extrn kcod_start:near





.nolist 
include l4const.inc
include l4kd.inc
include adrspace.inc
include intrifc.inc
include uid.inc
include tcb.inc
include cpucb.inc
include syscalls.inc
include kpage.inc
include apic.inc
.list


ok_for i486,pentium,ppro,k6

  IF    kernel_type NE i486
        extrn init_apic:near
  ENDIF      
 
  IF    kernel_type NE i486
        extrn init_small_address_spaces:near
  ENDIF      

  IF    kernel_x2
        extrn prepare_dual_processor_init:near
        extrn init_dual_processor:near
        extrn dual_link_table:dword
  ENDIF      
 
;*********************************************************************
;******                                                         ******
;******         System Start                                    ******
;******                                                         ******
;******                                                         ******
;*********************************************************************
 
 
 
  strtseg


;----------------------------------------------------------------------------
;
;       link table
;
;----------------------------------------------------------------------------
;
;       In the strt segment, *only* the start code of
;       module start.pc is located before this table.
;
;       Start.pc *MUST* ensure that it occupies position
;       0 ... 1008 so that the following table is placed 
;       exactly at location 1008h.
;
;----------------------------------------------------------------------------

  dd    0
  IF    kernel_x2
        dd    dual_link_table
  ELSE
        dd    0
  ENDIF            

  dd    init_default_kdebug,default_kdebug_exception ; 1010 ; kdebug
  dd    0,default_kdebug_end

  dd    default_sigma0_stack,default_sigma0_start    ; 1020 ; sigma0 ESP, EIP
  dd    default_sigma0_begin,default_sigma0_end

  dd    0,0                                          ; 1030 ; sigma1 ESP, EIP
  dd    0,0                                     

  dd    ktest0_stack,ktest0_start                    ; 1040 ; booter ESP, EIP
  dd    ktest_begin,ktest_end
  dd    ktest1_stack,ktest1_start                    ; 1050 ; sigma2 ESP, EIP
  dd    ktest_begin,ktest_end

  dd    0,0           ; main_mem                     ; 1060
  dd    0,0           ; reserved_mem0 

  IF    kernel_x2
        dd  MB16,MB64 ; reserved_mem1                ; 1070
  ELSE
        dd  0,0       ; reserved_mem1                ; 1070
  ENDIF
  
  dd    0,0           ; dedicated_mem0 

  IF    kernel_x2
        dd  MB16,MB64
  ELSE      
        dd    0,0     ; dedicated_mem1               ; 1080
  ENDIF
  
  dd    0,0           ; dedicated_mem2 

  dd    0,0           ; dedicated_mem3               ; 1090
  dd    0,0           ; dedicated_mem4 
;;  dd    32*MB1,GB1    ; speacial for broken PCS 320 !!!!!!!!!!!!!!!

  dd    0,0,0,0                                      ; 10A0 ; user clock
  
  dd    0,0,0,0                                      ; 10B0 ; clock freqencies         

  strt ends



;---------------------------------------------------------------------
;
;       system start
;
;---------------------------------------------------------------------
; PRECONDITION:
;
;       protected mode
;
;       CS    executable & readable segment, starting at 0, size 4G
;             USE32
;       DS    readable & writable segment, starting at 0, size 4G
;
;       interrupts disabled
;
;---------------------------------------------------------------------
 
 assume ds:codseg
 

  icode
  
 
kernel_start:
 
  mov   eax,ds
  mov   es,eax
  mov   ss,eax
  
  sub   eax,eax
  mov   fs,eax
  mov   gs,eax
  
  

  IF    kernel_x2
        call  prepare_dual_processor_init
  ENDIF
        

  mov   edi,offset physical_kernel_info_page

  mov   [edi+L4_magic_word],4BE6344Ch     ;  'L4',0E6h,'K'
  mov   [edi+L4_version_word],kernelver
  
  mov   [edi+kpage_version],current_kpage_version

  mov   eax,offset labseg_start
  sub   eax,edi
  shr   eax,4
  mov   [edi+L4_label_link],al

  IF    kernel_x2
  ELSE
        sub   eax,eax
        mov   [edi+next_kpage_link],eax
  ENDIF        
  
  
  
kernel_start_x2:

  call  dword ptr cs:[physical_kernel_info_page+init_kdebug]

  call  init_memctr
  call  enable_paging_mode
  call  init_sgmctr
  call  init_intctr 
   
 
  kd____clear_page
  mov   al,0
  mov   ah,22
  kd____cursor
 

  mov   eax,offset kernelstring
  kd____outcstring
  kd____disp <13,10,10>

  call  init_cpuctr
  call  init_pagfault
  call  init_pagmap
  call  init_fresh_frame_pool
  call  init_schedcb
  call  init_tcbman
  call  init_dispatcher
  call  init_ipcman
  call  init_adrsman
  IF    kernel_type NE i486
        call  init_small_address_spaces
  ENDIF     
 
  call  init_emuctr
  call  init_basic_hw_interrupts

  IF    kernel_type EQ i486
        call  init_rtc_timer
  ELSE
        bt    ds:[cpu_feature_flags],on_chip_apic_bit
        IFC
              call  init_apic
        ELSE_
              call  init_rtc_timer
        FI
  ENDIF
                            
  IF    kernel_x2
        call  init_dual_processor
  ENDIF      

  ke    'debug'

  cli

 include pagmac.inc
 
  mov   ebx,offset gdt+(linear_kernel_space AND -8)
  mov   eax,cr3
  and   eax,-pagesize
  xpdir ecx,ebx
  mov   eax,dword ptr [eax+ecx*4+PM]
  and   eax,-pagesize
  xptab ecx,ebx
  mov   eax,dword ptr [eax+ecx*4+PM]
  and   eax,-pagesize
  and   ebx,pagesize-1
  lea   ebp,[eax+ebx+PM]

;  DO
;        rdtsc
;        mov   ebx,eax
;        rdtsc
;        sub   eax,ebx
;        mov   ebx,eax
;        mov   edi,linear_kernel_space
;        mov   ebp,linear_space
;        rdtsc
;        mov   ecx,eax
;        mov   ds,edi
;        rdtsc
;        sub   eax,ecx
;        sub   eax,ebx
;        mov   ecx,eax
;     ;;   add   ds:[ldt+(linear_space AND -8)],0
;     ;;   add   dword ptr ss:[ebp-(linear_kernel_space AND -3)],0
;        
;        rdtsc
;        mov   esi,eax
;        mov   ds,ebp
;        mov   ds,edi
;        mov   eax,phys_mem
;        mov   ds,eax
;        rdtsc
;        sub   eax,esi
;        sub   eax,ebx
;        mov   esi,eax
;        ke    'xx'
;     ;;   REPEAT
;  OD       


  push  linear_kernel_space
  pop   ds        
  
  call  init_sigma0_1 
  call  ptabman_init

  mov   eax,booter_task
  mov   ebx,offset booter_ktask+offset logical_info_page
  call  create_kernel_including_task
  IFZ   ds:[logical_info_page+booter_ktask].ktask_start,<offset ktest0_start>
        mov   eax,sigma2_task
        mov   ebx,offset sigma2_ktask+offset logical_info_page
  CANDNZ <[ebx].ktask_stack>,0
        call  create_kernel_including_task
  FI
  
  IF    kernel_x2
  
 extrn p6_workaround_init:near
 call  p6_workaround_init 

  ENDIF

 
  jmp   ptabman_start






  icod  ends



  code ends
  end

























 