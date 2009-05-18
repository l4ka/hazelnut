 
rtc_pulses_per_second    equ 512
rtc_millis_per_pulse     equ 2
rtc_thousand_div_millis  equ 500
rtc_micros_per_pulse     equ 2000

 
 

;----------------------------------------------------------------------------
;
;     reset timer intr condition  (exported to dispatch)
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;     EAX   scratch
;
;----------------------------------------------------------------------------


reset_rtc_timer_intr macro

  mov   al,0Ch                   ; rtc reg C
  out   70h,al                   ; rtc address
  jmp   $+2
  jmp   $+2
  jmp   $+2
  jmp   $+2
  in    al,71h                   ; rtc data

  mov   al,60h                   ; seoi_rtc
  out   0A0h,al                  ; pic2_ocw2
  mov   al,1011b                 ; read_isr
  out   0A0h,al
  jmp   $+2
  jmp   $+2
  in    al,0A0h
  test  al,al
  IFZ
        mov   al,62h             ; seoi master
        out   20h,al             ; pic1_ocw2
  FI

  mov   al,0Ch
  out   70h,al
  jmp   $+2
  jmp   $+2
  jmp   $+2
  jmp   $+2
  in    al,71h
  test  al,0C0h
; IFNZ
;       ke  'RTC_dead'
; FI

  endm
