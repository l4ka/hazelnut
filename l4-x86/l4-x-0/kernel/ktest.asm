include l4pre.inc 

  scode
 
  Copyright IBM+UKA, L4.KTEST, 02,09,97, 15

;*********************************************************************
;******                                                         ******
;******         Kernel Test                                     ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 02.09.97    ******
;******                                                         ******
;*********************************************************************
 

 
  public ktest0_start
  public ktest1_start
  public ktest0_stack
  public ktest1_stack
  public ktest0_stack2
  public ktest1_stack2
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
include perfmon.inc




ok_for x86





  assume ds:codseg


ktest_begin   equ $


ping_thread   equ booter_thread
pong_thread   equ (sigma1_task+3*sizeof tcb)  
;pong_thread   equ (sigma1_thread+sizeof tcb)
;pong_thread   equ (booter_thread+sizeof tcb)


  align 16

              dd    31 dup (0)
ktest0_stack  dd    0
              dd    31 dup (0)
ktest0_stack2 dd    0
              dd    31 dup (0)
ktest1_stack  dd    0
              dd    31 dup (0)
ktest1_stack2 dd    0

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

			  dd    1,2,3,4
counter       dd 0

cycles	dd 0
public cycles	


large_space   db 0


                                                           



;------------------------------------------------------
;
;       ping
;
;------------------------------------------------------


              align 4

order         msg_vector  <0,4 SHL 13,4 SHL 13>
              dd          0,0,0,0


  assume ds:codseg


ktest0_start:

 
  mov   ecx,1000
  DO
        push  ecx
        sub   esi,esi
        int   thread_switch
        pop   ecx
        dec   ecx
        REPEATNZ
  OD


  
  
  

  sub   ecx,ecx
  mov   eax,ecx
  lea   edx,[ecx+1]
  mov   ebx,edx
  mov   ebp,1000h+(12 SHL 2)+map_msg
  mov   esi,sigma0_task
  int   ipc

; call  get_rtc
; push  eax
; mov   ecx,1000000
; DO
;       sub   edx,edx
;       mov   eax,12345678h
;       div   ecx
;       dec   ecx
;       REPEATNZ
; OD
; pop   ebx
; call  get_rtc
; sub   eax,ebx
; add   eax,rtc_micros_per_pulse/2
; sub   edx,edx
; mov   ebx,rtc_micros_per_pulse
; div   ebx
; mul   ebx
; mov   ebx,42000
; call  microseconds
; 
; ke    'ns per cycle'
;
;
;
;  mov   esi,MB1          ; map memory
;  mov   ecx,MB1/4+KB4
;  cld
;  rep   lodsd
;
;  DO
;        cli 
;
;  mov   esi,MB1           ; flush L1 + L2
;  mov   ecx,MB1/4
;  cld
;  rep   lodsd
;  mov   ecx,1000
;  loop  $
; 
;  mov   eax,[esi]
;  mov   ecx,1000
;  loop  $
;
;  rdtsc
;  mov   ebx,eax
;  
;  mov   esi,MB1
;  mov   ecx,1024
;  DO
;        mov   eax,[esi]
;        mov   eax,[esi+28]
;        add   esi,32
;        dec   ecx
;        REPEATNZ
;  OD
;  
;  rdtsc
;  sub   eax,ebx
;  sub   eax,12
;  add   eax,511
;  shr   eax,10
;  push  eax        
;
;
;  
;  rdtsc
;  mov   ebx,eax
;  rdtsc
;  sub   eax,ebx
;  mov   edi,eax
;  
;  rdtsc
;  mov   ebx,eax
;  mov   eax,[esi+64]
;  rdtsc
;  sub   eax,ebx
;  sub   eax,edi
;  mov   ecx,eax
;  
;  rdtsc
;  mov   ebx,eax
;  mov   eax,[esi+256]
;  mov   eax,[esi+256+32-8]
;  rdtsc
;  sub   eax,ebx
;  sub   eax,edi
;  
;  push  eax
;  push  ecx
;  
;  mov   esi,MB1+KB4        ; flush L1
;  mov   ecx,KB16/4
;  cld
;  rep   lodsd
;  mov   ecx,1000
;  loop  $
;  
;  mov   esi,MB1+MB1
; 
;  rdtsc
;  mov   ebx,eax
;  mov   eax,[esi+64]
;  rdtsc
;  sub   eax,ebx
;  sub   eax,edi
;  mov   ecx,eax
;  
;  rdtsc
;  mov   ebx,eax
;  mov   eax,[esi+256]
;  mov   eax,[esi+256+32-8]
;  rdtsc
;  sub   eax,ebx
;  sub   eax,edi
;  mov   ebx,eax
;  
;  kd____disp <13,10,'L1 cache delay: '>
;  mov   eax,ecx
;  kd____outdec
;  mov   al,'-'
;  kd____outchar
;  mov   eax,ebx
;  kd____outdec
;  kd____disp <' cycles'>
;  
;  kd____disp <13,10,'L2 cache delay: '>
;  pop   eax
;  kd____outdec
;  mov   al,'-'
;  kd____outchar
;  pop   eax
;  kd____outdec
;  kd____disp <' cycles',13,10>
;  
;  
;  kd____disp <13,10,'RAM cache-line read: '>
;  pop   eax
;  kd____outdec
;  kd____disp <' cycles',13,10>
;  
;  
;  
;    
;  
; 
;  
;  
;  ke    'cache'
;  
;  REPEAT
;  OD


  
  mov   eax,0AA00010h + ((3*64/4*2+64/4) SHL 8)
  mov   ebx,0FFFFFFFFh
  mov   esi,sigma0_task
  int   thread_schedule


  DO  
  
        call  enter_ktest


        mov   al,[large_space]
        kd____outhex8
        
        mov   eax,0AA00010h  + ((1*64/4*2+64/4) SHL 8)
        test  [large_space],01h
        IFNZ
              mov   ah,0   
        FI      
        mov   ebx,0FFFFFFFFh
        mov   esi,pong_thread
        int   thread_schedule

        mov   eax,0AA00010h  + ((2*64/4*2+64/4) SHL 8)
        test  [large_space],02h
        IFNZ
              mov   ah,0
        FI      
        mov   ebx,0FFFFFFFFh
        mov   esi,ping_thread
        int   thread_schedule
        
        inc   [large_space]
   

        kd____disp  <13,10,10,'PageFault:  '>
        call  pf_1024

        mov   [ping_dest_vec],pong_thread

        mov   [ping_snd_msg+msg_dope],0
        mov   [pong_snd_msg+msg_dope],0


        sub   eax,eax
        mov   dword ptr ds:[ps0+1],eax
        mov   dword ptr ds:[ps1+1],eax
        mov   dword ptr ds:[ps2+1],eax
        mov   dword ptr ds:[ps3+1],eax
        
        
    ;    mov   eax,2
    ;    sub   ecx,ecx
    ;    mov   edx,4711h
    ;    mov   ebx,1000h
    ;    cmp   byte ptr ds:[ebx],0
    ;    sub   ebp,ebp
    ;    mov   esi,pong_thread
    ;    int   ipc
    ;    ke    'translate 1'
    ;    
    ;    mov   eax,2
    ;    sub   ecx,ecx
    ;    mov   edx,4711h
    ;    mov   ebx,MB16
    ;    and   ebx,-pagesize
    ;    sub   ebp,ebp
    ;    mov   esi,pong_thread
    ;    int   ipc
    ;    ke    'translate 2'
    ;    
        
        
        
        

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

              
              

        call  exit_ktest
        

        ke    'done'
        
        IF    kernel_x2
              lno___prc eax
              test  eax,eax
             ; jz    $
              sti
        ENDIF
        
        
;       ke 'pre_GB1'
;
;       mov   edx,GB1
;       sub   eax,eax
;       sub   ecx,ecx
;       mov   ebp,32*4+map_msg
;       mov   esi,sigma0_task
;       int   ipc
;
;       ke 'GB1'





; mov   esi,sigma1_task
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

  mov   [cycles],0

  call  get_rtc
  push  eax
  
; P_count rw_tlb,cnt_event,ex_tlb,cnt_event
; P_count rw_miss,cnt_event,ex_miss,cnt_event
; P_count r_stall,cnt_event,w_stall,cnt_event
; P_count a_stall,cnt_event,x_stall,cnt_event
; P_count ncache_refs,cnt_event,r_stall,cnt_event
; P_count mem2pipe,cnt_event_PL0,bank_conf,cnt_event_PL0
; P_count instrs_ex,cnt_event_PL0,instrs_ex_V,cnt_event_PL0
;  P_count instrs_ex,cnt_clocks_pl0,instrs_ex,cnt_clocks_pl3
  rdtsc
  push  eax
   
  clign 16
  DO
        sub   ecx,ecx
        sub   eax,eax
        sub   ebp,ebp
      ; mov   esi,[ping_dest_vec]
      ; mov   edi,[ping_dest_vec+4]
        mov   esi,pong_thread
        int   ipc
        test  al,al
        EXITNZ
        sub   ecx,ecx
        sub   eax,eax
        sub   ebp,ebp
      ; mov   esi,[ping_dest_vec]
      ; mov   edi,[ping_dest_vec+4]
        mov   esi,pong_thread
        int   ipc
        test  al,al
        EXITNZ
        sub   ecx,ecx
        sub   eax,eax
        sub   ebp,ebp
      ; mov   esi,[ping_dest_vec]
      ; mov   edi,[ping_dest_vec+4]
        mov   esi,pong_thread
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
        sub   eax,eax
      ; mov   edx,ecx
        sub   ebp,ebp
      ; mov   esi,[ping_dest_vec]
      ; mov   edi,[ping_dest_vec+4]
        mov   esi,pong_thread
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

 ; P_count off
  rdtsc
  pop   ebx
  sub   eax,ebx
  
  pop   ebx
  
  push  eax

  call  get_rtc

  sub   eax,ebx
  add   eax,rtc_micros_per_pulse/2
  sub   edx,edx
  mov   ebx,rtc_micros_per_pulse
  div   ebx
  mul   ebx
  mov   ebx,2*100000
  call  microseconds


  pop   eax
  
  kd____disp  <'  cycles: '>
  sub   edx,edx
  mov   ebx,2*100000
  div   ebx
  kd____outdec
  
 ; kd____disp  <', events: ca='>  
 ; P_read P_event_counter0
 ; div   ebx
 ; kd____outdec     
 ;
 ; kd____disp  <',  cb='>  
 ; P_read P_event_counter1
 ; div   ebx
 ; kd____outdec     
   
  IFNZ	[cycles],0

	kd____disp <', spec: '>
	mov   eax,[cycles]
	sub   edx,edx
	div   ebx
	kd____outdec
	mov	al,'.'
	kd____outchar
	imul	eax,edx,200
	kd____outdec

	kd____disp <' cycles'>
  FI		
  
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


  mov   eax,3
  mov   ecx,offset ktest1_stack
  mov   edx,offset kktest1_start
  sub   ebx,ebx
  mov   ebp,ebx
  mov   esi,sigma0_task
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
 
  sub   ecx,ecx
  mov   eax,ecx
  lea   edx,[ecx+1]
  mov   ebx,edx
  mov   ebp,1000h+(12 SHL 2)+map_msg
  mov   esi,sigma0_task
  int   ipc
 
 
  mov   eax,-1

  ;------------------- for translate test only:
  ;      or    byte ptr ds:[MB4],0
  ;----------------------------------------      

  clign 16
 DO
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
  test  al,ipc_error_mask
  mov   al,0
  REPEATZ
  ke    '-pong_err'
  REPEAT
 OD






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


     


pf_1024:

  lno___prc eax
  test  al,al
  IFZ
        mov   ebx,2*MB1
  ELSE_
        mov   ebx,MB16
  FI            
  lea   ecx,[ebx+128*pagesize]

  mov   eax,[ebx]
  add   ebx,pagesize

  timer_start
  
; P_count rw_tlb,cnt_event,ex_tlb,cnt_event
; P_count rw_miss,cnt_event,ex_miss,cnt_event
; P_count r_stall,cnt_event,w_stall,cnt_event
; P_count a_stall,cnt_event,x_stall,cnt_event
; P_count ncache_refs,cnt_event,r_stall,cnt_event
; P_count mem2pipe,cnt_event_PL0,bank_conf,cnt_event_PL0
; P_count mem2pipe,cnt_event,bank_conf,cnt_event
; P_count instrs_ex,cnt_event_PL0,instrs_ex_V,cnt_event_PL0
; P_count instrs_ex,cnt_event_PL3,instrs_ex_V,cnt_event_PL3
; P_count instrs_ex,cnt_clocks_pl0,instrs_ex,cnt_clocks_pl3
;  P_count r_stall,cnt_event_PL3,w_stall,cnt_event_PL3
  rdtsc
  push  eax

  clign 16
  DO
        mov   eax,ebx
        mov   eax,[ebx]
        add   ebx,pagesize
        cmp   ebx,ecx
        REPEATB
  OD
  
 ; P_count off
  rdtsc
  pop   ebx
  sub   eax,ebx
  push  eax
  
  timer_stop

  mov   ebx,127*1000
  call  microseconds

  pop   eax
  
  kd____disp  <'  cycles: '>
  sub   edx,edx
  mov   ebx,127
  div   ebx
  kd____outdec
  
 ; kd____disp  <', events: ca='>  
 ; P_read P_event_counter0
 ; div   ebx
 ; kd____outdec     
 ;
 ; kd____disp  <',  cb='>  
 ; P_read P_event_counter1
 ; div   ebx
 ; kd____outdec     

  
  
  lno___prc eax
  test  al,al
  IFZ
        mov   eax,2*MB1
  ELSE_
        mov   eax,MB16
  FI            
  add   eax,21*4
  mov   ecx,80000002h
  int   fpage_unmap
  
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






;---------------------------------------------------------------------
;
;       enter/exit ktest mutex
;
;---------------------------------------------------------------------
; PRECONDITION:
;
;	      DS    phys mem
;
;---------------------------------------------------------------------



ktest_mutex   db 0FFh
ktest_depth   db 0



enter_ktest:

  push  eax
  push  ecx
    
  lno___prc ecx
  DO
        mov   al,0FFh
        lock  cmpxchg ds:[ktest_mutex],cl
        EXITZ
        cmp   al,cl
        REPEATNZ
  OD
  inc   ds:[ktest_depth]            
  
  pop   ecx
  pop   eax
  ret
  
  
exit_ktest:  
  
  dec   ds:[ktest_depth]
  IFZ
        mov   ds:[ktest_mutex],0FFh
  FI
  
  ret
        



ktest_end     equ $


  scod  ends

  code  ends
  end