include l4pre.inc 

  scode
 
  Copyright GMD, L4.KTEST.4, 02,06,96, 15

;*********************************************************************
;******                                                         ******
;******         Kernel Test                                     ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  14.06.91    ******
;******                                   modified: 02.06.96    ******
;******                                                         ******
;*********************************************************************
 

 
  public ktest0_start
  public ktest1_start
  public ktest0_stack
  public ktest1_stack
  public ktest_begin
  public ktest_end
  public rdtsc_clocks



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
include pagmac.inc
include syscalls.inc
include kpage.inc
include l4kd.inc
.list




ok_for i486





  assume ds:codseg


ktest_begin   equ $


ping_thread   equ booter_thread
pong_thread   equ (sigma2_thread+sizeof tcb)
;pong_thread   equ (booter_thread+sizeof tcb)


  align 16

              dd    63 dup (0)
ktest0_stack  dd    0
              dd    63 dup (0)
ktest1_stack  dd    0

rdtsc_clocks  dd    0


              align 16
ping_dest_vec dd    0,0
              clign 16-8
ping_snd_msg  dd    0,0,0,128 dup (0)
              align 16
ping_rcv_msg  dd    0,128 SHL md_mwords,0,128 dup (0)

              clign 16-8
pong_snd_msg  dd    0,0,0,128 dup (0)
              align 16
pong_rcv_msg  dd    0,128 SHL md_mwords,0,128 dup (0)

counter       dd 0





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



;------------------------------------------------------
;
;       ping
;
;------------------------------------------------------

;include msg.mac
;include gdp3
;
;disk_driver    dd sigma0_disk_driver,0
;
;
;order:  sr_msg 4,,4
;        dd 0,0,0,0
;

  assume ds:codseg


ktest0_start:

  mov   ecx,100
  DO
        push  ecx
        sub   esi,esi
        int   thread_switch
        pop   ecx
        dec   ecx
        REPEATNZ
  OD




; mov   eax,1
; mov   ecx,offset ktest1_stack
; mov   edx,offset ktest1_start
; sub   ebx,ebx
; mov   ebp,ebx
; mov   esi,ebx
; mov   edi,ebx
; int   lthread_ex_regs
;


;  DO
;        int   thread_switch
;
;        ipc___call  [disk_driver],gdp_index+open_order,ebx
;
;        ke 'dopen'
;
;        test  al,ipc_error_mask
;        REPEATNZ
;  OD
;
;  mov   eax,offset order
;  mov   [eax].msg_w2,10*512
;  mov   [eax].msg_w3,3*MB1
;
;  ipc___call  [disk_driver],gdp_index+block_data_seq_ptr+exec_in,0,<offset order>,<offset order>
;
;  ke    'dread'
;

  sub   ecx,ecx
  mov   eax,ecx
  lea   edx,[ecx+1]
  mov   ebx,edx
  mov   ebp,1000h+(12 SHL 2)+map_msg
  mov   esi,sigma0_task
  mov   edi,root_chief
  int   ipc



   kd____disp  <13,10,10,'PageFault:  '>
   call  pf_1024

   DO

        mov   [ping_dest_vec],pong_thread
        mov   [ping_dest_vec+4],root_chief

        mov   [ping_snd_msg+msg_dope],0
        mov   [pong_snd_msg+msg_dope],0


        sub   eax,eax
        mov   dword ptr ds:[ps0+1],eax
        mov   dword ptr ds:[ps1+1],eax
        mov   dword ptr ds:[ps2+1],eax
        mov   dword ptr ds:[ps3+1],eax

        kd____disp  <13,10,10,'ipc_8    :  '>
        call  ping_short_100000


        mov   eax,offset pong_snd_msg
        mov   dword ptr ds:[ps0+1],eax
        mov   dword ptr ds:[ps1+1],eax
        mov   dword ptr ds:[ps2+1],eax
        mov   dword ptr ds:[ps3+1],eax


        kd____disp  <13,10,'ipc_12   :  '>
        mov   [ping_snd_msg+msg_dope],3 SHL md_mwords
        mov   [pong_snd_msg+msg_dope],3 SHL md_mwords
        call  ping_100000


        kd____disp  <13,10,'ipc_128  :  '>
        mov   [ping_snd_msg+msg_dope],32 SHL md_mwords
        mov   [pong_snd_msg+msg_dope],32 SHL md_mwords
        call  ping_100000

        kd____disp  <13,10,'ipc_512  :  '>
        mov   [ping_snd_msg+msg_dope],128 SHL md_mwords
        mov   [pong_snd_msg+msg_dope],128 SHL md_mwords
        call  ping_100000

;       kd____disp  <13,10,'ipc_1024 :  '>
;       mov   [ping_snd_msg+msg_dope],256 SHL md_mwords
;       mov   [pong_snd_msg+msg_dope],256 SHL md_mwords
;       call  ping_100000
;
;       kd____disp  <13,10,'ipc_2048 :  '>
;       mov   [ping_snd_msg+msg_dope],512 SHL md_mwords
;       mov   [pong_snd_msg+msg_dope],512 SHL md_mwords
;       call  ping_100000
;
;       kd____disp  <13,10,'ipc_4096 :  '>
;       mov   [ping_snd_msg+msg_dope],1024 SHL md_mwords
;       mov   [pong_snd_msg+msg_dope],1024 SHL md_mwords
;       call  ping_100000

        ke 'done'

        sti
        jmp $
        
        
;       ke 'pre_GB1'
;
;       mov   edx,GB1
;       sub   eax,eax
;       sub   ecx,ecx
;       mov   ebp,32*4+map_msg
;       mov   esi,sigma0_task
;       mov   edi,root_chief
;       int   ipc
;
;       ke 'GB1'





; mov   esi,sigma2_task
; mov   edi,root_chief
; mov   eax,edi
; sub   ebx,ebx
; sub   ebp,ebp
; ke 'xx'
; int   task_new
; ke '-yy'

        REPEAT
 OD






ping_short_100000:

  sub   ecx,ecx
  mov   eax,ecx
  mov   ebp,ecx
  mov   esi,[ping_dest_vec]
  mov   edi,[ping_dest_vec+4]
  int   ipc

  mov   [counter],100000

  call  get_rtc
  push  eax


  clign 16
  DO

        sub   ecx,ecx
        mov   eax,ecx
        mov   ebp,ecx
     ;; mov   esi,[ping_dest_vec]
     ;; mov   edi,[ping_dest_vec+4]
        mov   esi,pong_thread
        mov   edi,root_chief
        int   ipc
        test  al,al
        EXITNZ
        sub   ecx,ecx
        mov   eax,ecx
        mov   ebp,ecx
     ;; mov   esi,[ping_dest_vec]
     ;; mov   edi,[ping_dest_vec+4]
        mov   esi,pong_thread
        mov   edi,root_chief
        int   ipc
        test  al,al
        EXITNZ
        sub   ecx,ecx
        mov   eax,ecx
        mov   ebp,ecx
     ;; mov   esi,[ping_dest_vec]
     ;; mov   edi,[ping_dest_vec+4]
        mov   esi,pong_thread
        mov   edi,root_chief
        int   ipc
; push  eax
; push  edx
; rdtsc
; mov   [rdtsc_clocks],eax
; pop   edx
; pop   eax

; push  eax
; push  edx
; rdtsc
; sub   eax,[rdtsc_clocks]
; ke    'rdtsc'
; pop   edx
; pop   eax
        test  al,al
        EXITNZ
        sub   ecx,ecx
        mov   eax,ecx
        mov   edx,ecx
        mov   ebp,ecx
     ;; mov   esi,[ping_dest_vec]
     ;; mov   edi,[ping_dest_vec+4]
        mov   esi,pong_thread
        mov   edi,root_chief
        int   ipc
        test  al,al
        EXITNZ
        sub   [counter],4
        REPEATNZ
  OD
  test  al,al
  IFNZ
        ke    'ping_err'
  FI

  pop   ebx

  call  get_rtc

  sub   eax,ebx
  add   eax,rtc_micros_per_pulse/2
  sub   edx,edx
  mov   ebx,rtc_micros_per_pulse
  div   ebx
  mul   ebx
  mov   ebx,2*100000
  call  microseconds

  ret






ping_100000:

  mov   eax,offset ping_snd_msg
  sub   ecx,ecx
  mov   ebp,offset ping_rcv_msg
  mov   esi,[ping_dest_vec]
  mov   edi,[ping_dest_vec+4]
  int   ipc

  mov   [counter],100000

  call  get_rtc
  push  eax


  clign 16
  DO

        mov   eax,offset ping_snd_msg
        sub   ecx,ecx
        mov   ebp,offset ping_rcv_msg
        mov   esi,[ping_dest_vec]
        mov   edi,[ping_dest_vec+4]
        int   ipc
        test  al,al
        EXITNZ
        mov   eax,offset ping_snd_msg
        sub   ecx,ecx
        mov   ebp,offset ping_rcv_msg
        mov   esi,[ping_dest_vec]
        mov   edi,[ping_dest_vec+4]
        int   ipc
        test  al,al
        EXITNZ
        mov   eax,offset ping_snd_msg
        sub   ecx,ecx
        mov   ebp,offset ping_rcv_msg
        mov   esi,[ping_dest_vec]
        mov   edi,[ping_dest_vec+4]
        int   ipc
        test  al,al
        EXITNZ
        mov   eax,offset ping_snd_msg
        sub   ecx,ecx
        mov   ebp,offset ping_rcv_msg
        mov   esi,[ping_dest_vec]
        mov   edi,[ping_dest_vec+4]
        int   ipc
        test  al,al
        EXITNZ
        sub   [counter],4
        REPEATNZ
  OD
  test  al,al
  IFNZ
        ke    'ping_err'
  FI

  pop   ebx

  call  get_rtc

  sub   eax,ebx
  add   eax,rtc_micros_per_pulse/2
  sub   edx,edx
  mov   ebx,rtc_micros_per_pulse
  div   ebx
  mul   ebx
  mov   ebx,2*100000
  call  microseconds

  ret




;------------------------------------------------------
;
;       pong
;
;------------------------------------------------------


ktest1_start:

  mov   eax,1
  mov   ecx,offset ktest1_stack
  mov   edx,offset kktest1_start
  sub   ebx,ebx
  mov   ebp,ebx
  mov   esi,ebx
  mov   edi,ebx
  int   lthread_ex_regs

  DO
        sub   ebp,ebp
        lea   eax,[ebp-1]
        sub   esi,esi
        sub   edi,edi
        sub   ecx,ecx
        int   ipc
        REPEAT
  OD


 kktest1_start:

  mov   eax,-1

  clign 16
  DO
        mov   ebp,offset pong_rcv_msg+open_receive
        sub   ecx,ecx
        int   ipc
        test  al,al
        EXITNZ
        mov   ebp,offset pong_rcv_msg+open_receive
ps1:    mov   eax,0
        sub   ecx,ecx
        int   ipc
        test  al,al
        EXITNZ
        mov   ebp,offset pong_rcv_msg+open_receive
ps2:    mov   eax,0
        sub   ecx,ecx
        int   ipc
        test  al,al
        EXITNZ
        mov   ebp,offset pong_rcv_msg+open_receive
ps3:    mov   eax,0
        sub   ecx,ecx
        int   ipc
        test  al,al
ps0:    mov   eax,0
        REPEATZ
  OD
  ke    '-pong_err'






timer_counter        equ 40h
timer_control        equ 43h

counter0_mode0_16_cmd   equ 00110000b
counter0_mode2_16_cmd   equ 00110100b
counter0_mode3_16_cmd   equ 00110110b
counter0_latch_cmd      equ 00000000b




timer_start macro

mov al,counter0_mode0_16_cmd
out [timer_control],al
jmp $+2
jmp $+2
mov al,0FFh
out [timer_counter],al
jmp $+2
jmp $+2
out [timer_counter],al
endm



timer_stop macro

mov al,counter0_latch_cmd
out [timer_control],al
jmp $+2
jmp $+2
in  al,[timer_counter]
mov ah,al
jmp $+2
jmp $+2
in  al,[timer_counter]
xchg ah,al
neg  ax
movzx eax,ax
lea  eax,[eax+eax-1]
imul eax,(1000*1000/1193)/2
endm


        align 4


memptr  dd    2*MB1



pf_1024:

  mov   ebx,[memptr]
  lea   ecx,[ebx+128*pagesize]
  mov   [memptr],ecx

  mov   eax,[ebx]
  add   ebx,pagesize

  timer_start
  clign 16
  DO
        mov   eax,[ebx]
        add   ebx,pagesize
        cmp   ebx,ecx
        REPEATB
  OD
  timer_stop

  mov   ebx,127*1000
  call  microseconds

  ret

;  call  get_rtc
;  push  eax
;
;
;  mov   ebx,MB2
;  clign 16
;  DO
;        mov   eax,[ebx]
;        add   ebx,pagesize
;        cmp   ebx,MB2+512*pagesize
;        REPEATB
;  OD
;
;  pop   ebx
;
;  call  get_rtc
;
;  sub   eax,ebx
;  inc   eax
;  shr   eax,1
;  imul  eax,100000/512
;  call  microseconds
;
;  ret


  align 16



microseconds:

  pushad

  sub   edx,edx
  div   ebx
  kd____outdec
  mov   al,'.'
  kd____outchar
  imul  eax,edx,200
  add   eax,ebx
  shr   eax,1
  sub   edx,edx
  div   ebx
  mov   edx,eax
  IFB_  edx,10
        mov   al,'0'
        kd____outchar
  FI
  mov   eax,edx
  kd____outdec
  kd____disp  <' us'>

  popad
  ret






;----------------------------------------------------------------------------
;
;       get real time clock
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;       EAX    ms (low)
;
;----------------------------------------------------------------------------

        align 16


get_rtc:

  mov   eax,ds:[user_clock+1000h]
  ret





ktest_end     equ $


  scod  ends

  code  ends
  end