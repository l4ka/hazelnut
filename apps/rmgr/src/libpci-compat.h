#define __KERNEL__
#define __L4__
#define CONFIG_PCI
#define CONFIG_PCI_BIOS

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#if 0
#include "l4/pci/libpci.h"
#else /* 0 */
#include "libpci.h"
#endif /* 0 */
#include <stdio.h>

#define printk(args...) printf(args)
#define pcilock()
#define spin_lock_irqsave(a,b)
#define spin_unlock_irqrestore(a,b)
#define __initfunc(a) a
#define __initdata
#define __save_flags(x)
#define __restore_flags(x)
#define __cli()
#define __sti()
#define PAGE_OFFSET 0
#define KERN_ERR
#define GFP_KERNEL 0
#define GFP_ATOMIC 0
#define NR_IRQS 224
#define __va(a) a
#define pcibios_fixup()
#define pcibios_fixup_bus(x)

#define kmalloc(x,y) libpci_compat_kmalloc(x)

#define libpci_compat_MAX_DEV 16
#define libpci_compat_MAX_BUS 2

extern void* libpci_compat_kmalloc(int size);
