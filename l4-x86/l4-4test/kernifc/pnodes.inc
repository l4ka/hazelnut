;****************************************************************************
;*****                                                                  *****
;*****                                                                  *****
;*****        pnode data structures                                     *****
;*****                                                                  *****
;*****                                                                  *****
;****************************************************************************



pnodes_per_frame    equ 16


pnode struc

  pte_ptr     dd 0
  succ_pnode  dd 0

pnode ends



  IF    kernel_type EQ i486
  
  
high4M_pages        equ 8
 
 
M4_pnode_base       equ offset pnode_space 
 

  ELSE



M4_pnode_base       equ offset pnode_space

max_M4_frames       equ 1024                   ; 4GB/4MB

  ENDIF