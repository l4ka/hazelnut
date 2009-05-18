
include ctrpre 

  GMD L3.KTEST, 04.11.94, 30007
 
;*********************************************************************
;******                                                         ******
;******         Kernel Test                                     ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  14.06.91    ******
;******                                   modified: 04.11.94    ******
;******                                                         ******
;*********************************************************************
 

 
  public kernel_test


  extrn create_pl3_kernel_thread:near
  extrn thread_to_v3:near
  extrn page_phys_address:near
  extrn grab_frame:near
  extrn kdebug_outstring:near
  extrn kdebug_outchar:near
  extrn kdebug_outdec:near
  extrn ipc_sc:near
  extrn kernel_proot:dword
  extrn hw_config_descriptor:byte


include ctrconst
include adrspace
include mcb
include tcb
include msg
include intrifc
include cpucb
include schedcb
include lbmac
include pagmac1
include syscalls




disp macro string
        local xx

  mov   eax,offset $+5+5+1
  call  kdebug_outstring
  jmp   short xx
  db    string
xx:
  endm






  assume ds:codseg



kernel_test:

  call  put_hw_data

  mov   al,ds:[cpu_type]
  mov   [processor],al

  call  grab_frame
  mov   ebx,offset ping_snd_msg+size ping_snd_msg
  mov   [ebx].str_addr,eax
  mov   [ebx].buf_addr,eax
  mov   ebx,offset ping_rcv_msg+size ping_rcv_msg
  mov   [ebx].str_addr,eax
  mov   [ebx].buf_addr,eax
  call  grab_frame
  mov   ebx,offset pong_snd_msg+size pong_snd_msg
  mov   [ebx].str_addr,eax
  mov   [ebx].buf_addr,eax
  mov   ebx,offset pong_rcv_msg+size pong_rcv_msg
  mov   [ebx].str_addr,eax
  mov   [ebx].buf_addr,eax

  mov   edx,'gnip'
  mov   esi,offset ping_stack
  mov   edi,offset ping
  call  create_pl3_kernel_thread

  push  eax
  mov   eax,offset task_proot         ; by this trick both threads point in fact
  mov   ebx,kernel_task_no            ; to the same (kernel) proot, but address
  call  page_phys_address             ; space switch (CR3 load) will be done
  sub   eax,PM
  mov   ebx,[eax+2*4]
  mov   [eax],ebx
  pop   eax
  mov   ebx,kernel_task_no
  call  page_phys_address
  sub   eax,PM
  mov   [eax+proot_ptr],offset task_proot

  mov   [ping_tcb],eax

  mov   edx,'gnop'
  mov   esi,offset pong_stack
  mov   edi,offset pong
  call  create_pl3_kernel_thread


  DO
        mov   eax,offset pong_rcv_msg+2
        sub   ebx,ebx
        mov   ecx,-1
        sub   edx,edx
        sub   esi,esi
        mov   edi,-1
        int   ipc
        REPEAT
  OD



  align 16

              dd    63 dup (0)
ping_stack    dd    0
              dd    63 dup (0)
pong_stack    dd    0


              align 16
ping_dest_vec dd    0,0,never,never,0,0
              align 16
ping_snd_msg  dd    00007E00h,0,128 dup (0)
              dd    0,0,KB4,0
              align 16
ping_rcv_msg  dd    00017E00h,0,128 dup (0)
              dd    0,0,KB4,0

              align 16
pong_dest_vec dd    0,0,never,never,0,0
              align 16
pong_snd_msg  dd    00007E00h,0,128 dup (0)
              dd    0,0,KB4,0
              align 16
pong_rcv_msg  dd    00017E00h,0,128 dup (0)
              dd    0,0,KB4,0

counter       dd 0

ping_tcb      dd 0

processor     db 0


              align 16

test_idt      dw    0, linear_space_exec, 0EE00h, 0



rd_miss       equ   000011b
wr_miss       equ   000100b
rw_miss       equ   101001b
ex_miss       equ   001110b

d_wback       equ   000110b

rw_tlb        equ   000010b
ex_tlb        equ   001101b

a_stall       equ   011111b
w_stall       equ   011001b
r_stall       equ   011010b
x_stall       equ   011011b

ncache_refs   equ   011110b
locked_bus    equ   011100b



cnt_nothing   equ   000b SHL 6
cnt_event_pl0 equ   001b SHL 6
cnt_event     equ   011b SHL 6


select        equ   11h
cnt0          equ   12h
cnt1          equ   13h





ping:

  mov   al,sys_prc_id_v2
  mov   ebx,'gnop'
  int   7
  mov   ecx,ebx
  mov   ebx,eax
  call  thread_to_v3
  mov   [ping_dest_vec],ebx
  mov   [ping_dest_vec+4],ecx

  mov   [ping_snd_msg+msg_dope].msg_dwords,0
  mov   [ping_snd_msg+msg_dope].msg_strings,0
  mov   [pong_snd_msg+msg_dope].msg_dwords,0
  mov   [pong_snd_msg+msg_dope].msg_strings,0


  disp  <13,10,10,'ipc_8    :  '>
  call  ping_100000


  disp  <13,10,'ipc_12   :  '>
  mov   [ping_snd_msg+msg_dope].msg_dwords,1
  mov   [pong_snd_msg+msg_dope].msg_dwords,1
  call  ping_100000


  disp  <13,10,'ipc_128  :  '>
  mov   [ping_snd_msg+msg_dope].msg_dwords,32-2
  mov   [pong_snd_msg+msg_dope].msg_dwords,32-2
  call  ping_100000

  disp  <13,10,'ipc_512  :  '>
  mov   [ping_snd_msg+msg_dope].msg_dwords,128-2
  mov   [pong_snd_msg+msg_dope].msg_dwords,128-2
  call  ping_100000

  mov   [ping_snd_msg+msg_dope].msg_dwords,0
  mov   [ping_snd_msg+msg_dope].msg_strings,1
  mov   [pong_snd_msg+msg_dope].msg_dwords,0
  mov   [pong_snd_msg+msg_dope].msg_strings,1

  disp  <13,10,'ipc_1024 :  '>
  mov   [ping_snd_msg+size ping_snd_msg].str_len,1024
  mov   [pong_snd_msg+size ping_snd_msg].str_len,1024
  call  ping_100000

  disp  <13,10,'ipc_2048 :  '>
  mov   [ping_snd_msg+size ping_snd_msg].str_len,2048
  mov   [pong_snd_msg+size ping_snd_msg].str_len,2048
  call  ping_100000

  disp  <13,10,'ipc_4096 :  '>
  mov   [ping_snd_msg+size ping_snd_msg].str_len,4096
  mov   [pong_snd_msg+size ping_snd_msg].str_len,4096
  call  ping_100000

 DO
  ke 'xxx'
  REPEAT
 OD

  mov   ax,080Fh          ; invd
  IFNZ  <word ptr [f0]>,ax
        mov   word ptr [f0],ax
        mov   word ptr [f1],ax
        mov   word ptr [f2],ax
        mov   word ptr [f3],ax
        mov   word ptr [f4],ax
        mov   word ptr [f5],ax
        mov   word ptr [f6],ax
        mov   word ptr [f7],ax
        disp  <13,10,10,'----- mit cache flush ----'>
        jmp   ping
  FI



  disp  <13,10,'cache used: '>

  mov   [ping_snd_msg+msg_dope].msg_dwords,1
  mov   [ping_snd_msg+msg_dope].msg_strings,0

  mov   eax,[ping_dest_vec]
  leaw__tcb eax,eax
  mov   ebx,[kernel_proot]
  and   ebx,-KB4
  xpdir ecx,eax
  mov   ebx,[(ecx*4)+ebx]
  and   ebx,-KB4
  xptab ecx,eax
  mov   ecx,[(ecx*4)+ebx]
  and   ecx,-KB4
  and   eax,(-size tcb AND KB4-1)
  or    eax,ecx
  mov   [eax+size pl0_stack-size int_pm_stack + ip_eip],offset cache_count_pl3

  mov   eax,offset ping_rcv_msg
  sub   ebx,ebx
  mov   ecx,offset ping_snd_msg
  mov   edx,[ping_dest_vec]
  mov   esi,[ping_dest_vec+4]
  lea   edi,[ebx-1]

  db 0Fh,08h
  int   ipc

  jmp   $




cache_count_pl3:

  mov   byte ptr ds:[ipc_sc],0E9h  ; jmp
  mov   eax,offset cache_count
  sub   eax,offset ipc_sc+5
  mov   dword ptr ds:[ipc_sc+1],eax
  jmp   $+2
  int   ipc


cache_count:

  mov   eax,cr0
  bts   eax,30
  mov   cr0,eax

cache_is_off:

  mov   ebx,02h
  sub   ecx,ecx
  DO
        db    0fh,26h,0ebh  ; mov   tr5,ebx
        db    0fh,24h,0e0h  ; mov   eax,tr4
        bt    eax,10
        adc   ecx,0
        add   ebx,4
        cmp   ebx,2048
        REPEATB
  OD

  mov   eax,ecx
  shl   eax,4
  sub   eax,(offset cache_is_off+15 - offset cache_count_pl3) AND -16
  call  kdebug_outdec
  disp  <'  bytes'>


  DO
        ke 'done'
        REPEAT
  OD

; disp  <13,10,'user_exc   :  '>
;
; mov   eax,offset test_intr
; mov   [test_idt],ax
; shr   eax,16
; mov   [test_idt+6],ax
;
; sub   ebx,ebx
; mov   ecx,offset test_idt
; mov   edx,size test_idt
; mov   al,specop_v2
; int   7
;
; mov   eax,[ping_tcb]
; inc   [eax+mytask]                 ; preventing kdebug at test intr0 10000 times
;
; mov   [counter],100000
; mov   al,get_system_time
; int   7
; mov   edi,eax
;
; sub   ebx,ebx
; clign 16
; div   ebx
; ;     intr 0 forever



ping_100000:

  mov   [counter],100000

  mov   al,get_system_time
  int   7
  push  eax


  IFZ   [processor],pentium

        mov   ecx,select
        sub   edx,edx
        mov   eax,r_stall + cnt_event + ((rw_miss + cnt_event) SHL 16)
        wrmsr

        sub   eax,eax
        mov   ecx,cnt0
        wrmsr
        mov   ecx,cnt1
        wrmsr

        rdtsc
  FI
  push  eax
  push  edx


  clign 16
  DO

;    db 0Fh,08h
        mov   eax,offset ping_rcv_msg
        sub   ebx,ebx
        mov   ecx,offset ping_snd_msg
        mov   edx,[ping_dest_vec]
        mov   esi,[ping_dest_vec+4]
        lea   edi,[ebx-1]
        int   ipc
f0:     test  al,al
        EXITNZ
;    db 0Fh,08h
        mov   eax,offset ping_rcv_msg
        sub   ebx,ebx
        mov   ecx,offset ping_snd_msg
        mov   edx,[ping_dest_vec]
        mov   esi,[ping_dest_vec+4]
        lea   edi,[ebx-1]
        int   ipc
f1:     test  al,al
        EXITNZ
;    db 0Fh,08h
        mov   eax,offset ping_rcv_msg
        sub   ebx,ebx
        mov   ecx,offset ping_snd_msg
        mov   edx,[ping_dest_vec]
        mov   esi,[ping_dest_vec+4]
        lea   edi,[ebx-1]
        int   ipc
        test  al,al
f2:     EXITNZ
;    db 0Fh,08h
        mov   eax,offset ping_rcv_msg
        sub   ebx,ebx
        mov   ecx,offset ping_snd_msg
        mov   edx,[ping_dest_vec]
        mov   esi,[ping_dest_vec+4]
        lea   edi,[ebx-1]
        int   ipc
f3:     test  al,al
        EXITNZ
        sub   [counter],4
        REPEATNZ
  OD
  test  al,al
  IFNZ
        ke    'ping_err'
  FI

  pop   edi
  pop   esi
  pop   ebx

  IFZ   [processor],pentium

        rdtsc
        sub   eax,esi
        sbb   edx,edi
        mov   edi,200000
        div   edi
        mov   esi,eax

        mov   ecx,select
        sub   eax,eax
        sub   edx,edx
        wrmsr
  FI

  pushad

  push  ebx
  mov   al,get_system_time
  int   7
  pop   ebx
  sub   eax,ebx
  inc   eax
  shr   eax,1
  call  microseconds

  popad

  IFZ   ds:[processor],pentium

        disp  <'  cycles/ipc: '>
        mov   eax,esi
        call  kdebug_outdec
        disp  <', events/ipc: cnt0='>
        mov   ecx,cnt1
        rdmsr
        mov   esi,eax
        mov   ecx,cnt0
        rdmsr
        div   edi
        call  kdebug_outdec
        disp  <' cnt1='>
        mov   eax,esi
        sub   edx,edx
        div   edi
        call  kdebug_outdec
  FI

  ret




pong:

  mov   eax,offset pong_rcv_msg+2
  sub   ebx,ebx
  mov   ecx,-1
  sub   edx,edx
  sub   esi,esi
  mov   edi,-1
  int   ipc
  test  al,al
  IFNZ
        ke    'pong_wait_err'
  FI
  mov   [pong_dest_vec],edx
  mov   [pong_dest_vec+4],esi

  clign 16
  DO
        mov   eax,offset pong_rcv_msg+2
        sub   ebx,ebx
        mov   ecx,offset pong_snd_msg
        lea   edi,[ebx-1]
        int   ipc
f4:     test  al,al
        EXITNZ
        mov   eax,offset pong_rcv_msg+2
        sub   ebx,ebx
        mov   ecx,offset pong_snd_msg
        lea   edi,[ebx-1]
        int   ipc
f5:     test  al,al
        EXITNZ
        mov   eax,offset pong_rcv_msg+2
        sub   ebx,ebx
        mov   ecx,offset pong_snd_msg
        lea   edi,[ebx-1]
        int   ipc
f6:     test  al,al
        EXITNZ
        mov   eax,offset pong_rcv_msg+2
        sub   ebx,ebx
        mov   ecx,offset pong_snd_msg
        lea   edi,[ebx-1]
        int   ipc
f7:     test  al,al
        REPEATZ
  OD
  DO
        ke    'pong_err'
        REPEAT
  OD



        align 16


;test_intr:
;
; dec   [counter]
; IFNZ
;       iretd
; FI
; mov   al,get_system_time
; int   7
; sub   eax,edi
; call  microseconds
; disp  <13,10,10,10>
;
; mov   ebx,[ping_tcb]
; dec   [ebx+mytask]                 ; reenable kdebug
; DO
;       ke    'test'
;       REPEAT
; OD




  align 4


microseconds:

  pushad

  sub   edx,edx
  mov   ebx,100
  div   ebx
  call  kdebug_outdec
  mov   al,'.'
  call  kdebug_outchar
  IFB_  edx,10
        mov   al,'0'
        call  kdebug_outchar
  FI
  mov   eax,edx
  call  kdebug_outdec
  disp  <' us'>

  popad
  ret



hw_config_descr     struc
hw_cpu               db '        '
hw_clock_rate        dw 0               ;cpu_speed 
                     dw 0
                     dw 0               ;cpu_bus_width 
                     dw 0
                     db '        '      ;cache_type 
hw_cache_acc_time    dw 0               ;cache_speed 
                     dw 0               ;
                     dd 0               ;cache_size 
                     db '        '      ;ram_type 
hw_ram_min_acc_time  dw 0               ;ram_speed
hw_ram_max_acc_time  dw 0               ;
hw_ram_size          dd 0               ;ram_size 
hw_co1               db '        '      ;co1_type
                     dd 0               ;co1_speed
                     dd 0               ; 
                     db '        '      ;co2_type 
                     dd 0               ;co2_speed 
                     dd 0               ; 
                     db '        '      ;co3_type 
                     dd 0               ;co3_speed 
                     dd 0               ; 
                     db '        '      ;
                     dd 0               ; 
                     dd 0               ;
                     db '        '      ;
L2_cache_x           dd 0               ; 
L2_cache_y           dd 0               ; 
hw_config_descr     ends


put_hw_data:

  mov   ebx,offset hw_config_descriptor

  disp  <13,10,10,'cpu clock: '>
  movzx eax,[ebx+hw_clock_rate]
  call  kdebug_outdec

  disp  <'ns',13,10,'L2 cache acc: '>
  movzx eax,[ebx+hw_cache_acc_time]
  call  kdebug_outdec

  disp  <'ns,  RAM acc: '>
  movzx eax,[ebx+hw_ram_min_acc_time]
  call  kdebug_outdec
  disp  <'-'>
  movzx eax,[ebx+hw_ram_max_acc_time]
  call  kdebug_outdec

  disp  <'ns  , L2 Rd256K x:'>
  mov   eax,[ebx+L2_cache_x]
  call  kdebug_outdec
  disp  <', y:'>
  mov   eax,[ebx+L2_cache_y]
  call  kdebug_outdec
  disp  <13,10,10>

  ret


  code  ends
  end