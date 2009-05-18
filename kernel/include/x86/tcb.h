/*********************************************************************
 *                
 * Copyright (C) 2000, 2001,  Karlsruhe University
 *                
 * File path:     x86/tcb.h
 * Description:   x86 specific part of TCB structure.
 *                
 * @LICENSE@
 *                
 * $Id: tcb.h,v 1.12 2001/11/22 14:56:37 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __X86__TCB_H__
#define __X86__TCB_H__

typedef struct {
    byte_t data[X86_FLOATING_POINT_SIZE];
} x86_fpu_state_t;

typedef struct {
    dword_t	dr[8];
} x86_debug_regs_t;

typedef struct {
#if defined(X86_FLOATING_POINT_ALLOC_SIZE)
    x86_fpu_state_t	*fpu_ptr;
#else
    x86_fpu_state_t	fpu;
    int			fpu_used;
#endif
    x86_debug_regs_t	dbg;
} tcb_priv_t;


#endif /* __X86__TCB_H__ */
