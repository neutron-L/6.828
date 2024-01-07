#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

/* PCI Device IDs */
#define E1000_VENDOR_ID_82540EM             0x8086
#define E1000_DEVICE_ID_82540EM             0x100E


/** PCI Controller Register
  * RW - register is both readable and writable
  * RO - register is read only
  * WO - register is write only
  **/
#define E1000_CTRL                          0x0000   /* Device Control - RW */
#define E1000_STATUS                        0x0008   /* Device Status - RO */

 
#define E1000_RCTL                          0x0100   /* RX Control - RW */
#define E1000_RDBAL                         0x2800   /* Receive Descriptor Base Low - RW */
#define E1000_RDBAH                         0x2804   /* Receive Descriptor Base High - RW */
#define E1000_RDLEN                         0x2808   /* Receive Descriptor Length - RW */
#define E1000_RDH                           0x2810   /* RX Descriptor Head (1) - RW */
#define E1000_RDT                           0x2818   /* RX Descriptor Tail (1) - RW */


#define E1000_TCTL                          0x0400   /* TX Control - RW */
#define E1000_TIPG                          0x0410   /* TX IPG - RW */
#define E1000_TDBAL                         0x3800   /* Transmit Descriptor Base Low - RW */
#define E1000_TDBAH                         0x3804   /* Transmit Descriptor Base High - RW */
#define E1000_TDLEN                         0x3808   /* Transmit Descriptor Length - RW */
#define E1000_TDH                           0x3810   /* TX Descriptor Head (1) - RW */
#define E1000_TDT                           0x3818   /* TX Descriptor Tail (1) - RW */

/* Transmit Control */
#define E1000_TCTL_EN                       0x00000002    /* enable tx */
#define E1000_TCTL_PSP                      0x00000008    /* pad short packets */
#define E1000_TCTL_CT                       0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD                     0x003ff000    /* collision distance */


/* Transmit Descriptor bit definitions */
#define E1000_TXD_CMD_EOP                   0x01000000 /* End of Packet */
#define E1000_TXD_CMD_IFCS                  0x02000000 /* Insert FCS (Ethernet CRC) */
#define E1000_TXD_CMD_IC                    0x04000000 /* Insert Checksum */
#define E1000_TXD_CMD_RS                    0x08000000 /* Report Status */

#define E1000_TXD_STAT_DD                   0x00000001 /* Descriptor Done */
#define E1000_TXD_STAT_EC                   0x00000002 /* Excess Collisions */
#define E1000_TXD_STAT_LC                   0x00000004 /* Late Collisions */
#define E1000_TXD_STAT_TU                   0x00000008 /* Transmit underrun */

#define RING_SIZE                           0x0020
#define BUFFER_SIZE                         1518

#define INDEX(OFFSET)                       ((OFFSET) >> 2)


/* ransmit Descriptor(TDESC) Definition */
struct tx_desc
{
    /* data */
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t sta;
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
int e1000_transmit(void * pkt, uint32_t);
int pci_e1000_attach(struct pci_func *pcif);


int check_e1000_transmit();

#endif  // SOL >= 6
