 

;------------------------------------------------------------------------ 
; 
;        Hazelnut segment selectors 
; 
;------------------------------------------------------------------------ 
 
first_kernel_sgm          equ   0008h

kernel_space_exec         equ   0008h
linear_kernel_space       equ   0010h

linear_space_exec         equ   (0018h+3)
linear_space              equ   (0020h+3)

phys_mem                  equ   0030h

cpu0_tss                  equ   0028h


