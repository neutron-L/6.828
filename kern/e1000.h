#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

/* PCI Device IDs */
#define E1000_VENDOR_ID_82540EM             0x8086
#define E1000_DEVICE_ID_82540EM             0x100E



/* Transmit Descriptor(TDESC) Definition */
struct tx_desc
{
    /* data */
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t sta;
    uint8_t rsv;
    uint8_t css;
    uint16_t special;
};


/* Receive Descriptor(RDESC) Definition */
struct rx_desc
{
    /* data */
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
};


int e1000_init();
int pci_e1000_attach(struct pci_func *pcif);


#endif  // SOL >= 6
