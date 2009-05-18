 
;------------------------------------------------------------------------ 
; 
;        selectors 
; 
;------------------------------------------------------------------------ 
 
first_kernel_sgm  equ  0008h

linear_kernel_space equ 0008h

linear_space      equ  (0010h+3)
linear_space_exec equ  (0018h+3)

phys_mem_exec     equ  0020h
phys_mem          equ  0028h

cpu0_tss          equ  0030h

  sampling_space  equ  (0038h+3)


   
