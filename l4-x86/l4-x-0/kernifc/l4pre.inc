.nolist
.586p
option oldstructs,nokeyword:<REPEAT,SIZE,LENGTH>,expr32,offset:segment
.sall 

strt16 segment para public use16 'code'
strt16 ends

strt segment byte public use32 'code'
strt ends

labseg segment byte public use32 'code'
labseg ends

kcod segment para public use32 'code'
kcod ends
 
code segment para public use32 'code'
 assume ds:nothing, es:nothing, ss:nothing
               
 
dcod segment para public use32 'code'
dcod ends

scod segment para public use32 'code'
scod ends

icod segment para public use32 'code'
icod ends

ic16 segment para public use16 'code'
ic16 ends

lastseg segment para public use32 'code'
lastseg ends

strtseg macro
strt segment byte public use32 'code'
endm

kcode macro
kcod segment para public use32 'code'
endm

dcode macro
dcod segment para public use32 'code'
endm


scode macro
scod segment para public use32 'code'
endm


icode macro
icod segment para public use32 'code'
endm


icode16 macro
ic16 segment para public use16 'code'
endm

;codseg group c16,kcod,dcod,scod,code,icod,ic16

codseg group strt,kcod,dcod,scod,code,icod
c16seg group strt16,ic16


osp macro
db 66h
endm

asp macro
db 67h
endm

on equ 1
off equ 0
 
 
NST=0
doNST=0
clab=0

jY macro j,lab,lg
ifb <lg>
j short il&lab
else
j il&lab
endif
endm

jX macro j,lab,d,lg
jY j,%lab&d,lg
endm

dlY macro lab
il&lab:
endm

dlab macro lab,d
dlY %lab&d
endm

setv macro lab,d,val
lab&d=val
endm
 
IF_ macro j,l,r,lg
NST=NST+1
setv fil,%NST,clab+1
setv elsel,%NST,clab+2
ifnb <l>
cmp l,r
endif
ifnb <j>
jX j,elsel,%NST,lg
endif
dlY %clab
clab=clab+3
endm

FI macro
dlab elsel,%NST
dlab fil,%NST
NST=NST-1
endm

ELSE_ macro lg
jX jmp,fil,%NST,lg
dlab elsel,%NST
setv elsel,%NST,clab
clab=clab+1
endm

ELIF_ macro j,l,r,lg
jX jmp,fil,%NST,lg
dlab elsel,%NST
setv elsel,%NST,clab
clab=clab+1
ifnb <l>
cmp l,r
endif
ifnb <j>
jX j,elsel,%NST,lg
endif
endm

CAND macro j,l,r,lg
ifnb <l>
cmp l,r
endif
jX j,elsel,%NST,lg
endm

COR macro j,l,r
ifnb <l>
cmp l,r
endif
jX j,clab
endm

IFC macro l,r,lg
IF_ jnc,<l>,<r>,lg
endm
IFNC macro l,r,lg
IF_ jc,<l>,<r>,lg
endm
IFZ macro l,r,lg
IF_ jnz,<l>,<r>,lg
endm
IFNZ macro l,r,lg
IF_ jz,<l>,<r>,lg
endm
IFS macro l,r,lg
IF_ jns,<l>,<r>,lg
endm
IFNS macro l,r,lg
IF_ js,<l>,<r>,lg
endm
IFB_ macro l,r,lg
IF_ jnb,<l>,<r>,lg
endm
IFBE macro l,r,lg
IF_ jnbe,<l>,<r>,lg
endm
IFA macro l,r,lg
IF_ jna,<l>,<r>,lg
endm
IFAE macro l,r,lg
IF_ jnae,<l>,<r>,lg
endm
IFL macro l,r,lg
IF_ jnl,<l>,<r>,lg
endm
IFLE macro l,r,lg
IF_ jnle,<l>,<r>,lg
endm
IFG macro l,r,lg
IF_ jng,<l>,<r>,lg
endm
IFGE macro l,r,lg
IF_ jnge,<l>,<r>,lg
endm
IFPE macro l,r,lg
IF_ jpo,<l>,<r>,lg
endm
IFPO macro l,r,lg
IF_ jpe,<l>,<r>,lg
endm

ELIFC macro l,r,lg
ELIF_ jnc,<l>,<r>,lg
endm
ELIFNC macro l,r,lg
ELIF_ jc,<l>,<r>,lg
endm
ELIFZ macro l,r,lg
ELIF_ jnz,<l>,<r>,lg
endm
ELIFNZ macro l,r,lg
ELIF_ jz,<l>,<r>,lg
endm
ELIFS macro l,r,lg
ELIF_ jns,<l>,<r>,lg
endm
ELIFNS macro l,r,lg
ELIF_ js,<l>,<r>,lg
endm
ELIFB macro l,r,lg
ELIF_ jnb,<l>,<r>,lg
endm
ELIFBE macro l,r,lg
ELIF_ jnbe,<l>,<r>,lg
endm
ELIFA macro l,r,lg
ELIF_ jna,<l>,<r>,lg
endm
ELIFAE macro l,r,lg
ELIF_ jnae,<l>,<r>,lg
endm
ELIFL macro l,r,lg
ELIF_ jnl,<l>,<r>,lg
endm
ELIFLE macro l,r,lg
ELIF_ jnle,<l>,<r>,lg
endm
ELIFG macro l,r,lg
ELIF_ jng,<l>,<r>,lg
endm
ELIFGE macro l,r,lg
ELIF_ jnge,<l>,<r>,lg
endm

CANDC macro l,r,lg
CAND jnc,<l>,<r>,lg
endm
CANDNC macro l,r,lg
CAND jc,<l>,<r>,lg
endm
CANDZ macro l,r,lg
CAND jnz,<l>,<r>,lg
endm
CANDNZ macro l,r,lg
CAND jz,<l>,<r>,lg
endm
CANDS macro l,r,lg
CAND jns,<l>,<r>,lg
endm
CANDNS macro l,r,lg
CAND js,<l>,<r>,lg
endm
CANDB macro l,r,lg
CAND jnb,<l>,<r>,lg
endm
CANDBE macro l,r,lg
CAND jnbe,<l>,<r>,lg
endm
CANDA macro l,r,lg
CAND jna,<l>,<r>,lg
endm
CANDAE macro l,r,lg
CAND jnae,<l>,<r>,lg
endm
CANDL macro l,r,lg
CAND jnl,<l>,<r>,lg
endm
CANDLE macro l,r,lg
CAND jnle,<l>,<r>,lg
endm
CANDG macro l,r,lg
CAND jng,<l>,<r>,lg
endm
CANDGE macro l,r,lg
CAND jnge,<l>,<r>,lg
endm
CANDPE macro l,r,lg
CAND jpo,<l>,<r>,lg
endm
CANDPO macro l,r,lg
CAND jpe,<l>,<r>,lg
endm

CORC macro l,r
COR jc,<l>,<r>
endm
CORNC macro l,r
COR jnc,<l>,<r>
endm
CORZ macro l,r
COR jz,<l>,<r>
endm
CORNZ macro l,r
COR jnz,<l>,<r>
endm
CORS macro l,r
COR js,<l>,<r>
endm
CORNS macro l,r
COR jns,<l>,<r>
endm
CORB macro l,r
COR jb,<l>,<r>
endm
CORBE macro l,r
COR jbe,<l>,<r>
endm
CORA macro l,r
COR ja,<l>,<r>
endm
CORAE macro l,r
COR jae,<l>,<r>
endm
CORL macro l,r
COR jl,<l>,<r>
endm
CORLE macro l,r
COR jle,<l>,<r>
endm
CORG macro l,r
COR jg,<l>,<r>
endm
CORGE macro l,r
COR jge,<l>,<r>
endm


DO macro
doNST=doNST+1
setv dol,%doNST,clab
setv odl,%doNST,clab+1
clab=clab+2
dlab dol,%doNST
endm

OD macro
dlab odl,%doNST
doNST=doNST-1
endm

REPEAT macro
jX jmp,dol,%doNST,lg
endm
REPEATC macro
jX jc,dol,%doNST,lg
endm
REPEATNC macro
jX jnc,dol,%doNST,lg
endm
REPEATZ macro
jX jz,dol,%doNST,lg
endm
REPEATNZ macro
jX jnz,dol,%doNST,lg
endm
REPEATS macro
jX js,dol,%doNST,lg
endm
REPEATNS macro
jX jns,dol,%doNST,lg
endm
REPEATA macro
jX ja,dol,%doNST,lg
endm
REPEATAE macro
jX jae,dol,%doNST,lg
endm
REPEATB macro
jX jb,dol,%doNST,lg
endm
REPEATBE macro
jX jbe,dol,%doNST,lg
endm
REPEATL macro
jX jl,dol,%doNST,lg
endm
REPEATLE macro
jX jle,dol,%doNST,lg
endm
REPEATG macro
jX jg,dol,%doNST,lg
endm
REPEATGE macro
jX jge,dol,%doNST,lg
endm

RLOOP macro
jX loop,dol,%doNST,lg
endm
RLOOPZ macro
jX loopz,dol,%doNST,lg
endm
RLOOPNZ macro
jX loopnz,dol,%doNST,lg
endm


EXIT macro lg
jX jmp,odl,%doNST,lg
endm
EXITC macro lg
jX jc,odl,%doNST,lg
endm
EXITNC macro lg
jX jnc,odl,%doNST,lg
endm
EXITZ macro lg
jX jz,odl,%doNST,lg
endm
EXITNZ macro lg
jX jnz,odl,%doNST,lg
endm
EXITS macro lg
jX js,odl,%doNST,lg
endm
EXITNS macro lg
jX jns,odl,%doNST,lg
endm
EXITA macro lg
jX ja,odl,%doNST,lg
endm
EXITAE macro lg
jX jae,odl,%doNST,lg
endm
EXITB macro lg
jX jb,odl,%doNST,lg
endm
EXITBE macro lg
jX jbe,odl,%doNST,lg
endm
EXITL macro lg
jX jl,odl,%doNST,lg
endm
EXITLE macro lg
jX jle,odl,%doNST,lg
endm
EXITG macro lg
jX jg,odl,%doNST,lg
endm
EXITGE macro lg
jX jge,odl,%doNST,lg
endm


OUTER_LOOP macro exitmac
doNST=doNST-1
exitmac
doNST=doNST+1
endm



xxl=0

defretl macro lab
xr&lab:
endm

jret macro j,lab,lg
ifb <lg>
j short xr&lab
else
j xr&lab
endif
endm

pret macro lab
push offset xr&lab
endm

pret macro lab
push offset xr&lab
endm

XC macro cond,lab,lg
ifb <lg>
j&cond short xh&lab
else
j&cond xh&lab
endif
ifndef x1&lab
x&lab=xxl
xxl=xxl+1
x2&lab=0
endif
defretl %x&lab
endm

XHEAD macro lab
xh&lab:
ifndef x2&lab
x&lab=xxl
xxl=xxl+1
x1&lab=0
endif
xretl=x&lab
endm

XRET macro cond,lg
ifb <cond>
jret jmp,%xretl,lg
else
jret j&cond,%xretl,lg
endif
endm

push__xret macro
pret %xretl
endm

CANDNZ_xc_ELSE macro lab,lg
ifb <lg>
jnz short xh&lab
else
jnz xh&lab
endif
ifndef x1&lab
x&lab=xxl
xxl=xxl+1
x2&lab=0
endif
endm

FI_xr macro lab
FI
defretl %x&lab
endm

IF____xc macro cond,lab,lg
ifb <lg>
j&cond short xh&lab
else
j&cond xh&lab
endif
ifndef x1&lab
x&lab=xxl
xxl=xxl+1
x2&lab=0
endif
fi____lab=x&lab
&lab&_false:
endm

ELSE__ macro
endm

FI____ macro
defretl %fi____lab
endm



XXRET_FALSE macro lab,lg
ifb <lg>
jmp short &lab&_false
else
jmp &lab&_false
endif
endm


cmp___eax macro imm
if ((imm le 127) AND (imm ge 0))
db 83h,0F8h,imm
else
if ((-(imm) le 128) AND (-(imm) ge 0))
db 83h,0F8h,imm
else
cmp eax,imm
endif
endif
endm

add___eax macro imm
if ((imm le 127) AND (imm ge 0))
db 83h,0C0h,imm
else
if ((-(imm) le 128) AND (-(imm) ge 0))
db 83h,0C0h,imm
else
add eax,imm
endif
endif
endm


sub___eax macro imm
if ((imm le 127) AND (imm ge 0))
db 83h,0E8h,imm
else
if ((-(imm) le 128) AND (-(imm) ge 0))
db 83h,0E8h,imm
else
sub eax,imm
endif
endif
endm


movi macro reg,imm
if imm EQ 0
sub reg,reg
else
mov reg,imm
endif
endm



ccall macro cc,lab
j&cc& $+3
db 81h
call lab
endm


log2 macro v
if v AND 0FFFF0000h
log2 <%(v SHR 16)>
log2_=log2_+16
else
if v AND 0FF00h
log2 <%(v SHR 8)>
log2_=log2_+8
else
if v AND 0F0h
log2 <%(v SHR 4)>
log2_=log2_+4
else
if v AND 0Ch
log2 <%(v SHR 2)>
log2_=log2_+2
else
if v eq 2
log2_=1
else
if v eq 1
log2_=0
else
log2_ovfl
endif
endif
endif
endif
endif
endif
endm



eaxhl equ ax
ebxhl equ bx
ecxhl equ cx
edxhl equ dx
esihl equ si
edihl equ di
ebphl equ bp
eaxl equ al
ebxl equ bl
ecxl equ cl
edxl equ dl
eaxh equ ah
ebxh equ bh
ecxh equ ch
edxh equ dh
axl equ al
bxl equ bl
cxl equ cl
dxl equ dl
axh equ ah
bxh equ bh
cxh equ ch
dxh equ dh

eaxcode equ 0
ecxcode equ 1
edxcode equ 2
ebxcode equ 3
espcode equ 4
ebpcode equ 5
esicode equ 6
edicode equ 7

clign macro alignment,off

ifb <off>
aax=($-modstart) AND (alignment-1)
else
aax=($-modstart+off) AND (alignment-1)
endif
if aax eq 0
aax=alignment
endif
aax=aax+16-alignment
if aax le 10
db 8Dh,80h,0,0,0,0
aax=aax+6
endif
if aax le 10
db 8Dh,80h,0,0,0,0
aax=aax+6
endif
if aax eq 11
db 8Dh,40h,0
aax=aax+3
endif
if aax eq 12
mov eax,eax
mov eax,eax
endif
if aax eq 13
db 8Dh,40h,0
endif
if aax eq 14
mov eax,eax
endif
if aax eq 15
nop
endif
endm


klign macro alignment,off

ifb <off>
aax=($-kmodstart) AND (alignment-1)
else
aax=($-kmodstart+off) AND (alignment-1)
endif
if aax eq 0
aax=alignment
endif
aax=aax+16-alignment
if aax le 10
db 8Dh,80h,0,0,0,0
aax=aax+6
endif
if aax le 10
db 8Dh,80h,0,0,0,0
aax=aax+6
endif
if aax eq 11
db 8Dh,40h,0
aax=aax+3
endif
if aax eq 12
mov eax,eax
mov eax,eax
endif
if aax eq 13
db 8Dh,40h,0
endif
if aax eq 14
mov eax,eax
endif
if aax eq 15
nop
endif
endm


ke macro text
local lab
int 3
jmp short lab
db &text
lab:
endm


ko macro char 
int 3 
cmp al,'&char' 
endm 
 

 
true    equ 1
false   equ 0


kmeasure=off

inc___measure macro cnt
IF kmeasure
inc cnt
ENDIF
endm

jmpf32  macro off,seg
db 0EAh
dd off
dw seg
endm

jmpf16  macro off,seg
db 0EAh
dw off
dw seg
endm







Copyright macro who,name,dd,mm,yy,ver,type
title name
modstart equ $
labseg segment byte public use32 'code'
labstart equ $
ifidn <type>,<K>
kcod segment para public use32 'code'
kmodstart equ $
kcod ends
dw lowword offset kmodstart
else
dw 0
endif
dw lowword offset modstart
dw ((yy-90) SHL 9)+(mm SHL 5)+dd
dw ver+gver*1024
db '&name&',0
db '(C)'
db '&who&',0
if ($-labstart) LT 16
dd 0,0,0
endif
if ($-labstart) GT 32
(c)ovfl
endif
labseg ends
endm

ok_for macro k1,k2,k3 
xok %kernel_type,k1,k2,k3
endm 
 
xok macro k,k1,k2,k3 
IFB <k1> 
wrong kernel 
ELSE 
IF k NE k1 
xok k,k2,k3 
ENDIF 
ENDIF 
endm 
 
.list
