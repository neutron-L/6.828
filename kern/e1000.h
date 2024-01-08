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

#define E1000_MTA                           0x5200   /* [127:0] Multicast Table Array (n) */
#define E1000_RAL                           0x5400
#define E1000_RAH                           0x5404


/* Transmit Descriptor bit definitions */
#define E1000_TXD_CMD_EOP                   0x01 /* End of Packet */
#define E1000_TXD_CMD_IFCS                  0x02 /* Insert FCS (Ethernet CRC) */
#define E1000_TXD_CMD_IC                    0x04 /* Insert Checksum */
#define E1000_TXD_CMD_RS                    0x08 /* Report Status */

#define E1000_TXD_STAT_DD                   0x01 /* Descriptor Done */
#define E1000_TXD_STAT_EC                   0x02 /* Excess Collisions */
#define E1000_TXD_STAT_LC                   0x04 /* Late Collisions */
#define E1000_TXD_STAT_TU                   0x08 /* Transmit underrun */

/* Receive Descriptor bit definitions */
#define E1000_RXD_STAT_DD                   0x01 /* Descriptor Done */
#define E1000_RXD_STAT_EOP                  0x02 /* End of Packet */
#define E1000_RXD_STAT_IXSM                 0x04 /* Ignore checksum Indication */
#define E1000_RXD_STAT_VP                   0x08 /* Packet is 802.1Q */
#define E1000_RXD_STAT_RSV                  0x10 /* Reservered */
#define E1000_RXD_STAT_TCPCS                0x20 /* TCP Checksum calculated on Packet */
#define E1000_RXD_STAT_IPCS                 0x40 /* IP Checksum calculated on Packet */
#define E1000_RXD_STAT_PIF                  0x80 /* Passed in-exact filter */

#define TX_RING_SIZE                        0x0020
#define RX_RING_SIZE                        0x0080
#define TX_BUFFER_SIZE                      1518
#define RX_BUFFER_SIZE                      0x0800

#define INDEX(OFFSET)                       ((OFFSET) >> 2)


/* TIPG Register Definition */
struct tipg
{
    uint32_t ipgt       : 10;
    uint32_t ipgr1      : 10;
    uint32_t ipgr2      : 10;
    uint32_t reserved   : 2;
};


/* TCTL Register Definition */
struct tctl
{
    uint32_t reserved1  : 1;
    uint32_t en         : 1;
    uint32_t reserved2  : 1;
    uint32_t psp        : 1;
    uint32_t ct         : 8;
    uint32_t cold       : 10;
    uint32_t swxoff     : 1;
    uint32_t reserved3  : 1;
    uint32_t rtlc       : 1;
    uint32_t nrtu       : 1;
    uint32_t reserved4  : 6;
};



/* RCTL Register Definition */
struct rctl
{
    uint32_t reserved1  : 1;
    uint32_t en         : 1;
    uint32_t sbp        : 1;
    uint32_t upe        : 1;
    uint32_t mpe        : 1;
    uint32_t lpe        : 1;
    uint32_t lbm        : 2;
    uint32_t rdmts      : 2;
    uint32_t reserved2  : 2;
    uint32_t mo         : 2;
    uint32_t reserved3  : 1;

    uint32_t bam        : 1;
    uint32_t bsize      : 2;
    uint32_t vfe        : 1;
    uint32_t cfien      : 1;
    uint32_t cfi        : 1;

    uint32_t reserved4  : 1;
    uint32_t dpf        : 1;


    uint32_t pmcf       : 1;
    uint32_t reserved5  : 1;

    uint32_t bsex       : 1;
    uint32_t secrc      : 1;

    uint32_t reserved6  : 5;
};




/* Transmit Descriptor(TDESC) Definition */
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
    uint8_t sta;
    uint8_t errors;
    uint16_t special;
};


int e1000_init();
int e1000_transmit(void * pkt, uint32_t);
int pci_e1000_attach(struct pci_func *pcif);


int check_e1000_transmit();

#endif  // SOL >= 6
