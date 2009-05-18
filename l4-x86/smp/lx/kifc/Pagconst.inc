;----------------------------------------------------------------------------
;
;       page entry status bits
;
;----------------------------------------------------------------------------


page_present_bit         equ 0
page_write_permit_bit    equ 1
page_user_permit_bit     equ 2
page_write_through_bit   equ 3
page_cache_disable_bit   equ 4
page_accessed_bit        equ 5
page_dirty_bit           equ 6
superpage_bit            equ 7
global_page_bit          equ 8
                         
shadow_ptab_bit          equ 10
	         


page_present             equ (1 SHL page_present_bit)
page_write_permit        equ (1 SHL page_write_permit_bit)
page_user_permit         equ (1 SHL page_user_permit_bit)
page_write_through       equ (1 SHL page_write_through_bit)
page_cache_disable       equ (1 SHL page_cache_disable_bit)
page_accessed            equ (1 SHL page_accessed_bit)
page_dirty               equ (1 SHL page_dirty_bit)
superpage                equ (1 SHL superpage_bit)
global_page              equ (1 SHL global_page_bit)

shadow_ptab              equ (1 SHL shadow_ptab_bit)



page_fault_due_to_priv_viol_bit  equ page_present_bit
page_fault_due_to_write_bit      equ page_write_permit_bit
page_fault_from_user_level_bit   equ page_user_permit_bit

page_fault_due_to_priv_violation equ (1 SHL page_fault_due_to_priv_viol_bit)
page_fault_due_to_write          equ (1 SHL page_fault_due_to_write_bit)
page_fault_from_user_level       equ (1 SHL page_fault_from_user_level_bit)





 
 
;----------------------------------------------------------------------------
;
;       test page present / writable
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;    reg   address
;
;    CS    phys mem exec
;    DS,ES linear space
;
;----------------------------------------------------------------------------
; POSTCONDITION:
;
;    NC:   present / writable             (executes very fast)
;
;    C:    not present / not writable     (executes slow)
;
;----------------------------------------------------------------------------


test__page_present macro reg
  db 0F6h,40h+reg&code,0,0FFh
endm


test__page_writable macro reg
  db 80h,60h+reg&code,0,0FFh
endm
