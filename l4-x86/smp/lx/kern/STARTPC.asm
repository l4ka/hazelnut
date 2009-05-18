include l4pre.inc


                                    

  Copyright IBM, L4.START.PC, 26,06,98, 47 
 
;*********************************************************************
;******                                                         ******
;******                LN START.PC                              ******
;******                                                         ******
;******                                                         ******
;******                                   Jochen Liedtke        ******
;******                                                         ******
;******                                   modified: 26.06.98    ******
;******                                                         ******
;*********************************************************************
 
  public init_basic_hw_interrupts
  public init_rtc_timer
  public wait_for_one_second_tick
  public mask_hw_interrupt
  public reset
  public memory_failure
  public irq0
  public irq15
  public irq0_intr
  public irq8_intr
  public physical_kernel_info_page

  extrn  kernel_start:near
  extrn  kernelver:abs
  extrn  rtc_timer_int:near
  extrn  init_intr_control_block:near
  extrn  init_sgmctr:near
  extrn  max_kernel_end:near

  extrn  exception:near
  extrn  define_idt_gate:near
  


.nolist
include l4const.inc
include uid.inc
include adrspace.inc
include tcb.inc
include intrifc.inc
include syscalls.inc
include kpage.inc
include cpucb.inc
.list

include l4kd.inc


ok_for x86


 

;----------------------------------------------------------------------------
;
;       start jump   and jump at 100h
;
;----------------------------------------------------------------------------
;
;       Start.pc *MUST* ensure that it occupies position
;       0 ... 100X ( 0 < X < 10h ) so that the following
;       table is placed at location 1010h (becaus 16-byte align).
;
;----------------------------------------------------------------------------


start_offset  equ 1000h      ; preserves ROM BIOS area



physical_kernel_info_page equ start_offset




  strtseg


  
  
  dd 0               ; kernel length (dwords)
  dd 0               ; checksum
  dd kernelver

  org   100h


start100:

  jmp   start+2


  org   start_offset-4


bootstack     dd 0






start:

  nop                                ; to permit real mode and PM mode
  nop                                ; (in PM, will jump to start_start+2

  jmp         start_start+2          ; 32-bit jmp, as well executable as 16-bit jmp !!     


  nop
  
                                     ; start seg here ends at address 0x1008 . This is
                                     ; inportant for proper link table begin in start.asm!          
  strt ends




  align 4



;----------------------------------------------------------------------------
;
;      Port Addresses
;
;----------------------------------------------------------------------------


sys_port_a                    equ 92h
sys_port_b                    equ 61h

paritychk_signal_bit equ 7
iochk_disable_bit    equ 3
iochk_signal_bit     equ 6



kb_status          equ  64h
kb_cntl            equ  64h
kb_data            equ  60h

rtc_address        equ  70h
rtc_data           equ  71h
rtc_seconds        equ  00h
rtc_minutes        equ  02h
rtc_hour           equ  04h
rtc_day            equ  07h
rtc_month          equ  08h
rtc_year           equ  09h
rtc_reg_a          equ  0Ah
rtc_reg_b          equ  0Bh
rtc_reg_c          equ  0Ch
rtc_century        equ  32h
rtc_century_ps2    equ  37h


pic1_icw1          equ 20h
pic1_icw2          equ 21h
pic1_icw3          equ 21h
pic1_icw4          equ 21h
pic1_isr_irr       equ 20h
pic1_imr           equ 21h

pic1_ocw1          equ 21h
pic1_ocw2          equ 20h
pic1_ocw3          equ 20h

pic2_icw1          equ 0A0h
pic2_icw2          equ 0A1h
pic2_icw3          equ 0A1h
pic2_icw4          equ 0A1h

pic2_ocw1          equ 0A1h
pic2_ocw2          equ 0A0h
pic2_ocw3          equ 0A0h
pic2_isr_irr       equ 0A0h
pic2_imr           equ 0A1h


seoi               equ 60h

read_irr           equ 1010b
read_isr           equ 1011b


drive_control      equ 3F2h


irq0          equ 0h
irq15         equ 0Fh

irq0_intr     equ 20h
irq7_intr     equ 27h
irq8_intr     equ 28h
irq15_intr    equ 2Fh



seoi_master   equ (seoi + 2)
seoi_rtc      equ (seoi + 8 - 8)
seoi_co287    equ (seoi +13 - 8)

;C01 ms       rtc macros moved up, for use in nmi enabling/disabling
; from here to end here to this place moved 
 

inrtc macro rtcport

mov al,rtcport
out rtc_address,al
jmp $+2
jmp $+2
jmp $+2
jmp $+2
jmp $+2
jmp $+2
in  al,rtc_data
endm


outrt macro rtcport

push eax
mov al,rtcport
out rtc_address,al
jmp $+2
jmp $+2
jmp $+2
jmp $+2
jmp $+2
jmp $+2
pop eax
out rtc_data,al
endm

; end here




;-------------------------------------------------------------------------
;
;      memory
;
;-------------------------------------------------------------------------





        align 4





        icode16

multiboot_info_area struc

  mbi_flags         dd 0
  mbi_mem_low       dd 0
  mbi_mem_high      dd 0

multiboot_info_area ends



emulated_info_area  multiboot_info_area <1,0,0>


  align 16

initial_gdt           dd 0,0

initial_gdt_descr     dw 47h              ; gdt
initial_gdt_base_low  dw initial_gdt
initial_gdt_base_high db 0
                      db 92h
                      db 0
                      db 0

initial_idt_descr     dw 9*8-1            ; idt
initial_idt_base_low  dw initial_idt
initial_idt_base_high db 0
                      db 92h
                      db 0
                      db 0

initial_ds_descr      dw 0FFFFh           ; ds
                      dw 0
                      db 0
                      db 092h
                      db 0CFh
                      db 0

                      dw 0FFFFh           ; es
                      dw 0
                      db 0
                      db 092h
                      db 0CFh
                      db 0

                      dw 0FFFFh          ; ss
initial_ss_base_low   dw 0
initial_ss_base_high  db 0
                      db 092h
                      db 0CFh
                      db 0

initial_cs_descr      dw 0FFFFh           ; cs
initial_cs_base_low   dw 0
initial_cs_base_high  db 0
                      db 09Ah
                      db 0CFh
                      db 0

                      dd 0,0

initial_tss_descr     dw 67h
                      dw 0
                      db 0
                      db 89h
                      dw 0
                      

initial_ds    equ (offset initial_ds_descr-offset initial_gdt)
initial_cs    equ (offset initial_cs_descr-offset initial_gdt)
initial_tss   equ (offset initial_tss_descr-offset initial_gdt)



initial_idt         dw lowword offset ke_,6*8,8E00h,0     ; DIV0
                    dw lowword offset ke_,6*8,8E00h,0     ; DB
                    dw lowword offset ke_,6*8,8E00h,0     ; NMI
                    dw lowword offset ke_,6*8,8E00h,0     ; INT 3
                    dw lowword offset ke_,6*8,8E00h,0     ; OV
                    dw lowword offset ke_,6*8,8E00h,0     ; BD
                    dw lowword offset ke_,6*8,8E00h,0     ; UD
                    dw lowword offset ke_,6*8,8E00h,0     ; NA
                    dw lowword offset ke_,6*8,8E00h,0     ; DF


        ic16 ends



;---------------------------------------------------------------------
;
;                  LN-Start
;
; precondition:
;
;      real mode or 32-bit protected mode
;
;---------------------------------------------------------------------





  icode




start_start:

  nop                                  ; to permit real mode and PM mode
  nop                                  ; (in PM, will jump to start_start+2)           

  cli

  ;<cd> if started in real mode
  mov   ecx,cr0
  test  cl,01
  IFZ
                                       ; executes in 16-bit mode !
	;<cd> jump to real mode start
        osp                                           
        mov   eax,offset real_mode_start
        asp
        jmp   eax
        
  FI
  ;<cd> else jump to protected mode start
  jmp   protected_mode_start
  
  
  
  
  
  icod ends
  
  
  
  icode16
  
  
  assume ds:c16seg


real_mode_start:

  ;<cd> load cs = ds = es = 0..10000h
  mov   ax,cs
  mov   ds,ax
  mov   ss,ax
  mov   esp,offset bootstack

  

;----------------------------------------------------------------------------
;
;       initializations depending on hardware type
;
;----------------------------------------------------------------------------

  ;<cd> switch of ps/2 watchdog
  mov   ax,0C300h                        ; switch off PS/2 watchdog
  int   15h                              ;




                
;----------------------------------------------------------------------------
;
;       determine memory configuration
;
;----------------------------------------------------------------------------

  ;<cd> read lower memory configuration from bios
  int   12h                              ;    area 1    (0...640K)
  movzx eax,ax
  mov   [emulated_info_area].mbi_mem_low,eax

  ;<cd> read upper memory configuration from bios
  mov   ah,88h                           ;    area 2    (1MB...)
  int   15h
  movzx eax,ax
  IFAE  eax,63*MB1/KB1
        mov   eax,63*MB1/KB1
  FI      
  mov   [emulated_info_area].mbi_mem_high,eax




;----------------------------------------------------------------------------
;
;       switch to protected mode
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       DS = SS = CS
;
;----------------------------------------------------------------------------


  sub   eax,eax
  mov   ax,ss
  shl   eax,4
  mov   ebx,eax
  shr   ebx,16

  ;<cd> setup protected mode gdt	
  add   [initial_gdt_base_low],ax
  adc   [initial_gdt_base_high],bl

  ;<cd> setup protected mode idt		
  add   [initial_idt_base_low],ax
  adc   [initial_idt_base_high],bl

  ;<cd> setup protected mode ss		
  mov   [initial_ss_base_low],ax
  mov   [initial_ss_base_high],bl

  ;<cd> setup protected mode cs		
  mov   [initial_cs_base_low],ax
  mov   [initial_cs_base_high],bl

  ;<cd> load data semgment with 0
  sub   ax,ax
  mov   ds,ax
  ;<cd> load extra segment to code segment
  mov   ax,cs
  mov   es,ax
  ;<cd> load offset of initial gdt
  mov   si,offset initial_gdt
  ;<cd> load irq0, 8 handlers
  mov   bh,irq0_intr
  mov   bl,irq8_intr
  mov   ah,89h
  ;<cd> push far jump address of protected mode from real mode on stack 
  push  0
  push  cs
  push  lowword offset protected_mode_from_real_mode
  ;<cd> jump to pushed address
  jmp   dword ptr ds:[15h*4]





  ic16 ends




  icode
  
  assume ds:codseg
                    
                    
                    

protected_mode_from_real_mode:
  
  ;<cd> disable interrupts
  cli
	
  ;<cd> load stack pointer
  mov   esp,offset bootstack

  ;<cd> reset nested task flag
  pushfd
  btr   dword ptr ss:[esp],nt_flag
  popfd

  ;<cd> load initial code segment base
  mov   ecx,dword ptr ss:[initial_cs_base_low]
  and   ecx,00FFFFFFh

  ;<cd> align initial code segment base 
  sub   eax,eax
  mov   ss:[initial_cs_base_low],ax
  mov   ss:[initial_cs_base_high],al

  ;<cd> push eflags + far return address for iretd on stack
  pushfd
  push  cs
  lea   eax,[ecx+offset protected_mode_0_based_cs]
  push  eax
  iretd


protected_mode_0_based_cs:

  ;<cd> load ss, es with ds
  mov   edx,ds
  mov   ss,edx
  mov   es,edx

  ;<cd> load multiboot magic word, address of emulated info area
  mov   eax,2BADB002h
  lea   ebx,[ecx+emulated_info_area]




;----------------------------------------------------------------------------
;
;       PROTECTED MODE START
;
;----------------------------------------------------------------------------
; PRECONDITION: 
;
;
;       EAX   2BADB002h   (multiboot magic word)
;       EBX   pointing to boot info area:
;
;       <EBX+0>     flags (flags[0] = 1)
;       <EBX+4>     mem_lower (in K)
;       <EBX+8>     mem_upper (in K)
;
;       CS          0...4GB, 32-bit exec, CPL=0
;       DS,SS,ES    0...4GB, read_write
;
;       protected mode enabled
;       paging disabled
;       interrupts disabled
;
;----------------------------------------------------------------------------


protected_mode_start:

  ;<cd> hang if bap multiboot magic word
  DO
        cmp   eax,2BADB002h
        REPEATNZ
  OD

  ;<cd> hang if first flag of the mbi_flags is not set
  mov   ecx,[ebx].mbi_flags
  DO
        test  cl,01
        REPEATZ
  OD
  
  ;<cd> load stack pointer
  lea   esp,[ebx+4]
  call  current_eip
  current_eip:                           ; physical load address -> ecx
  pop   edx                              ;
  sub   edx,offset current_eip           ;

  ;<cd> load memory begin low, memory end into physical kernel info page
  mov   eax,[ebx].mbi_mem_low
  shl   eax,10
  and   eax,-pagesize
  mov   [edx+dedicated_mem0+physical_kernel_info_page].mem_begin,eax
  mov   [edx+dedicated_mem0+physical_kernel_info_page].mem_end,MB1

  ;<cd> load memory begin high, memory end into physical kernel info page  	
  mov   eax,[ebx].mbi_mem_high
  shl   eax,10
  add   eax,MB1
  and   eax,-pagesize
  mov   [edx+main_mem+physical_kernel_info_page].mem_begin,0
  mov   [edx+main_mem+physical_kernel_info_page].mem_end,eax

  ;<cd> load start ebx in physical kernel info page
  mov   [edx+start_ebx+physical_kernel_info_page],ebx

  ;<cd> load aliased boot memory area in physical kernel info page
  mov   [edx+aliased_boot_mem+physical_kernel_info_page].mem_begin,offset start_offset
  mov   [edx+aliased_boot_mem+physical_kernel_info_page].mem_end,offset max_kernel_end
  mov   [edx+alias_base+physical_kernel_info_page],edx
  IFB_  edx,<offset max_kernel_end>
        mov   [edx+aliased_boot_mem+physical_kernel_info_page].mem_end,edx
        mov   [edx+alias_base+physical_kernel_info_page],offset max_kernel_end
  FI
  


;----------------------------------------------------------------------------
;
;       relocate to abs 800h
;
;----------------------------------------------------------------------------
;
;       ensures CS=0,   offset addr = real addr
;
;
;       Remark: If LN kernel is loaded by DOS, INT 13h vector will be
;               reassigned by DOS. So the relocation must not happen
;               before real_mode_init_hard_disk and real_mode_init_floppy_
;               disk, because the relocation overwrites the DOS area.
;               The BIOS area (400h...4FFh) however is not damaged.
;
;----------------------------------------------------------------------------


  ;<cd> wait for hd and floppy turn of because int 13 location will be overwritten
  inrtc 80h                                            ;C01 ms




  ;<cd> copy relocation code to 800h
  mov   edi,start_offset-(continue_after_relocation-relocate)
  lea   esi,[edx+relocate]
  mov   ecx,offset continue_after_relocation-relocate
  cld   
  rep   movsb


  mov   edi,start_offset
  lea   esi,[edi+edx]
  mov   ecx,offset max_kernel_end-start_offset
  shr   ecx,2

  ;<cd> jump to relocate 
  mov   eax,start_offset-(continue_after_relocation-relocate)
  jmp   eax
  
  
relocate:
  ;<cd> mov kernel to start_offset
  DO
        mov   eax,[esi]
        xchg  [edi],eax
        mov   [esi],eax
        add   esi,4
        add   edi,4
        dec   ecx
        REPEATNZ
  OD      

  mov   eax,offset continue_after_relocation
  jmp   eax


;  mov   edi,start_offset
;  lea   esi,[edi+ecx]
;  mov   ecx,offset continue_after_relocation-start_offset
;  shr   ecx,2
;  DO
;        mov   eax,[esi]
;        xchg  [edi],eax
;        mov   [esi],eax
;        add   esi,4
;        add   edi,4
;        dec   ecx
;        REPEATNZ
;  OD      
;
;  mov   eax,offset reloc2
;  jmp   eax
;
;reloc2:
;
;  mov   ecx,offset max_kernel_end
;  sub   ecx,offset continue_after_relocation-start_offset
;  shr   ecx,2
;
;  DO
;        mov   eax,[esi]
;        xchg  [edi],eax
;        mov   [esi],eax
;        add   esi,4
;        add   edi,4
;        dec   ecx
;        REPEATNZ
;  OD      
;
;  jmp   $+2         ; flush prefetch que, because next code parts just moved
;                    ; to position 'continue_after_relocation'
;  align 4


continue_after_relocation:

  ;<cd> load initial gate descriptor table
  mov   eax,offset initial_gdt
  mov   ds:[initial_gdt_base_low],ax
  shr   eax,16
  mov   ds:[initial_gdt_base_high],al
  osp
  lgdt  fword ptr ds:[initial_gdt_descr]

  ;<cd> load initial interrupt descriptor table
  mov   eax,offset initial_idt
  mov   ds:[initial_idt_base_low],ax
  shr   eax,16
  mov   ds:[initial_idt_base_high],al
  osp
  lidt  fword ptr ds:[initial_idt_descr]

  ;<cd> load all segments to initial values
  mov   eax,initial_ds
  mov   ds,eax
  mov   es,eax
  mov   fs,eax
  mov   gs,eax
  mov   ss,eax
  ;<cd> load stack pointer
  mov   esp,offset bootstack

  ;<cd> load inital tss
  mov   eax,initial_tss
  ltr   ax

  ;<cd> jump far (reload code segment)
  jmpf32 cs_loaded,initial_cs


cs_loaded:


;----------------------------------------------------------------------------
;
;       prepare for shutdown desaster
;
;----------------------------------------------------------------------------

  ;<cd> define shutdown handler
  call  define_shutdown_handler


;----------------------------------------------------------------------------
;
;       inhibit hardware interrupts (even when STI)
;
;----------------------------------------------------------------------------
; Remark: Inhibiting the hardware interrupts independent from the processors
;         interrupt enable flag is necessary, because STI may happen before
;         all hardware drivers are installed.
;
;----------------------------------------------------------------------------
  ;<cd> inhibit hardware interrupts by masking them
  mov  al,11111111b                            ; set IMRs
  out  pic1_imr,al                             ;
  mov  al,11111111b                            ;
  out  pic2_imr,al                             ;


;----------------------------------------------------------------------------
;
;       deselect diskette and turn off motor
;
;----------------------------------------------------------------------------
  ;<cd> turn of disk motor
  mov   al,0
  mov   dx,drive_control
  out   dx,al




;----------------------------------------------------------------------------
;
;       start LN
;
;----------------------------------------------------------------------------


  ;<cd> start kernel
  jmp   kernel_start




;----------------------------------------------------------------------------
;
;       LN BIOS initialization
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       protected mode
;
;       LN kernel initialized
;
;----------------------------------------------------------------------------

  assume ds:codseg



init_basic_hw_interrupts:

  pushad
  ;<cd> enable NMIs
  inrtc 0                    ; enable NMI   !!!

  ;<cd> define 8259 base
  call  define_8259_base
  ;<cd> initialise interrupt catcher
  lno___prc eax
  IFZ eax, 1	
	call  init_interrupt_catcher	; bootstrap only
  FI

  ;<cd> initialise NMI handler
  call  init_nmi

  ;<cd> reserver irq 0 and 8 for kernel
  mov   eax,(1 SHL 0)+(1 SHL 8)    ; reserve irq 0 + 8 for kernel
  ;<cd> initialise interrupt control block
  call  init_intr_control_block

  popad
  ret


  icod  ends


                                                                ;...Ž


;----------------------------------------------------------------------------
;
;       NMI handling
;
;----------------------------------------------------------------------------


  icode


init_nmi:

  lno___prc eax
  IFZ eax,1	
	;<cd> define IDT gate for memory failure
	mov   bl,nmi
	mov   bh,0 SHL 5
	mov   eax,offset memory_failure
	call  define_idt_gate
  FI

  ;<cd> enable NMIs
  inrtc 0

  ;<cd> jump to memory failure if parity check signal bit or io check signal bit is turned on
  in    al,sys_port_b
  test  al,(1 SHL paritychk_signal_bit)+(1 SHL iochk_signal_bit)
  jnz   memory_failure

  ;<cd> disable io checking 
  and   al,NOT (1 SHL iochk_disable_bit)
  out   sys_port_b,al

  ret


  icod  ends




memory_failure:

  ipre  0
  ;<cd> enter kernel debugger if a memory failure happend
  ke    '-NMI'
  





;---------------------------------------------------------------------------
;
;      define interrupt base of 8259 interrupt controllers
;
;---------------------------------------------------------------------------

  icode


define_8259_base:

  push eax
  pushfd
  cli
	
  ;<cd> setup interrupt controller 8259
  mov   al,11h
  out   pic1_icw1,al                           ;
  mov   al,irq0_intr                           ;
  out   pic1_icw2,al                           ;
  mov   al,04h                                 ;
  out   pic1_icw3,al                           ;
  mov   al,11h                                 ; important difference to AT:
  out   pic1_icw4,al                           ; special fully nested mode !!
  mov   al,0C1h                                ; prio := 8..15,3..7,0,1
  out   pic1_ocw2,al                           ;
                                               ; KB must have low prio because
                                               ; intr may arrive during paging
                                               ; of KB driver process !!
  mov   al,11h
  out   pic2_icw1,al
  mov   al,irq8_intr
  out   pic2_icw2,al
  mov   al,02h
  out   pic2_icw3,al
  mov   al,11h
  out   pic2_icw4,al
  mov   al,0C7h
  IF    kernel_x2
        mov   al,0C5h		; xxxxxxxxxxxxxxxx
  ENDIF      
  out   pic2_ocw2,al


  mov  al,11111011b                            ; set IMRs
  out  pic1_imr,al                             ;
  mov  al,11111111b                            ;
  out  pic2_imr,al                             ;

  mov   al,60h
  out   pic1_ocw2,al

  popfd
  pop  eax
  ret


  icod  ends



;----------------------------------------------------------------------------
;
;       mask interrupt
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ECX   intr no
;
;----------------------------------------------------------------------------


mask_hw_interrupt:

  pushfd
  cli
  push  eax

  ;<cd> if interrupt is hardware interrupt
  IFB_   ecx,16

	;<cd> mask hardware interrupt
        in    al,pic2_ocw1
        mov   ah,al
        in    al,pic1_ocw1

        bts   eax,ecx

        out   pic1_ocw1,al
        mov   al,ah
        out   pic2_ocw1,al
  FI

  pop   eax
  popfd                                    ; Rem: change of NT impossible
  ret



;----------------------------------------------------------------------------
;
;       lost interrupt catcher (IRQ 7 and ISR bit 7 = 0)
;
;----------------------------------------------------------------------------

  icode


init_interrupt_catcher:

  lno___prc eax
  IFZ eax,1
	;<cd> define IDT gate for lost interrupt catcher
	mov   bl,irq7_intr
	mov   bh,0 SHL 5
	mov   eax,offset lost_interrupt_catcher
	call  define_idt_gate
  FI
  ret


  icod  ends


lost_interrupt_catcher:                  ; in the moment hardware IRQ 7
                                         ; is disabled
  iretd








;----------------------------------------------------------------------------
;
;       rtc timer
;
;----------------------------------------------------------------------------

  assume ds:codseg


  icode


init_rtc_timer:
  ;<cd> define idt gate for rtc timer interrupt
  lno___prc eax
  IFZ eax,1
	mov   bl,irq8_intr
	mov   bh,0 SHL 5
	mov   eax,offset rtc_timer_int
	call  define_idt_gate
  FI

  ;<cd> wait for rtc to tick
  DO
        inrtc rtc_reg_a
        bt    eax,7
        REPEATC
  OD
  ;<cd> set clock to 512 Hz
  and   al,0F0h
  add   al,7                 ; set to 512 Hz
  outrt rtc_reg_a

  ;<cd> set rtc_reg_b
  inrtc rtc_reg_b
  or    al,01001000b
  outrt rtc_reg_b

  ;<cd> reset timer intr line
  inrtc rtc_reg_c           ; reset timer intr line            

  ;<cd> enable timer interrupt
  in    al,pic2_imr
  and   al,11111110b
  out   pic2_imr,al
  
  ret


  icod  ends



;----------------------------------------------------------------------------
;
;       End Of System Run
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       ECX   0 : reset , no reboot
;           <>0 : reset and reboot
;
;       DS    phys mem
;
;       PL0 !
;
;----------------------------------------------------------------------------

  assume ds:codseg

;#######################################################

reset:

  mov   edx,ecx

  cli
  ;<cd> disable NMIs
  inrtc 80h                              ; disable NMI

  ;<cd> disable all breakpoints
  sub   eax,eax
  mov   dr7,eax                          ; disable all breakpoints

  ;<cd> if paging enabled
  mov   eax,cr0                          ; IF paging already on
  bt    eax,31                           ;
  IFC                                    ;
	;<cd> load data segment with kernel space
        mov   ebx,linear_kernel_space    ;
        mov   ds,ebx                     ;
  FI                                     ;


  ;<cd> hang if edx = 0
  test  edx,edx                          ; no reboot if edx = 0
  jz    $                                ;


                                         ; REBOOT:
  ;<cd> load eax with 0 / PM
  sub   eax,eax                          ;
  IFA   esp,<virtual_space_size>         ;
        mov   eax,PM                     ;
  FI                                     ;
  ;<cd> inhibit memory test at reboot
  mov   word ptr ds:[eax+472h],1234h     ; inhibit memory test at reboot
  DO                                     ;
	;<cd> wait 2nd bit set in keyboard status
       in   al,kb_status                 ;
       test al,10b                       ;
       REPEATNZ                          ;
  OD                                     ;
                                         ;
  ;<cd> cmos shutdown with boot loader request
  mov   al,0                             ; cmos: shutdown with boot loader req
  outrt 8Fh                              ; NMI disabled
                                         ;
  ;<cd> reset pulse command
  mov  al,0FEh                           ; reset pulse command
  out  kb_cntl,al                        ;
                                         ;
  ;<cd> hang and wait for reboot
  jmp  $                                 ;

  align 4

end_of_reset_routine:




;----------------------------------------------------------------------------
;
;       wait for one second tick
;
;----------------------------------------------------------------------------

  icode
  
  

wait_for_one_second_tick:

  push  eax

  ; Select status register A from RTC
  ;<cd> set tc to one second ticks
  mov 	al,rtc_reg_a
  out 	rtc_address,al
  jmp 	$+2
  jmp 	$+2
  jmp 	$+2
  jmp 	$+2
  jmp 	$+2
  jmp 	$+2

  ; Wait for UIP (update in progress) flag to disappear

  ;<cd> wait for update in progress flag to be set (1 tick)
  DO
	in  al,rtc_data
	and al,80h	
        REPEATNZ
  OD

  ; Wait for UIP flag to appear again. This indicates
  ; that a second has passed
	
  ;<cd> wait for next update in progress flag to be set (1 tick)
  DO
	in  al,rtc_data
	and al,80h
        REPEATZ
  OD

  pop   eax
  ret
 
   
  icod ends          
        


;----------------------------------------------------------------------------
;
;       shutdown desaster
;
;
;       called if 386 CPU shutdown occurrs
;
;----------------------------------------------------------------------------


  icode


define_shutdown_handler:

  ret



;  push  eax
;
;  mov   dword ptr ds:[467h],offset shutdown_desaster   ; cs = 0 !
;
;  mov   al,5
;  outrt 0Fh
;
;  pop   eax
;  ret

  icod  ends



;  code16
;
;
;shutdown_desaster:
;
;  DO
;        sub   ax,ax
;        mov   ds,ax
;        mov   ss,ax
;        mov   esp,offset bootstack
;
;        mov   di,0B000h
;        mov   es,di
;        mov   di,0
;        mov   al,'S'
;        mov   ah,0Fh
;        mov   es:[di],ax
;        mov   es:[di+8000h],ax
;
;        mov   [initial_gdt_base_low],offset initial_gdt
;        mov   [initial_gdt_base_high],0
;        mov   [initial_idt_base_low],offset initial_idt
;        mov   [initial_idt_base_high],0
;        sub   ax,ax
;        mov   [initial_ss_base_low],ax
;        mov   [initial_ss_base_high],al
;        mov   [initial_cs_base_low],ax
;        mov   [initial_cs_base_high],al
;        mov   es,ax
;        mov   si,offset initial_gdt
;        mov   bh,irq0_intr
;        mov   bl,irq8_intr
;        mov   ah,89h
;        push  0
;        push  cs
;        push  offset protected_mode_desaster
;        jmp   dword ptr ds:[15h*4]
;
;        c16 ends
;
;
;
;protected_mode_desaster:
;
;        DO
;              ke    'desaster'
;              REPEAT
;        OD
;
;;        int   19h
;;        mov   di,0B000h
;;        mov   es,di
;;        mov   di,2
;;        mov   al,'S'
;;        mov   ah,0Fh
;;        mov   es:[di],ax
;;        mov   es:[di+8000h],ax
;;        REPEAT
;  OD
;






;----------------------------------------------------------------------------
;
;       ke_         provisional INT 3 entry before intctr initialized
;
;----------------------------------------------------------------------------

  icode


ke_:

  ipre  trap1,no_ds_load

  mov   al,3

  jmp   cs:[kdebug_exception+physical_kernel_info_page]


  icod  ends






  code  ends
  end   start100


















