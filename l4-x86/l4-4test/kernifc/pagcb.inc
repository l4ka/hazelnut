

pagman_control_block struc

                          db offset pag_cb dup (?)
 
  linktab_base            dd 0
  linktab_base_PM         dd 0
  free_pnode_root         dd 0
  do_fpage_counter        dd 0

  first_free_fresh_frame  dd 0
  free_fresh_frames       dd 0 
  
  IF kernel_type NE i486
        
        small_space_size_DIV_MB4        db 0
        log2_small_space_size_DIV_MB4   db 0
        
  ENDIF      


pagman_control_block ends 