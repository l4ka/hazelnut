   

virtual_space_size              equ   0C0000000h


tcb_space				equ	0E0000000h
tcb_space_size			equ	(16*MB)

com_space_size                  equ   (8*MB)
com0_base				equ	(tcb_space+32*MB)
com1_base				equ   (com0_base+com_space_size)

shared_table_base               equ   (tcb_space)
shared_table_size               equ   (com1_base+com_space_size-shared_table_base)

PM					equ   0F0000000h

KR                              equ   PM

physical_kernel_mem_size        equ   128*MB

kdebug_cached_alias_mem         equ   0FF000000h  

kernel_begin  			equ   1*MB
kernel_size				equ   72*KB

shared_pseudotask_no            equ   1


