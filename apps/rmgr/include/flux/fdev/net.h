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
 * Definitions specific to network devices.
 */
#ifndef _FLUX_FDEV_NET_H_
#define _FLUX_FDEV_NET_H_

#include <flux/c/common.h>

struct fdev_net
{
	fdev_t		fdev;

	/* Network type; see below. */
	unsigned char	type;

	fdev_error_t (*send)(fdev_t *dev, void *buf, fdev_buf_vect_t *bufvec,
				vm_size_t pkt_size);
};
typedef struct fdev_net fdev_net_t;

#define fdev_net_send(dev, buf, bufvec, size)	\
	((dev)->send((dev), (buf), (bufvec), (size)))

/* Specific network types */
#define FDEV_NET_ETHERNET	0x01

__FLUX_BEGIN_DECLS
/* 
 * Network drivers call this OS-supplied function
 * to allocate a packet buffer to receive a packet into.
 */
void fdev_net_alloc(fdev_net_t *fdev,
		    vm_size_t buf_size, fdev_memflags_t flags,
		    void **out_buf, fdev_buf_vect_t **out_bufvec);

/*
 * Network drivers call this OS-supplied function
 * when a packet has been received from the network.
 * Calling this function consumes the packet buffer.
 */
void fdev_net_recv(fdev_net_t *fdev, void *buf, fdev_buf_vec_t *bufvec,
		   vm_offset_t pkt_size);

/*
 * Free a packet buffer without having received a packet into it.
 * Typically only used during shutdown.
 */
void fdev_net_free(fdev_net_t *fdev, void *buf, fdev_buf_vec_t *bufvec);
__FLUX_END_DECLS

#endif /* _FLUX_FDEV_NET_H_ */
