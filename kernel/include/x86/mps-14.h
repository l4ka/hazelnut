/*********************************************************************
 *                
 * Copyright (C) 2001,  Karlsruhe University
 *                
 * File path:     x86/mps-14.h
 * Description:   Structures from the Multiprocessor Specification
 *                version 1.4 (Intel document 24201601.pdf).
 *                
 * @LICENSE@
 *                
 * $Id: mps-14.h,v 1.3 2001/11/22 15:08:08 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __X86__MPS_14_H__
#define __X86__MPS_14_H__

#include <universe.h>
#include <types.h>


typedef struct intel_mp_floating
{
    dword_t	signature;		/* "_MP_" 			*/
#define MPS_MPF_SIGNATURE	(('_'<<24)|('P'<<16)|('M'<<8)|'_')
    dword_t	physptr;		/* Configuration table address	*/
    byte_t	length;			/* in paragraphs		*/
    byte_t	specification;
    byte_t	checksum;		/* makes sum 0			*/
    byte_t	feature[5];
} intel_mp_floating_t;

typedef struct mp_config_table
{
    dword_t	signature;
#define MPS_MPC_SIGNATURE ('P'|('C'<<8)|('M'<<16)|('P'<<24))
    word_t	length;			/* Size of table */
    byte_t	spec;			/* 0x01 */
    byte_t	checksum;
    char	oem[8];
    char	productid[12];
    dword_t	oemptr;			/* 0 if not present */
    word_t	oemsize;		/* 0 if not present */
    word_t	entries;
    dword_t	lapic;			/* APIC address */
    dword_t	reserved;
} mp_config_table_t;

/* entry types */
#define	MP_PROCESSOR	0
#define	MP_BUS		1
#define	MP_IOAPIC	2
#define	MP_INTSRC	3
#define	MP_LINTSRC	4

typedef struct mpc_config_processor
{
    byte_t	type;
    byte_t	apicid;			/* Local APIC number		*/
    byte_t	apicver;		/* Local APIC version		*/
    byte_t	cpuflag;		/* bit 0: available
					   bit 1: boot processor	*/
    dword_t	cpufeature;		
    dword_t	featureflag;		/* CPUID feature value		*/
    dword_t	reserved[2];
} mpc_config_processor_t;

typedef struct mpc_config_bus
{
    byte_t	type;
    byte_t	busid;
    byte_t	bustype[6] __attribute((packed));
} mpc_config_bus_t;

typedef struct mpc_config_ioapic
{
    byte_t	type;
    byte_t	apicid;
    byte_t	apicver;
    byte_t	flags;		/* bit 0: usable	*/
    dword_t	apicaddr;
} mpc_config_ioapic_t;

typedef struct mpc_config_intsrc
{
    byte_t	type;
    byte_t	irqtype;
    word_t	irqflag;
    byte_t	srcbus;
    byte_t	srcbusirq;
    byte_t	dstapic;
    byte_t	dstirq;
} mpc_config_intsrc_t;

enum mp_irq_source_types {
    mp_INT = 0,
    mp_NMI = 1,
    mp_SMI = 2,
    mp_ExtINT = 3
};

typedef struct mpc_config_lintsrc
{
    byte_t	type;
    byte_t	irqtype;
    word_t	irqflag;
    byte_t	srcbus;
    byte_t	srcbusirq;
    byte_t	dstapic;	/* 0xFF = broadcast	*/
    byte_t	dstlint;
} mpc_config_lintsrc_t;

enum mp_bustype {
    MP_BUS_ISA = 1,
    MP_BUS_EISA,
    MP_BUS_PCI,
    MP_BUS_MCA
};











/* PCI IRQ Routing table */


struct irq_info {
    byte_t	bus;		/* Bus, device and function */
    byte_t	devfn;
    struct {
	byte_t	link;		/* IRQ line ID, chipset dependent,
				   0=not routed */
	word_t	bitmap;		/* Available IRQs */
    } __attribute__((packed)) irq[4];
    byte_t	slot;		/* Slot number, 0=onboard */
    byte_t	rfu;
} __attribute__((packed));
	
struct irq_routing_table {
    dword_t	signature;	/* PIRQ_SIGNATURE should be here */
    word_t	version;	/* PIRQ_VERSION */
    word_t	size;		/* Table size in bytes */
    byte_t	rtr_bus;
    byte_t	rtr_devfn;	/* Where the interrupt router lies */
    word_t	exclusive_irqs;	/* IRQs devoted exclusively to PCI usage */
    word_t	rtr_vendor;
    word_t	rtr_device;	/* Vendor and device ID of interrupt router */
    dword_t	miniport_data;	/* Crap */
    byte_t	rfu[11];
    byte_t	checksum;	/* Modulo 256 checksum must give zero */
    struct irq_info slots[0];
} __attribute__((packed));

#define MPS_PIRT_SIGNATURE	(('$' << 0) + ('P' << 8) + ('I' << 16) + ('R' << 24))

#endif /* !__X86__MPS_14_H__ */
