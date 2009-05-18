;*********************************************************************
;******                                                         ******
;******         Controller Constants                            ******
;******                                                         ******
;*********************************************************************
   
;----------------------------------------------------------------------------
;
;                 System Flags
;
;----------------------------------------------------------------------------
           
c_flag            equ 0
z_flag            equ 6
s_flag            equ 7
t_flag            equ 8
i_flag            equ 9
d_flag            equ 10
o_flag            equ 11
nt_flag           equ 14
r_flag            equ 16
vm_flag           equ 17
ac_flag           equ 18
vif_flag          equ 19
vip_flag          equ 20
id_flag           equ 21

iopl_field        equ 12
    
         
 
;------------------------------------------------------------------------ 
; 
;        selectors 
; 
;------------------------------------------------------------------------ 
 
first_kernel_sgm  equ  0008h

kernel_exec       equ  0008h
linear_kernel_space equ 0010h

linear_space      equ  (0018h+3)
linear_space_exec equ  (0020h+3)

phys_mem          equ  0028h

cpu0_tss          equ  0030h


   
;-------------------------------------------------------------------------
;
;       miscellaneous constants
;
;-------------------------------------------------------------------------
 
KB      equ     1024
MB      equ     (KB*KB)
GB      equ     (KB*MB)

KB1     equ      400h
KB2     equ      800h                                           
KB4     equ     1000h
KB16    equ     4000h
KB32    equ     8000h
KB64    equ    10000h
KB128   equ    20000h
KB256   equ    40000h
KB512   equ    80000h
MB1     equ   100000h
MB2     equ   200000h
MB4     equ   400000h
MB8     equ   800000h
MB16    equ  1000000h
MB32    equ  2000000h
MB64    equ  4000000h
GB1     equ 40000000h
 


;-------------------------------------------------------------------------
;
;       processor types
;
;-------------------------------------------------------------------------
 

i386            equ 30h
i486            equ 40h
x86             equ 50h
pentium         equ 51h
ppro            equ 60h
pII             equ 62h
pIII            equ 63h

p5_family       equ 5
p6_family       equ 6

other_family    equ 0
    