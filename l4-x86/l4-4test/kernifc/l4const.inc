;*********************************************************************
;******                                                         ******
;******         Controller Constants               (91-02-23)   ******
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
 
first_kernel_sgm          equ  0008h

cpu0_tss                  equ  0008h
phys_mem                  equ  0010h

linear_kernel_space_exec  equ  0018h
linear_kernel_space       equ  0020h

linear_space              equ  (0028h+3)
linear_space_exec         equ  (0030h+3)




   
;-------------------------------------------------------------------------
;
;       miscellaneous constants
;
;-------------------------------------------------------------------------
 
KB      equ     1024
MB      equ     (KB*KB)
GB      equ     (KB*MB)

KB1     equ      400h
KB2     equ      800h                                           ;Ž89-11-08 
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
 

i386            equ 3
i486            equ 4
pentium         equ 5
ppro            equ 6
amd_k6          equ 16
p4              equ i486
p5              equ pentium
p6              equ ppro
k6              equ amd_k6

pentium_x2	equ 200+pentium