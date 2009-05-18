.386p
 NAME kernel
 PAGE 60, 132
 TITLE MODULE kernel module
 
 
 PUBLIC kernelver
 PUBLIC kernelstring
 PUBLIC kcod_start
 PUBLIC cod_start
 PUBLIC dcod_start
 PUBLIC scod_start
 PUBLIC max_kernel_end 
 PUBLIC labseg_start 
 PUBLIC first_lab 
 PUBLIC icod_start

strt16 segment para public use16 'code'    ; only to ensure that 16-bit offset are
strt16 ends                                ; calculated relative to 0 by masm

strt segment para public use32 'code'
strt ends 

labseg segment byte public use32 'code'
labseg_start:


kernelver EQU 21000
kerneltxt EQU '24.08.99'

dver macro ver 
db '&ver&'
endm
 
kernelstring:
 db 'L4-X Nucleus (x86),   Copyright (C) IBM 1997 & University of Karlsruhe 1999',13,10,'Version ' 
 dver %kernelver 
 db ', ',kerneltxt 
 db 0

first_lab:

labseg ends


c16 segment para public use16 'code'
c16_start:
c16 ends


kcod segment para public use32 'code'
kcod_start:
kcod ends


code segment  para public use32 'code'
cod_start: 
 code ends 

dcod segment para public use32 'code'
dcod_start:
dcod ends

scod segment para public use32 'code'
scod_start:
scod ends

icod segment para public use32 'code'
icod_start: 
icod ends
ic16 segment para public use16 'code'
ic16 ends
 
lastseg segment para public use32 'code'
max_kernel_end: 
lastseg ends
 end
 





