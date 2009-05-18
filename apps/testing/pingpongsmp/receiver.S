#define __ASSEMBLY__
#include <linux/linkage.h>

#if 1
#define IPC_SYSENTER	int	$0x30
#else
#define IPC_SYSENTER				\
push	%ecx			; 		\
push	%ebp			; 		\
push	$0x23			; 		\
push	$0f			; 		\
mov	%esp,%ecx		; 		\
sysenter			; 		\
mov	%ebp,%edx		; 		\
0:			
#endif

#define PONG	\
    movl    SYMBOL_NAME(sender_id)+4, %edi  ;\
    movl    SYMBOL_NAME(sender_id), %esi    ;\
    xorl    %ebp, %ebp	;\
    xorl    %ecx, %ecx	;\
    IPC_SYSENTER	;\
			;\
    test    %al, %al	;\
    jnz	    pong_exit	;\
			;\
    xorl    %eax, %eax	

#define PING	\
    movl    SYMBOL_NAME(receiver_id)+4, %edi  ;\
    movl    SYMBOL_NAME(receiver_id), %esi    ;\
    xorl    %eax, %eax	;\
    xorl    %ebp, %ebp	;\
    xorl    %ecx, %ecx	;\
    IPC_SYSENTER	;\
			;\
    test    %al, %al	;\
    jnz	    ping_exit
    .globl  counter
counter:    .long 0
    
    
ENTRY(pong)
    movl    $-1, %eax
    ALIGN
pong_loop:  
    PONG
    PONG
    PONG
    PONG
    jmp	pong_loop
pong_exit:  
    int	$3
    jmp	1f
    .ascii "pong error"
1:
    jmp	1b
    
ENTRY(ping)
    pushl   %ebp
    pushl   %ebx
    pushl   %edi
    pushl   %esi        
ping_loop:  
    PING
    PING
    PING
    PING
    subl    $4, SYMBOL_NAME(counter)
    jnz	ping_loop

    popl    %esi
    popl    %edi
    popl    %ebx
    popl    %ebp
    ret
    
ping_exit:  
    int	$3
    jmp	1f
    .ascii "ping error"
1:  
    popl    %esi
    popl    %edi
    popl    %ebx
    ret
                    
        