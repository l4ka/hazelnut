include l4pre.inc 
include l4const.inc
   
 test byte ptr ds:[ebx+4],0FFH
 
.list 
include kpage.inc
include uid.inc
page 
include adrspace.inc
page 
include tcb.inc
.list 
include schedcb.inc
page 
include cpucb.inc
page 
include pagconst.inc  
include pagmac.inc 
page 
include syscalls.inc 
page 
include msg.inc
include msgmac.inc
 

 
  code ends
  end