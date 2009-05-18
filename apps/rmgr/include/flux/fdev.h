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
 * Device interface definitions.
 */

#ifndef _FLUX_FDEV_H_
#define _FLUX_FDEV_H_

#include <flux/c/common.h>
#include <flux/x86/types.h>
#include <flux/x86/fdev.h>

/*
 * Description of a device.
 */
struct fdev {
	struct	fdev_drv_ops *drv_ops;	/* driver operations vector */
	void	*data;			/* opaque driver data */
};

typedef struct fdev fdev_t;

/*
 * Buffer operations vector table.
 *
 * XXX: Unused at present.
 */
struct fdev_buf_vec {
	int	(*copyin)(void *src, vm_offset_t offset, void *dest,
			  unsigned count);
	int	(*copyout)(void *src, void *dest, vm_offset_t offset,
			   unsigned count);
	int	(*wire)(void *buf, unsigned size, vm_offset_t *page_list,
			unsigned *amt_wired);
	void	(*unwire)(void *buf, vm_offset_t offset, unsigned size);
};

typedef struct bufvec fdev_buf_vec_t;

/*
 * Driver operations vector table.
 */
struct fdev_drv_ops {
	void	(*close)(fdev_t *dev);
	int	(*read)(fdev_t *dev, void *buf, fdev_buf_vec_t *bufvec,
			vm_offset_t off, unsigned count, unsigned *amount_read);
	int	(*write)(fdev_t *dev, void *buf,
			 fdev_buf_vec_t *bufvec, vm_offset_t off,
			 unsigned count, unsigned *amount_written);
};

/*
 * Flags to fdev_drv_open.
 */
#define FDEV_OPEN_READ	0x01	/* open device for reading */
#define FDEV_OPEN_WRITE	0x02	/* open device for writing */
#define FDEV_OPEN_ALL	(FDEV_OPEN_WRITE|FDEV_OPEN_READ)

/*
 * Error codes.
 */
#define FDEV_NO_SUCH_DEVICE	1
#define FDEV_BAD_OPERATION	2
#define FDEV_INVALID_PARAMETER	3
#define FDEV_IO_ERROR		4
#define FDEV_MEMORY_SHORTAGE	5

/*
 * Memory allocation flags.
 */
#define FDEV_AUTO_SIZE		0x01
#define FDEV_PHYS_WIRED		0x02
#define FDEV_VIRT_EQ_PHYS	0x04
#define FDEV_PHYS_CONTIG	0x08
#define FDEV_NONBLOCKING	0x10
#define FDEV_ISA_DMA_MEMORY	0x20

/*
 * Prototypes.
 */

__FLUX_BEGIN_DECLS

/*
 * Initialization.
 */
void fdev_init(void);

/*
 * Device interface.
 */
int fdev_drv_open(const char *name, unsigned flags, fdev_t **dev);

#define fdev_drv_close(dev) \
	((dev)->drv_ops->close)(dev)
#define fdev_drv_read(dev, buf, bufvec, off, count, amount_read) \
	((dev)->drv_ops->read)(dev, buf, bufvec, off, count, amount_read)
#define fdev_drv_write(dev, buf, bufvec, off, count, amount_written) \
	((dev)->drv_ops->write)(dev, buf, bufvec, off, count, amount_written)

/*
 * Buffer manipulation.
 */

#define fdev_buf_copyin(src, bufvec, offset, dest, count) \
	((bufvec)->copyin)(src, offset, dest, count)
#define fdev_buf_copyout(src, dest, bufvec, offset, count) \
	((bufvec)->copyout)(src, dest, offset, count)
#define fdev_buf_wire(buf, bufvec, size, page_list, amt_wired) \
	((bufvec)->wire)(buf, size, page_list, amt_wired)
#define fdev_buf_unwire(buf, bufvec, offset, size) \
	((bufvec)->unwire)(buf, offset, size)

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
 * Memory allocation/deallocation.
 */
void *fdev_mem_alloc(unsigned size, int flags, unsigned align);
void fdev_mem_free(void *p, int flags, unsigned size);

/*
 * Memory mapping.
 */
int fdev_map_phys_mem(vm_offset_t pa, vm_size_t size, void **addr, int flags);

/*
 * Scheduler support.
 */
void fdev_event_block(void *event);
void fdev_event_unblock(void *event);

/*
 * Software interrupt support.
 */
void fdev_register_soft_handler(void (*func)(void));

/*
 * Timer interrupt support.
 */
void fdev_register_timer_handler(void (*func)(void), int freq);
__FLUX_END_DECLS

#endif /* _FLUX_FDEV_H_ */
