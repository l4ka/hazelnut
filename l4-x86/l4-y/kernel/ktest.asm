include l4pre.inc 
 
  scode
 
  Copyright IBM+UKA, L4.KTEST, 03,04,00, 16

;*********************************************************************
;******                                                         ******
;******         Kernel Test                                     ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                                         ******
;******                                   modified: 03.04.00    ******
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




ok_for x86,pIII





  assume ds:codseg


ktest_begin   equ $


ping_thread   equ booter_thread
pong_thread   equ (sigma1_task+2*sizeof tcb)  
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
              
ping_snd_msg  dd    0,0,0,128 dup (0)
              align 16
ping_rcv_msg  dd    0,128 SHL md_mwords,0,128 dup (0)

              
pong_snd_msg  dd    0,0,0,128 dup (0)
              align 16
pong_rcv_msg  dd    0,128 SHL md_mwords,0,128 dup (0)

			dd    1,2,3,4
counter       dd 0

cycles	dd 0
public cycles	


large_space   db 0



kernel_info_page    equ 1000h                                                           



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
  ipc___syscall



  
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
        

        kd____disp  <13,10,10,'ipc_8    :  '>
        call  ping_short_1000


        mov   eax,offset pong_snd_msg
        mov   dword ptr ds:[ps0+1],eax
        mov   dword ptr ds:[ps1+1],eax
        mov   dword ptr ds:[ps2+1],eax
        mov   dword ptr ds:[ps3+1],eax


        kd____disp  <13,10,'ipc_16   :  '>
        mov   eax,4 SHL md_mwords
        call  ping_1000
     
        kd____disp  <13,10,'ipc_128  :  '>
        mov   eax,32 SHL md_mwords
        call  ping_1000

        kd____disp  <13,10,'ipc_512  :  '>
        mov   eax,128 SHL md_mwords
        call  ping_1000

        kd____disp  <13,10,'ipc__4K  :  '>

        mov   [ping_snd_msg+msg_w3].str_len,KB4
        mov   [ping_snd_msg+msg_w3].str_addr,MB2
        mov   [ping_rcv_msg+msg_w3].buf_size,KB4
        mov   [ping_rcv_msg+msg_w3].buf_addr,MB2

        mov   [pong_snd_msg+msg_w3].str_len,KB4
        mov   [pong_snd_msg+msg_w3].str_addr,MB2+KB64
        mov   [pong_rcv_msg+msg_w3].buf_size,KB4
        mov   [pong_rcv_msg+msg_w3].buf_addr,MB2+KB64

        mov   eax,(2 SHL md_mwords) + (1 SHL md_strings)
        call  ping_1000


        kd____disp  <13,10,'ipc_rd4K :  '>

        mov   [ping_snd_msg+msg_w3].str_len,KB4
        mov   [ping_snd_msg+msg_w3].str_addr,MB2
        mov   [ping_rcv_msg+msg_w3].buf_size,0
        mov   [ping_rcv_msg+msg_w3].buf_addr,0

        mov   [pong_snd_msg+msg_w3].str_len,0
        mov   [pong_snd_msg+msg_w3].str_addr,0
        mov   [pong_rcv_msg+msg_w3].buf_size,KB4
        mov   [pong_rcv_msg+msg_w3].buf_addr,MB2+KB64

        mov   eax,(2 SHL md_mwords) + (1 SHL md_strings)
        call  read_16


   if 0

        kd____disp  <13,10,'ipc_16K  :  '>

        mov   [ping_snd_msg+msg_w3].str_len,KB16
        mov   [ping_snd_msg+msg_w3].str_addr,MB2
        mov   [ping_rcv_msg+msg_w3].buf_size,KB16
        mov   [ping_rcv_msg+msg_w3].buf_addr,MB2

        mov   [pong_snd_msg+msg_w3].str_len,KB16
        mov   [pong_snd_msg+msg_w3].str_addr,MB2+KB64
        mov   [pong_rcv_msg+msg_w3].buf_size,KB16
        mov   [pong_rcv_msg+msg_w3].buf_addr,MB2+KB64

        mov   eax,(2 SHL md_mwords) + (1 SHL md_strings)
        call  ping_1000


        kd____disp  <13,10,'ipc_64K  :  '>

        mov   [ping_snd_msg+msg_w3].str_len,KB64
        mov   [ping_snd_msg+msg_w3].str_addr,MB2
        mov   [ping_rcv_msg+msg_w3].buf_size,KB64
        mov   [ping_rcv_msg+msg_w3].buf_addr,MB2

        mov   [pong_snd_msg+msg_w3].str_len,KB64
        mov   [pong_snd_msg+msg_w3].str_addr,MB2+KB64
        mov   [pong_rcv_msg+msg_w3].buf_size,KB64
        mov   [pong_rcv_msg+msg_w3].buf_addr,MB2+KB64

        mov   eax,(2 SHL md_mwords) + (1 SHL md_strings)
        call  ping_1000


     endif
        call  exit_ktest


        mov   eax,2
        cpuid

        ke    'done'

        
        IF    kernel_x2
              lno___prc eax
              test  eax,eax
             ; jz    $
              sti
        ENDIF
        
        
        REPEAT
 OD






ping_short_1000:

  sub   ecx,ecx
  mov   eax,ecx
  mov   ebp,ecx
  mov   esi,[ping_dest_vec]
  mov   edi,[ping_dest_vec+4]
  ipc___syscall

  mov   [counter],1000

  mov   [cycles],0

  rdtsc
  push  eax
  clign 16
   
  DO
        sub   ecx,ecx
        sub   eax,eax
        sub   ebp,ebp
        mov   esi,pong_thread
        ipc___syscall
        test  al,al
        EXITNZ
        sub   ecx,ecx
        sub   eax,eax
        sub   ebp,ebp
        mov   esi,pong_thread
        ipc___syscall
        test  al,al
        EXITNZ
        sub   ecx,ecx
        sub   eax,eax
        sub   ebp,ebp
        mov   esi,pong_thread
        ipc___syscall
        test  al,al
        EXITNZ
        sub   ecx,ecx
        sub   eax,eax
        sub   ebp,ebp
        mov   esi,pong_thread
        ipc___syscall
        test  al,al
        EXITNZ
        sub   [counter],4
        REPEATNZ
  OD
  test  al,al
  IFNZ
        ke    'ping_err'
  FI

  rdtsc
  pop   ebx
  sub   eax,ebx
  mov   ebx,2*1000
  
  call  display_time_and_cycles

  ret




display_time_and_cycles:
  
  sub   edx,edx
  div   ebx

  push  eax

  mov   ebx,1000000000
  mul   ebx
  mov   ebx,ds:[kernel_info_page].cpu_clock_freq
  div   ebx

  sub   edx,edx
  mov   ebx,1000
  div   ebx
  IFB_  eax,100
        kd____disp <' '>
  FI
  IFB_  eax,10
        kd____disp <' '>
  FI
  kd____outdec
  mov   al,'.'
  kd____outchar
  mov   eax,edx
  sub   edx,edx
  mov   ebx,10
  div   ebx
  IFB_  eax,10
        kd____disp <'0'>
  FI
  kd____outdec
  kd____disp  <' us,    '>

  pop   eax

  IFB_  eax,100000
        kd____disp <' '>
  FI
  IFB_  eax,10000
        kd____disp <' '>
  FI
  IFB_  eax,1000
        kd____disp <' '>
  FI
  IFB_  eax,100
        kd____disp <' '>
  FI
  IFB_  eax,10
        kd____disp <' '>
  FI
  kd____outdec
  kd____disp  <' cycles'>
  
  
  ret






ping_1000:

  mov   [ping_snd_msg+msg_dope],eax
  mov   [ping_snd_msg+msg_size_dope],eax
  mov   [ping_rcv_msg+msg_dope],eax
  mov   [ping_rcv_msg+msg_size_dope],eax
  mov   [pong_snd_msg+msg_dope],eax
  mov   [pong_snd_msg+msg_size_dope],eax
  mov   [pong_rcv_msg+msg_dope],eax
  mov   [pong_rcv_msg+msg_size_dope],eax


  mov   eax,offset ping_snd_msg
  sub   ecx,ecx
  mov   ebp,offset ping_rcv_msg
  mov   esi,[ping_dest_vec]
  mov   edi,[ping_dest_vec+4]
  ipc___syscall

  mov   [counter],1000

  rdtsc
  push  eax
  clign 16

  DO

        mov   eax,offset ping_snd_msg
        sub   ecx,ecx
        mov   ebp,offset ping_rcv_msg
        mov   esi,[ping_dest_vec]
        mov   edi,[ping_dest_vec+4]
        ipc___syscall
        test  al,al
        EXITNZ long
        mov   eax,offset ping_snd_msg
        sub   ecx,ecx
        mov   ebp,offset ping_rcv_msg
        mov   esi,[ping_dest_vec]
        mov   edi,[ping_dest_vec+4]
        ipc___syscall
        test  al,al
        EXITNZ
        mov   eax,offset ping_snd_msg
        sub   ecx,ecx
        mov   ebp,offset ping_rcv_msg
        mov   esi,[ping_dest_vec]
        mov   edi,[ping_dest_vec+4]
        ipc___syscall
        test  al,al
        EXITNZ
        mov   eax,offset ping_snd_msg
        sub   ecx,ecx
        mov   ebp,offset ping_rcv_msg
        mov   esi,[ping_dest_vec]
        mov   edi,[ping_dest_vec+4]
        ipc___syscall
        test  al,al
        EXITNZ
        sub   [counter],4
        REPEATNZ
  OD
  test  al,al
  IFNZ
        ke    'ping_err'
  FI

  rdtsc
  pop   ebx
  sub   eax,ebx
  mov   ebx,2*1000

  call  display_time_and_cycles

  ret








read_16:

  mov   ebx,[ping_snd_msg+msg_w3].str_addr
  mov   ecx,16
  DO
        mov   edx,[ebx]
        add   ebx,KB4
        dec   ecx
        REPEATNZ
  OD
  

  mov   [ping_snd_msg+msg_dope],eax
  mov   [ping_snd_msg+msg_size_dope],eax
  mov   [ping_rcv_msg+msg_dope],eax
  mov   [ping_rcv_msg+msg_size_dope],0
  mov   [pong_rcv_msg+msg_dope],eax
  mov   [pong_rcv_msg+msg_size_dope],eax

  sub   eax,eax
  mov   dword ptr ds:[ps0+1],eax
  mov   dword ptr ds:[ps1+1],eax
  mov   dword ptr ds:[ps2+1],eax
  mov   dword ptr ds:[ps3+1],eax

  mov   eax,offset ping_snd_msg
  sub   ecx,ecx
  mov   ebp,offset ping_rcv_msg
  mov   esi,[ping_dest_vec]
  mov   edi,[ping_dest_vec+4]
  ipc___syscall

  mov   [counter],16

  rdtsc
  push  eax
  clign 16

  DO

        mov   eax,offset ping_snd_msg
        sub   ecx,ecx
        mov   ebp,offset ping_rcv_msg
        mov   edx,MB2+100
        mov   esi,[ping_dest_vec]
        mov   edi,[ping_dest_vec+4]
        prefetch edx
        add   edx,32
        prefetch edx
        ipc___syscall
        test  al,al
        EXITNZ long
        add   [ping_snd_msg+msg_w3].str_addr,KB4
        sub   [counter],1
        REPEATNZ
  OD
  test  al,al
  IFNZ
        ke    'ping_err'
  FI

  rdtsc
  pop   ebx
  sub   eax,ebx
  mov   ebx,16

  call  display_time_and_cycles

  mov   eax,offset pong_snd_msg
  mov   dword ptr ds:[ps0+1],eax
  mov   dword ptr ds:[ps1+1],eax
  mov   dword ptr ds:[ps2+1],eax
  mov   dword ptr ds:[ps3+1],eax

  ret




;------------------------------------------------------
;
;       pong
;
;------------------------------------------------------


ktest1_start:



  if    pong_thread ne sigma1_task

        mov   eax,(pong_thread-sigma1_task)/(sizeof tcb)
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
              ipc___syscall
              REPEAT
        OD

  endif


 kktest1_start:
 
  sub   ecx,ecx
  mov   eax,ecx
  lea   edx,[ecx+1]
  mov   ebx,edx
  mov   ebp,1000h+(12 SHL 2)+map_msg
  mov   esi,sigma0_task
  ipc___syscall
 
 
  mov   eax,-1


 DO
  clign 16
  DO
        mov   ebp,offset pong_rcv_msg+open_receive
        sub   ecx,ecx
        ipc___syscall
        test  al,al
        EXITNZ
        mov   ebp,offset pong_rcv_msg+open_receive
ps1:    mov   eax,0
        sub   ecx,ecx
        ipc___syscall
        test  al,al
        EXITNZ
        mov   ebp,offset pong_rcv_msg+open_receive
ps2:    mov   eax,0
        sub   ecx,ecx
        ipc___syscall
        test  al,al
        EXITNZ
        mov   ebp,offset pong_rcv_msg+open_receive
ps3:    mov   eax,0
        sub   ecx,ecx
        ipc___syscall
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







          align 16


     


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

  rdtsc
  push  eax

  DO
        mov   eax,ebx
        mov   eax,[ebx]
        add   ebx,pagesize
        cmp   ebx,ecx
        REPEATB
  OD
  
  rdtsc
  pop   ebx
  sub   eax,ebx
  mov   ebx,127

  call  display_time_and_cycles
  
  
  
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