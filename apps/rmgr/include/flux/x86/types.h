/*
 * Mach Operating System
 * Copyright (c) 1992,1991,1990,1989,1988 Carnegie Mellon University.
 * Copyright (c) 1996,1995 The University of Utah and
 * the Computer Systems Laboratory (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON, THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF
 * THIS SOFTWARE IN ITS "AS IS" CONDITION, AND DISCLAIM ANY LIABILITY
 * OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF
 * THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 *	Header file containing basic types for the x86 architecture.
 */
#ifndef	_FLUX_X86_TYPES_H_
#define _FLUX_X86_TYPES_H_

/*
 * A natural_t is the type for the native
 * integer type, e.g. 32 or 64 or.. whatever
 * register size the machine has.  Unsigned, it is
 * used for entities that might be either
 * unsigned integers or pointers, and for
 * type-casting between the two.
 * For instance, the IPC system represents
 * a port in user space as an integer and
 * in kernel space as a pointer.
 */
typedef unsigned int	natural_t;

/*
 * An integer_t is the signed counterpart
 * of the natural_t type. Both types are
 * only supposed to be used to define
 * other types in a machine-independent
 * way.
 */
typedef int		integer_t;

/*
 * A vm_offset_t is a type-neutral pointer,
 * e.g. an offset into a virtual memory space.
 */
typedef	natural_t	vm_offset_t;

/*
 * A vm_size_t is the proper type for e.g.
 * expressing the difference between two
 * vm_offset_t entities.
 */
typedef	natural_t	vm_size_t;

/*
 * On any anchitecture,
 * these types are _exactly_ as wide as indicated in their names.
 */
typedef signed char		signed8_t;
typedef signed short		signed16_t;
typedef signed long		signed32_t;
typedef signed long long	signed64_t;
typedef unsigned char		unsigned8_t;
typedef unsigned short		unsigned16_t;
typedef unsigned long		unsigned32_t;
typedef unsigned long long	unsigned64_t;
typedef float			float32_t;
typedef double			float64_t;

/*
 * On any given architecture,
 * these types are guaranteed to be _at_least_
 * as wide as indicated in their names,
 * but may be wider if a wider type can be more efficiently accessed.
 *
 * On the x86, bytes and 32-bit words are fast,
 * but 16-bit words are slow;
 * this property is reflected in these type definitions.
 */
typedef unsigned char		boolean_t;
typedef signed char		signed_min8_t;
typedef unsigned char		unsigned_min8_t;
typedef signed int		signed_min16_t;
typedef unsigned int		unsigned_min16_t;
typedef signed int		signed_min32_t;
typedef unsigned int		unsigned_min32_t;

#endif	/* _FLUX_X86_TYPES_H_ */
