/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     arm/space.h
 * Description:   Address space definition for the ARM
 *                
 * @LICENSE@
 *                
 * $Id: space.h,v 1.1 2001/12/07 19:05:12 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __ARM__SPACE_H__
#define __ARM__SPACE_H__

class space_t
{
    // Normal page directory.
    dword_t pgdir[1024];

public:
    inline dword_t * pagedir_phys (void)
	{ return (dword_t *) this; }

    inline dword_t * pagedir (void)
	{ return (dword_t *) phys_to_virt (this); }

    inline pgent_t * pgent (int n)
	{ return (pgent_t *) phys_to_virt (&pgdir[n]); }
};


INLINE space_t * get_current_space (void)
{
    return (space_t *) get_ttb ();
}

INLINE space_t * get_kernel_space (void)
{
    extern ptr_t kernel_ptdir;
    return (space_t *) virt_to_phys (kernel_ptdir);
}


#endif /* !__ARM_SPACE_H__ */

