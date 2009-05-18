

;----------------------------------------------------------------------------
;
;       ipc macros
;
;----------------------------------------------------------------------------


never = -1

max_fpage = 31 SHL 2


snd_timeout = never
spf_timeout = never
rcv_timeout = never
rpf_timeout = never

iiic=-1
iiia=-1
iiib=-1


ipc___pre macro
iiic=-1
iiia=-1
iiib=-1
endm



encode_timeout macro v
IF &v eq never
 iiim=0
 iiie=0
ELSE
IF &v eq 0
 iiim=0
 iiie=1
ELSE
 iiim=&v
 iiie=10
 IF iiim ge 100h*10000h
  iiie=iiie-8
  iiim=iiim/10000h
 ENDIF
 IF iiim ge 100h*100h
  iiie=iiie-4
  iiim=iiim/100h
 ENDIF
 IF iiim ge 100h*10h
  iiie=iiie-2
  iiim=iiim/10h
 ENDIF
 IF iiim ge 100h*4
  iiie=iiie-1
  iiim=iiim/4
 ENDIF
ENDIF
ENDIF
endm

ipc___ld_timeout macro
 iiic=0
 encode_timeout %snd_timeout
 iiic=iiic+(iiim SHL 16)+(iiie SHL 4)
 encode_timeout %rcv_timeout
 iiic=iiic OR (iiim SHL 24)+iiie
 encode_timeout %spf_timeout
 iiic=iiic+(iiie SHL 12)
 encode_timeout %rpf_timeout
 iiic=iiic+(iiie SHL 8)
 IF iiic eq 0
  xor ecx,ecx
 ELSE
  mov ecx,iiic
 ENDIF
endm


ipc___ldc macro reg,v

 IF v eq 0
  xor reg,reg
 ELSE
 IF iiic ge 0 and v-iiic le 127 and -v+iiic le 128
  lea reg,[ecx+v-iiic]
 ELSE
 IF iiia ge 0 and v-iiia le 126 and -v+iiia le 129
  lea reg,[eax+v-iiia+1]
 ELSE
 IF iiib ge 0 and v-iiib le 126 and -v+iiib le 129
  lea reg,[ebp+v-iiib+1]
 ELSE
 mov reg,v
 ENDIF
 ENDIF
 ENDIF
 ENDIF
 endm


ipc___no_snd macro
 ipc___ldc eax,-1
 iiia=0
 endm



ipc___ld_snd macro mw0,mw1,msg
 IFDIF <mw0>,<edx>
  mov edx,mw0
 ENDIF
 IFDIF <mw1>,<ebx>
  mov ebx,mw1
 ENDIF
 IFB <msg>
  xor eax,eax
  iiia=1
 ELSE
 IFIDN <msg>,<map_msg>
  ipc___ldc eax,map_msg
  iiia=map_msg+1
 ELSE
 IFDIF <msg>,<eax>
  mov eax,msg
  iiia=-1
 ENDIF
 ENDIF
 ENDIF
 endm


ipc___no_rcv macro
 ipc___ldc ebp,-1
 iiib=0
 endm


ipc___ld_rcv macro msg
 IFB <msg>
  xor ebp,ebp
  iiib=1
 ELSE
 IFIDN <msg>,<max_fpage+map_msg>
  ipc___ldc ebp,max_fpage+map_msg
  iiib=max_fpage+map_msg+1
 ELSE
  mov ebp,msg
  iiib=-1
 ENDIF
 ENDIF
 endm


ipc___ld_thread macro dest
 IFDIF <dest>,<esi>
  mov esi,dest
  mov edi,dest.4
 ENDIF
 endm


ipc___ld_intr macro intr
 ipc___ldc esi,intr
 xor edi,edi
 endm



ipc___exec macro
 int ipc
snd_timeout=never
spf_timeout=never
rcv_timeout=never
rpf_timeout=never
iiic=-1
iiia=-1
iiib=-1
 endm



ipc___call macro dest,sw0,sw1,smsg,rmsg
 ipc___pre
 ipc___ld_timeout
 ipc___ld_snd sw0,sw1,<smsg>
 ipc___ld_rcv <rmsg>
 ipc___ld_thread dest
 ipc___exec
 endm


ipc___sndwt macro dest,sw0,sw1,smsg,rmsg
 ipc___pre
 ipc___ld_timeout
 ipc___ld_snd sw0,sw1,<smsg>
 ipc___ld_rcv <rmsg+open_receive>
 ipc___ld_thread dest
 ipc___exec
 endm


ipc___send macro dest,sw0,sw1,smsg
 ipc___pre
 ipc___ld_timeout
 ipc___ld_snd sw0,sw1,<smsg>
 ipc___no_rcv
 ipc___ld_thread dest
 ipc___exec
 endm


ipc___receive macro source,rmsg
 ipc___pre
 ipc___ld_rcv <rmsg>
 ipc___ld_timeout
 ipc___no_snd
 ipc___ld_thread source
 ipc___exec
 endm


ipc___wait macro rmsg
 ipc___pre
 ipc___ld_rcv <rmsg+open_receive>
 ipc___ld_timeout
 ipc___no_snd
 ipc___exec
 endm


ipc___receive_intr macro intr
 ipc___pre
 ipc___ld_rcv
 ipc___no_snd
 ipc___ld_timeout
 ipc___ld_intr intr+1
 ipc___exec
 endm









sndmsg macro dwords,strings
 d=0
 s=0
 IF dwords GT 2
  d=dwords
 ENDIF
 IFNB <strings>
  s=strings
 ENDIF
 dd 0
 dd (d SHL md_mwords)+(s SHL md_strings)
 dd (d SHL md_mwords)+(s SHL md_strings)
 endm


rcvmsg macro dwords,strings,fpage
 IFB <fpage>
  dd 0
 ELSE
  dd fpage
 ENDIF
 d=0
 s=0
 IF dwords GT 2
  d=dwords
 ENDIF
 IFNB <strings>
  s=strings
 ENDIF
 dd (d SHL md_mwords)+(s SHL md_strings)
 dd 0
 endm



sr_msg macro sdwords,sstrings,rdwords,rstrings,rfpage
 IFB <rfpage>
  dd 0
 ELSE
  dd rfpage
 ENDIF
 d=0
 s=0
 IF rdwords GT 2
  d=rdwords
 ENDIF
 IFNB <rstrings>
  s=rstrings
 ENDIF
 dd (d SHL md_mwords)+(s SHL md_strings)
 d=0
 s=0
 IF sdwords GT 2
  d=sdwords
 ENDIF
 IFNB <rstrings>
  s=sstrings
 ENDIF
 dd (d SHL md_mwords)+(s SHL md_strings)
 endm


msg_vec1 struc

                    dd 0   ; msg_rcv_fpage
                    dd 0   ; msg_size_dope
                    dd 0   ; msg_dope
                    dd 0   ; msg_w0
                    dd 0   ; msg_w1
                    dd 0   ; msg_w2
  msg_w3            dd 0
  msg_w4            dd 0
  msg_w5            dd 0
  msg_w6            dd 0
  msg_w7            dd 0

msg_vec1 ends





;---------------------------------------------------------------------------------------
;
;       miscellaneous macros
;
;---------------------------------------------------------------------------------------


thrd__self macro

  sub   esi,esi
  sub   edi,edi
  int   id_nearest
  
  endm   
  

thrd__switch macro dest

  IFB   <dest>
        sub   esi,esi
        int   thread_switch
  ELSE
        mov   esi,&dest
        int   thread_switch
  ENDIF
  
  endm              
  
  
  