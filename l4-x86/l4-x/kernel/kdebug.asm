include l4pre.inc
                                       
        
  dcode
                                              
  Copyright IBM+UKA, L4.KDEBUG, 02,02,01, 2001
                  

;*********************************************************************
;******                                                         ******
;******                L4 Kernel Debug                          ******
;******                                                         ******
;******                                                         ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 02.02.01    ******
;******                                                         ******
;*********************************************************************
  

.nolist
include l4const.inc
include uid.inc     
                                     
include ktype.inc
include adrspace.inc
include tcb.inc
include segs.inc
 
include intrifc.inc
include pagconst.inc
include syscalls.inc
include pagmac.inc
  IF    kernel_family EQ lime_pip
include schedcb.inc
include pnodes.inc
  ENDIF
include kpage.inc
include l4kd.inc
.list


ok_for x86






  public init_default_kdebug
  public default_kdebug_exception
  public default_kdebug_end
  public kdebug_reset_pc


  extrn init_kdio:near
  extrn open_debug_keyboard:near
  extrn close_debug_keyboard:near
  extrn set_remote_info_mode:near
  extrn open_debug_screen:near
  extrn kd_outchar:near
  extrn kd_incharety:near
  extrn kd_inchar:near
  extrn kd_kout:near
  extrn old_pic1_imr:byte

  extrn first_lab:byte
  extrn kcod_start:byte
  extrn cod_start:byte
  extrn dcod_start:byte
  extrn scod_start:byte
  extrn kernelstring:byte

  IF    kernel_x2
        extrn enter_single_processor_mode:near
        extrn exit_single_processor_mode:near
  ENDIF      




  assume ds:codseg
  

  
;----------------------------------------------------------------------------
;
;       System Interface Pointers
;
;----------------------------------------------------------------------------

gidt_dope     struc
  limit       dw    0
  base        dd    0
gidt_dope     ends

  
                                align 4

                                dw    0
gdt_descr                       gidt_dope <0,0>

                                dw    0
idt_descr                       gidt_dope <0,0>


permissions                     kdebug_permissions_word <0,0,0>
configuration                   kdebug_configuration_word <0,0,0>

kernel_proot_ptr_ptr            dd    0

phys_kernel_info_page_ptr       dd    0

irq0_intr                       dd    0

grab_frame                      dd    offset kdebug_grab_frame
kdebug_extension                dd    0
              
io_apic_base                    dd    0
local_apic_base                 dd    0






;----------------------------------------------------------------------------
;
;       screen
;
;----------------------------------------------------------------------------


lines         equ 25
columns       equ 80


;----------------------------------------------------------------------------
;
;       kd intr area
;
;----------------------------------------------------------------------------





kd_xsave_area struc

  kd_es       dw    0
  kd_dr7      db    0,0
  kd_ds       dw    0,0

kd_xsave_area ends

kd_save_area struc

              dw    0     ; kd_es
              db    0,0   ; kd_dr7

kd_save_area ends




kdpre  macro

  push  es

  push  eax
  mov   eax,dr7
  mov   ss:[esp+kd_dr7+4],al
  mov   al,0
  mov   dr7,eax
  pop   eax

  endm



kdpost macro

  push  eax
  mov   eax,dr7
  mov   al,ss:[esp+kd_dr7+4]
  mov   dr7,eax
  pop   eax

  pop   es

  endm




ktest_page_present macro reg

  push  eax
  push  ebx
  mov   eax,reg
  sub   ebx,ebx                 ;self
  call  page_phys_address
  pop   ebx
  pop   eax
  endm




;----------------------------------------------------------------------------
;
;       data
;
;----------------------------------------------------------------------------

              align 4

kdebug_sema          dd 0

kdebug_esp           dd 0
kdebug_text          dd 0

kdebug_stack_bottom       dd    0

kdebug_invocation_stamp   dd    0

kdebug_rdtsc_corrector    dd    0,0

perf_counter0_value       dd    0
perf_counter1_value       dd    0


kdebug_segs_struc struc

  ds_sreg     dw 0
  es_sreg     dw 0

kdebug_segs_struc ends


                    align 4

breakpoint_base     dd 0

breakpoint_thread   dd 0
no_breakpoint_thread dd 0

debug_breakpoint_counter_value  dd 0
debug_breakpoint_counter        dd 0

bx_low              dd 0
bx_high             dd 0
bx_addr             dd 0
bx_size             db 0

debug_exception_active_flag db false

                    align 4

debug_exception_handler   dd 0


int3_exception_handler    dd 0


kdebug_uncacheable_PDE    dd 0

performance_count_params  dd 0




ipc_prot_state	      db 0
ipc_prot_handler_active   db 0
ipc_prot_mask             db 0FFh
                          align 4
ipc_prot_thread0          dd 0
ipc_prot_thread1          dd 0
ipc_prot_thread2          dd 0
ipc_prot_thread3          dd 0
ipc_prot_thread4          dd 0
ipc_prot_thread5          dd 0
ipc_prot_thread6          dd 0
ipc_prot_thread7          dd 0
ipc_prot_non_thread       dd 0
                    
ipc_handler		dd 0


niltext             db 0

page_fault_prot_state           db 0
page_fault_prot_handler_active  db 0
                    align 4
page_fault_low      dd 0
page_fault_high     dd 0FFFFFFFFh
page_fault_handler  dd 0




timer_intr_handler  dd 0

kdebug_timer_intr_counter       db 0,0,0,0

monitored_exception_handler     dd 0
monitored_ec_min                dw 0
monitored_ec_max                dw 0
monitored_exception             db 0
exception_monitoring_flag       db false
                                db 0,0


kdebug_buffer             db 32 dup (0)



;----------------------------------------------------------------------------
;
;       kdebug cache snapshots
;
;----------------------------------------------------------------------------

              align 4

cache_snapshots_begin                 dd    0
cache_snapshots_end                   dd    0
current_cache_snapshot                dd    0

current_cache_snapshot_stamp          dd    0

typical_cached_data_cycles            dd    0

dcache_sets                           equ   128
dcache_associativity                  equ   4
dcache_line_size                      equ   32
log2_dcache_line_size                 equ   5

icache_sets                           equ   128
icache_associativity                  equ   4
icache_line_size                      equ   32
log2_icache_line_size                 equ   5


snapshot_dwords_per_dcache_set        equ   8
log2_snapshot_dwords_per_dcache_set   equ   3
snapshot_bytes_per_dcache_set         equ   (snapshot_dwords_per_dcache_set*4)
log2_snapshot_bytes_per_dcache_set    equ   (log2_snapshot_dwords_per_dcache_set+2)

snapshot_dwords_per_icache_set        equ   8
log2_snapshot_dwords_per_icache_set   equ   3



cache_snapshot struc
  dcache                        dd    (dcache_sets*snapshot_dwords_per_dcache_set) dup (?)
cache_snapshot ends   

  



;----------------------------------------------------------------------------
;
;       kdebug trace buffer
;
;----------------------------------------------------------------------------

              align 4
              

debug_trace_buffer_size   equ KB64


trace_buffer_entry  struc

  trace_entry_type        dd    0
  trace_entry_string      dd    0,0,0,0,0
  trace_entry_timestamp   dd    0,0
  trace_entry_perf_count0 dd    0
  trace_entry_perf_count1 dd    0
                          dd    0,0,0,0,0,0
  
trace_buffer_entry  ends



get___timestamp macro

  IFA   esp,virtual_space_size
        rdtsc
  FI
  
  endm


    
 
 

trace_buffer_begin              dd 0
trace_buffer_end                dd 0
trace_buffer_in_pointer         dd 0

trace_buffer_active_stamp       dd 0,0

trace_display_mask              dd 0,0


no_references                   equ 0
forward_references              equ 1
backward_references             equ 2
performance_counters            equ 3

display_trace_index_mode        equ 0
display_trace_delta_time_mode   equ 1
display_trace_offset_time_mode  equ 2

no_perf_mon                     equ 0
kernel_perf_mon                 equ 1
user_perf_mon                   equ 2
kernel_user_perf_mon            equ 3
kernel_versus_user_perf_mon     equ 4

trace_link_presentation         db display_trace_index_mode
trace_reference_mode            db no_references
trace_perf_monitoring_mode      db no_perf_mon

processor_family                db  0
processor_feature_flags         dd  0



;---------------------------------------------------------------------------------
;
;       performance counters
;
;---------------------------------------------------------------------------------


P5_event_select_msr    equ 11h
P5_event_counter0_msr  equ 12h
P5_event_counter1_msr  equ 13h



P5_rd_miss       equ    000011b
P5_wr_miss       equ    000100b
P5_rw_miss       equ    101001b
P5_ex_miss       equ    001110b

P5_d_wback       equ    000110b

P5_rw_tlb        equ    000010b
P5_ex_tlb        equ    001101b

P5_a_stall       equ  00011111b
P5_w_stall       equ  00011001b
P5_r_stall       equ  00011010b
P5_x_stall       equ  00011011b
P5_agi_stall     equ  00011111b

P5_bus_util      equ  00011000b

P5_pipline_flush equ    010101b

P5_non_cache_rd  equ    011110b
P5_ncache_refs   equ    011110b
P5_locked_bus    equ    011100b

P5_mem2pipe      equ    001001b
P5_bank_conf     equ    001010b


P5_instrs_ex     equ    010110b
P5_instrs_ex_V   equ    010111b






P6_event_select0_msr      equ   186h
P6_event_select1_msr      equ   187h
P6_event_counter0_msr     equ   0C1h
P6_event_counter1_msr     equ   0C2h




;P6_rd_miss       equ      
P6_wr_miss       equ      46h
P6_rw_miss       equ      45h
P6_ex_miss       equ      81h

P6_d_wback       equ      47h

;P6_rw_tlb        equ  
P6_ex_tlb        equ      85h

P6_stalls        equ      0A2h

;P6_L2_rd_miss    equ      
P6_L2_wr_miss    equ      25h
P6_L2_rw_miss    equ      24h
P6_L2_ex_miss    equ      

P6_L2_d_wback    equ      27h



P6_bus_util      equ      62h


P6_instrs_ex     equ      0C0h



;----------------------------------------------------------------------------
;
;       KD thread nickname table
;
;----------------------------------------------------------------------------


thread_nickname_entry struc
  
  number      dd    0
  nickname    db    0,0,0,0,0,0
              db    0
              db    0

thread_nickname_entry ends



thread_nickname_table     db (16*sizeof thread_nickname_entry) dup (0)






;----------------------------------------------------------------------------
;
;       init kdebug
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       paging enabled
;
;       DS    phys mem idempotent
;
;       kd_init data on stack
;
;----------------------------------------------------------------------------



  IF    kernel_type NE lime_pip
        strtseg
        jmp   init_default_kdebug
        strt ends
  ENDIF


  icode


        assume ds:codseg


init_default_kdebug:

  pop   ebx

  pop   [kdebug_extension]
  pop   eax
  call  adjust_flat_address
  mov   [io_apic_base],eax
  pop   eax
  call  adjust_flat_address
  mov   [local_apic_base],eax
  pop   [phys_kernel_info_page_ptr]
  pop   eax
  call  adjust_flat_address
  mov   [kernel_proot_ptr_ptr],eax
  pop   [configuration]          
  pop   [permissions]            
  pop   [irq0_intr]              
  pop   eax
  IFNZ  eax,0              
        mov   [grab_frame],eax
  FI

  push  ebx

  call  determine_processor_family

  call  determine_gidt

  mov   al,'a'
  call  init_kdio
  movzx eax,[configuration].start_port
  IFA   eax,16
        call  set_remote_info_mode
  FI      
  call  init_trace_buffer
  call  init_kdebug_cacheability_control
  call  init_cache_snapshots


  mov   eax,offset default_kdebug_exception

  ret




adjust_flat_address:

  IFB_  eax,virtual_space_size
        add   eax,PM
  FI
  ret

  



determine_processor_family:

  pushfd
  pop   eax
  mov   ebx,eax
  xor   eax,(1 SHL id_flag)
  push  eax
  popfd
  pushfd
  pop   eax
  xor   eax,ebx

  sub   edx,edx
  and   eax,1 SHL id_flag
  IFNZ 
        mov   eax,1
        cpuid

  FI

  and   ah,0Fh
  CORA  ah,p6_family
  IFB_  ah,p5_family
        mov   ah,other_family
  FI
  mov   ds:[processor_family],ah

  mov   ds:[processor_feature_flags],edx

  ret





  kdebug_grab_frame:            ; kdebug-internal grab frame if NO grab_frame provided
                                ; MUST be called BEFORE kernel reads main.mem from kernel_info_page
                                ; -- phys mem assumed -- (does require PM but not PG)
                                ;
                                ; EAX : grabbed page frame address

  push  edi

  mov   edi,ds:[phys_kernel_info_page_ptr]

  mov   eax,[edi+reserved_mem1].mem_begin
  and   eax,-pagesize
  IFZ
        mov   eax,[edi+main_mem].mem_end
        and   eax,-pagesize
        mov   [edi+reserved_mem1].mem_begin,eax
        mov   [edi+reserved_mem1].mem_end,eax
  FI
  sub   eax,pagesize

  IFAE   eax,[edi+reserved_mem0].mem_end
  CANDAE eax,[edi+dedicated_mem0].mem_end
  CANDAE eax,[edi+dedicated_mem1].mem_end
  CANDAE eax,[edi+dedicated_mem2].mem_end
  CANDAE eax,[edi+dedicated_mem3].mem_end
  CANDAE eax,[edi+dedicated_mem4].mem_end

        mov   [edi+reserved_mem1].mem_begin,eax
        
  ELSE_
        ke    '-kd mem underflow'

  FI

  pop   edi
  ret



  icod  ends






determine_gidt:           ;-- can also be invoked after initialization phase ! ---------

  push  ds

  IFAE  esp,virtual_space_size
        push  phys_mem
        pop   ds
  FI

  sgdt  ds:[gdt_descr]
  sidt  ds:[idt_descr]

  pop   ds
  ret





;----------------------------------------------------------------------------
;
;       prep ds / prep ds & eax
;
;----------------------------------------------------------------------------


prep_ds_es:

  IFAE  esp,virtual_space_size
        push  phys_mem
        pop   ds
        push  phys_mem
        pop   es
  FI

  ret



;----------------------------------------------------------------------------
;
;       kdebug IO call
;
;----------------------------------------------------------------------------

        align 4

kd_io_call_tab_phys dd    kd_outchar  ;  0
                    dd    outstring   ;  1
                    dd    outcstring  ;  2
                    dd    clear_page  ;  3
                    dd    cursor      ;  4
 
                    dd    outhex32    ;  5
                    dd    outhex20    ;  6
                    dd    outhex16    ;  7
                    dd    outhex12    ;  8
                    dd    outhex8     ;  9
                    dd    outhex4     ; 10
                    dd    outdec      ; 11
 
                    dd    kd_incharety; 12
                    dd    kd_inchar   ; 13
                    dd    inhex32     ; 14
                    dd    inhex16     ; 15
                    dd    inhex8      ; 16
                    dd    inhex32     ; 17

kd_io_call_tab_virt dd    kd_outchar+KR  ;  0
                    dd    outstring+KR   ;  1
                    dd    outcstring+KR  ;  2
                    dd    clear_page+KR  ;  3
                    dd    cursor+KR      ;  4
 
                    dd    outhex32+KR    ;  5
                    dd    outhex20+KR    ;  6
                    dd    outhex16+KR    ;  7
                    dd    outhex12+KR    ;  8
                    dd    outhex8+KR     ;  9
                    dd    outhex4+KR     ; 10
                    dd    outdec+KR      ; 11
 
                    dd    kd_incharety+KR; 12
                    dd    kd_inchar+KR   ; 13
                    dd    inhex32+KR     ; 14
                    dd    inhex16+KR     ; 15
                    dd    inhex8+KR      ; 16
                    dd    inhex32+KR     ; 17

kdebug_io_calls     equ 18




kdebug_io_call:

  IFAE  esp,virtual_space_size
        push  phys_mem
        pop   ds                      
        push  phys_mem
        pop   es

        movzx ebx,ah
        IFB_  ebx,kdebug_io_calls
              mov   eax,[ebp+ip_eax]
              call  [ebx*4+kd_io_call_tab_virt]
              mov   [ebp+ip_eax],eax
        ELSE_
              mov   al,ah
              call  kd_kout
        FI
        jmp   ret_from_kdebug
  FI

  movzx ebx,ah
  IFB_  ebx,kdebug_io_calls
        mov   eax,[ebp+ip_eax]
        call  [ebx*4+kd_io_call_tab_phys]
        mov   [ebp+ip_eax],eax
  ELSE_
        mov   al,ah
        call  kd_kout
  FI

  jmp   ret_from_kdebug




void:

  ret



;----------------------------------------------------------------------------
;
;       kdebug display
;
;----------------------------------------------------------------------------


kdebug_display:

  IFAE  esp,virtual_space_size
        push  phys_mem
        pop   ds
        push  phys_mem
        pop   es
  FI

  lea   eax,[ebx+2]
  call  outstring

  jmp   ret_from_kdebug



;----------------------------------------------------------------------------
;
;       outstring
;
;----------------------------------------------------------------------------
; outstring PRECONDITION:
;
;       EAX   string addr (phys addr or linear addr (+PM))
;             string format: len_byte,text
;
;----------------------------------------------------------------------------
; outcstring PRECONDITION:
;
;       EAX   string addr (phys addr or linear addr (+PM))
;             string format: text,00
;
;----------------------------------------------------------------------------
 
 
outstring:

  and   eax,NOT PM

  mov   cl,[eax]
  inc   eax

  mov   ebx,eax
  IFNZ  cl,0
        DO
              mov   al,[ebx]
              call  kd_outchar
              inc   ebx
              sub   cl,1
              REPEATNZ
        OD
  FI
 
  ret



outcstring:

  and   eax,NOT PM

  mov   cl,255
  mov   ebx,eax
  IFNZ  cl,0
        DO
              mov   al,[ebx]
              test  al,al
              EXITZ
              call  kd_outchar
              inc   ebx
              sub   cl,1
              REPEATNZ
        OD
  FI
 
  ret
 
 
 
;----------------------------------------------------------------------------
;
;       cursor
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       AL    x
;       AH    y
;
;----------------------------------------------------------------------------


cursor:
 
  push  eax
  mov   al,6
  call  kd_outchar
  mov   al,byte ptr ss:[esp+1]
  call  kd_outchar
  pop   eax
  jmp   kd_outchar



;----------------------------------------------------------------------------
;
;       clear page
;
;----------------------------------------------------------------------------


clear_page:
 
  push  eax
  push  ebx

  mov   bl,lines-1
  mov   al,1
  call  kd_outchar
  DO
        mov   al,5
        call  kd_outchar
        mov   al,10
        call  kd_outchar
        dec   bl
        REPEATNZ
  OD
  mov   al,5
  call  kd_outchar

  pop   ebx
  pop   ecx
  ret


 
 
 
;----------------------------------------------------------------------------
;
;       outhex
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       AL / AX / EAX   value
;
;----------------------------------------------------------------------------


outhex32:

  rol   eax,16
  call  outhex16
  rol   eax,16


outhex16:

  xchg  al,ah
  call  outhex8
  xchg  al,ah


outhex8:

  ror   eax,4
  call  outhex4
  rol   eax,4



outhex4:

  push  eax
  and   al,0Fh
  add   al,'0'
  IFA   al,'9'
        add   al,'a'-'0'-10
  FI
  call  kd_outchar
  pop   eax
  ret



outhex20:

  ror   eax,16
  call  outhex4
  rol   eax,16
  call  outhex16
  ret



outhex12:

  xchg  al,ah
  call  outhex4
  xchg  ah,al
  call  outhex8
  ret





;----------------------------------------------------------------------------
;
;       outdec
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   value
;
;----------------------------------------------------------------------------


outdec:

  sub   ecx,ecx
  
outdec_:  

  push  eax
  push  edx

  sub   edx,edx
  push  ebx
  mov   ebx,10
  div   ebx
  pop   ebx
  test  eax,eax
  IFNZ
        inc   ecx
        
        call  outdec_
        
        CORZ  ecx,9
        CORZ  ecx,6
        IFZ   ecx,3
  ;            mov   al,','
  ;            call  kd_outchar
        FI
        dec   ecx
  FI
  mov   al,'0'
  add   al,dl
  call  kd_outchar

  pop   edx
  pop   eax
  ret


;----------------------------------------------------------------------------
;
;       inhex
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       AL / AX / EAX   value
;
;----------------------------------------------------------------------------


inhex32:

  push  ecx
  mov   cl,8
  jmp   short inhex


inhex16:

  push  ecx
  mov   cl,4
  jmp   short inhex


inhex8:

  push  ecx
  mov   cl,2


inhex:

  push  edx

  sub   edx,edx
  DO
        kd____inchar

        IFZ   al,'.'
        CANDZ ebx,17
        CANDA cl,2

              call  kd_outchar
              call  inhex8
              and   eax,lthreads-1
              shl   edx,width lthread_no
              add   edx,eax
              EXIT
        FI

        mov   ch,al
        sub   ch,'0'
        EXITC
        IFA   ch,9
              sub   ch,'a'-'0'-10
              EXITC
              cmp   ch,15
              EXITA
        FI
        call  kd_outchar
        shl   edx,4
        add   dl,ch
        dec   cl
        REPEATNZ
  OD
  mov   eax,edx

  pop   edx
  pop   ecx
  ret







;----------------------------------------------------------------------------
;
;       show
;
;----------------------------------------------------------------------------


show macro string,field,aoff

xoff=0
IFNB <aoff>
xoff=aoff
ENDIF

  IFDEF field
  kd____disp  <string>
        IF sizeof field eq 1
              mov   al,[esi+field+xoff]
              kd____outhex8
        ENDIF
        IF sizeof field eq 2
              mov   ax,[esi+field+xoff]
              kd____outhex16
        ENDIF
        IF sizeof field eq 4
              mov   eax,[esi+field+xoff]
              kd____outhex32
        ENDIF
        IF sizeof field eq 8
              mov   eax,[esi+field+xoff]
              kd____outhex32
              mov   al,' '
              kd____outchar
              mov   eax,[esi+field+xoff+4]
              kd____outhex32
        ENDIF
  ENDIF
  endm


;--------------------------------------------------------------------------
;
;       
;       Kdebug cacheable/uncacheable
;
;
; PRECONDITION      phys mem idempotent
;
;--------------------------------------------------------------------------


  icode


init_kdebug_cacheability_control:

  call  [grab_frame]
  mov   edi,eax

  add   eax,page_present+page_write_permit+global_page+page_cache_disable
  mov   ds:[kdebug_uncacheable_PDE],eax

  mov   ebx,offset dcod_start
  and   ebx,-pagesize
  mov   ecx,offset scod_start
  add   ecx,pagesize-1
  and   ecx,-pagesize

  sub   eax,eax
  DO
        IFAE  eax,ebx
        CANDB eax,ecx
              or    eax,page_cache_disable
        FI
        or    eax,page_present+page_write_permit+global_page

        mov   [edi],eax

        and   eax,-pagesize
        add   edi,4
        add   eax,pagesize
        cmp   eax,MB4
        REPEATB
  OD

  ret


  icod  ends






make_kdebug_uncacheable:

  mov   bl,breakpoint
  call  get_exception_handler
  IFNZ  eax,offset direct_kdebug_exception_handler      ;
        mov   ds:[int3_exception_handler],eax           ; ensures that no kernel code is executed
        mov   eax,offset direct_kdebug_exception_handler; to prevent instr fetches from cacheable
        call  set_exception_handler                     ; pages
  FI

  mov   eax,ds:[kdebug_uncacheable_PDE]
  mov   edi,PM/MB4*4
  call  set_pde_in_all_pdirs

  mov   ecx,page_cache_disable
  mov   esi,PM+MB4
  mov   edi,PM+physical_kernel_mem_size
  call  modify_cacheability_through_all_pdirs

  mov   eax,0+page_present+superpage
  mov   edi,offset kdebug_cached_alias_mem/MB4*4
  call  set_pde_in_all_pdirs

  call  flush_tlb_including_global_pages

  push  ds
  push  linear_kernel_space
  pop   ds

  mov   esi,esp
  call  test_cache
  mov   esi,esp
  call  test_cache
  
  pop   ds

  mov   [typical_cached_data_cycles],eax

  ret




make_kdebug_and_kernel_cacheable:

  mov   bl,breakpoint
  call  get_exception_handler
  IFZ   eax,offset direct_kdebug_exception_handler      ;
        mov   eax,ds:[int3_exception_handler]           ; resetting to normal INT 3 handling 
        call  set_exception_handler                     ; reason: see above
  FI

  mov   eax,0+page_present+page_write_permit+superpage+global_page
  mov   edi,PM/MB4*4
  call  set_pde_in_all_pdirs

  mov   ecx,NOT page_cache_disable
  mov   esi,PM+MB4
  mov   edi,-1
  call  modify_cacheability_through_all_pdirs

  call  flush_tlb_including_global_pages
  ret




make_kernel_uncacheable:

  mov   bl,breakpoint
  call  get_exception_handler
  IFZ   eax,offset direct_kdebug_exception_handler      ;
        mov   eax,ds:[int3_exception_handler]           ; resetting to normal INT 3 handling 
        call  set_exception_handler                     ; reason: see above
  FI

  mov   eax,0+page_present+page_write_permit+superpage+global_page+page_cache_disable
  mov   edi,PM/MB4*4
  call  set_pde_in_all_pdirs

  mov   ecx,page_cache_disable
  mov   esi,PM+MB4
  mov   edi,-1
  call  modify_cacheability_through_all_pdirs

  mov   eax,0+page_present+superpage
  mov   edi,offset kdebug_cached_alias_mem/MB4*4
  call  set_pde_in_all_pdirs

  call  flush_tlb_including_global_pages
  
  wbinvd

  ret




modify_cacheability_through_all_pdirs:                  ; ECX : mask
                                                        ; ESI : start addr (4MB aligned)
                                                        ; EDI : end addr (FFFFFFFF = 4G) (4K aligned)
  IF    kernel_family EQ lime_pip

        push  ds
        push  linear_kernel_space
        pop   ds

        mov   edx,offset task_proot
        DO
              mov   ebx,[edx].proot_ptr
              add   edx,8
              and   ebx,0FFFFFF00h
              REPEATZ
        
              EXITS long

              push  esi

              mov   eax,esi
              shr   eax,22
              lea   ebx,[ebx+eax*4+PM]
              sub   esi,MB4
              sub   ebx,4

              DO
                    add   esi,MB4
                    EXITC
                    add   ebx,4
                    cmp   esi,edi
                    EXITAE

                    mov   eax,[ebx]
                    test  eax,page_present
                    REPEATZ

                    test  ecx,ecx
                    IFS
                          and   eax,ecx
                    ELSE_
                          or    eax,ecx
                    FI
                    mov   [ebx],eax

                    test  eax,superpage
                    REPEATNZ

                    IFDEF pdir_space
                          cmp   esi,offset pdir_space
                          REPEATZ
                    ENDIF
                    IFDEF ptab_space
                          cmp   esi,offset ptab_space
                          REPEATZ
                    ENDIF

                    push  ebx
                    push  esi
                    mov   ebx,eax
                    and   ebx,-pagesize
                    add   ebx,PM
                    DO
                          mov   eax,[ebx]
                          test  eax,page_present
                          IFNZ
                          IFDEF shadow_pdir
                                CANDNZ esi,offset shadow_pdir
                          ENDIF
                                test  ecx,ecx
                                IFS
                                      and   eax,ecx
                                ELSE_
                                      or    eax,ecx
                                FI
                                mov   [ebx],eax
                          FI
                          add   esi,pagesize
                          add   ebx,4
                          cmp   esi,edi
                          EXITAE
                          test  ebx,pagesize-1
                          REPEATNZ
                    OD
                    pop   esi
                    pop   ebx
                    REPEAT
              OD

              pop   esi
              REPEAT                
        OD

        pop   ds
        ret

  
  ELSE

        ke    '-task_proot'
        ret

  ENDIF






        


set_pde_in_all_pdirs:                   ; EAX : new PDE, EDI : PDE offset in pdirs


  IF    kernel_family EQ lime_pip

        push  ds
        push  linear_kernel_space
        pop   ds

        push  eax
        push  edx
        push  ebx
            
        mov   edx,offset task_proot
        DO
              mov   ebx,[edx].proot_ptr
              add   edx,8
              and   ebx,0FFFFFF00h
              REPEATZ
        
              EXITS

              mov   dword ptr [ebx+edi+PM],eax
  
              REPEAT                
        OD
        
        pop   ebx
        pop   edx
        pop   eax
        pop   ds
        ret

  ELSE

        ke    '-task_proot'
        ret

  ENDIF




flush_tlb_including_global_pages:

  push  eax
  mov   eax,cr3
  mov   cr3,eax
  mov   eax,PM
  DO
        invlpg [eax]  
        add    eax,pagesize
        REPEATNC
  OD
  pop   eax
  ret







kdebug_caching_control:

  kd____inchar
  kd____outchar
  IFZ   al,'+'
        call  make_kdebug_and_kernel_cacheable
        kd____disp <'  L4 and L4KD are now cached (default).'>
        ret
  FI
  IFZ   al,'-'
        call  make_kernel_uncacheable
        kd____disp <'  Neither L4 nor L4KD (code+data) are now cached.'>
        ret
  FI
  IFZ   al,'*'
        call  make_kdebug_uncacheable
        
     ;   mov   ecx,286
     ;   rdmsr
     ;   btr   eax,8
     ;   wbinvd
     ;   wrmsr
        
        kd____disp <'  L4 is now cached, L4KD is not (good for perf tracing).'>
        ret
  FI
  kd____disp <'??'>
  ret
















;----------------------------------------------------------------------------
;
;       kdebug exception  (kernel exception)
;
;       direct kdebug exception 
;
;----------------------------------------------------------------------------
; kdebug exception PRECONDITION:
;
;       stack like ipre
;
;----------------------------------------------------------------------------
; direct kdebug exception PRECONDITION:
;
;       no stack prep happened
;
;----------------------------------------------------------------------------




direct_kdebug_exception_handler:


  ipre  trap1,no_ds_load

  rdtsc

  IFZ   ss:[kdebug_stack_bottom+PM],0

        mov   esi,eax
        mov   edi,edx

        mov   ecx,P6_event_select0_msr
        rdmsr
        btr   eax,22      ; disable counters
        wrmsr

        mov   ecx,cr0
        bts   ecx,cr0_cd_bit
        mov   cr0,ecx

        mov   ss:[kdebug_stack_bottom+PM],esp
        add   ss:[kdebug_rdtsc_corrector+PM],esi
        adc   ss:[kdebug_rdtsc_corrector+PM+4],edi

        inc   ss:[kdebug_invocation_stamp+PM]

  FI

  push  breakpoint
  push  0





default_kdebug_exception:

  pop   ebx                           ; drop ret address
  pop   eax                           ; get exception no

  kdpre

  lea   ebp,[esp+sizeof kd_save_area]


  IFAE  ebp,virtual_space_size
        push  phys_mem
        pop   ds


        IFNZ  [performance_count_params],0

              push  eax
              mov   al,[processor_family]

              IFZ   al,p5_family
                    mov   ecx,P5_event_counter0_msr
                    rdmsr
                    mov   [perf_counter0_value],eax
                    mov   ecx,P5_event_counter1_msr
                    rdmsr
                    mov   [perf_counter1_value],eax

              ELIFZ al,p6_family
                    mov   ecx,P6_event_counter0_msr
                    rdmsr
                    mov   [perf_counter0_value],eax
                    mov   ecx,P6_event_counter1_msr
                    rdmsr
                    mov   [perf_counter1_value],eax
              FI
              pop   eax
        FI                               

        push  eax 
        movzx eax,[configuration].start_port
        IFA   eax,16
              call  set_remote_info_mode
        FI      
        pop   eax
  FI
 
  
  IF    kernel_x2
        call  enter_single_processor_mode
  ENDIF      

  movzx eax,al
  lea   esi,[(eax*2)+id_table]

  IFZ   al,3,long
        mov   ebx,[ebp+ip_eip]

        IFZ   [ebp+ip_cs],linear_space_exec
        CANDA ebp,virtual_space_size

              push  ds
              push  es
              push  eax

              push  ds
              pop   es
              push  linear_space
              pop   ds
              mov   edi,offset kdebug_buffer
              push  edi
              mov   al,sizeof kdebug_buffer
              DO
                    mov   ah,0                    
                    ktest_page_present ebx
                    IFNZ
                          mov   ah,ds:[ebx]
                    FI
                    mov   es:[edi],ah
                    inc   ebx
                    inc   edi
                    dec   al
                    REPEATNZ
              OD
              pop   ebx

              mov   ebx,offset kdebug_buffer

              pop   eax
              pop   es
              pop   ds

        ELSE_
              and   ebx,NOT PM
        FI

        mov   ax,[ebx]
        cmp   al,3Ch           ; cmp al
        jz    kdebug_io_call
        cmp   al,90h           ; nop
        jz    kdebug_display


        inc   ebx
        IFZ   ah,4
        CANDZ <byte ptr [ebx+4]>,<PM SHR 24>
              mov  ebx,[ebx+1]
              add  ebx,4
        FI
        
        mov   al,[ebx+1]
        IFNZ  al,'*'
              cmp   al,'#'
              IFNZ
                    cmp   al,'/'
              FI
        FI      
        jz    trace_event 
              

  ELIFAE al,8
  CANDBE al,17
  CANDNZ al,16

        mov    cl,12
        mov    edi,offset ec_exception_error_code
        DO
               mov     eax,ss:[ebp+ip_error_code]
               shr     eax,cl
               and     eax,0Fh
               IFB_    al,10
                       add     al,'0'
               ELSE_
                       add     al,'a'-10
               FI
               mov     [edi],al
               inc     edi
               sub     cl,4
               REPEATNC
        OD
        mov    ax,[esi]
        mov    word ptr [ec_exception_id],ax
        mov    ebx,offset ec_exception_string+PM
  ELSE_
        mov    ax,[esi]
        mov    word ptr [exception_id],ax
        mov    ebx,offset exception_string+PM
  FI


  cli

  IFAE  ebp,<physical_kernel_mem_size>
        mov   edi,phys_mem
        mov   ds,edi
        mov   es,edi
        and   ebx,NOT PM

        DO
              mov   edi,[kdebug_sema]
              test  edi,edi
              EXITZ
              xor   edi,esp
              and   edi,-sizeof tcb
              EXITZ
              pushad
              push  ds
              push  es
              sub   esi,esi
              int   thread_switch
              pop   es
              pop   ds
              popad
              REPEAT
        OD
        mov   [kdebug_sema],ebp
  FI


  push  [kdebug_esp]
  push  [kdebug_text]

  mov   [kdebug_esp],ebp
  mov   [kdebug_text],ebx

  if    precise_cycles
        IFAE  esp,virtual_space_size
              push  ds
              push  linear_kernel_space
              pop   ds
              push  ebx
              mov   ebx,esp
              and   ebx,-sizeof tcb
              cpu___cycles ebx
              pop   ebx
              pop   ds
        FI
  endif

;;call  open_debug_keyboard
  call  open_debug_screen

  call  show_active_trace_buffer_tail


  kd____disp  <6,lines-1,0,13,10>
  mov   ecx,columns-12
  DO
        mov   al,'-'
        kd____outchar
        RLOOP
  OD
 
  mov   eax,[ebp+ip_eip]
  kd____outhex32

  kd____disp  <'=EIP',13,10,6,lines-1,6>
  call  out_id_text

  call  determine_gidt

  DO
        call  kernel_debug
        cmp   bl,'g'
        REPEATNZ
  OD

  call  flush_active_trace_buffer

  pop   [kdebug_text]
  pop   [kdebug_esp]

  mov   [kdebug_sema],0

  IFZ   [ebp+ip_error_code],debug_ec
  mov   eax,dr7
  mov   al,[esp+kd_dr7]
  test  al,10b
  CANDNZ
  shr   eax,16
  test  al,11b
  CANDZ
        bts   [ebp+ip_eflags],r_flag
  FI


  if    precise_cycles
        IFAE  esp,virtual_space_size
              push  ds
              push  linear_kernel_space
              pop   ds
              push  ebx
              mov   ebx,esp
              and   ebx,-sizeof tcb
              cpu___cycles ebx
              pop   ebx
              pop   ds
        FI
  endif





ret_from_kdebug:

  IF    kernel_x2
        call  exit_single_processor_mode
  ENDIF      

  kdpost

  IFAE  esp,virtual_space_size
  CANDZ esp,ss:[kdebug_stack_bottom+PM]

        mov   ss:[kdebug_stack_bottom+PM],0

        mov   ecx,cr0
        btr   ecx,cr0_cd_bit
        mov   cr0,ecx

        mov   ebx,ss:[performance_count_params+PM]
        test  ebx,ebx
        IFNZ
              mov   al,ss:[trace_perf_monitoring_mode+PM]
              call  set_performance_counters
        FI

        rdtsc
        sub   ss:[kdebug_rdtsc_corrector+PM],eax
        sbb   ss:[kdebug_rdtsc_corrector+PM+4],edx
  FI

  ipost






id_table db 'DVDBNM03OVBNUD07DF09TSNPSFGPPF15FPAC'

exception_string        db 14,'LN Kernel: #'
exception_id            db 'xx'

ec_exception_string     db 21,'LN Kernel: #'
ec_exception_id         db 'xx ('
ec_exception_error_code db 'xxxx)'




;----------------------------------------------------------------------------
;
;       kernel debug
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       DS,ES    phys mem
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       BL       exit char
;
;       EAX,EBX,ECX,EDX,ESI,EDI,EBP  scratch
;
;----------------------------------------------------------------------------


kernel_debug:
 
  push  ebp

  call  open_debug_keyboard
  call  open_debug_screen
  DO
        kd____disp  <6,lines-1,0,10>
        call  get_kdebug_cmd

        DO
              cmp   al,'g'
              OUTER_LOOP   EXITZ long

              mov   ah,[permissions].flags
              
              IFZ   al,'a'
                    call  display_module_addresses
              ELIFZ al,'b',long
                    call  set_breakpoint
              ELIFZ al,'t',long
                    call  display_tcb
                    cmp   al,0
                    REPEATNZ
              ELIFZ al,'d',long
                    call  dump_cmd
                    cmp   al,0
                    REPEATNZ
              ELIFZ al,'P',long
                    call  page_fault_prot
              ELIFZ al,'v',long
                    call  virtual_address_info
              ELIFZ al,'k',long
                    call  display_kernel_data
              ELIFZ al,'X',long
                    call  monit_exception
              ELIFZ al,'I',long
                    call  ipc_prot
              ELIFZ al,'S',long
                    call  thread_switch_prot
              ELIFZ al,'i',long
                    call  port_io
              ELIFZ al,'o',long
                    call  port_io      
              ELIFZ al,'C',long
                    call  configure_kdebug
              ELIFZ al,'D'
                    call  analyze_data_cache
              IF    kernel_family NE hazelnut
                    ELIFZ al,'H'
                          call  halt_current_thread
              ENDIF
              ELIFZ al,'K'
                    call  ke_disable_reenable      
              ELIFZ al,' '
                    call  out_id_text
              ELIFZ al,'T'
                    call  dump_trace_buffer
                    cmp   al,0
                    REPEATNZ
              IF    random_profiling
              ELIFZ al,'F'
                    call  profile_cmd
                    cmp   al,0
                    REPEATNZ
              ENDIF
              ELIFZ al,'^'
                    call  reset_system      
              ELIFZ al,'y'
              CANDNZ [kdebug_extension],0
                    call  [kdebug_extension]
              ELSE_
                    call  out_help
              FI
        OD
        REPEAT
  OD

  call  close_debug_keyboard
  mov   bl,al
  kd____disp  <13,10>

  pop   ebp

  ret




configure_kdebug:

  call  out_config_help

  DO
        kd____disp <13,10,'L4KD config:'>
        kd____inchar
        kd____outchar

        IFZ   al,'R'
              call  remote_kd_intr
              REPEAT
        FI
        IFZ   al,'N'
              call  enter_thread_nickname
              REPEAT
        FI
        IFZ   al,'C'
              call  kdebug_caching_control
              REPEAT
        FI
        IFZ   al,'V'
              call  set_video_mode
              REPEAT
        FI
        cmp   al,'q'
        EXITZ
        cmp   al,'g'
        EXITZ
        cmp   al,27
        EXITZ
        
        call  out_config_help
        REPEAT
  OD
  ret





get_kdebug_cmd:

  IF    kernel_x2
        kd____disp  <6,lines-1,0,'L4KD('>
        lno___prc eax
        add   al,'a'
        kd____outchar
        kd____disp  <'): '>
  ELSE      
        kd____disp  <6,lines-1,0,'L4KD: '>
  ENDIF     
  kd____inchar
  push  eax
  IFAE  al,20h
        kd____outchar
  FI
  
  pop   eax

  ret




is_main_level_command_key:

  IFNZ   al,'a'
  CANDNZ al,'b'
  CANDNZ al,'t'
  CANDNZ al,'d'
  CANDNZ al,'k'
  CANDNZ al,'P'
  CANDNZ al,'I'
  CANDNZ al,'X'
  CANDNZ al,'T'
  CANDNZ al,'R'
  CANDNZ al,'S'
  CANDNZ al,'i'
  CANDNZ al,'o'
  CANDNZ al,'C'
  CANDNZ al,'F'
  CANDNZ al,'H'
  CANDNZ al,'K'
  CANDNZ al,'g'
  CANDNZ al,'<'
  CANDNZ al,'>'
        IFZ   al,'q'
              mov   al,0
        FI      
  FI
  ret
  
  
  
;----------------------------------------------------------------------------
;
;       reset system
;
;----------------------------------------------------------------------------

reset_system:

  push  ds
  push  phys_mem
  pop   ds
  
  kd____disp <' RESET ? (y/n)'>
  kd____inchar
  mov   ecx,esp
  cmp   al,'y'
  jz    kdebug_reset_pc
  
  pop   ds
  ret
  


;----------------------------------------------------------------------------
;
;       out id text
;
;----------------------------------------------------------------------------


out_id_text:

  mov   al,'"'
  kd____outchar
  mov   eax,[kdebug_text]
  kd____outstring
  mov   al,'"'
  kd____outchar
  ret


;----------------------------------------------------------------------------
;
;       help
;
;----------------------------------------------------------------------------

out_help:

  mov   al,ah
  
  kd____disp  <13,10,'a : modules, all / xxxx            : find module and rel addr'>
  kd____disp  <13,10,'b : bkpnt,  i/w/a/p/b/r/-: set instr/wr/rdwr/ioport bkpnt/base/restrict/off'>
  kd____disp  <13,10,'t : tcb, current / ttttt / TTT.ll'>
  kd____disp  <13,10,'d : b/w/t/p/m/c/T/G/I/S/R    : dump byte/word/text/ptab/mapping/cache/Task/Gdt/Idt/Sig0/Redir'>
  kd____disp  <13,10,'k : kernel data'>              
  test  al,kdebug_protocol_enabled
  IFNZ  ,,long
  kd____disp  <13,10,'P : monit PF     +/-/*/r : on/off/trace/restrict'>
  kd____disp  <13,10,'I : monit ipc    +/-/*/r : on/off/trace/restrict'>
  kd____disp  <13,10,'X : monit exc    +xx/-/*xx: on/off/trace'>
  kd____disp  <13,10,'S : monit tswtch */-     : trace/off'> 
  FI
  IFNZ  [configuration].trace_pages,0
  kd____disp  <13,10,'T :  dump trace'>          
  FI
  test  al,kdebug_io_enabled
  IFNZ  ,,long
  kd____disp  <13,10,'i : in port      1/2/4xxxx: byte/word/dword'>
  kd____disp  <13,10,'    apic/PCIconf a/i/pxxxx : apic/ioapic/PCIconf-dword'>
  kd____disp  <13,10,'o : out port/apic...'>
  FI
  IF    kernel_family NE hazelnut
  kd____disp  <13,10,'H : halt current thread'>
  ENDIF
  kd____disp  <13,10,'^ : reset system'>
  kd____disp  <13,10,'K : ke           -/+xxxxxxxx: disable/reenable'>
  kd____disp  <13,10,'D : data cache analysis'>
  IF    random_profiling
  kd____disp  <13,10,'F : profile analysis'>
  ENDIF
  kd____disp  <13,10,'C : configure kdebug'>
  kd____disp  <13,10,'  : id text'>

  ret


out_config_help:

  kd____disp  <13,10,10,'       Configure Kdebug',13,10,10>
  kd____disp  <13,10,'N : enter thread nickname'>
  kd____disp  <13,10,'R : remote kdint    +/-     : on/off'>
  kd____disp  <13,10,'V : video mode      a/c/m/h/1/2/-  : auto/cga/mono/hercules/com1/com2/no-com'>
  IFAE  [processor_family],p6_family,long
  kd____disp  <13,10,'C : L4/L4KD caching +/-/*   : on (default) / off / L4 on, L4KD off (for perf tracing)'>
  FI
  kd____disp  <13,10,'q : quit config mode',13,10>

  ret


;----------------------------------------------------------------------------
;
;       set video mode
;
;----------------------------------------------------------------------------

set_video_mode:

  kd____inchar
  kd____outchar
  CORZ  al,'2'
  IFZ   al,'1'
        IFZ   al,'1'
              mov   ebx,3F8h SHL 4 
        ELSE_ 
              mov   ebx,2F8h SHL 4 
        FI
        mov   al,byte ptr [configuration].start_port
        and   eax,0Fh
        or    eax,ebx
        mov   [configuration].start_port,ax
        call  set_remote_info_mode
  
  ELIFZ al,'p',long
        kd____disp <'ort: '>
        kd____inhex16  
        
        push  eax      
        kd____disp <' ok? (y/n) '>
        kd____inchar 
        IFNZ  al,'y'
        CANDNZ al,'z'
              pop   eax
              ret
        FI
        pop   eax
  
        shl   eax,4
        mov   bl,byte ptr [configuration].start_port
        and   bl,0Fh
        or    al,bl
        mov   [configuration].start_port,ax
        call  set_remote_info_mode
        
  ELIFZ al,'b',long
        kd____disp <'aud rate divisor: '>
        kd____inhex8
        CORZ  al,0
        IFA   al,15
              mov   al,1     
        FI
        and   byte ptr [configuration].start_port,0F0h
        or    byte ptr [configuration].start_port,al
        IFZ   al,12
              kd____disp <' 9600'>
        ELIFZ al,6
              kd____disp <' 19200'>
        ELIFZ al,3
              kd____disp <' 38400'>
        ELIFZ al,2
              kd____disp <' 57600'>      
        ELIFZ al,1
              kd____disp <' 115200'>      
        FI            

  ELSE_
        kd____outchar
        call  init_kdio
  FI

  ret



;----------------------------------------------------------------------------
;
;       ke disable / reenable
;
;----------------------------------------------------------------------------


ke_disable_reenable:

  IFA   esp,virtual_space_size
  
        kd____inchar
        mov   bl,al
        kd____outchar
        kd____inhex32
        
        mov   ebp,[kdebug_esp]

        test  eax,eax
        IFZ
              mov   eax,ss:[ebp+ip_eip]
              test  byte ptr ss:[ebp+ip_cs],11b
              IFZ
                    add   eax,PM
              FI
        FI
        
        push  ds
        push  linear_kernel_space
        pop   ds
        
        dec   eax
        test__page_writable eax
        IFNC
              IFZ   bl,'-'
              CANDZ <byte ptr ds:[eax]>,0CCh
                    mov   byte ptr ds:[eax],90h
              ELIFZ bl,'+'
              CANDZ <byte ptr ds:[eax]>,90h
                    mov   byte ptr ds:[eax],0CCh
              FI      
        FI
        pop   ds                    
  FI
  ret
              


;----------------------------------------------------------------------------
;
;       halt current thread
;
;----------------------------------------------------------------------------

  IF    kernel_family NE hazelnut

halt_current_thread:

  kd____disp  <' are you sure? '>
  kd____inchar
  CORZ  al,'j'
  IFZ   al,'y'
        kd____disp <'y',13,10>
        call  close_debug_keyboard
        
        sub   eax,eax
        mov   ss:[kdebug_sema+PM],eax

        mov   ebp,esp
        and   ebp,-sizeof tcb

        mov   ss:[ebp+coarse_state],unused_tcb
        mov   ss:[ebp+fine_state],aborted

        DO
              ke 'H'
              sub   esi,esi
              int   thread_switch
              REPEAT
        OD

  FI
  mov   al,'n'
  kd____outchar
  ret

  ENDIF

;----------------------------------------------------------------------------
;
;       display_module_addresses
;
;----------------------------------------------------------------------------


display_module_addresses:

  kd____inhex16
  test  eax,eax
  IFNZ  ,,long

        mov   esi,offset first_lab
        IFB_  eax,<offset cod_start>
              DO
                    call  is_module_header
                    EXITNZ
                    movzx edi,word ptr [esi]
                    test  edi,edi
                    IFZ
                          call  to_next_lab
                          REPEAT
                    FI
                    cmp   eax,edi
                    EXITB
                    mov   ebx,edi
                    mov   edx,esi
                    call  to_next_lab
                    REPEAT
              OD
        ELSE_
              DO
                    call  is_module_header
                    EXITNZ
                    movzx edi,word ptr [esi+2]
                    cmp   eax,edi
                    EXITB
                    mov   ebx,edi
                    mov   edx,esi
                    call  to_next_lab
                    REPEAT
              OD
        FI
        mov   esi,edx
        sub   eax,ebx
        IFNC
              push  eax
              mov   ah,lines-1
              mov   al,20
              kd____cursor
              call  display_module
              kd____disp  <' : '>
              pop   eax
              kd____outhex16
        FI


  ELSE_ long

        kd____clear_page
        mov   al,0
        mov   ah,1
        kd____cursor

        mov   eax,offset kernelstring
        kd____outcstring
        kd____disp <13,10,10,'kernel: '>

        mov   esi,offset first_lab

        DO
              call  is_module_header
              EXITNZ

              movzx edi,word ptr [esi+2]
              IFZ   edi,<offset dcod_start>
                    kd____disp <13,'kdebug: '>
              ELIFZ edi,<offset scod_start>
                    kd____disp <13,'sigma:  '>
              FI

              call  display_module
              kd____disp  <13,10,'        '>

              call  to_next_lab
              REPEAT
        OD
  FI
  ret



is_module_header:
  
  mov   ecx,32
  push  esi
  DO
        cmp   word ptr [esi+8],'C('
        EXITZ
        inc   esi
        RLOOP
  OD
  pop   esi
  ret




to_next_lab:

  add   esi,7
  DO
        inc   esi
        cmp   byte ptr [esi],0
        REPEATNZ
  OD
  DO
        inc   esi
        cmp   byte ptr [esi],0
        REPEATNZ
  OD
  inc   esi
  ret




display_module:

  movzx eax,word ptr [esi]
  test  eax,eax
  IFZ
        kd____disp <'     '>
  ELSE_
        kd____outhex16
        kd____disp <','>
  FI
  movzx eax,word ptr [esi+2]
  kd____outhex16
  kd____disp <', '>

  lea   ebx,[esi+8]
  mov   eax,ebx
  kd____outcstring
  kd____disp <', '>

  DO
        cmp   byte ptr [ebx],0
        lea   ebx,[ebx+1]
        REPEATNZ
  OD
  mov   eax,ebx
  kd____outcstring

  mov   edx,[esi+4]

  kd____disp <', '>

  mov   eax,edx
  and   eax,32-1
  IFB_  eax,10
        push  eax
        mov   al,'0'
        kd____outchar
        pop   eax
  FI
  kd____outdec
  mov   al,'.'
  kd____outchar
  mov   eax,edx
  shr   eax,5
  and   eax,16-1
  IFB_  eax,10
        push  eax
        mov   al,'0'
        kd____outchar
        pop   eax
  FI
  kd____outdec
  mov   al,'.'
  kd____outchar
  mov   eax,edx
  shr   eax,5+4
  and   eax,128-1
  add   eax,90
  IFAE  eax,100
        sub   eax,100
  FI
  IFB_  eax,10
        push  eax
        mov   al,'0'
        kd____outchar
        pop   eax
  FI
  kd____outdec

  mov   al,' '
  kd____outchar
  mov   eax,edx
  shr   eax,16+10
  kd____outdec
  mov   al,'.'
  kd____outchar
  mov   eax,edx
  shr   eax,16
  and   ah,3
  kd____outdec

  ret



;----------------------------------------------------------------------------
;
;       set breakpoint
;
;----------------------------------------------------------------------------


set_breakpoint:

  mov   ebp,[kdebug_esp]

  kd____inchar
  IFZ   al,13

        mov   ah,lines-1
        mov   al,20
        kd____cursor

        mov   eax,dr7
        mov   al,[ebp-sizeof kd_save_area+kd_dr7]
        test  al,10b
        IFZ
              mov   al,'-'
              kd____outchar
        ELSE_
              shr   eax,8
              mov   al,'I'
              test  ah,11b
              IFNZ
                    mov   al,'W'
                    test  ah,10b
                    IFNZ
                          mov    al,'A'
                    FI
                    kd____outchar
                    mov   eax,dr7
                    shr   eax,18
                    and   al,11b
                    add   al,'1'
              FI
              kd____outchar
              kd____disp  <' at '>
              mov   ebx,dr0
              mov   eax,[breakpoint_base]
              test  eax,eax
              IFNZ
                    sub   ebx,eax
                    kd____outhex32
                    mov   al,'+'
                    kd____outchar
              FI
              mov   eax,ebx
              kd____outhex32
        FI
        ret
  FI

  IFZ   al,'-'
        kd____outchar
        sub   eax,eax
        mov   dr7,eax
        mov   [ebp-sizeof kd_save_area+kd_dr7],al
        mov   dr6,eax
        mov   [bx_size],al
        sub   eax,eax
        mov   [debug_breakpoint_counter_value],eax
        mov   [debug_breakpoint_counter],eax

        mov   eax,[debug_exception_handler]
        test  eax,eax
        IFNZ
              mov   bl,debug_exception
              call  set_exception_handler
              mov   [debug_exception_handler],0
        FI
        ret
  FI

  push  eax
  IFZ   [debug_exception_handler],0
        mov   bl,debug_exception
        call  get_exception_handler
        mov   [debug_exception_handler],eax
  FI
  mov   bl,debug_exception
  mov   eax,offset kdebug_debug_exception_handler
  call  set_exception_handler
  pop   eax

  IFZ   al,'b'
        kd____outchar
        kd____inhex32
        mov   [breakpoint_base],eax
        ret
  FI

  CORZ  al,'p'
  CORZ  al,'i'
  CORZ  al,'w'
  IFZ   al,'a'
        sub   ecx,ecx
        IFNZ  al,'i'
              IFZ   al,'w'
                    mov   cl,01b
              FI
              IFZ   al,'p'
                    mov   cl,10b
              FI            
              IFZ   al,'a'
                    mov   cl,11b
              FI      
              kd____outchar
              kd____inchar
              IFZ   al,'2'
                    or    cl,0100b
              ELIFZ al,'4'
                    or    cl,1100b
              ELSE_
                    mov   al,'1'
              FI
        FI
        kd____outchar
        shl   ecx,16
        mov   cx,202h
        kd____disp  <' at: '>
        mov   eax,[breakpoint_base]
        test  eax,eax
        IFNZ
              kd____outhex32
              mov   al,'+'
              kd____outchar
        FI
        kd____inhex32
        add   eax,[breakpoint_base]
        mov   dr0,eax
        mov   dr7,ecx
        mov   [ebp-sizeof kd_save_area+kd_dr7],cl
        sub   eax,eax
        mov   dr6,eax

        ret
  FI

  IFZ   al,'r',long
        kd____disp <'r',6,lines-1,columns-58,'t/T/124/e/-: thread/non-thread/monit124/reg/reset restrictions',6,lines-1,8>
        kd____inchar
        kd____disp <5>
        kd____outchar
        
        IFZ   al,'-'
              sub   eax,eax
              mov   [bx_size],al
              mov   [breakpoint_thread],eax
              mov   [no_breakpoint_thread],eax
              ret
        FI


        CORZ  al,'e'
        CORZ  al,'1'
        CORZ  al,'2'
        IFZ   al,'4',long
              sub   al,'0'
              mov   [bx_size],al
              IFZ   al,'e'-'0',long
                    kd____disp <8,' E'>
                    sub   ebx,ebx
                    kd____inchar
                    and   al,NOT ('a'-'A')
                    mov   ah,'X'
                    IFZ   al,'A'
                          mov   bl,7*4
                    ELIFZ al,'B'
                          kd____outchar
                          kd____inchar
                          and   al,NOT ('a'-'A')
                          IFZ   al,'P'
                                mov     bl,2*4
                                mov     ah,al
                          ELSE_            
                                mov     bl,4*4
                                mov     ah,'X'
                          FI
                          mov   al,0
                    ELIFZ al,'C'
                          mov   bl,6*4
                    ELIFZ al,'D'
                          kd____outchar
                          kd____inchar
                          and   al,NOT ('a'-'A')
                          IFZ   al,'I'
                                mov     bl,0*4
                                mov     ah,al
                          ELSE_            
                                mov     bl,5*4
                                mov     ah,'X'
                          FI
                          mov   al,0
                    ELIFZ al,'S'
                          mov   bl,1*4
                          mov   ah,'I'
                    ELIFZ al,'I'
                          mov   bl,8*4+iret_eip+4
                          mov   ah,'P'      
                    FI
                    IFNZ  al,0
                          push  eax
                          kd____outchar
                          pop   eax
                    FI
                    mov   al,ah
                    kd____outchar            
                    mov   eax,ebx
                                      
              ELSE_                                
                    
                    kd____disp  <' at ',>
                    kd____inhex32
                    
              FI      
              mov   [bx_addr],eax
              kd____disp  <'  ['>
              kd____inhex32
              mov   [bx_low],eax
              mov   al,','
              kd____outchar
              kd____inhex32
              mov   [bx_high],eax
              mov   al,']'
              kd____outchar
              ret
        FI      

        IFZ   al,'t'
              kd____inhex16
              mov   [breakpoint_thread],eax
              ret
        FI

        IFZ   al,'T'
              kd____inhex16
              mov   [no_breakpoint_thread],eax
              ret
        FI
        
        mov   al,'?'
        kd____outchar
        ret
  FI       

  IFZ   al,'#'
        kd____outchar
        kd____inhex32
        mov   [debug_breakpoint_counter_value],eax
        mov   [debug_breakpoint_counter],eax
  FI
        
  ret




kdebug_debug_exception_handler:


  push  eax
  mov   eax,dr6
  and   al,NOT 1b
  mov   dr6,eax

  lno___thread eax,esp

  IFZ   ss:[no_breakpoint_thread+PM],eax
        pop   eax
        bts   [esp+iret_eflags],r_flag
        iretd
  FI

  IFNZ  ss:[breakpoint_thread+PM],0
  cmp   ss:[breakpoint_thread+PM],eax
  CANDNZ
        pop   eax
        bts   [esp+iret_eflags],r_flag
        iretd
  FI
  pop   eax

  
  call  check_monitored_data
  IFNZ
        bts   [esp+iret_eflags],r_flag
        iretd
  FI      
  
  IFA   esp,virtual_space_size
  CANDA ss:[debug_breakpoint_counter_value+PM],0
        dec   ss:[debug_breakpoint_counter+PM]
        IFNZ 

              bts   [esp+iret_eflags],r_flag
              iretd

        FI      
        push  eax
        mov   eax,ss:[debug_breakpoint_counter_value+PM]     
        mov   ss:[debug_breakpoint_counter+PM],eax
        pop   eax
  FI                 
        

  jmp   ss:[debug_exception_handler+PM]



;----------------------------------------------------------------------------
;
;       check monitored data
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       Z     monitored data meets condition
;
;       NZ    no data monitored OR NOT monitored data meets condition
;
;----------------------------------------------------------------------------


check_monitored_data:

  IFAE   esp,virtual_space_size,long
  CANDNZ ss:[bx_size+PM],0,long
  CANDZ  ss:[debug_exception_active_flag+PM],false,long

        pushad

        mov   ss:[debug_exception_active_flag+PM],true
        mov   ebx,ss:[bx_addr+PM]
        mov   ecx,ss:[bx_low+PM]
        mov   edx,ss:[bx_high+PM]
        mov   al,ss:[bx_size+PM]
        IFZ   al,1
              movzx eax,byte ptr ss:[ebx]
        ELIFZ al,2
              movzx eax,word ptr ss:[ebx]
        ELIFZ al,4      
              mov   eax,ss:[ebx]
        ELSE_
              mov   eax,ss:[esp+ebx]
        FI
        mov   ss:[debug_exception_active_flag+PM],false

        IFBE  ecx,edx
              CORB  eax,ecx
              IFA   eax,edx
                    popad
                    test  esp,esp              ; NZ !
                    ret
              FI
        ELSE_
              IFA   eax,edx
              CANDB eax,ecx
                    popad
                    test  esp,esp              ; NZ !
                    ret
              FI
        FI
        popad
  FI
  
  cmp   eax,eax                                ; Z !
  ret




;----------------------------------------------------------------------------
;
;       display tcb
;
;----------------------------------------------------------------------------


display_tcb:

  IFB_  esp,virtual_space_size

        kd____clear_page
        mov   ebp,[kdebug_esp]
        add   ebp,sizeof tcb/2
        mov   esi,ebp
        call  display_regs_and_stack
        
        ret
  FI

  push  ds
  push  es
  mov   eax,linear_kernel_space
  mov   ds,eax
  mov   es,eax

  mov   ebp,esp
  and   ebp,-sizeof tcb

  kd____inhext
  test  eax,eax
  IFZ
        mov   esi,ebp
  ELSE_
        IFB_  eax,threads
              shl   eax,thread_no
        FI
        lea___tcb esi,eax
  FI

  ktest_page_present esi
  IFC
        kd____disp  <' not mapped, force mapping (y/n)? '>
        kd____inchar
        IFNZ  al,'y'
        CANDNZ al,'j'
              mov   al,'n'
              kd____outchar
              mov   al,0
              pop   es
              pop   ds
              ret
        FI
        or    byte ptr [esi],0
  FI
 
  kd____clear_page

  kd____disp  <6,0,0,'thread: '>
  mov   eax,[esi+myself]
  lno___thread eax,eax
  push  eax
  kd____outhex16
  kd____disp <' ('>
  pop   eax
  
  call  outthread

  show  <') ',60>,myself
  mov   al,62
  kd____outchar

  IFNZ  [esi+ressources],0
        kd____disp  <6,0,45,'resrc: '>
        IF    kernel_family NE hazelnut
              mov   al,[esi+ressources]
              test  al,mask x87_used
              IFNZ
                    push  eax
                    kd____disp <'num '>
                    pop   eax
              FI
              test  al,mask dr_used
              IFNZ
                    push  eax
                    kd____disp <'dr '>
                    pop   eax
              FI
              and   al,NOT (x87_used+dr_used)
              IFNZ
                    kd____outhex8
              FI
        ELSE
              mov   eax,[esi+ressources]
              kd____outhex32
        ENDIF
  FI


  show  <6,1,0,'state : '>,coarse_state
  kd____disp  <', '>
  mov   bl,byte ptr [esi+fine_state]
  IFDEF nwait
        test  bl,nwait
        IFZ
              kd____disp  <'wait '>
        FI
  ENDIF
  IFDEF nclos
        test  bl,nclos
        IFZ
              kd____disp  <'clos '>
        FI
  ENDIF
  test  bl,nlock
  IFZ
        kd____disp  <'lock '>
  FI
  test  bl,npoll
  IFZ
        kd____disp  <'poll '>
  FI
  test  bl,nready
  IFZ
        kd____disp  <'ready '>
  FI
  IFDEF nwake
        test  bl,nwake
  ELSE
        mov   eax,[esi+list_state]
        not   eax
        test  eax,is_wakeup
  ENDIF
  IFZ
        show  <',  wakeup: '>,wakeup
        show  <'+'>,wakeup+4
  FI
  show  <6,1,45,'lists: '>,list_state

  show  <6,0,72,'prio: '>,prio
  IFDEF max_controlled_prio
        IFNZ  [esi+max_controlled_prio],0
              show  <6,1,73,'mcp: '>,max_controlled_prio
        FI
  ENDIF

  IFDEF state_sp
        movzx eax,[esi+state_sp]
        shl   eax,2
        IFNZ
              push  eax
              kd____disp <6,2,42,'state_sp: '>
              pop   eax
              kd____outhex12
        FI
  ENDIF
  

  IFDEF wait_for
        kd____disp <6,3,0, 'wait for: '>
        lea   ebx,[esi+waiting_for]
        call  show_thread_id
  ENDIF

  kd____disp  <6,4,0, 'sndq    : '>
  IFDEF sndq_root
        lea   ecx,[esi+sndq_root]
        call  show_llinks
  ELSE
        lea   ebx,[esi+send_queue_head]
        call  show_thread_id
  ENDIF      
  mov   al,' '
  kd____outchar
  lea   ecx,[esi+sndq_llink]
  call  show_llinks

  show  <6,3,40,' rcv descr: '>,rcv_descriptor
  show  <6,4,40,'  timeouts: '>,timeouts

  show  <6,3,60,'   partner: '>,com_partner
  IFDEF waddr
        kd____disp  <6,4,60,'   waddr0/1: '>
        mov   eax,[esi+waddr]
        shr   eax,22
        kd____outhex12
        mov   al,'/'
        kd____outchar
        movzx eax,word ptr [esi+waddr]
        shr   eax,22-16
        kd____outhex12
  ENDIF

  if    precise_cycles
        kd____disp  <6,5,0, 'cpu cycles: '>
        mov   eax,[esi+cpu_cycles+4]
        kd____outhex32
        mov   eax,[esi+cpu_cycles]
        kd____outhex32
  else
        IFDEF cpu_clock
              kd____disp  <6,5,0, 'cpu time: '>
              mov   eax,[esi+cpu_clock+4]
              kd____outhex8
              mov   eax,[esi+cpu_clock]
              kd____outhex32
        ENDIF
  endif

  show  <' timeslice: '>,rem_timeslice
  mov   al,'/'
  kd____outchar
  IF    sizeof timeslice eq 1
        mov   al,[esi+timeslice]
        kd____outhex8
  ELSE
        mov   eax,[esi+timeslice]
        kd____outhex32
  ENDIF

  IFDEF pager
        kd____disp <6,7,0, 'pager   : '>
        lea   ebx,[esi+pager]
        call  show_thread_id
  ENDIF      

  IFDEF int_preepmter
        kd____disp <6,8,0, 'ipreempt: '>
        lea   ebx,[esi+int_preempter]
        call  show_thread_id
  ENDIF

  IFDEF ext_preempter
        kd____disp <6,9,0, 'xpreempt: '>
        lea   ebx,[esi+ext_preempter]
        call  show_thread_id
  ENDIF

  kd____disp  <6, 7,40, 'prsent lnk: '>
  test  [esi+list_state],is_present
  IFNZ
        lea   ecx,[esi+present_llink]
        call  show_llinks
  FI
  kd____disp  <6, 8,40, 'ready link : '>
  IFDEF ready_llink
        test  [esi+list_state],is_ready
        IFNZ
              lea   ecx,[esi+ready_llink]
              call  show_llinks
        FI
  ELSE
        lea   ecx,[esi+ready_link]
        call  show_link
        kd____disp  <6,9,40, 'intr link : '>
        lea   ecx,[esi+interrupted_link]
        call  show_link
  ENDIF

  IFDEF soon_wakeup_link
        kd____disp  <6,10,40, 'soon wakeup lnk: '>
        test  [esi+list_state],is_soon_wakeup
        IFNZ
              lea   ecx,[esi+soon_wakeup_link]
              call  show_link
        FI
  ENDIF
  IFDEF late_wakeup_link
        kd____disp  <6,11,40, 'late wakeup lnk: '>
        test  [esi+list_state],is_late_wakeup
        IFNZ  
              lea   ecx,[esi+late_wakeup_link]
              call  show_link
        FI      
  ENDIF

  IFNZ  [esi+thread_idt_base],0
        kd____disp  <6,7,63,'IDT: '>
        mov   eax,[esi+thread_idt_base]
        kd____outhex32
  FI

  IFDEF thread_dr0
        mov   eax,[esi+thread_dr7]
        test  al,10101010b
        IFZ   ,,long
        test  al,01010101b
        CANDNZ
              kd____disp  <6,9,63,'DR7: '>
              mov   eax,[esi+thread_dr7]
              kd____outhex32
              kd____disp  <6,10,63,'DR6: '>
              mov   al,[esi+thread_dr6]
              mov   ah,al
              and   eax,0000F00Fh
              kd____outhex32
              kd____disp  <6,11,63,'DR3: '>
              mov   eax,[esi+thread_dr3]
              kd____outhex32
              kd____disp  <6,12,63,'DR2: '>
              mov   eax,[esi+thread_dr2]
              kd____outhex32
              kd____disp  <6,13,63,'DR1: '>
              mov   eax,[esi+thread_dr1]
              kd____outhex32
              kd____disp  <6,14,63,'DR0: '>
              mov   eax,[esi+thread_dr0]
              kd____outhex32
        FI
  ENDIF


  call  display_regs_and_stack

  pop   es
  pop   ds
  ret




show_thread_id:

  IFZ   <dword ptr [ebx]>,0

        kd____disp <'--'>
  ELSE_
        mov   eax,[ebx]
        lno___thread eax,eax
        kd____outhex16
        kd____disp  <'  ',60>
        mov   eax,[ebx]
        kd____outhex32
        mov   al,' '
        kd____outchar
        mov   eax,[ebx+4]
        kd____outhex32
        mov   al,62
        kd____outchar
  FI

  ret


outthread:

  push  ds
  push  phys_mem
  pop   ds

  push  eax
  push  ecx

  mov   ecx,offset thread_nickname_table
  DO
        IFZ   eax,ds:[ecx].number
              
              lea   eax,[ecx].nickname
              kd____outcstring

              pop   ecx
              pop   eax
              pop   ds
              ret
        FI
        add   ecx,sizeof thread_nickname_entry
        cmp   ecx,offset thread_nickname_table+sizeof thread_nickname_table
        REPEATB
  OD

  pop   ecx
  pop   eax           
  pop   ds     

  push  eax

  push  eax
  shr   eax,width lthread_no
  kd____outhex12
  mov   al,'.'
  kd____outchar
  pop   eax
  
  and   al,lthreads-1
  kd____outhex8

  pop   eax
  ret




show_llinks:

  mov   eax,[ecx].succ
  test  eax,eax
  IFNZ
  CANDNZ eax,-1
        call  show_link
        mov   al,1Dh
        kd____outchar
        add   ecx,offset pred
        call  show_link
        sub   ecx,offset pred
  FI
  ret



show_link:

  mov   eax,[ecx]
  IFDEF intrq_llink
        IFAE  eax,<offset intrq_llink>
        CANDB eax,<offset intrq_llink+sizeof intrq_llink>
              push  eax
              kd____disp  <' i'>
              pop   eax
              sub   eax,offset intrq_llink
              shr   eax,3
              kd____outhex8
              ret
        FI
        IFAE  eax,<offset dispatcher_tcb>
        CANDB eax,<offset dispatcher_tcb+sizeof tcb>
              kd____disp  <' -- '>
              ret
        FI
        IFAE  eax,<offset intrq_llink>
        CANDB eax,<scheduler_control_block_size>
              kd____disp  <' -- '>
              ret
        FI
  ENDIF
  IFAE  eax,virtual_space_size
        lno___thread eax,eax
        kd____outhex16
        ret
  FI
  test  eax,eax
  IFZ
        kd____disp  <' -- '>
        ret
  FI
  kd____outhex32
  ret




show_reg macro txt,reg

  kd____disp  <txt>
  mov   eax,[ebp+ip_&reg]
  kd____outhex32
  endm



show_sreg macro txt,sreg

  kd____disp  <txt>
  mov   ax,[ebp+ip_&sreg]
  kd____outhex16
  endm




display_regs_and_stack:

  sub   eax,eax
  IFAE  esp,virtual_space_size
        mov   eax,PM
  FI

  test  ss:[eax+permissions].flags,kdebug_dump_regs_enabled
  IFZ
        mov   al,0
        ret
  FI
        

  IFZ   esi,ebp
  mov   eax,ss:[eax+kdebug_esp]
  test  eax,eax
  CANDNZ
        mov   ebx,eax
        mov   ecx,eax
        lea   ebp,[eax+sizeof int_pm_stack-sizeof iret_vec]
  ELSE_
        mov   ebx,[esi+thread_esp]
        test  bl,11b
        CORNZ
        CORB  ebx,esi
        lea   ecx,[esi+sizeof tcb]
        IFAE  ebx,ecx
              sub   ebx,ebx
        FI
        sub   ecx,ecx                 ; EBX : stack top
        mov   ebp,ebx                 ; ECX : reg pointer / 0
  FI                                  ; EBP : cursor pointer

; IFAE  ebx,KB256
;       CORB  ebx,esi
;       lea   eax,[esi+sizeof pl0_stack]
;       IFAE  ebx,eax
;             mov   al,0
;             ret
;       FI
; FI


  DO
        pushad
        call  show_regs_and_stack
        popad

        call  get_kdebug_cmd
        call  is_main_level_command_key
        EXITZ

        IFZ   al,2
              add   ebp,4
        FI
        IFZ   al,8
              sub   ebp,4
        FI
        IFZ   al,10
              add   ebp,32
        FI
        IFZ   al,3
              sub   ebp,32
        FI
        mov   edx,ebp
        and   edx,-sizeof tcb
        add   edx,sizeof pl0_stack-4
        IFA   ebp,edx
              mov   ebp,edx
        FI
        IFB_  ebp,ebx
              mov   ebp,ebx
        FI

        IFZ   al,13
              lea   ecx,[ebp-(sizeof int_pm_stack-sizeof iret_vec)]
              IFB_  ecx,ebx
                    mov   ecx,ebx
              FI
        FI

        REPEAT
  OD
  ret



show_regs_and_stack:

  test  ecx,ecx
  IFNZ  ,,long
        push  ebp
        mov   ebp,ecx
        show_reg <6,11,0, 'EAX='>,eax
        show_reg <6,12,0, 'EBX='>,ebx
        show_reg <6,13,0, 'ECX='>,ecx
        show_reg <6,14,0, 'EDX='>,edx
        show_reg <6,11,14,'ESI='>,esi
        show_reg <6,12,14,'EDI='>,edi
        show_reg <6,13,14,'EBP='>,ebp

        sub   eax,eax
        IFAE  esp,virtual_space_size
              mov   eax,PM
        FI
        IFZ   ebp,[eax+kdebug_esp]

              kd____disp <6,11,28,'DS='>
              mov   ax,[ebp-sizeof kd_save_area].kd_ds
              kd____outhex16
              kd____disp <6,12,28,'ES='>
              mov   ax,[ebp-sizeof kd_save_area].kd_es
              kd____outhex16
        ELSE_
              kd____disp <6,11,28,'       ',6,12,28,'       '>
        FI
        pop   ebp
  FI

  kd____disp  <6,14,14,'ESP='>
  mov   eax,ebp
  kd____outhex32

  test  ebp,ebp
  IFNZ  ,,long

        lea   ebx,[esi+sizeof pl0_stack-8*8*4]
        IFA   ebx,ebp
              mov   ebx,ebp
        FI
        and   bl,-32
        mov   cl,16
        DO
              mov   ah,cl
              mov   al,0
              kd____cursor
              mov   eax,ebx
              kd____outhex12
              mov   al,':'
              kd____outchar
              mov   al,5
              kd____outchar
              add   ebx,32
              inc   cl
              cmp   cl,16+8
              REPEATB
        OD
        lea   ebx,[esi+sizeof pl0_stack]
        sub   ebx,ebp
        IFC
              sub   ebx,ebx
        FI
        shr   ebx,2
        DO
              cmp   ebx,8*8
              EXITBE
              sub   ebx,8
              REPEAT
        OD
        sub   ebx,8*8
        neg   ebx
        DO
              mov   eax,ebx
              and   al,7
              imul  eax,9
              IFAE  eax,4*9
                    add   eax,3
              FI
              add   eax,6
              mov   ah,bl
              shr   ah,3
              add   ah,16
              kd____cursor
              mov   eax,[ebp]
              kd____outhex32
              inc   ebx
              add   ebp,4
              cmp   ebx,8*8
              REPEATB
        OD
  FI

  ret



;----------------------------------------------------------------------------
;
;       dump cmd
;
;----------------------------------------------------------------------------


dump_cmd:

  kd____inchar
  kd____outchar

  mov   ah,[permissions].flags

  IFZ   al,'d'
        mov   al,'w'
  FI
  CORZ  al,'b'
  CORZ  al,'w'
  IFZ   al,'t'

        test  ah,kdebug_dump_mem_enabled
        IFZ
              mov   al,0
              ret
        FI
        
        mov   dl,al
        sub   ebx,ebx
        mov   ecx,linear_kernel_space
        lsl   ecx,ecx
        and   ecx,-4
        kd____inhex32
        call  display_mem
        ret
  FI

  mov   ecx,ss
  IFZ   ecx,linear_kernel_space,long

        mov   dl,'w'

        IFZ   al,'G'
              mov   eax,ss:[gdt_descr+PM].base
              mov   ebx,eax
              movzx ecx,ss:[gdt_descr+PM].limit
              add   ecx,32
              call  display_mem
              ret
        FI

        IFZ   al,'I'
              mov   eax,ss:[idt_descr+PM].base
              mov   ebx,eax
              movzx ecx,ss:[idt_descr+PM].limit
              add   ecx,32
              call  display_mem
              ret
        FI

        IFDEF task_proot
              IFZ   al,'T'
                    mov   eax,offset task_proot
                    mov   ebx,eax
                    mov   ecx,sizeof task_proot
                    call  display_mem
                    ret
              FI
        ENDIF   
                 
        IFZ   al,'S'
        mov   edi,[phys_kernel_info_page_ptr]
        CANDNZ edi,0
              mov   dl,'b'
              mov   eax,ss:[edi+reserved_mem1+PM].mem_begin
              mov   ebx,ss:[edi+main_mem+PM].mem_end
              shr   ebx,log2_pagesize
              mov   ecx,ebx
              sub   eax,ebx
              and   eax,-pagesize
              add   eax,PM
              mov   ebx,eax
              call  display_mem
              ret
        FI

        IFDEF redirection_table
              IFZ   al,'R'                                    ;REDIR begin ----------------
                    kd____disp <'task: '>                     ;
                    kd____inhex16                             ;
                    and   eax,tasks-1                         ;
                    shl   eax,log2_tasks+2                    ;
                    add   eax,offset redirection_table        ;
                   ; mov   [dump_area_size],tasks*4            ;REDIR ends -----------------
                    mov   ebx,eax
                    mov   ecx,sizeof redirection_table
                    call  display_mem
                    ret
              FI
        ENDIF

  FI                        

  IFZ   al,'p'
        call  display_ptabs
        ret
  FI

  IFZ   al,'m'
        call  display_mappings
        ret
  FI

  IFZ   al,'c'
        call  dump_cache
        ret
  FI


  ret




;----------------------------------------------------------------------------
;
;       display mem
;
;----------------------------------------------------------------------------


display_mem:


  mov   [dump_area_base],ebx
  mov   [dump_area_size],ecx

  mov   esi,eax
  mov   edi,eax

  kd____clear_page

  push  esi
  push  edi
  mov   ebp,offset dump_dword
  IFAE  esp,virtual_space_size
        add   ebp,KR
  FI
  DO
        mov   al,dl
        call  dump
        IFZ   al,13
        CANDNZ edx,0
              pop   eax
              pop   eax
              push  esi
              push  edi
              mov   edi,edx
              mov   esi,edx
              REPEAT
        FI
        IFZ   al,1
              pop   edi
              pop   esi
              push  esi
              push  edi
              REPEAT
        FI
        call  is_main_level_command_key
        REPEATNZ
  OD
  pop   esi
  pop   edi

  push  eax
  mov   ah,lines-1
  mov   al,0
  kd____cursor
  kd____disp  <'L4KD: ',5>
  pop   eax
  push  eax  
  IFAE  al,20h
        kd____outchar
  FI
  pop   eax

  ret



dump_dword:

  call  display_dword
  mov   ebx,esi
  ret




;----------------------------------------------------------------------------
;
;       display ptab
;
;----------------------------------------------------------------------------




display_ptabs:

  test  ah,kdebug_dump_map_enabled
  IFZ
        mov   al,0
        ret
  FI


  mov   [dump_area_size],pagesize

  kd____inhex32

  test  eax,eax
  IFZ
        mov   eax,cr3
  ELIFB eax,tasks
  mov   ebx,cr0
  bt    ebx,31
  CANDC
        push  ds
        push  linear_kernel_space
        pop   ds
        load__proot eax,eax
        pop   ds
  FI
  and   eax,-pagesize
  mov   [dump_area_base],eax

  kd____clear_page

  DO
        mov   esi,[dump_area_base]

        mov   edi,esi
        mov   ebp,offset dump_pdir
        IFAE  esp,virtual_space_size
              add   ebp,KR
        FI

        DO
              mov   al,'p'
              call  dump

              cmp   al,13
              EXITNZ long

              test  edx,edx
              REPEATZ

              push  esi
              push  edi
              push  ebp
              mov   esi,edx
              mov   edi,edx
              mov   ebp,offset dump_ptab
              IFAE  esp,virtual_space_size
                    add   ebp,KR
              FI
              xchg  [dump_area_base],edx
              push  edx
              DO
                    mov   al,'p'
                    call  dump

                    IFZ   al,'m'
                          push  esi
                          push  edi
                          push  ebp
                          mov   eax,edx
                          call  display_mappings_of
                          pop   ebp
                          pop   edi
                          pop   esi
                          cmp   al,1
                          REPEATZ
                          EXIT
                    FI

                    cmp   al,13
                    EXITNZ

                    test  edx,edx
                    REPEATZ

                    test  [permissions].flags,kdebug_dump_mem_enabled
                    REPEATZ
                    
                    push  esi
                    push  edi
                    push  ebp
                    mov   esi,edx
                    mov   edi,esi
                    mov   ebp,offset dump_page
                    IFAE  esp,virtual_space_size
                          add   ebp,KR
                    FI
                    xchg  [dump_area_base],edx
                    push  edx
                    DO
                          mov   al,'w'
                          call  dump
                          cmp   al,13
                          REPEATZ
                    OD
                    pop   [dump_area_base]
                    pop   ebp
                    pop   edi
                    pop   esi

                    cmp   al,1
                    REPEATZ
              
                    call  is_main_level_command_key
                    REPEATNZ

              OD

              pop   [dump_area_base]
              pop   ebp
              pop   edi
              pop   esi

              cmp   al,1
              REPEATZ
        OD

        cmp   al,1
        REPEATZ
  OD

  push  eax
  mov   ah,lines-1
  mov   al,0
  kd____cursor
  kd____disp  <'L4KD: ',5>
  pop   eax
  push  eax
  IFAE  al,20h
        kd____outchar
  FI
  pop   eax

  ret




dump_pdir:

  push  esi
  mov   eax,cr0
  bt    eax,31
  IFC
        add   esi,PM
  FI
  call  display_dword
  pop   esi

  and   edx,-pagesize

  mov   ebx,esi
  and   ebx,pagesize-1
  shl   ebx,22-2
  mov   [virt_4M_base],ebx

  ret



dump_ptab:

  push  esi
  mov   eax,cr0
  bt    eax,31
  IFC
        add   esi,PM
  FI
  call  display_dword
  pop   esi

  and   edx,-pagesize

  mov   ebx,esi
  and   ebx,pagesize-1
  shl   ebx,12-2
  add   ebx,[virt_4M_base]
  mov   [virt_4K_base],ebx

  ret




dump_page:

  push  esi
  mov   eax,cr0
  bt    eax,31
  IFC
        add   esi,PM
  FI
  call  display_dword
  pop   esi

  mov   ebx,esi
  and   ebx,pagesize-1
  add   ebx,[virt_4K_base]

  ret
  
  
  


  align 4


virt_4M_base  dd 0
virt_4K_base  dd 0

dump_area_base dd 0
dump_area_size dd -1

dump_type     db 'w'


;----------------------------------------------------------------------------
;
;       dump
;
;----------------------------------------------------------------------------
;PRECONDITION:
;
;       AL    dump type
;       ESI   actual dump dword address (0 mod 4)
;       EDI   begin of dump address (will be 8*4-aligned)
;       EBP   dump operation
;
;----------------------------------------------------------------------------
;POSTCONDITION:
;
;       ESI   actual dump dword address (0 mod 4)
;       EDI   begin of dump address (will be 8*4-aligned)
;       EBP   dump operation
;
;       EBX,EDX can be loaded by dump operation
;
;       EAX,ECX scratch
;
;----------------------------------------------------------------------------

dumplines      equ (lines-1)


dump:

  mov   [dump_type],al

  mov   al,0
  DO
        mov   ecx,[dump_area_base]
        IFB_  esi,ecx
              mov   esi,ecx
        FI
        IFB_  edi,ecx
              mov   edi,ecx
        FI
        add   ecx,[dump_area_size]
        sub   ecx,4
        IFA   esi,ecx
              mov   esi,ecx
        FI
        sub   ecx,dumplines*8*4-4
        IFA   edi,ecx
              mov   edi,ecx
        FI

        and   esi,-4

        IFB_  esi,edi
              mov   edi,esi
              mov   al,0
        FI
        lea   ecx,[edi+dumplines*8*4]
        IFAE  esi,ecx
              lea   edi,[esi-(dumplines-1)*8*4]
              mov   al,0
        FI
        and   edi,-8*4

        IFZ   al,0

              push  esi
              mov   esi,edi
              mov   ch,lines-dumplines-1
              DO
                    mov   cl,0
                    mov   eax,ecx
                    kd____cursor
                    mov   eax,esi
                    IFZ   [dump_type],'P'
                          sub   eax,[dump_area_base]
                          shl   eax,5-2
                    ELIFZ [dump_type],'C'
                          sub   eax,[dump_area_base]
                    FI
                    kd____outhex32
                    mov   al,':'
                    kd____outchar
                    add   cl,8+1

                    DO
                          call  ebp
                          add   esi,4
                          add   cl,8+1
                          cmp   cl,80
                          REPEATB
                    OD

                    inc   ch
                    cmp   ch,lines-1
                    EXITAE
                    mov   eax,[dump_area_base]
                    add   eax,[dump_area_size]
                    dec   eax
                    cmp   esi,eax
                    REPEATB
              OD
              pop   esi
        FI

        mov   ecx,esi
        sub   ecx,edi
        shr   ecx,2

        mov   ch,cl
        shr   ch,3
        add   ch,lines-dumplines-1
        mov   al,cl
        and   al,8-1
        mov   ah,8+1
        IFZ   [dump_type],'t'
              mov   ah,4
        FI
        imul  ah
        add   al,9
        mov   cl,al

        mov   eax,ecx
        kd____cursor

        call  ebp
        kd____disp <6,lines-1,0,'L4KD: '>
        mov   al,[dump_type]
        kd____outchar
        mov   al,'<'
        kd____outchar
        mov   eax,ebx
        kd____outhex32
        mov   al,'>'
        kd____outchar
        kd____disp <6,lines-1,columns-35,'++KEYS: ',24,' ',25,' ',26,' ',27,' Pg',24,' Pg',25,' CR Home    '>
        IFDEF task_proot
              IFZ   ebp,<offset dump_ptab+KR>
                    kd____disp <6,lines-1,columns-3,3Ch,'m',3Eh>
              FI
        ENDIF      
        mov   eax,ecx
        kd____cursor

        kd____inchar

        IFZ   al,2
              add   esi,4
        FI
        IFZ   al,8
              sub   esi,4
        FI
        IFZ   al,10
              add   esi,8*4
        FI
        IFZ   al,3
              sub   esi,8*4
        FI
        CORZ  al,'+'
        IFZ   al,11h
              add   esi,dumplines*8*4 AND -100h
              add   edi,dumplines*8*4 AND -100h
              mov   al,0
        FI
        CORZ  al,'-'
        IFZ   al,10h
              sub   esi,dumplines*8*4 AND -100h
              sub   edi,dumplines*8*4 AND -100h
              mov   al,0
        FI
        IFZ   al,' '
              mov   al,[dump_type]
              IFZ   al,'w'
                    mov   al,'b'
              ELIFZ al,'b'
                    mov   al,'t'
              ELIFZ al,'t'
                    mov   al,'p'
              ELIFZ al,'p'
                    mov   al,'w'
              FI
              mov   [dump_type],al
              kd____clear_page
              mov   al,0
        FI

        cmp   al,1
        EXITZ
        cmp   al,13
        REPEATB
  OD

  ret





display_dword:

  mov   eax,esi
  ;;;   lno___task ebx,esp
  sub   ebx,ebx                 ; self
  call  page_phys_address

  IFZ
        IFZ   [dump_type],'t'
              kd____disp <250,250,250,250>
        ELSE_
              kd____disp <250,250,250,250,250,250,250,250,250>
        FI
        sub   edx,edx
        ret
  FI

  mov   edx,[eax]

  mov   al,[dump_type]
  IFZ   al,'b'
        mov   al,dl
        kd____outhex8
        mov   al,dh
        kd____outhex8
        shr   edx,16
        mov   al,dl
        kd____outhex8
        mov   al,dh
        kd____outhex8
        sub   edx,edx
        mov   al,' '
        kd____outchar
        ret
  FI
  IFZ   al,'t'
        call  out_dump_char
        shr   edx,8
        call  out_dump_char
        shr   edx,8
        call  out_dump_char
        shr   edx,8
        call  out_dump_char
        sub   edx,edx
        ret
  FI
  IFZ   al,'p',long

        IFZ   edx,0
              kd____disp <'    -    '>
        ELSE_
              test  dl,page_present
              ;; IFZ
              ;;      mov   eax,edx
              ;;      kd____outhex32
              ;;      mov   al,' '
              ;;      kd____outchar
              ;;      ret
              ;; FI
              call  dump_pte
        FI
        ret

  FI

  IFZ   edx,0
        kd____disp <'       0'>
  ELIFZ edx,-1
        kd____disp <'      -1'>
        sub   edx,edx
  ELSE_
        mov   eax,edx
        kd____outhex32
  FI
  mov   al,' '
  kd____outchar
  ret





out_dump_char:

  mov   al,dl
  IFB_  al,20h
        mov   al,'.'
  FI
  kd____outchar
  ret




dump_pte:


  mov   eax,edx
  shr   eax,28
  IFZ
        mov   al,' '
  ELIFB al,10
        add   al,'0'
  ELSE_
        add   al,'A'-10
  FI            
  kd____outchar
  mov   eax,edx
  test  dl,superpage
  CORZ
  test  edx,(MB4-1) AND -pagesize
  IFNZ
        shr   eax,12
        kd____outhex16
  ELSE_
        shr   eax,22
        shl   eax,2
        kd____outhex8
        mov   al,'/'
        bt    edx,shadow_ptab_bit
        IFC
              mov   al,'*'
        FI
        kd____outchar
        mov   al,'4'      
        kd____outchar
  FI
  mov   al,'-'
  kd____outchar
  test  dl,page_write_through
  IFNZ
        mov   al,19h
  FI
  test  dl,page_cache_disable
  IFNZ
        mov   al,17h
  FI
  kd____outchar
  test  dl,page_present
  IFNZ
        mov   al,'r'
        test  dl,page_write_permit
        IFNZ
              mov   al,'w'
        FI
  ELSE_
        mov   al,'y'
        test  dl,page_write_permit
        IFNZ
              mov   al,'z'
        FI
  FI
  test  dl,page_user_permit
  IFZ
        sub   al,'a'-'A'
  FI
  kd____outchar
  mov   al,' ' 
  kd____outchar

  ret










;----------------------------------------------------------------------------
;
;       display mappings
;
;----------------------------------------------------------------------------

  
  IFDEF pnode_base
  
  

display_mappings:

  IFB_  esp,virtual_space_size
        ret
  FI

  kd____inhex32
  shl   eax,log2_pagesize



display_mappings_of:

  push  ds
  push  es

  push  linear_kernel_space
  pop   ds
  push  linear_kernel_space
  pop   es


  mov   esi,eax

  shr   eax,log2_pagesize-log2_size_pnode
  and   eax,-sizeof pnode
  lea   edi,[eax+pnode_base]
  mov   ebx,edi

  kd____clear_page
  sub   eax,eax
  kd____cursor

  kd____disp <'phys frame: '>
  mov   eax,esi
  kd____outhex32

  kd____disp <'  cache: '>
  mov   eax,[edi+cache0]
  kd____outhex32
  mov   al,','
  kd____outchar
  mov   eax,[edi+cache1]
  kd____outhex32

  kd____disp <13,10,10>

  mov   cl,' '
  DO
        mov   eax,edi
        kd____outhex32
        kd____disp <'  '>
        mov   al,cl
        kd____outchar

        kd____disp <'  pte='>
        mov   eax,[edi+pte_ptr]
        kd____outhex32
        kd____disp <' ('>
        mov   eax,[edi+pte_ptr]
        mov   edx,[eax]
        call  dump_pte
        kd____disp <') v=...'>
        mov   eax,[edi+pte_ptr]
        and   eax,pagesize-1
        shr   eax,2
        kd____outhex12
        kd____disp <'000   ',25>
        mov   eax,[edi+child_pnode]
        kd____outhex32
        mov   al,' '
        kd____outchar
        IFNZ  edi,ebx
              mov   eax,[edi+pred_pnode]
              kd____outhex32
              mov   al,29
              kd____outchar
              mov   eax,[edi+succ_pnode]
              kd____outhex32
        FI

        kd____inchar

        IFZ   al,10
              mov   cl,25
              mov   edi,[edi+child_pnode]
        ELIFZ al,8
        CANDNZ edi,ebx
              mov   cl,27
              mov   edi,[edi+pred_pnode]
        ELIFZ al,2
        CANDNZ edi,ebx
              mov   cl,26
              mov   edi,[edi+succ_pnode]
        ELSE_
              call  is_main_level_command_key
              EXITZ
        FI
        kd____disp <13,10>

        and   edi,-sizeof pnode
        REPEATNZ

        mov   edi,ebx
        REPEAT
  OD

  push  eax
  mov   ah,lines-1
  mov   al,0
  kd____cursor
  kd____disp  <'L4KD: ',5>
  pop   eax
  push  eax
  IFAE  al,20h
        kd____outchar
  FI
  pop   eax

  pop   es
  pop   ds
  ret



  ELSE


display_mappings:
display_mappings_of:

  ke    'no_pnodes'

  ret


  ENDIF




;----------------------------------------------------------------------------
;
;       display kernel data
;
;----------------------------------------------------------------------------


display_kernel_data:


  IFB_  esp,virtual_space_size
        ret
  FI

  push  ds
  push  es

  mov   eax,linear_kernel_space
  mov   ds,eax
  mov   es,eax

  kd____clear_page

  sub   esi,esi                       ; required for show macro !

  kd____disp  <6,2,1,'sys clock : '>

  IFDEF V4_clock_features
        IF    V4_clock_features
              extrn get_clock_usc:near
              call  get_clock_usc
        ELSE
              IFDEF system_clock
                    mov   eax,ds:[system_clock]
                    mov   edx,ds:[system_clock+4]
              ELSE
                    sub   eax,eax
                    mov   edx,ds:[phys_kernel_info_page_ptr+PM]
                    IFNZ  edx,0
                          mov   eax,ds:[edx+PM].user_clock
                          mov   edx,ds:[edx+PM].user_clock+4
                    FI
              ENDIF
        ENDIF
  ELSE
        IFDEF system_clock
              mov   eax,ds:[system_clock]
              mov   edx,ds:[system_clock+4]
        ELSE
              sub   eax,eax
              mov   edx,ds:[phys_kernel_info_page_ptr+PM]
              IFNZ  edx,0
                    mov   eax,ds:[edx+PM].user_clock
                    mov   edx,ds:[edx+PM].user_clock+4
              FI
        ENDIF
  ENDIF
  push  eax
  mov   eax,edx
  kd____outhex32
  pop   eax
  kd____outhex32

  IFDEF present_root
        kd____disp  <6,7,40,'present root : '>
        mov   eax,offset present_root
        lno___thread eax,eax
        kd____outhex16
  ENDIF
  
  IFDEF highest_active_prio
        kd____disp <6,6,1,'highest prio  : '>
        mov   eax,ds:[highest_active_prio]
        lno___thread eax,eax
        kd____outhex16

  ELSE
        IFDEF ready_actual
              kd____disp <6,6,1,'ready actual   : '>
              mov   eax,ds:[ready_actual]
              lno___thread eax,eax
              kd____outhex16
        ENDIF
        IFDEF dispatcher_tcb
              kd____disp  <6,8,1,'ready root     : '>
              mov   ecx,offset dispatcher_tcb+ready_link
              call  show_link
              kd____disp  <6,9,1,'intr root    : '>
              mov   ecx,offset dispatcher_tcb+interrupted_link
              call  show_link
        ENDIF
  ENDIF

  IFDEF dispatcher_tcb
        kd____disp  <6,11,1, 'soon wakeup root :'>
        mov   ecx,offset dispatcher_tcb+soon_wakeup_link
        call  show_link
        kd____disp  <6,12,1, 'late wakeup root :'>
        mov   ecx,offset dispatcher_tcb+late_wakeup_link
        call  show_link
  ENDIF

  IFDEF intrq_llink
        kd____disp  <6,11,40, 'intrq link :'>
        sub   ebx,ebx
        DO
              mov   eax,ebx
              and   al,7
              imul  eax,5
              add   al,41
              mov   ah,bl
              shr   ah,3
              add   ah,12
              kd____cursor
              lea   ecx,[(ebx*4)+intrq_llink]
              call  show_llinks
              add   ebx,2
              cmp   ebx,lengthof intrq_llink
              REPEATB
        OD
  ENDIF

  kd____disp  <6,18,61,' CR3 : '>
  mov   eax,cr3
  kd____outhex32

  kd____disp  <6,19,61,'ESP0 : '>
 
  str   bx
  and   ebx,0000FFF8h
  add   ebx,ds:[gdt_descr+PM].base
  mov   ah,byte ptr ds:[ebx+7]
  mov   al,byte ptr ds:[ebx+4]
  shl   eax,16
  mov   ax,word ptr ds:[ebx+2]  ; base of TSS

  mov   eax,ds:[eax+4]          ; ESP0
  kd____outhex32

  pop   es
  pop   ds
  ret



;----------------------------------------------------------------------------
;
;       enter thread nickname
;
;----------------------------------------------------------------------------


enter_thread_nickname:

  push  ds
  push  phys_mem
  pop   ds

  mov   ebp,esp
  and   ebp,-sizeof tcb

  kd____disp <' t:'>
  kd____inhext
  test  eax,eax
  IFZ
        lno___thread eax,ebp
        call  outthread
  FI

  mov   ecx,offset thread_nickname_table
  DO
        IFZ   [ecx].number,eax
              mov   [ecx].number,0
              EXIT
        FI
        add   ecx,sizeof thread_nickname_entry
        cmp   ecx,offset thread_nickname_table+sizeof thread_nickname_table
        REPEATB
  OD

  mov   ecx,offset thread_nickname_table
  DO
        cmp   [ecx].number,0
        EXITZ
        add   ecx,sizeof thread_nickname_entry
        cmp   ecx,offset thread_nickname_table+sizeof thread_nickname_table
        REPEATB

        kd____disp <' nickname table full ',13,10>

        pop   ds
        ret
  OD

  kd____disp <' = '>
  
  mov   [ecx].number,eax
  sub   edx,edx
  DO
        kd____inchar
        cmp   al,32
        EXITBE
        mov   [ecx+edx].nickname,al
        kd____outchar
        inc   edx
        cmp   edx,sizeof nickname
        REPEATB
  OD
  mov   [ecx+edx].nickname,0
  IFZ   edx,0
        mov   [ecx].number,0
  FI

  pop   ds
  ret

;----------------------------------------------------------------------------
;
;       page fault prot
;
;----------------------------------------------------------------------------


  

page_fault_prot:

  test  ah,kdebug_protocol_enabled
  IFZ
        mov   al,0
        ret
  FI


  mov   eax,cr0
  bt    eax,31
  CORNC
  mov   eax,ss
  IFNZ  eax,linear_kernel_space

        mov   al,'-'
        kd____outchar
        ret
  FI


  kd____inchar

  CORZ  al,'+'
  IFZ   al,'*'
        mov   [page_fault_prot_state],al
        kd____outchar
        IFZ   [page_fault_prot_handler_active],0
              mov   [page_fault_prot_handler_active],1
              mov   bl,page_fault
              call  get_exception_handler
              mov   [page_fault_handler],eax
        FI
        mov   eax,offset show_page_fault
        mov   bl,page_fault
        call  set_exception_handler
        ret
  FI
  IFZ   al,'-'
        mov   [page_fault_prot_state],al
        kd____outchar
        sub   ecx,ecx
        mov   [page_fault_low],ecx
        dec   ecx
        mov   [page_fault_high],ecx
        IFNZ  [page_fault_prot_handler_active],0
              mov   [page_fault_prot_handler_active],0
              mov   eax,[page_fault_handler]
              mov   bl,page_fault
              call  set_exception_handler
        FI
        ret
  FI
  IFZ   al,'x'
        mov   [page_fault_prot_state],al
        kd____disp  'x ['
        kd____inhex32
        mov   [page_fault_low],eax
        mov   al,','
        kd____outchar
        kd____inhex32
        mov   [page_fault_high],eax
        mov   al,']'
        kd____outchar
        ret
  FI

  mov   al,'?'
  kd____outchar

  ret



show_page_fault:

  ipre  ec_present

  mov   eax,cr2
  and   eax,-pagesize
  IFAE   eax,[page_fault_low+PM]
  CANDBE eax,[page_fault_high+PM]
        CORB  eax,com0_base                             ; do not protocol pseudo PFs in comspace
        IFA   eax,com1_base-1+com_space_size            ; otherwise a periodic interrupt (kb, e.g.) leads
                                                        ; to starvation if prot is on
              mov   ebx,cr2
              mov   ecx,[esp+ip_eip]
              lno___thread eax,esp
              shl   eax,8
        
              IFZ   [page_fault_prot_state+PM],'*'
        
                    call  put_into_trace_buffer
        
              ELSE_      
                    kd____disp <13,10>
                    call  display_page_fault
                    call  event_ack
              FI
        FI
  FI

  pop   es
  pop   ds

  popad
  jmp   ss:[page_fault_handler+PM]





display_page_fault:                            ; EBX  fault address
                                               ; ECX  fault EIP
                                               ; EAX  thread no SHL 8 + 00
                                               
                                               ; --> EAX scratch
  mov   edx,eax
  shr   edx,8                                             
  kd____disp  <'#PF: '>
  mov   eax,ebx
  kd____outhex32
  kd____disp  <', eip='>
  mov   eax,ecx
  kd____outhex32
  kd____disp  <', thread='>
  mov   eax,edx
  kd____outhex16
  
  ret
  
  



event_ack:

 call  open_debug_keyboard
 kd____inchar
 IFZ   al,'i'
       ke    'kdebug'
 FI
 call  close_debug_keyboard

 ret
 
 
 
;----------------------------------------------------------------------------
;
;       IPC prot
;
;----------------------------------------------------------------------------


ipc_prot:

  test  ah,kdebug_protocol_enabled
  IFZ
        mov   al,0
        ret
  FI


  mov   eax,cr0
  bt    eax,31
  CORNC
  mov   eax,ss
  IFNZ  eax,linear_kernel_space

        mov   al,'-'
        kd____outchar
        ret
  FI


  kd____inchar
  kd____outchar

  CORZ  al,'+'
  IFZ   al,'*'
        mov   [ipc_prot_state],al
        IFZ   [ipc_prot_handler_active],0
              mov   [ipc_prot_handler_active],1
              mov   bl,ipc
              call  get_exception_handler
              mov   [ipc_handler],eax
        FI
        mov   eax,offset show_ipc
        mov   bl,ipc
        call  set_exception_handler
        ret
  FI
  IFZ   al,'-'
        IFNZ  [ipc_prot_handler_active],0
              mov   [ipc_prot_handler_active],0
              mov   eax,[ipc_handler]
              mov   bl,ipc
              call  set_exception_handler
        FI
        ret
  FI

  IFZ   al,'r',long
        kd____disp <6,lines-1,columns-58,'t/T/s/-    : thread/non-thread/send-only/reset restrictions',6,lines-1,8>
        kd____inchar
        kd____disp <5>
        kd____outchar
        
        IFZ   al,'-'
              sub   eax,eax
              mov   [ipc_prot_thread0],eax
              mov   [ipc_prot_thread1],eax
              mov   [ipc_prot_thread2],eax
              mov   [ipc_prot_thread3],eax
              mov   [ipc_prot_thread4],eax
              mov   [ipc_prot_thread5],eax
              mov   [ipc_prot_thread6],eax
              mov   [ipc_prot_thread7],eax
              mov   [ipc_prot_non_thread],eax
              mov   [ipc_prot_mask],0FFh
              ret
        FI

        IFZ   al,'t'
              kd____inhex16
              test  eax,eax
              IFNZ
                    xchg  [ipc_prot_thread0],eax
                    xchg  [ipc_prot_thread1],eax
                    xchg  [ipc_prot_thread2],eax
                    xchg  [ipc_prot_thread3],eax
                    xchg  [ipc_prot_thread4],eax
                    xchg  [ipc_prot_thread5],eax
                    xchg  [ipc_prot_thread6],eax
                    mov   [ipc_prot_thread7],eax
              ELSE_
                    xchg  [ipc_prot_thread7],eax
                    xchg  [ipc_prot_thread6],eax
                    xchg  [ipc_prot_thread5],eax
                    xchg  [ipc_prot_thread4],eax
                    xchg  [ipc_prot_thread3],eax
                    xchg  [ipc_prot_thread2],eax
                    xchg  [ipc_prot_thread1],eax
                    mov   [ipc_prot_thread0],eax
              FI                    
              ret
        FI
        IFZ   al,'T'
              kd____inhex16
              mov   [ipc_prot_non_thread],eax
              ret
        FI
        
        IFZ   al,'s'
              mov   [ipc_prot_mask],08h
              ret
        FI      
  FI       

  mov   al,'?'
  kd____outchar

  ret



show_ipc:

  ipre  fault

  sub   ecx,ecx
  IFB_  ebp,virtual_space_size
        mov   ecx,ebp
        and   cl,011b
        or    cl,100b
  FI
  and   al,11b
  IFB_  eax,virtual_space_size
        or    al,100b
  ELSE_
        mov   al,0
  FI
  shl   cl,3
  add   cl,al
  add   cl,40h
  lno___thread eax,esp

  DO  
        cmp   [ipc_prot_thread0+PM],0
        EXITZ
        cmp   [ipc_prot_thread0+PM],eax
        EXITZ
        cmp   [ipc_prot_thread1+PM],eax
        EXITZ
        cmp   [ipc_prot_thread2+PM],eax
        EXITZ
        cmp   [ipc_prot_thread3+PM],eax
        EXITZ
        cmp   [ipc_prot_thread4+PM],eax
        EXITZ
        cmp   [ipc_prot_thread5+PM],eax
        EXITZ
        cmp   [ipc_prot_thread6+PM],eax
        EXITZ
        cmp   [ipc_prot_thread7+PM],eax
  OD
  IFZ
  CANDNZ eax,[ipc_prot_non_thread+PM]
  test  cl,[ipc_prot_mask+PM]
  CANDNZ      
         
        shl   eax,8
        mov   al,cl
        mov   ecx,esi
  
        IFZ   [ipc_prot_state+PM],'*'
  
              call  put_into_trace_buffer
        
        ELSE_      
              kd____disp <13,10>
              call  display_ipc
              call  event_ack
        FI
  FI       

  pop   es
  pop   ds

  popad
	add	esp,4
  jmp   ss:[ipc_handler+PM]




display_ipc:                                   ; EAX  :  src SHL 8 + ipc type
                                               ; EBX  :  msg w1
                                               ; ECX  :  dest 
                                               ; EDX  :  msg w0
                                               
                                               ; --> EAX scratch
  kd____disp <'ipc: '>                                             
  push  eax
  shr   eax,8
  call  outthread
  pop   eax
  
  mov   ah,al
  and   al,00101100b
  push  eax
  
  IFZ   al,00100000b
		kd____disp	<' waits for '>
	ELIFZ al,00101000b
	      kd____disp  <' waits '>
  ELIFZ al,00000100b
	      kd____disp  <' -sends--> '>
  ELIFZ al,00100100b
        kd____disp  <' -calls--> '>
  ELIFZ al,00101100b
	      kd____disp  <' replies-> '>
  ELSE_ kd____disp  <' -'>
        kd____outhex8
        kd____disp  <'- '>
	FI	
	IFNZ  al,00101000b	
        lno___thread eax,ecx
        test  eax,eax
        IFZ
              kd____disp <' - '>
        ELSE_      
              call  outthread
        FI      
  FI      
  pop   eax
         
  push  eax
  test  al,00000100b
	IFNZ
        test  ah,00000010b
		IFZ
  		kd____disp	<'     ('>
		ELSE_
			kd____disp  <' map ('>
		FI
		mov	eax,edx
		kd____outhex32
		mov	al,','
		kd____outchar
		mov	eax,ebx
		kd____outhex32
		mov	al,')'
		kd____outchar
	FI       
  pop   eax
  
  IFZ   al,00101100b
        kd____disp  <' and waits'>
  FI      

  ret
  
  

;----------------------------------------------------------------------------
;
;       monit exception
;
;----------------------------------------------------------------------------


monit_exception:

  test  ah,kdebug_protocol_enabled
  IFZ
        mov   al,0
        ret
  FI


  kd____inchar
  kd____outchar

  push  eax  
  CORZ  al,'*'
  CORZ  al,'+'  
  IFZ   al,'-'
  
        mov   al,false
        xchg  al,[exception_monitoring_flag]
        IFZ   al,true
              mov   eax,[monitored_exception_handler]
              mov   bl,[monitored_exception]
              call  set_exception_handler
        FI
  FI
  pop   eax
  
  
  CORZ  al,'*'
  IFZ   al,'+'
  
        kd____disp <' #'>
        kd____inhex8

     
        CORZ  al,debug_exception
        CORZ  al,breakpoint
        movzx ecx,[idt_descr].limit
        shr   ecx,3
        IFA   al,cl
              mov   al,'-'
              kd____outchar
              ret
        FI
        
        mov   [exception_monitoring_flag],true
        mov   [monitored_exception],al
        mov   bl,al
        
        IFAE  al,11
        CANDB al,15
        
              kd____disp <' ['>
              kd____inhex16
              mov   [monitored_ec_min],ax
              mov   al,','
              kd____outchar
              kd____inhex16
              mov   [monitored_ec_max],ax
              mov   al,']'
              kd____outchar
        FI
        
        call  get_exception_handler
        mov   [monitored_exception_handler],eax
        
        mov   eax,offset exception_monitor
        call  set_exception_handler
  FI
  
  ret
  
  



exception_monitor:

  ipre  ec_present

  sub   edx,edx
  IFAE  esp,virtual_space_size
        mov   edx,PM
  FI

  
  mov   al,ss:[edx+monitored_exception]
  mov   ebp,esp
  DO
        
        IFZ   al,general_protection
        CANDZ ss:[ebp+ip_cs],linear_space_exec
        bt    ss:[ebp+ip_eflags],vm_flag
        CANDNC
              cmp   ss:[ebp+ip_ds],0
              EXITZ
              
              mov   ebx,ss:[ebp+ip_eip]
              mov   ecx,ebx
              and   ecx,pagesize-1
              IFBE  ecx,pagesize-4                       
                    push  ds
                    mov   ds,ss:[ebp+ip_cs]
                    mov   ebx,[ebx]
                    pop   ds
                    cmp   bx,010Fh       ; LIDT (emulated) etc.
                    EXITZ
              FI
        FI
              
        IFAE  al,11
        CANDB al,15
              movzx eax,word ptr ss:[ebp+ip_error_code]        
              movzx ebx,ss:[edx+monitored_ec_min]
              movzx ecx,ss:[edx+monitored_ec_max]
              IFBE  ebx,ecx
                    cmp   eax,ebx
                    EXITB
                    cmp   eax,ecx
                    EXITA
              ELSE_
                    IFBE  eax,ebx
                          cmp   eax,ecx
                          EXITAE
                    FI
              FI
        FI
        
        ke    'INTR'
        
  OD
                        

  pop   es
  pop   ds

  popad

  IFAE  esp,virtual_space_size
  
        CORB  ss:[monitored_exception+PM],8           
        IFA   ss:[monitored_exception+PM],14
              IFNZ  ss:[monitored_exception+PM],17           
                    add   esp,4
              FI      
        FI
        jmp   ss:[monitored_exception_handler+PM]
  FI

  CORB  [monitored_exception],8           
  IFA   [monitored_exception],14
        IFNZ  [monitored_exception],17           
              add   esp,4
        FI      
  FI
  jmp   [monitored_exception_handler]






;----------------------------------------------------------------------------
;
;       monitor thread switch
;
;----------------------------------------------------------------------------


thread_switch_prot:

  IFDEF ts_prot

        IFB_  esp,virtual_space_size
              ret
        FI

        mov   ebp,esp
        and   ebp,-sizeof tcb

        kd____inchar

        IFZ   al,'*'
              kd____outchar
              or    ss:[ebp+ressources],mask ts_prot
              ret
        FI

        IFZ   al,'-'
              kd____outchar
              mov   eax,ebp
              DO
                    and   ss:[eax+ressources],NOT mask ts_prot
                    mov   eax,ss:[eax+present_llink].succ
                    cmp   eax,ebp
                    REPEATNZ
              DO
              ret
        FI      

  ENDIF

  mov   al,'?'
  kd____outchar
  ret






;----------------------------------------------------------------------------
;
;       remote kd intr
;
;----------------------------------------------------------------------------


remote_kd_intr:

  kd____inchar

  IFZ   al,'+'
  CANDAE esp,virtual_space_size

        kd____outchar
        IFZ   [timer_intr_handler],0
              mov   ebx,[irq0_intr]
              add   ebx,8 
              call  get_exception_handler
              mov   [timer_intr_handler],eax
        FI
        mov   eax,offset kdebug_timer_intr_handler
  ELSE_
        mov   al,'-'
        kd____outchar
        sub   eax,eax
        xchg  eax,[timer_intr_handler]
  FI
  test  eax,eax
  IFNZ
        mov   ebx,[irq0_intr]
        add   ebx,8 
        call  set_exception_handler
  FI
  ret



kdebug_timer_intr_handler:

  dec   byte ptr ss:[kdebug_timer_intr_counter+PM]
  IFZ
  
        ipre  fault,no_load_ds

        kd____incharety
        IFZ   al,27
              ke    'ESC'
        FI

        ko    T

        pop   es
        pop   ds

        popad
        add   esp,4
  FI
                
  jmp   ss:[timer_intr_handler+PM]



;----------------------------------------------------------------------------
;
;       single stepping on/off
;
;----------------------------------------------------------------------------
;
;
;
;single_stepping_on_off:
;
;  kd____inchar
;  mov   edi,[kdebug_esp]
;  IFA   edi,<virtual_space_size>
;        push  ds
;        push  linear_kernel_space
;        pop   ds
;  FI
;
;  IFZ   al,'+'
;        bts   [edi+ip_eflags],t_flag
;  else_
;        btr   [edi+ip_eflags],t_flag
;        mov   al,'-'
;  FI
;
;  IFA   edi,<virtual_space_size>
;        pop   ds
;  FI
;  kd____outchar
;  ret



;----------------------------------------------------------------------------
;
;       virtual address info
;
;----------------------------------------------------------------------------


  

virtual_address_info:

  kd____inhex32
  mov   ebx,eax
  kd____disp  <'  Task='>
  kd____inhex16
  test  eax,eax
  IFZ
        lno___task eax,esp
  FI
  xchg  eax,ebx
  call  page_phys_address
  IFZ
        kd____disp  <'  not mapped'>
  ELSE_
        push  eax
        kd____disp  <'  phys address = '>
        pop   eax
        kd____outhex32
  FI

  ret



  
;----------------------------------------------------------------------------
;
;       port io
;
;----------------------------------------------------------------------------


pic1_imr            equ 21h


pci_address_port    equ 0CF8h
pci_data_port       equ 0CFCh


port_io:

  test  ah,kdebug_io_enabled
  IFZ
        mov   al,0
        ret
  FI



  mov   bh,al
  IFZ   al,'i'
        kd____disp <'n  '>
  ELSE_
        kd____disp <'ut '>
  FI            
  
  kd____inchar
  mov   bl,al
  kd____outchar
  IFZ   al,'a'
        kd____disp <'pic '>
  ELIFZ al,'i'
        kd____disp <'o apic '>
  ELIFZ al,'p'
        kd____disp <'ci conf dword '>      
  ELSE_
        kd____disp <'-byte port '>                  
  FI
        
  kd____inhex16
  mov   edx,eax
  
  kd____disp <': '>
  IFZ   bh,'o'
        kd____inhex32
  FI      
        
  IFZ   bl,'1'
  
        IFZ   bh,'o'
              IFZ   dx,pic1_imr
                    mov   [old_pic1_imr],al
              ELSE_
                    out   dx,al
              FI
        ELSE_
              IFZ   dx,pic1_imr
                    mov   al,[old_pic1_imr]
              ELSE_
                    in    al,dx
              FI
              kd____outhex8
        FI
        ret
  FI

  IFZ   bl,'2'
  
        IFZ   bh,'o'
              out   dx,ax
        ELSE_
              in    ax,dx
              kd____outhex16
        FI
        ret
  FI
  
  IFZ   bl,'4'
  
        IFZ   bh,'o'
              out   dx,eax
        ELSE_
              in    eax,dx
              kd____outhex32
        FI
        ret
  FI
  
  
  IFZ   bl,'p'
  
        push  eax
        mov   eax,edx
        or    eax,8000000h
        mov   dx,pci_address_port
        out   dx,eax
        pop   eax
        
        mov   dx,pci_data_port
        IFZ   bh,'o'
              out   dx,eax
        ELSE_
              in    eax,dx
              kd____outhex32
        FI
        ret
  FI
        
        
  
  
  


  
  IFB_  esp,virtual_space_size
        ret
  FI
  
  
  push  ds
  push  linear_kernel_space
  pop   ds
  
  
  IFZ   bl,'a'
  
        and   edx,00000FF0h
        add   edx,ds:[local_apic_base+PM]
        IFZ   bh,'o'
              mov   ds:[edx],eax
        ELSE_
              mov   eax,ds:[edx]
              kd____outhex32
        FI
  
  ELIFZ bl,'i'
  
        and   edx,000000FFh
        mov   ebx,ds:[io_apic_base+PM]
        mov   byte ptr ds:[ebx+0],dl
        IFZ   bh,'o'
              mov   ds:[ebx+10h],eax
        ELSE_
              mov   eax,ds:[ebx+10h]
              kd____outhex32
        FI
  FI      
  
  pop   ds
        
        
  ret            
              



;----------------------------------------------------------------------------
;
;       page phys address
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX   linear address
;       EBX   task no
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       page present:
;
;             NZ
;             EAX   phys address (lower 12 bits unaffected)
;
;
;       page not present:
;
;             Z
;
;----------------------------------------------------------------------------


page_phys_address:
        

  push  eax
  mov   eax,cr0
  bt    eax,31
  pop   eax

  IFNC
        test  esp,esp     ; NZ !
        ret
  FI


  push  ds
  push  ecx
  push  edx

  mov   edx,linear_kernel_space
  mov   ds,edx

  test  ebx,ebx
  IFZ
        mov   edx,cr3
        and   edx,-pagesize
  ELSE_
        load__proot edx,ebx
  FI

  IFAE  eax,shared_table_base
  CANDBE eax,shared_table_base-1+shared_table_size
        mov   edx,ds:[kernel_proot_ptr_ptr+PM]
        mov   edx,ds:[edx]
  FI

  xpdir ebx,eax
  xptab ecx,eax
  mov   ebx,dword ptr [(ebx*4)+edx+PM]
  mov   dl,bl
  and   ebx,-pagesize

  test  dl,page_present
  IFNZ
        test  dl,superpage
        IFZ
              mov   ecx,dword ptr [(ecx*4)+ebx+PM]
              mov   dl,cl
              and   ecx,-pagesize
        ELSE_
              and   ebx,-1 SHL 22
              shl   ecx,12
              add   ecx,ebx
        FI
        IFAE  ecx,<physical_kernel_mem_size>      ; no access beyond PM
              mov   dl,0                          ; ( 0 ... 64 M )
        FI                                        ;
  FI
  test  dl,page_present
  IFNZ
        and   eax,pagesize-1
        add   eax,ecx
        test  esp,esp     ; NZ !
  FI

  pop   edx
  pop   ecx
  pop   ds
  ret




;--------------------------------------------------------------------------
;
;       set / get exception handler
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX    offset
;       BL     interrupt number
;
;       SS     linear_kernel_space
;
;---------------------------------------------------------------------------


set_exception_handler:
 
  push  eax
  push  ebx

  call  address_idt

  IFAE  esp,virtual_space_size
        and   eax,NOT PM
        add   eax,KR
  FI
  mov   ss:[ebx],ax
  shr   eax,16
  mov   ss:[ebx+6],ax

  pop   ebx
  pop   eax
  ret



get_exception_handler:
 
  push  ebx

  call  address_idt

  mov   ax,ss:[ebx+6]
  shl   eax,16
  mov   ax,ss:[ebx]

  pop   ebx
  ret



address_idt:

  movzx ebx,bl
  shl   ebx,3
  IFAE  esp,virtual_space_size
        add   ebx,ss:[idt_descr+PM].base
  ELSE_
        add   ebx,ss:[idt_descr].base
  FI
  ret










;--------------------------------------------------------------------------
;
;       set / get exception handler
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX    offset
;       BL     interrupt number
;
;       SS     linear_kernel_space
;
;---------------------------------------------------------------------------


csr1    equ 08h
csr5    equ 28h
csr6    equ 30h



wait100 macro

  mov   eax,ecx
  DO
        dec   eax
        REPEATNZ
  OD
  endm
        


special_test:

  kd____disp <'21140 base: '>
  kd____inhex16
  mov   ebx,eax

  kd____disp <' snoop interval: '>  
  kd____inhex8
  mov   ecx,eax
  
  kd____disp <' : '>
  
  lea   edx,[ebx+csr1]
  sub   eax,eax
  out   dx,eax
  
  lea   edx,[ebx+csr5]
  DO
        in    eax,dx
        and   eax,00700000h
        cmp   eax,00300000h
        REPEATNZ
  OD       

  rdtsc
  mov   edi,eax
  lea   edx,[ebx+csr5]
  DO
        wait100
        in    eax,dx
        and   eax,00700000h
        cmp   eax,00300000h
        REPEATZ
  OD       
  rdtsc
  sub   eax,edi
  sub   edx,edx
  mov   edi,6
  div   edi
  kd____outdec
  kd____disp <' PCI cycles'>
  
  ret        




;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
; 
;       random-profiling analysis
;
;--------------------------------------------------------------------------
;--------------------------------------------------------------------------

  if    random_profiling

        extrn profiling_space_begin:dword
        extrn profiling_space_size:abs




profile_cmd:

  IFB_  esp,virtual_space_size
        kd____disp <'-'>
        ret
  FI

  kd____disp <6,lines-1,columns-58,'  d/a/0  : profiling dump / analysis / set to 0',6,lines-1,8> 

  kd____inchar
  kd____outchar


  mov   edi,ds:[profiling_space_begin]
  add   edi,PM
  mov   ecx,profiling_space_size


  IFZ   al,'d'
        mov   eax,edi
        mov   ebx,edi
        mov   dl,'P'
        call  display_mem
        ret
  FI


  IFZ   al,'0'
        push  es
        push  linear_kernel_space
        pop   es

        shr   ecx,2
        cld
        sub   eax,eax
        rep   stosd

        pop   es
        ret
  FI


  IFZ   al,'a',long

        push  ds
        push  es

        push  linear_kernel_space
        pop   ds
        push  ds
        pop   es
        
        mov   esi,edi
        mov   edi,ds:[current_cache_snapshot+PM]

        push  ecx
        push  edi

        mov   ecx,sizeof cache_snapshot/4
        sub   eax,eax
        cld
        rep   stosd

        pop   edi
        pop   ecx

        kd____disp <' skip profile lines accessed lessequal than: '>
        kd____inhex32
        mov   edx,eax

        mov   ebx,esi
        DO
              mov   eax,ds:[esi]
              and   eax,pagesize-1
              IFG   eax,edx

                    push  ecx
                    push  esi

                    mov   eax,esi
                    sub   eax,ebx
                    shr   eax,2
                    and   eax,icache_sets-1
                    shl   eax,log2_snapshot_dwords_per_icache_set
                    lea   eax,[eax*4+edi]

                    mov   ecx,ds:[esi]
                    and   ecx,-pagesize
                    sub   esi,ebx
                    shl   esi,log2_icache_line_size-2
                    and   esi,pagesize-1
                    or    esi,ecx

                    mov   ecx,snapshot_dwords_per_icache_set
                    DO
                          IFZ   dword ptr ds:[eax],0
                                mov   dword ptr ds:[eax],esi
                                EXIT
                          FI
                          add   eax,4
                          dec   ecx
                          REPEATNZ
                    OD

                    pop   esi
                    pop   ecx
              FI
              add   esi,4
              sub   ecx,4
              REPEATNZ
        OD

        pop   es
        pop   ds

        mov   eax,edi
        mov   ebx,eax
        mov   ecx,sizeof cache_snapshot
        mov   dl,'C'
        call  display_mem

        ret

  FI


  kd____disp <'??'>

  ret




                    

        endif



;--------------------------------------------------------------------------
;--------------------------------------------------------------------------
; 
;       data cache analysis
;
;--------------------------------------------------------------------------
;
; PRECONDITION:     phys mem idempotent
;
;--------------------------------------------------------------------------


  icode


init_cache_snapshots:

  pushad

  DO
        cmp   [configuration].trace_pages,0
        EXITZ long

        call  [grab_frame]
        mov   ebx,eax
        mov   edi,eax
        mov   ecx,pagesize

        cmp   [configuration].trace_pages,1
        EXITZ
        
        call  [grab_frame]
        IFB_  eax,edi

              DO
                    sub   ebx,eax
                    EXITC
                    cmp   ebx,pagesize
                    EXITNZ

                    add   ecx,ebx
                    mov   ebx,eax
                    mov   edi,eax

                    movzx eax,[configuration].trace_pages
                    shl   eax,log2_pagesize
                    cmp   ecx,eax
                    EXITAE

                    call  [grab_frame]
                    REPEAT
              OD

        ELSE_
              DO
                    sub   eax,ebx
                    cmp   eax,pagesize
                    EXITNZ

                    add   ecx,eax
                    add   ebx,eax

                    movzx eax,[configuration].trace_pages
                    shl   eax,log2_pagesize
                    cmp   ecx,eax
                    EXITAE

                    call  [grab_frame]
                    REPEAT
              OD
        FI
                    


        mov   eax,edi
        add   eax,PM
        mov   [cache_snapshots_begin],eax
        mov   [current_cache_snapshot],eax
        add   eax,ecx
        mov   [cache_snapshots_end],eax
  
        shr   ecx,2
        mov   eax,-1
        cld
        rep   stosd

  OD      
  
  popad
  ret


  icod  ends





dump_cache:

  CORB  esp,virtual_space_size
  IFZ   [cache_snapshots_end],0
        kd____disp <'-'>
        ret
  FI

  mov   eax,[kdebug_invocation_stamp]
  IFNZ  eax,[current_cache_snapshot_stamp]
        
        mov   [current_cache_snapshot_stamp],eax

        mov   edi,[current_cache_snapshot]
        add   edi,sizeof cache_snapshot
        IFAE  edi,[cache_snapshots_end]
              mov   edi,[cache_snapshots_begin]
        FI
        mov   [current_cache_snapshot],edi

        call  dump_data_cache_snapshot

  FI

  mov   cl,0
  DO
        mov   eax,[current_cache_snapshot]
        mov   edi,eax
        mov   ch,cl
        mov   cl,0
        DO
              cmp   cl,ch
              EXITAE
              IFBE  edi,[cache_snapshots_begin]
                    mov   edi,[cache_snapshots_end]
              FI
              sub   edi,sizeof cache_snapshot
              cmp   edi,[current_cache_snapshot]
              EXITZ

              inc   cl
              mov   eax,edi
              REPEAT
        OD
        
        push  ecx
        mov   ebx,eax
        mov   ecx,sizeof cache_snapshot
        mov   dl,'w'

        call  display_mem

        pop   ecx

        cmp   al,'<'
        IFZ
              inc   cl
              REPEAT
        FI
        cmp   al,'>'
        IFZ
              cmp   cl,0
              REPEATZ
              dec   cl
              REPEAT
        FI
  OD
              
  ret



dump_data_cache_snapshot:

  push  ds
  push  es

  mov   edi,[current_cache_snapshot]

  mov   edx,[typical_cached_data_cycles]
  mov   eax,edx
  shr   eax,1
  add   edx,eax

  mov   eax,linear_kernel_space
  mov   ds,eax
  mov   es,eax

  mov   ecx,sizeof cache_snapshot/4
  mov   eax,-1
  cld
  push  edi
  rep   stosd
  pop   edi

  mov   ecx,ds:[phys_kernel_info_page_ptr]
  mov   ecx,[ecx+main_mem].mem_end

  pushad
  call  make_kernel_uncacheable
  popad

  mov   eax,cr0
  btr   eax,cr0_cd_bit
  mov   cr0,eax

  mov   esi,esp
  call  test_cache


  mov   esi,PM
  add   ecx,PM
  DO
        call  test_cache

        IFB_  eax,200h
              
              mov   eax,esi
              shr   eax,log2_dcache_line_size
              and   eax,dcache_sets-1
              shl   eax,log2_snapshot_bytes_per_dcache_set
              add   eax,edi

              sub   ebx,ebx
              DO
                    IFZ   dword ptr ds:[ebx*4+eax],-1
                          sub   esi,PM
                          mov   dword ptr ds:[ebx*4+eax],esi
                          add   esi,PM
                          EXIT
                    FI
                    inc   ebx
                    cmp   ebx,dcache_associativity
                    REPEATB
                    ke    'dcache_set_overflow??'
              OD

        ELIFB eax,400h
              ke    'hit L2?'
              
        FI

        add   esi,dcache_line_size
        cmp   esi,ecx
        REPEATB
  OD

  pushad
  call  make_kdebug_uncacheable
  popad
  mov   eax,cr0
  bts   eax,cr0_cd_bit
  mov   cr0,eax
  wbinvd

  pop   es
  pop   ds

  ret

  

analyze_data_cache:

  IFB_  esp,virtual_space_size
        kd____disp <'-'>
        ret
  FI

  kd____disp <'disabled'>
  ret

  push  ds
  push  linear_kernel_space
  pop   ds

 ; call  make_kernel_uncacheable

  mov   eax,cr0
  btr   eax,cr0_cd_bit
  mov   cr0,eax

  mov   esi,esp
  call  test_cache

  mov   eax,cr0
  bts   eax,cr0_cd_bit
  mov   cr0,eax

  mov   esi,esp
  call  test_cache

  push  eax

  mov   esi,PM+32
  call  test_cache

  pop   ebx

  ke    'cread'

;  call  make_kdebug_uncacheable

  pop   ds
  ret




test_cache:                           ; ESI : cache addr

  push  ebx
  push  ecx
  push  edx
  push  esi
  push  edi

  or    esi,PM
  and   esi,-32
  cld

  mov   eax,offset test_cache_in_cached_alias
  add   eax,offset kdebug_cached_alias_mem-PM
  call  eax

  pop   edi
  pop   esi
  pop   edx
  pop   ecx
  pop   ebx
  ret


test_cache_in_cached_alias:

  cpuid
  rdtsc 
  mov   edi,eax
  sub   eax,eax
  cpuid

  mov   ecx,32
  rep   lodsb

  sub   eax,eax
  cpuid
  rdtsc
  sub   eax,edi

  ret





;--------------------------------------------------------------------------
; 
;       trace events
;
;--------------------------------------------------------------------------


trace_event:

  IFAE  esp,virtual_space_size
  
        mov   esi,ebx
        mov   cl,al
        lno___thread eax,esp
        shl   eax,8

        push  ds
        push  linear_kernel_space
        pop   ds

        add   esi,PM
        
        IFZ   cl,'*'
              mov   al,80h
              mov   ebx,[esi+13]
              mov   ecx,[esi+17]
              call  put_words_4_to_5_into_trace_buffer
              mov   ebx,[esi+1]
              mov   bl,[esi]
              dec   bl
              IFA   bl,19
                    mov   bl,19
              FI      
              mov   ecx,[esi+5]
              mov   edx,[esi+9]
        ELSE_
              mov   al,81h
              IFZ   cl,'/'
                    mov   al,82h
              FI
              mov   ebx,[esi+9]
              mov   ebx,[esi+13]
              call  put_words_4_to_5_into_trace_buffer
              mov   ebx,[ebp+ip_eax]
              mov   ecx,[esi+1]
              mov   cl,[esi]
              dec   cl
              IFA   cl,15
                    mov   cl,15
              FI      
              mov   edx,[esi+5]
        FI
        call  put_into_trace_buffer
        DO
        
        pop   ds
  FI        

  jmp   ret_from_kdebug
  



display_event:

  IFZ   al,80h
        push  eax
        kd____disp <'ke : *'>
        lea   eax,[esi+trace_entry_string]
        kd____outstring
        kd____disp <', thread='>
        pop   eax
        shr   eax,8
        call  outthread
        ret
  FI

  IFZ al,81h
        kd____disp <'ke : #'>
        lea   eax,[esi+trace_entry_string+4]
        kd____outstring
        kd____disp <' ('>
        mov   eax,ebx
        kd____outhex32
        kd____disp <')'>
        kd____disp <', thread='>
        pop   eax
        shr   eax,8
        call  outthread
        ret        
  FI

  IFZ   al,82h
        kd____disp <'      '>
        shr   eax,8
        call  outthread
        kd____disp <' '>
        lea   eax,[esi+trace_entry_string+4]
        kd____outstring
        kd____disp <' '>
        mov   eax,ebx
        call  outthread
        ret
  FI

  kd____disp <'???'>  
  ret
  



display_thread_event:

  shr   eax,8
  call  outthread

  kd____disp <' '>
  lea   eax,[esi+trace_entry_string+4]
  kd____outstring
  kd____disp <' '>
  mov   eax,ebx
  call  outthread
  
  ret



                    


;--------------------------------------------------------------------------
;
;       init trace buffer
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       DS    phys mem
;
;---------------------------------------------------------------------------


init_trace_buffer:

  pushad
  
  IFNZ  [configuration].trace_pages,0,long

        call  [grab_frame]
        mov   ebx,eax
        mov   ecx,KB4
        DO
              call  [grab_frame]
              sub   ebx,eax
              IFNC
              CANDZ ebx,KB4
                    add   ecx,ebx
                    mov   ebx,eax
                    movzx edx,[configuration].trace_pages
                    shl   edx,log2_pagesize
                    cmp   ecx,edx
                    REPEATB
              ELSE_
                    lea   eax,[eax+ebx]
              FI      
        OD

        mov   [trace_buffer_begin],eax
        mov   edi,eax
        add   eax,ecx
        mov   [trace_buffer_end],eax
  
        call  flush_active_trace_buffer
  
        mov   dword ptr [edi+trace_entry_type],80h
        mov   dword ptr [edi+trace_entry_string],'-4L '
        mov   byte ptr  [edi+trace_entry_string],8
        mov   dword ptr [edi+trace_entry_string+4],'RATS'
        mov   dword ptr [edi+trace_entry_string+8],'T'        
        mov   [edi+trace_entry_timestamp],eax
        mov   [edi+trace_entry_timestamp+4],edx
          
        add   edi,sizeof trace_buffer_entry
        mov   [trace_buffer_in_pointer],edi
        sub   ecx,sizeof trace_buffer_entry  

        shr   ecx,2
        sub   eax,eax
        cld
        rep   stosd

  FI      
  
  popad
  ret
  
        


;--------------------------------------------------------------------------
;
;       put into trace buffer
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       EAX         0                          src/ipc type
;       EBX         fault addr                 msg w1
;       ECX         fault EIP                  dest
;       EDX         thread                     msg w0
;
;       DS    linear kernel space   
;
;---------------------------------------------------------------------------



put_into_trace_buffer:
  
  mov   edi,[trace_buffer_in_pointer+PM]
  test  edi,edi
  IFNZ  ,,long
        add   edi,PM
        
        mov   [edi+trace_entry_type],eax
        mov   [edi+trace_entry_string],ebx
        mov   [edi+trace_entry_string+4],ecx
        mov   [edi+trace_entry_string+8],edx
        
        get___timestamp
        mov   [edi+trace_entry_timestamp],eax
        mov   [edi+trace_entry_timestamp+4],edx
        
        IFNZ  [trace_perf_monitoring_mode+PM],no_perf_mon

              mov   al,[processor_family+PM]
              IFZ   al,p5_family

                    mov   ecx,P5_event_select_msr
                    rdmsr
                    mov   ebx,eax
                    and   eax,NOT ((11b SHL 22)+(11b SHL 6))
                    wrmsr
                    mov   ecx,P5_event_counter0_msr
                    rdmsr
                    mov   [edi+trace_entry_perf_count0],eax
                    mov   ecx,P5_event_counter1_msr
                    rdmsr
                    mov   [edi+trace_entry_perf_count1],eax
                    mov   ecx,P5_event_select_msr
                    mov   eax,ebx
                    wrmsr

              ELIFZ al,p6_family

                    mov   ecx,P6_event_select0_msr
                    rdmsr
                    mov   ebx,eax
                    btr   eax,22      ; disable counters
                    wrmsr
                    mov   ecx,P6_event_counter0_msr
                    rdmsr
                    mov   [edi+trace_entry_perf_count0],eax
                    mov   ecx,P6_event_counter1_msr
                    rdmsr
                    mov   [edi+trace_entry_perf_count1],eax
                    mov   ecx,P6_event_select0_msr
                    mov   eax,ebx
                    wrmsr
              FI
        FI
  
        add   edi,sizeof trace_buffer_entry-PM
        IFAE  edi,[trace_buffer_end+PM]
              mov   edi,[trace_buffer_begin+PM]
        FI
        mov   [trace_buffer_in_pointer+PM],edi  
  FI      
        
  ret
  
  
  
put_words_4_to_5_into_trace_buffer:

  mov   edi,[trace_buffer_in_pointer+PM]
  test  edi,edi
  IFNZ
        add   edi,PM
        
        mov   [edi+trace_entry_string+12],ebx
        mov   [edi+trace_entry_string+16],ecx
  FI
  ret
        
        
        
        
flush_active_trace_buffer:

  get___timestamp
  mov   [trace_buffer_active_stamp],eax
  mov   [trace_buffer_active_stamp+4],edx
  
  ret
  
          


open_trace_buffer:

  mov   ebx,[trace_buffer_begin]
  test  ebx,ebx
  IFNZ
  mov   eax,[ebx+trace_entry_timestamp]
  or    eax,[ebx+trace_entry_timestamp+4]
  CANDNZ
        DO
              mov   esi,ebx
              mov   eax,[ebx+trace_entry_timestamp]
              mov   edx,[ebx+trace_entry_timestamp+4]
              add   ebx,sizeof trace_buffer_entry
              IFAE  esi,[trace_buffer_end]
                    mov   ebx,[trace_buffer_begin]
              FI
              sub   eax,[ebx+trace_entry_timestamp]
              sbb   edx,[ebx+trace_entry_timestamp+4]
              REPEATC
        OD         
              ret               ; NC!
  FI

  stc
  ret
      
  
  
  
  
forward_trace_buffer:

  push  ebx
  
  mov   ebx,esi
  DO
        mov   eax,[ebx+trace_entry_timestamp]
        mov   edx,[ebx+trace_entry_timestamp+4]
        add   ebx,sizeof trace_buffer_entry
        IFAE  ebx,[trace_buffer_end]
              mov   ebx,[trace_buffer_begin]
        FI
        sub   eax,[ebx+trace_entry_timestamp]
        sbb   edx,[ebx+trace_entry_timestamp+4]
        EXITNC
        
        IFNZ  [trace_display_mask],0
              mov   eax,[ebx+trace_entry_type]
              cmp   eax,[trace_display_mask]
              REPEATNZ
              IFNZ  [trace_display_mask+4],0
                    mov   eax,[ebx+trace_entry_string]
                    cmp   eax,[trace_display_mask+4]
                    REPEATNZ
              FI         
        FI      
        mov   esi,ebx
        sub   cl,1              ; NC!
        REPEATNZ
        
        pop   ebx
        ret
        
  OD      
  stc
  
  pop   ebx
  ret
  
  
backward_trace_buffer:

  push  ebx
  
  mov   ebx,esi
  DO
        mov   eax,[ebx+trace_entry_timestamp]
        mov   edx,[ebx+trace_entry_timestamp+4]
        sub   ebx,sizeof trace_buffer_entry
        IFB_  ebx,[trace_buffer_begin]
              mov   ebx,[trace_buffer_end]
              sub   ebx,sizeof trace_buffer_entry
        FI
        sub   eax,[ebx+trace_entry_timestamp]
        sbb   edx,[ebx+trace_entry_timestamp+4]
        EXITC
        mov   eax,[ebx+trace_entry_timestamp]
        or    eax,[ebx+trace_entry_timestamp+4]
        EXITZ
        
        IFNZ  [trace_display_mask],0
              mov   eax,[ebx+trace_entry_type]
              cmp   eax,[trace_display_mask]
              REPEATNZ
              IFNZ  [trace_display_mask+4],0
                    mov   eax,[ebx+trace_entry_string]
                    cmp   eax,[trace_display_mask+4]
                    REPEATNZ
              FI         
        FI     
        mov   esi,ebx 
        sub   cl,1
        REPEATNZ          ; NC!
        
        pop   ebx
        ret
  OD       
  stc 
  
  pop   ebx
  ret
  
                              
                      
        
;--------------------------------------------------------------------------
;
;       show active trace buffer tail
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       DS    phys mem
;
;--------------------------------------------------------------------------


show_active_trace_buffer_tail:


  CORB  esp,virtual_space_size
  call  open_trace_buffer
  IFC
        ret
  FI

  mov   eax,[trace_buffer_in_pointer]
  mov   ebx,[eax+trace_entry_timestamp]
  or    ebx,[eax+trace_entry_timestamp+4]
  IFZ
  sub   eax,[trace_buffer_begin]
  CANDZ eax,sizeof trace_buffer_entry
        ret
  FI


  sub   eax,eax
  mov   [trace_display_mask],eax
  mov   cl,lines-3
  call  backward_trace_buffer
  
  DO
        mov   eax,[esi+trace_entry_timestamp]
        mov   edx,[esi+trace_entry_timestamp+4]
        sub   eax,[trace_buffer_active_stamp]
        sbb   edx,[trace_buffer_active_stamp+4]
        IFAE
              kd____disp <13,10>
              mov   eax,[esi+trace_entry_type]
              mov   ebx,[esi+trace_entry_string]
              mov   ecx,[esi+trace_entry_string+4]
              mov   edx,[esi+trace_entry_string+8]
              IFB_  al,40h
                    call  display_page_fault
              ELIFB al,80h
                    call  display_ipc
              ELSE_
                    call  display_event      
              FI
        FI      
        mov   cl,1
        call  forward_trace_buffer
        REPEATNC
  OD
        
  ret                   
  
              

;--------------------------------------------------------------------------
;
;       dump trace buffer
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       DS    phys mem
;
;--------------------------------------------------------------------------


dump_trace_buffer:

  mov   al,0
  
  CORB  esp,virtual_space_size
  call  open_trace_buffer
  IFC
        kd____disp <' no trace buffer',13,10>
        ret
  FI      
  
  mov   [trace_link_presentation],display_trace_index_mode
  mov   [trace_reference_mode],no_references
  sub   eax,eax
  mov   [trace_display_mask],eax
  
  mov   cl,lines-2
  call  backward_trace_buffer
  
  DO
  
        kd____clear_page
        mov   al,1
        kd____outchar
        sub   ecx,ecx
        
        sub   ecx,ecx
        mov   ebp,esi
        mov   edi,esi
        DO           
              push  ecx
              mov   eax,[esi+trace_entry_type]
              mov   ebx,[esi+trace_entry_string]
              mov   ecx,[esi+trace_entry_string+4]
              mov   edx,[esi+trace_entry_string+8]
              IFB_  al,40h
                    call  display_page_fault
              ELIFB al,80h
                    call  display_ipc
              ELSE_
                    call  display_event      
              FI
              mov   al,5
              kd____outchar                
              pop   ecx

              IFNZ  [trace_reference_mode],no_references
                    mov   al,columns-40
                    mov   ah,cl
                    kd____cursor
                    kd____disp <5,' '> 
                    
                    IFZ   [trace_reference_mode],performance_counters
                          call   display_trace_performance_counters
                    ELSE_
                          push  ebp
                          mov   ebx,offset backward_trace_buffer+KR
                          IFZ   [trace_reference_mode],forward_references
                                mov   ebx,offset forward_trace_buffer+KR
                          FI      
                          call  display_trace_reference
                          pop   ebp
                    FI      
              FI      
                                  
              push  ecx
              mov   al,columns-19
              mov   ah,cl
              kd____cursor
              mov   al,[trace_link_presentation]
              IFZ   al,display_trace_index_mode
                    mov   ch,' '
                    call  display_trace_index
              ELIFZ al,display_trace_delta_time_mode
                    mov   ch,' '
                    call  display_trace_timestamp
              ELIFZ al,display_trace_offset_time_mode
                    mov   ch,'t'
                    xchg  ebp,edi
                    call  display_trace_timestamp
                    xchg  ebp,edi
              FI      
              kd____disp <13,10>
              mov   ebp,esi
              mov   cl,1
              call  forward_trace_buffer
              pop   ecx
              EXITC
              inc   ecx
              cmp   ecx,lines-1
              REPEATB
        OD
        
        call  backward_trace_buffer
        
        call  get_kdebug_cmd
        
        mov   cl,0
        IFZ   al,10
              mov   cl,1
        FI      
        IFZ   al,3
              mov   cl,-1
        FI    
        CORZ  al,'+'  
        IFZ   al,11h
              mov   cl,lines-1
        FI    
        CORZ  al,'-'  
        IFZ   al,10h
              mov   cl,-(lines-1)
        FI
        
        IFZ   cl,0,long
              
              IFZ   al,8
                    mov   ebx,offset trace_reference_mode
                    IFZ   <byte ptr [ebx]>,forward_references
                          mov   byte ptr [ebx],no_references
                    ELIFZ <byte ptr [ebx]>,performance_counters
                          mov  byte ptr [ebx],forward_references
                    ELSE_      
                          mov   byte ptr [ebx],backward_references
                    FI
              
              ELIFZ al,2,long
                    mov   ebx,offset trace_reference_mode
                    IFZ   <byte ptr [ebx]>,backward_references
                          mov   byte ptr [ebx],no_references
                    ELIFZ <byte ptr [ebx]>,forward_references
                    CANDNZ [trace_perf_monitoring_mode],no_perf_mon
                          mov  byte ptr [ebx],performance_counters
                    ELSE_      
                          mov   byte ptr [ebx],forward_references
                    FI
                      
              ELIFZ al,13
                    mov   ebx,offset trace_display_mask
                    sub   eax,eax
                    IFZ   [ebx],eax
                          mov   [ebx+4],eax
                          mov   eax,[esi]
                          mov   [ebx],eax
                    ELSE_
                          mov   eax,[esi+4]
                          mov   [ebx+4],eax
                    FI            
              
              ELIFZ al,1
                    mov   ebx,offset trace_display_mask
                    sub   eax,eax
                    IFNZ  [ebx+4],eax
                          mov   [ebx+4],eax
                    ELSE_      
                          mov   [ebx],eax
                    FI      
                    
                    
              ELIFZ al,' '
                    mov   al,[trace_link_presentation]
                    IFZ   al,display_trace_index_mode
                          mov   al,display_trace_delta_time_mode
                    ELIFZ al,display_trace_delta_time_mode
                          mov   al,display_trace_offset_time_mode
                    ELSE_
                          mov   al,display_trace_index_mode
                    FI
                    mov   [trace_link_presentation],al       
              
              ELIFZ al,'P'
              bt    [processor_feature_flags],pentium_style_msrs_bit      
              CANDC
                    call  set_performance_tracing
                    EXITZ              
                    
              ELSE_
                    call  is_main_level_command_key
                    EXITZ
              FI
        FI
                          
        IFG   cl,0  
              mov   ch,cl
              call  forward_trace_buffer
              push  esi
              mov   cl,lines-2
              call  forward_trace_buffer
              pop   esi
              IFNZ  cl,0
                    IFB_  ch,cl
                          mov   cl,ch
                    FI      
                    call  backward_trace_buffer
              FI      
        ELIFL cl,0
              neg   cl
              call  backward_trace_buffer
        FI            
        
        REPEAT      
        
  OD                 
              
  
  ret                   
  
              


;--------------------------------------------------------------------------
;
;       display trace index
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   pointer to current trace entry
;       CH    prefix char
;
;       DS    phys mem
;
;--------------------------------------------------------------------------


display_trace_index:

  push  eax
  
  mov   al,ch
  kd____outchar
  mov   eax,esi
  sub   eax,[trace_buffer_in_pointer]
  IFC
        add   eax,[trace_buffer_end]
        sub   eax,[trace_buffer_begin]
  FI      
  log2  <(sizeof trace_buffer_entry)>
  shr   eax,log2_
  kd____outhex12
  
  pop   eax
  ret
  
  
  
              
;--------------------------------------------------------------------------
;
;       display trace timestamp
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   pointer to current trace entry
;       EBP   pointer to reference trace entry
;       CH    prefix char
;
;       DS    phys mem
;
;--------------------------------------------------------------------------


display_trace_timestamp:

  push  eax
  push  ebx
  push  ecx
  push  edx
  
  
  mov   eax,[esi+trace_entry_timestamp]
  mov   edx,[esi+trace_entry_timestamp+4]
  
  IFNZ  esi,ebp      
        mov   cl,'+'
        sub   eax,ds:[ebp+trace_entry_timestamp]
        sbb   edx,ds:[ebp+trace_entry_timestamp+4]
        IFC
              mov   cl,'-'
              neg   eax
              adc   edx,0
              neg   edx
        FI            
  FI 

    
  push  ecx
  push  edi
  mov   edi,ds:[phys_kernel_info_page_ptr]
  IFNZ  edi,0
        mov   ebx,eax
        mov   ecx,edx
  
        mov   eax,2000000000
        sub   edx,edx
        div   ds:[edi].cpu_clock_freq
        shr   eax,1
        adc   eax,0
  
        imul  ecx,eax
        mul   ebx
        add   edx,ecx                    ; eax,edx : time in nanoseconds
  FI
  pop   edi
  pop   ecx
  
  IFZ   esi,ebp
        IFZ   ch,'t'
              kd____disp <' t=.'>
        ELSE_      
              kd____disp <'   .'>
        FI      
        mov   cl,'.'
        mov   ch,'.'
        mov   ebx,1000/200
        call  outdec2
        kd____disp <' us'>
        
  ELSE_
        CORA  edx,0
        IFAE  eax,1000000000
              mov   ebx,1000000000/200
              call  outdec2
              kd____disp <' s'>
        
        ELIFAE eax,1000000
              kd____disp <'    '>
              mov   ebx,1000000/200
              call  outdec2
              kd____disp <' ms'>
        ELSE_           
              kd____disp <'        '>
              mov   ebx,1000/200
              call  outdec2
              kd____disp <' us'>
        FI
  FI            
          
  
  pop   edx
  pop   ecx
  pop   ebx
  pop   eax
  ret
  
  
  
outdec2:

  sub   edx,edx
  div   ebx
  shr   eax,1
  adc   eax,0
    
  mov   ebx,100
  sub   edx,edx
  div   ebx
  
  IFB_  eax,10
        kd____disp <'  '>
  ELIFB eax,100
        kd____disp <' '>
  FI                  
  
  push  eax
  mov   al,ch
  kd____outchar
  mov   al,cl
  kd____outchar
  pop   eax
    
  kd____outdec
  kd____disp <'.'>
  mov   eax,edx
  IFB_  eax,10
        kd____disp <'0'>
  FI
  kd____outdec
  
  ret                                




;--------------------------------------------------------------------------
;
;       display reference
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   pointer to current trace entry
;
;       DS    phys mem
;
;--------------------------------------------------------------------------


display_trace_reference:

  push  eax
  push  ecx
  push  ebp
  
  mov   ebp,esi
  
  DO
        mov   cl,1
        call  ebx
        EXITC
        
        mov   eax,[esi+trace_entry_type]
        cmp   eax,ds:[ebp+trace_entry_type]
        REPEATNZ
        mov   eax,[esi+trace_entry_string]
        cmp   eax,ds:[ebp+trace_entry_string]
        REPEATNZ
        
        mov   ch,'@'
        IFZ   [trace_link_presentation],display_trace_index_mode
              call  display_trace_index
        ELSE_
              call  display_trace_timestamp      
        FI
  OD
  
  mov   esi,ebp
  pop   ebp
  pop   ecx
  pop   eax
  ret            
  


;--------------------------------------------------------------------------
;
;       set performance tracing
;
;--------------------------------------------------------------------------









set_performance_tracing:

  mov   al,[processor_family]
  IFNZ  al,p5_family
  CANDNZ al,p6_family
        kd____disp <' not supported by this processor',13,10>
        sub   eax,eax     ; Z!
        ret
  FI


  kd____clear_page
  call  show_trace_perf_monitoring_mode
  
  kd____disp <' Performance Monitoring',13,10,10,'- : off, + : kernel+user, k : kernel, u : user, / : kernel/user',13,10,10>

  mov   al,[processor_family]
  IFZ   al,p5_family,long
        
        kd____disp <'i : Instructions   (total/V-pipe)',13,10>
        kd____disp <'c : Cache Misses  (DCache/ICache)',13,10>
        kd____disp <'t : TLB Misses      (DTLB/ITLB)',13,10>
        kd____disp <'m : Memory Stalls   (read/write)',13,10>
        kd____disp <'a : Interlocks       (AGI/Bank Conflict)',13,10>
        kd____disp <'b : Bus Utilization  (Bus/Instructions)',13,10>

  ELIFZ al,p6_family,long
        
        kd____disp <'1 : L1 Dcache Misses  (rd/wback)',13,10>
        kd____disp <'2 : L2 Cache Misses   (rd/wback)',13,10>
        kd____disp <'i : Icache/ITLB Misses',13,10>
        kd____disp <'I : Instructions/stalls',13,10>
        kd____disp <'b : Bus Utilization/Instructions',13,10> 
        kd____disp <'# : enter performance-counter selectors',13,10> 
  FI 
  
  DO
        kd____inchar
        kd____outchar
  
  
        IFZ   al,'-'
              mov   ah,[processor_family]
              IFZ   ah,p5_family
                    mov   [trace_perf_monitoring_mode],no_perf_mon
                    sub   eax,eax
                    mov   ecx,P5_event_select_msr
                    wrmsr
              ELIFZ ah,p6_family
                    sub   eax,eax
                    mov   ecx,P6_event_select0_msr
                    wrmsr
                    mov   ecx,P6_event_select1_msr
                    wrmsr
                    mov   ecx,P6_event_counter0_msr
                    wrmsr
                    mov   ecx,P6_event_counter1_msr
                    wrmsr
              FI      
              ret
        FI
         
        CORZ  al,'&'
        CORZ  al,'/'
        CORZ  al,'+'
        CORZ  al,'k'
        IFZ   al,'u'      
              IFZ   al,'+'
                    mov   al,kernel_user_perf_mon
              ELIFZ al,'k'
                    mov   al,kernel_perf_mon
              ELIFZ al,'u'
                    mov   al,user_perf_mon
              ELIFZ al,'/'
                    mov   al,kernel_versus_user_perf_mon
              ELIFZ al,'&'
                    mov   al,kernel_versus_user_perf_mon
              FI
              mov   [trace_perf_monitoring_mode],al
              call  show_trace_perf_monitoring_mode
              REPEAT      
        FI
  OD        

  mov   ah,[processor_family]
  IFZ   ah,p5_family,long

        sub   ebx,ebx
        IFZ   al,'i'
              mov   ebx,P5_instrs_ex + (P5_instrs_ex_V SHL 16)
        FI
        IFZ   al,'c'
              mov   ebx,P5_rw_miss + (P5_ex_miss SHL 16)
        FI
        IFZ   al,'t'
              mov   ebx,P5_rw_tlb + (P5_ex_tlb SHL 16)
        FI
        IFZ   al,'m'
              mov   ebx,P5_r_stall + (P5_w_stall SHL 16)
        FI
        IFZ   al,'a'
              mov   ebx,P5_agi_stall + (P5_bank_conf SHL 16)
        FI
        IFZ   al,'b'
              mov   ebx,P5_bus_util + (P5_instrs_ex SHL 16)
        FI            

        mov   [performance_count_params],ebx


  ELIFZ ah,p6_family,long

        sub   ebx,ebx

        IFZ   al,'1'
              mov   ebx,P6_rw_miss + (P6_d_wback SHL 16)
        FI
        IFZ   al,'2'
              mov   ebx,P6_L2_rw_miss + (P6_L2_d_wback SHL 16)
        FI
        IFZ   al,'i'
              mov   ebx,P6_ex_miss + (P6_ex_tlb SHL 16)
        FI
        IFZ   al,'I'
              mov   ebx,P6_instrs_ex + (P6_stalls SHL 16)
        FI
        IFZ   al,'b'
              mov   ebx,P6_bus_util + (P6_instrs_ex SHL 16)
        FI            
        IFZ   al,'#'
              kd____disp <' PerfC0:'>
              kd____inhex16
              mov   ebx,eax
              IFNZ  [trace_perf_monitoring_mode],kernel_versus_user_perf_mon
                    kd____disp <' PerfC1:'>
                    kd____inhex16
                    shl   eax,16
                    add   ebx,eax
              FI
        FI

        mov   [performance_count_params],ebx


  FI

  sub   eax,eax
  mov   [perf_counter0_value],eax
  mov   [perf_counter1_value],eax


  test  esp,esp           ; NZ !
  ret                            
  







set_performance_counters:
  
  push  eax
  push  ecx
  push  ebx


  IFZ   [processor_family],p5_family
        push  eax
        sub   eax,eax
        mov   ecx,P5_event_select_msr
        wrmsr
        mov   ecx,P5_event_counter0_msr
        mov   eax,[perf_counter0_value]
        wrmsr
        mov   ecx,P5_event_counter1_msr
        mov   eax,[perf_counter1_value]
        wrmsr
        pop   eax

        IFZ   al,kernel_perf_mon
              mov   al,01b
        ELIFZ al,user_perf_mon
              mov   al,10b
        ELSE_
              mov   al,11b
        FI
  
        shl   eax,6
        or    ebx,eax
        shl   eax,22-6
        or    eax,ebx
        mov   ecx,P5_event_select_msr
        wrmsr
  
  ELIFAE [processor_family],p6_family,long

        push  eax
        sub   eax,eax
        mov   ecx,P6_event_select0_msr
        wrmsr
        mov   ecx,P6_event_select1_msr
        wrmsr
        mov   ecx,P6_event_counter0_msr
        mov   eax,[perf_counter0_value]
        wrmsr
        mov   ecx,P6_event_counter1_msr
        mov   eax,[perf_counter1_value]
        wrmsr
        pop   eax

        IFZ   al,kernel_versus_user_perf_mon

              and   ebx,0000FFFFh
              mov   ecx,ebx
              or    ebx,10b SHL 16
              or    ecx,01b SHL 16
              bts   ebx,22      ; enable counters

        ELSE_

              mov   ecx,ebx
              shr   ecx,16
              and   ebx,0000FFFFh

              bts   ebx,22      ; enable counters

              IFZ   al,kernel_perf_mon
                    mov   al,10b
              ELIFZ al,user_perf_mon
                    mov   al,01b
              ELSE_
                    mov   al,11b
              FI
              shl   eax,16
              or    ebx,eax
              or    ecx,eax
        FI

        mov   eax,ecx
        mov   ecx,P6_event_select1_msr
        wrmsr
        mov   eax,ebx
        mov   ecx,P6_event_select0_msr
        wrmsr
  FI

  pop   ebx
  pop   ecx
  pop   eax
  ret




read_performance_counters:

  push  ecx

  mov   al,[processor_family]
  IFZ   al,p5_family
  
        mov   ecx,P5_event_select_msr
        rdmsr
        and   eax,NOT ((11b SHL 22)+(11b SHL 6))
        wrmsr
        mov   ecx,P5_event_counter0_msr
        rdmsr
        mov   ebx,eax
        mov   ecx,P5_event_counter1_msr
        rdmsr
  
  ELIFZ al,p6_family

        mov   ecx,P6_event_select0_msr
        rdmsr
        btr   eax,22      ; disable counters
        wrmsr
        mov   ecx,P6_event_counter0_msr
        rdmsr
        mov   ebx,eax
        mov   ecx,P6_event_counter1_msr
        rdmsr
  FI
  xchg  eax,ebx

  pop   ecx
  ret





show_trace_perf_monitoring_mode:

  mov   al,1
  mov   ah,1
  kd____cursor
  
  mov   al,[trace_perf_monitoring_mode]
  
  IFZ   al,no_perf_mon
        kd____disp <'           '>
  ELIFZ al,kernel_user_perf_mon
        kd____disp <'Kernel+User'>
  ELIFZ al,kernel_perf_mon
        kd____disp <'     Kernel'>
  ELIFZ al,kernel_versus_user_perf_mon
        kd____disp <'Kernel/User'>
  ELSE_
        kd____disp <'       User'>
  FI                  
              
  ret



;--------------------------------------------------------------------------
;
;       display trace performance counters
;
;--------------------------------------------------------------------------
; PRECONDITION:
;
;       ESI   pointer to current trace entry
;       EBP   pointer to reference trace entry
;
;       DS    phys mem
;
;--------------------------------------------------------------------------

  
  

display_trace_performance_counters:

  push  eax
  
  IFNZ  esi,ebp
  
        kd____disp <'P: '>
  
        mov   eax,[esi+trace_entry_perf_count0]
        sub   eax,ds:[ebp+trace_entry_perf_count0]
        kd____outdec
        
        kd____disp <' / '>
        
        mov   eax,[esi+trace_entry_perf_count1]
        sub   eax,ds:[ebp+trace_entry_perf_count1]
        kd____outdec
  
  FI
  
  pop   eax
  ret
  


;---------------------------------------------------------------------------
;---------------------------------------------------------------------------
;---------------------------------------------------------------------------



;---------------------------------------------------------------------------
;
;             RESET
;
;---------------------------------------------------------------------------


kb_status          equ  64h
kb_cntl            equ  64h
kb_data            equ  60h


rtc_address        equ  70h
rtc_data           equ  71h



inrtc macro rtcport

  mov al,rtcport
  out rtc_address,al
  jmp $+2
  in  al,rtc_data
  endm


outrt macro rtcport

  push eax
  mov al,rtcport
  out rtc_address,al
  jmp $+2
  pop eax
  out rtc_data,al
  endm


  
kdebug_reset_pc:

  mov   edx,ecx

  cli
  inrtc 80h                              ; disable NMI

  sub   eax,eax
  mov   dr7,eax                          ; disable all breakpoints

  mov   eax,cr0                          ; IF paging already on
  bt    eax,31                           ;
  IFC                                    ;
        mov   ebx,linear_kernel_space    ;
        mov   ds,ebx                     ;
  FI                                     ;

                                         ; REBOOT:
                                         ;
  sub   eax,eax                          ;
  IFA   esp,<virtual_space_size>         ;
        mov   eax,PM                     ;
  FI                                     ;
  mov   word ptr ds:[eax+472h],1234h     ; inhibit memory test at reboot
  DO                                     ;
       in   al,kb_status                 ;
       test al,10b                       ;
       REPEATNZ                          ;
  OD                                     ;
                                         ;
  mov   al,0                             ; cmos: shutdown with boot loader req
  outrt 8Fh                              ; NMI disabled
                                         ;
  mov  al,0FEh                           ; reset pulse command
  out  kb_cntl,al                        ;
                                         ;
  jmp  $                                 ;
  




;---------------------------------------------------------------------------

default_kdebug_end    equ $



  dcod  ends


  code  ends
  end