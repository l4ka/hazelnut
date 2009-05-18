;
;  Constructs a gate descriptor for x86:
;
;<ds> 31-24:Base 23:G 22:D/B 21:0 20:AVL 19-16:SGM Limit 15:P 14-13:DPL 12:S 11-8:Type 7-0:Base 
;<ds> 31-16:Base 15-0:SGM Limit

;-------------------------------------------------------------------------
;
;        descriptor types
;
;-------------------------------------------------------------------------
 
 
r32               equ  0100000000010000b
rw32              equ  0100000000010010b
r16               equ             10000b
rw16              equ             10010b
 
x32               equ  0100000000011000b
xr32              equ  0100000000011010b
x16               equ             11000b
xr16              equ             11010b
 
ldtseg            equ  2
taskgate          equ  5
tsseg             equ  9
callgate          equ  0Ch
intrgate          equ  0Eh
trapgate          equ  0Fh
 
 
;---------------------------------------------------------------------------
;
;        descriptor privilege levels codes
;
;---------------------------------------------------------------------------
 
dpl0    equ (0 shl 5)
dpl1    equ (1 shl 5)
dpl2    equ (2 shl 5)
dpl3    equ (3 shl 5)



;----------------------------------------------------------------------------
;
;       descriptor entry
;
;----------------------------------------------------------------------------


descriptor macro dtype,dpl,dbase,dsize


IF dsize eq 0

  dw    0FFFFh
  dw    lowword        dbase
  db    low highword   dbase
  db    low            (dtype+dpl+80h)
  db    high           (dtype+8000h) + 0Fh
  db    high highword  dbase

ELSE
IF dsize AND -KB4

  dw    lowword        ((dsize SHR 12)-1)
  dw    lowword        dbase
  db    low highword   dbase
  db    low            (dtype+dpl+80h)
  db    high           ((dtype+8000h) + highword ((dsize SHR 12)-1))
  db    high highword  dbase

ELSE

  dw    lowword        (dsize-1)
  dw    lowword        dbase
  db    low highword   dbase
  db    low            (dtype+dpl+80h)
  db    high           dtype
  db    high highword  dbase

ENDIF
ENDIF

  endm
