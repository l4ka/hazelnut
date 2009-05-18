include l4pre.inc 


  Copyright xxx, L4.ATRACE, 17,01,98, 1
 
 
  dcode
 
;*********************************************************************
;******                                                         ******
;******         Address Trace Handler                           ******
;******                                                         ******
;******                                   Author:   J.Liedtke   ******
;******                                                         ******
;******                                   created:  17.01.98    ******
;******                                   modified: 17.01.98    ******
;******                                                         ******
;*********************************************************************
 

  public init_atrace
  public trace_phys_addr


.nolist 
 include l4const.inc
 include adrspace.inc
.list
 

ok_for x86



cachelinesize       equ   32

min_icache_sets     equ   32
min_dcache_sets     equ   64
min_itlb_sets       equ   16
mib_dtlb_sets       equ   32
min_i4Mtlb_sets     equ    1
min_d4Mtlb_sets     equ    1




instr_access        equ   001b
read_access         equ   010b
write_access        equ   100b


nil_precache_entry  equ   0



  log2  <cachelinesize>
  
log2_cachelinesize  equ   log2_
  


              align 16


atrace_data_area    equ   $
              
atrace_counter      dd    0,0
btrace_counter      dd    0,0
              
btrace_pointer      dd    0
btrace_begin        dd    0
btrace_end          dd    0


              align 16
              
              
i_precache          dd    min_icache_sets      dup (0)

d_precache          dd    min_dcache_sets      dup (0)
  





  assume ds:codseg
        


init_atrace:

  mov   [btrace_end],MB16
  ret


  
  

;----------------------------------------------------------------------------
;
;       trace physical address
;
;----------------------------------------------------------------------------
; PRECONDITION:
;
;       EBX   physical EIP AND FFFFFFE0 + instr access / nil
;       ECX   physical data address AND FFFFFFE0 + data access / nil
;
;----------------------------------------------------------------------------



trace_phys_addr:

  push  ebp
  push  esi
  push  edi
  
X equ   offset atrace_data_area  
  
  mov   ebp,offset atrace_data_area+PM

  mov   esi,ebx
  shr   esi,log2_cachelinesize
  
  inc   dword ptr ss:[ebp+atrace_counter-X]
  xc    z,inc_atrace_high
  
  mov   edi,esi
  and   esi,sizeof d_precache/4-1
  and   edi,sizeof i_precache/4-1
  add   esi,offset d_precache-X
  add   edi,offset i_precache-X
  test  ebx,instr_access
  IFNZ
        mov   eax,esi
        mov   edi,esi
        mov   esi,eax
  FI            
  
  mov   eax,ss:[edi*4+ebp]
  xor   eax,ebx
  cmp   eax,cachelinesize
  xc    b,flush_alternate_precache_line
  
  mov   eax,ss:[esi*4+ebp]
  xor   eax,ebx
  CORAE eax,cachelinesize
  mov   edi,ebx
  or    ebx,eax
  IFNZ  ebx,edi
        mov   [esi*4+ebp],ebx
        mov   edi,ss:[ebp+btrace_pointer-X]
        
        inc   dword ptr ss:[ebp+btrace_counter-X]
        xc    z,inc_btrace_high
  
     ;;   mov   ss:[edi],ebx
        add   edi,4
        IFAE  edi,ss:[ebp+btrace_end-X]
              mov   edi,ss:[ebp+btrace_begin-X]
        FI
        mov   ss:[ebp+btrace_pointer-X],edi
  FI              

  pop   edi
  pop   esi
  pop   ebp
  ret
    
  
                     
  
  
XHEAD   flush_alternate_precache_line

  sub   eax,eax
  mov   [edi*4+ebp],eax
  xret
  


XHEAD   inc_atrace_high

  inc   dword ptr ss:[ebp+atrace_counter+4-X]
  xret
  
  
XHEAD   inc_btrace_high

  inc   dword ptr ss:[ebp+btrace_counter+4-X]
  xret
  
  
    
    



  dcod ends
  code ends
  end
