include l4pre.inc
                               
                                    

;*********************************************************************
;******                                                         ******
;******                LN BOOTER                                ******
;******                                                         ******
;******                               Author: Jochen Liedtke    ******
;******                                                         ******
;******                               created:  16.03.98        ******
;******                               modified: 14.04.98        ******
;******                                                         ******
;*********************************************************************
 


.nolist
include l4const.inc
include kpage.inc
.list



strt16 segment para public use16 'code'

  
  ;     MS DOS function calls
  
set_dta       equ   1Ah
open_file     equ   0Fh
read_seq      equ   14h
display_str   equ   09h
terminate     equ   00h


  ;     ELF   codes
  
executable_file     equ 2
em_386              equ 3

e_header      struc
  e_magic     dd    0,0,0,0
  e_type      dw    0
  e_machine   dw    0
              dd    0
  e_entry     dd    0
  e_phoff     dd    0
e_header      ends

p_header      struc
              dd    0
  p_offset    dd    0
  p_vaddr     dd    0
              dd    0
  p_filesz    dd    0
  p_memsz     dd    0
p_header      ends                
                
  
  
  
  

  org   100h


 assume ds:c16seg


start:

  mov   ax,cs
  mov   ds,ax
  mov   ss,ax
  mov   sp,offset stack
  
  
  mov   dx,offset ln_buffer
  mov   ah,set_dta
  int   21h
  
  mov   dx,offset ln_fcb
  mov   ah,open_file
  int   21h
  
  mov   dx,offset ln_open_failed
  test  al,al
  jnz   boot_error
  
  mov   ax,[ln_file_size]
  mov   [ln_buffer_len],ax

  mov   dx,offset ln_fcb
  mov   ah,read_seq
  int   21h
  
  mov   dx,offset ln_read_error
  test  al,al
  jnz   boot_error
  
  
  
  push  ds
  mov   ax,ds
  add   ax,KB64/16
  mov   ds,ax
  mov   dx,offset root_buffer-KB64
  mov   ah,set_dta
  int   21h
  pop   ds
  
  mov   dx,offset root_fcb
  mov   ah,open_file
  int   21h
  
  mov   dx,offset root_open_failed
  test  al,al
  jnz   boot_error
  
  mov   ax,[root_file_size]
  mov   [root_buffer_len],ax

  mov   dx,offset root_fcb
  mov   ah,read_seq
  int   21h
  
  mov   dx,offset root_read_error
  test  al,al
  jnz   boot_error
 
 
  mov   ax,ds
  add   ax,KB64/16
  mov   es,ax
  mov   bp,root_buffer-KB64
  
  CORNZ es:[bp+e_magic],464C457Fh              ; 7Fh,ELF
  CORNZ es:[bp+e_type],executable_file
  IFNZ  es:[bp+e_machine],em_386
        mov   dx,offset no_elf
        jmp   boot_error
  FI
  mov   ecx,es:[bp+e_entry]                    ; EBX   begin addr       
  add   bp,word ptr es:[bp+e_phoff]            ; ECX   start addr
  mov   edx,es:[bp+p_offset]                   ; EDX   offset in elf file
  mov   esi,es:[bp+p_filesz]
  mov   edi,es:[bp+p_memsz]
  mov   ebx,es:[bp+p_vaddr]
  
  pushad
  mov   ecx,edi
  sub   ecx,esi
  movzx edi,bp
  add   edi,esi
  add   edi,edx
  mov   ax,es
  movzx eax,ax
  shl   eax,4
  add   eax,edi
  mov   edi,eax
  and   edi,0Fh
  shr   eax,4
  mov   es,ax
  mov   al,0
  cld
  rep   stosb
  popad
  
  mov   di,offset ln_buffer+200h+1000h         ; kernel info page
  lea   eax,[ebx+edi]
  mov   [di+booter_ktask].ktask_stack,eax
  mov   [di+booter_ktask].ktask_start,ecx
  mov   [di+booter_ktask].ktask_begin,ebx
  mov   [di+booter_ktask].ktask_end,eax
  mov   [di+dedicated_mem1].mem_begin,ebx
  mov   [di+dedicated_mem1].mem_end,eax
  
  mov   eax,ds
  shl   eax,4
  add   eax,root_buffer
  add   eax,edx
  mov   [source_descr+2],ax
  shr   eax,16
  mov   byte ptr [source_descr+4],al
  mov   byte ptr [source_descr+7],ah
  
  mov   eax,ebx
  mov   [target_descr+2],ax
  shr   eax,16
  mov   byte ptr [target_descr+4],al
  mov   byte ptr [target_descr+7],ah
    
  push  ds
  pop   es
  mov   si,offset bios_gdt
  mov   cx,[root_file_size]
  shr   cx,1
  mov   ah,87h
  int   15h
  
  
  mov   dx,offset mov_failed
  jc    boot_error
  
 
    
    
  
  cli
  push  cs
  pop   ax
  mov   bx,offset ln_buffer+200h
  shr   bx,4
  add   ax,bx
  push  ax
  push  100h
  retf
  
  
boot_error:
  DO
        push  dx
        mov   bx,dx
        mov   dl,[bx]
        cmp   dl,'$'
        EXITZ
        mov   ah,6
        int   21h
        pop   dx
        inc   dx
        REPEAT
  OD
  pop   dx      

  mov   ah,terminate
  int   21h
  
              
              
ln_open_failed      db 'LN.EXE open failed$'              
ln_read_error       db 'LN.EXE read error$'
root_open_failed    db 'ROOT.ELF open failed$'
root_read_error     db 'ROOT.ELF read error$'
no_elf              db 'no executable elf$'
mov_failed          db 'mov failed$'
  
        align 4
        
ln_fcb              db 0
                    db 'LN      '
                    db 'EXE'  
                    dw 0
ln_buffer_len       dw 80h
ln_file_size        dw 0,0
                    db 0,0
                    db 0,0
                    dd 0,0
                    db 0
                    dd 0
        
        align 4
        
root_fcb            db 0
                    db 'ROOT    '
                    db 'ELF'  
                    dw 0
root_buffer_len     dw 80h
root_file_size      dw 0,0
                    db 0,0
                    db 0,0
                    dd 0,0
                    db 0
                    dd 0
                    
                    
        align 4
        
bios_gdt            dd    0,0

                    dd    0,0

source_descr        dw    0FFFFh
                    dw    0
                    db    0
                    db    0F3h
                    db    0Fh
                    db    0

target_descr        dw    0FFFFh
                    dw    0
                    db    0
                    db    0F3h
                    db    0Fh
                    db    0

                    dd    0,0
                    
                    dd    0,0
                    
                    
                                        
        align 4
              

              dw    128 dup (0)
stack         dw    0



        align 16
        
ln_buffer:    jmp   $        


root_buffer   equ   (offset ln_buffer+KB64)
                        
              
              

  strt16 ends
  code  ends
  end   start
