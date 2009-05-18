
P_event_select      equ 11h

P_event_counter0    equ 12h
P_event_counter1    equ 13h


;P_event_sel_register record Pcounter:8,Pinv:1,Pen:1,Px:3,Pedge:1,Pos:1,Pusr:1,Punit:8,Pevent:8

P_event_sel_register record P_event1:16,P_event0:16



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

agi_stall     equ   011111b

pipline_flush equ   010101b

non_cache_rd  equ   011110b
ncache_refs   equ   011110b
locked_bus    equ   011100b

mem2pipe      equ   001001b
bank_conf     equ   001010b


instrs_ex     equ   010110b
instrs_ex_V   equ   010111b





cnt_nothing   equ   000b SHL 6
cnt_event_pl0 equ   001b SHL 6
cnt_event_pl3 equ   010b SHL 6
cnt_event     equ   011b SHL 6
cnt_clocks_pl0 equ  101b SHL 6
cnt_clocks_pl3 equ  110b SHL 6
cnt_clocks    equ   111b SHL 6


P_count macro event0,cmd0,event1,cmd1

  IFIDN <event0>,<off>
        e=0
  ELSE
        e=event0
        IFB   <cmd0>
              e=e+cnt_event
        ELSE
              e=e+cmd0
        ENDIF
        IFNB  <event1>
              e=e+(event1 SHL P_event1)
              IFB   <cmd1>
                    e=e+(cnt_event SHL P_event1)
              ELSE 
                    e=e+(cmd1 SHL P_event1)
              ENDIF
        ENDIF
  ENDIF        
              
  push  eax
  push  ecx
  push  edx
  mov   eax,e
  sub   ecx,ecx
  sub   edx,edx
  add   ecx,P_event_select
  wrmsr
  IFDIF <event0>,<off>
        sub   eax,eax
        add   ecx,P_event_counter0-P_event_select
        wrmsr
        add   ecx,P_event_counter1-P_event_counter0
        wrmsr
  ENDIF      
  pop   edx
  pop   ecx
  pop   eax
  
  endm
  
  
  
P_read macro counter  
  
  push  ecx
  mov   ecx,counter
  rdmsr
  pop   ecx
  
  endm
  
