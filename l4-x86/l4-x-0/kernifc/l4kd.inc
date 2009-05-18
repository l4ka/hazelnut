 
kd____outchar macro 
int 3 
cmp al,0 
endm 
 
kd____outstring macro 
int 3 
cmp al,1 
endm 
 
kd____outcstring macro 
int 3 
cmp al,2 
endm 
 
kd____clear_page macro 
int 3 
cmp al,3 
endm 
 
kd____cursor macro 
int 3 
cmp al,4 
endm 
 
kd____outhex32 macro 
int 3 
cmp al,5 
endm 
 
kd____outhex20 macro 
int 3 
cmp al,6 
endm 
 
kd____outhex16 macro 
int 3 
cmp al,7 
endm 
 
kd____outhex12 macro 
int 3 
cmp al,8 
endm 
 
kd____outhex8 macro 
int 3 
cmp al,9 
endm 
 
kd____outhex4 macro 
int 3 
cmp al,10 
endm 
 
kd____outdec macro 
int 3 
cmp al,11 
endm 
 
kd____incharety macro 
int 3 
cmp al,12 
endm 
 
kd____inchar macro 
int 3 
cmp al,13 
endm 
 
kd____inhex32 macro 
int 3 
cmp al,14 
endm 
 
kd____inhex16 macro 
int 3 
cmp al,15 
endm 
 
kd____inhex8 macro 
int 3 
cmp al,16 
endm 
 
kd____inhext macro 
int 3 
cmp al,17 
endm 
 
 
kd____disp macro string 
 local xx 
 
int 3 
nop 
jmp short xx 
db    string 
xx: 
endm 


