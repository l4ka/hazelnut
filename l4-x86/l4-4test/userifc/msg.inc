;----------------------------------------------------------------------------
;
;             message vector
;
;----------------------------------------------------------------------------


msg_vector struc

  msg_rcv_fpage     dd 0
  msg_size_dope     dd 0
  msg_dope          dd 0

msg_vector ends


xmsg_vector struc

                    dd 0,0,0
  msg_w0            dd 0
  msg_w1            dd 0
  msg_w2            dd 0
  msg_w3            dd 0

xmsg_vector ends



msg_dope_bits record  md_mwords:19,md_strings:5,md_cc:8



string_vector struc

  str_len     dd 0
  str_addr    dd 0
  buf_size    dd 0
  buf_addr    dd 0

string_vector ends

string_length_bits  record sl_free:10,string_length:22


max_message_string_length     equ MB4


.errnz  (1 SHL (width string_length)) - max_message_string_length
.erre   (1 SHL (width md_mwords))*4+sizeof msg_vector LE max_message_string_length



fpage_vector struc

  snd_base    dd 0
  snd_fpage   dd 0

fpage_vector ends


fpage         record fpage_base:24,fpage_size:6,fpopn_write:1,fpopn_grant:1



fpage_map                 equ 0
fpage_map_read_only       equ fpage_map
fpage_map_read_write      equ (fpage_map + mask fpopn_write)
fpage_grant               equ mask fpopn_grant


.errnz        fpage_grant AND fpage_map_read_write
.errnz        fpage_grant AND fpage_map_read_only
.errnz        fpage_map_read_write AND fpage_map_read_only



;----------------------------------------------------------------------------
;
;       completion codes
;
;----------------------------------------------------------------------------


msgccbyte record ccec:4,cca:1,ccfree:1,ccm:1,ccd:1


deceit              equ mask ccd
map_msg             equ mask ccm
mem_msg             equ mask cca




ipc_ok                         equ 00h   SHL ccec

ipc_not_existent_or_illegal    equ 01h   SHL ccec

ipc_s                          equ  1    SHL ccec

ipc_timeout                    equ 02h   SHL ccec
ipc_cancelled                  equ 04h   SHL ccec
ipc_map_failed                 equ 06h   SHL ccec
ipc_snd_pf_timeout             equ 08h   SHL ccec
ipc_rcv_pf_timeout             equ 0Ah   SHL ccec
ipc_aborted                    equ 0Ch   SHL ccec
ipc_cut                        equ 0Eh   SHL ccec


ipc_error_mask      equ mask ccec

ipc_control_mask    equ (deceit+map_msg+mem_msg+mask ccfree)




