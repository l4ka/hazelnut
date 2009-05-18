

pagman_control_block struc

                          db offset pag_cb dup (?)
 
  free_pnode_root         dd 0
  do_fpage_counter        dd 0

  first_free_fresh_frame  dd 0
  free_fresh_frames       dd 0 
          
  small_space_size_DIV_MB4        db 0
  log2_small_space_size_DIV_MB4   db 0
        

pagman_control_block ends 