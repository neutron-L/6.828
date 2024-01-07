#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

/* PCI Device IDs */
#define E1000_VENDOR_ID_82540EM             0x8086
#define E1000_DEVICE_ID_82540EM             0x100E


int e1000_init();
int pci_e1000_attach(struct pci_func *pcif);


#endif  // SOL >= 6
