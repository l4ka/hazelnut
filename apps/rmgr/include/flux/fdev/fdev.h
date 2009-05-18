/* 
 * Copyright (c) 1996 The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 */
/*
 * Flux device driver interface definitions.
 */
#ifndef _FLUX_FDEV_FDEV_H_
#define _FLUX_FDEV_FDEV_H_

#include <flux/c/common.h>

struct fdev
{
	fdev_set_t	*fdev_set;

	unsigned char	devclass;	/* General device class; see below */
	const char	*name;		/* Short device type identifier */
	const char	*description;	/* Longer description of device */
	const char	*vendor;	/* Vendor name, if available */
	const char	*model;		/* Specific hardware model */
	const char	*version;	/* Origin and version of driver */

	fdev_error_t (*ioctl)(fdev_t *dev, int cmd,
			      void *buf, fdev_buf_vect_t *bufvec);
};
typedef struct fdev fdev_t;

#define fdev_ioctl(dev, cmd, buf, bufvec)	\
	((dev)->ioctl((dev), (cmd), (buf), (bufvec)))

/* Basic device classes */
#define FDEV_CLASS_BUS		0x01	/* Interfaces to other devices */
#define FDEV_CLASS_BLOCK	0x02	/* Random-access storage devices */
#define FDEV_CLASS_SEQ		0x03	/* Sequential storage devices (tape) */
#define FDEV_CLASS_CHAR		0x04	/* Character devices (ser, par) */
#define FDEV_CLASS_NET		0x05	/* Network interface devices */


/*
 * Memory allocation/deallocation.
 */
typedef unsigned fdev_memflags_t;

__FLUX_BEGIN_DECLS
void *fdev_mem_alloc(vm_size_t size, fdev_memflags_t flags, unsigned align);
void fdev_mem_free(void *block, fdev_memflags_t flags, vm_size_t size);
vm_offset_t fdev_mem_get_phys(void *block, fdev_memflags_t flags,
			      vm_size_t size);
__FLUX_END_DECLS

#define FDEV_AUTO_SIZE		0x01
#define FDEV_PHYS_WIRED		0x02
#define FDEV_VIRT_EQ_PHYS	0x04
#define FDEV_PHYS_CONTIG	0x08
#define FDEV_NONBLOCKING	0x10
#define FDEV_ISA_DMA_MEMORY	0x20

/*
 * Buffer management.
 */
struct fdev_buf_vec {
	int	(*copyin)(void *src, vm_offset_t offset, void *dest,
			  vm_size_t size);
	int	(*copyout)(void *src, void *dest, vm_offset_t offset,
			   vm_size_t size);
	int	(*wire)(void *buf, unsigned size, vm_offset_t *page_list,
			unsigned *amt_wired);
	void	(*unwire)(void *buf, vm_offset_t offset, vm_size_t size);
	int	(*map)(void *buf, void **kaddr, vm_size_t size);
};

#define fdev_buf_copyin(src, bufvec, offset, dest, count) \
	((bufvec)->copyin)(src, offset, dest, count)
#define fdev_buf_copyout(src, dest, bufvec, offset, count) \
	((bufvec)->copyout)(src, dest, offset, count)
#define fdev_buf_wire(buf, bufvec, size, page_list, amt_wired) \
	((bufvec)->wire)(buf, size, page_list, amt_wired)
#define fdev_buf_unwire(buf, bufvec, offset, size) \
	((bufvec)->unwire)(buf, offset, size)
#define fdev_buf_map(buf, bufvec, kaddr, size) \
	((bufvec)->map)(buf, kaddr, size)

__FLUX_BEGIN_DECLS
/* XXX encapsulate these in a 'fdev_bufvec_lmm' or somesuch. */
int fdev_default_buf_copyin(void *buf, vm_offset_t off, void *dest,
			    unsigned amt);
int fdev_default_buf_copyout(void *src, void *buf, vm_offset_t off,
			     unsigned amt);
int fdev_default_buf_wire(void *buf, unsigned size, vm_offset_t *page_list,
			  unsigned *amt_wired);
void fdev_default_buf_unwire(void *buf, vm_offset_t offset, unsigned size);

/*
 * Interrupts.
 */
int fdev_intr_alloc(unsigned irq, void (*handler)(void *),
		    void *data, int flags);
void fdev_intr_free(unsigned irq);
void fdev_intr_disable(void);		/* XXX: not used at present */
void fdev_intr_enable(void);		/* XXX: not used at present */

/*
 * Memory mapping.
 */
int fdev_map_phys_mem(vm_offset_t pa, vm_size_t size, void **addr, int flags);

/*
 * Sleep/wakeup support.
 */
void fdev_sleep(void);
void fdev_wakeup(void);

/*
 * Software interrupt support.
 * XXX get rid of this.
 */
void fdev_register_soft_handler(void (*func)(void));

/*
 * Timer interrupt support.
 */
void fdev_register_timer_handler(void (*func)(void), int freq);
__FLUX_END_DECLS

#endif /* _FLUX_FDEV_FDEV_H_ */
