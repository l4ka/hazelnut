;****************************************************************************
;*****                                                                  *****
;*****                                                                  *****
;*****        pnode data structures                                     *****
;*****                                                                  *****
;*****                                                                  *****
;****************************************************************************



pnodes_per_frame_default    equ 16


pnode struc

  pte_ptr     dd 0
  child_pnode dd 0
  succ_pnode  dd 0
  pred_pnode  dd 0

pnode ends


root_pnode struc

              dd 0
              dd 0
  cache0      dd 0
  cache1      dd 0

root_pnode ends


free_pnode struc

  next_free   dd 0
              dd 0
              dd 0
              dd 0

free_pnode ends




log2_size_pnode     equ 4

.errnz  ((1 SHL log2_size_pnode) - sizeof pnode)

.errnz  (offset succ_pnode - offset cache0)
.errnz  (offset pred_pnode - offset cache1)


.errnz  offset pnode_space AND (pnode_space_size-1)





M4_pnode_base       equ offset pnode_space

max_M4_frames       equ 1024                   ; 4GB/4MB

pnode_base          equ (offset pnode_space + max_M4_frames * sizeof pnode)
