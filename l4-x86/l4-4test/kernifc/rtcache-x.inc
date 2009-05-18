;************************************************
;
;       included in special rt versions of
;       ktest5, sigma0, memctr
;                  
;       all these have to be recompiled 
;
;************************************************


L2_cache_size       equ KB256
L2_cache_ways       equ 1

L2_cache_colors     equ (L2_cache_size / (L2_cache_ways*pagesize))




unpartitioned             equ 0
partitioned               equ 1
single_writer_partitioned equ 2

write_through             equ 0
write_back                equ 1


none                      equ 0
flooder                   equ 1
filter                    equ 2


partitioning_strategy     equ unpartitioned
cache_strategy            equ write_back

interfering_with          equ flooder

matrix_columns            equ 64
filtersize                equ 512


