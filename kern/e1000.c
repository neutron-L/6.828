#include <kern/e1000.h>
#include <kern/pmap.h>

#include <inc/string.h>

// LAB 6: Your driver code here

static volatile uint32_t* e1000;

static uint8_t        rx_buffer[RX_RING_SIZE][RX_BUFFER_SIZE];
static struct rx_desc rx_queue[RX_RING_SIZE] __attribute__((aligned(16)));

static uint8_t        tx_buffer[TX_RING_SIZE][TX_BUFFER_SIZE];
static struct tx_desc tx_queue[TX_RING_SIZE] __attribute__((aligned(16)));

int e1000_init()
{
    assert((sizeof(tx_queue) & 0x7F) == 0);
    assert((sizeof(rx_queue) & 0x7F) == 0);
    assert(sizeof(struct tipg) == 4);
    assert(sizeof(struct tctl) == 4);
    assert(sizeof(struct rctl) == 4);

    /* Init tx descriptor queue */
    for (int i = 0; i < TX_RING_SIZE; ++i) {
        tx_queue[i].addr = PADDR(&tx_buffer[i]);
        tx_queue[i].sta  = E1000_TXD_STAT_DD;
    }

    e1000[INDEX(E1000_TDBAL)] = PADDR(tx_queue);
    e1000[INDEX(E1000_TDBAH)] = 0;
    e1000[INDEX(E1000_TDLEN)] = sizeof(tx_queue);
    e1000[INDEX(E1000_TDH)] = e1000[INDEX(E1000_TDT)] = 0;

    struct tctl* reg_tctl = &e1000[INDEX(E1000_TCTL)];
    reg_tctl->en          = 1;
    reg_tctl->psp         = 1;
    reg_tctl->ct          = 0x10;
    reg_tctl->cold        = 0x40;

    struct tipg* reg_tipg = &e1000[INDEX(E1000_TIPG)];
    reg_tipg->ipgt        = 10;
    reg_tipg->ipgr1       = 8;
    reg_tipg->ipgr2       = 6;


    /* Init rx descriptor queue */
    for (int i = 0; i < RX_RING_SIZE; ++i) {
        rx_queue[i].addr = PADDR(&rx_buffer[i]);
    }

    e1000[INDEX(E1000_RDBAL)] = PADDR(rx_queue);
    e1000[INDEX(E1000_RDBAH)] = 0;
    e1000[INDEX(E1000_RDLEN)] = sizeof(rx_queue);
    e1000[INDEX(E1000_RDH)]   = 0;
    e1000[INDEX(E1000_RDT)]   = RX_RING_SIZE - 1;

    struct rctl* reg_rctl = &e1000[INDEX(E1000_RCTL)];
    reg_rctl->en = 1;
    reg_rctl->lbm = 0;
    reg_rctl->secrc = 1;

    e1000[INDEX(E1000_RAL)] = 0x12005452;
    e1000[INDEX(E1000_RAH)] = 0x80005634; // have to set AV bit 


    // check_e1000_transmit();
}


/// @brief transmit packet
/// @param pkt pointer to data
/// @param len the length of data in bytes
/// @return    0  transmit successfully
///           -1  transmit failed
int e1000_transmit(void* pkt, uint32_t len)
{
    int idx = e1000[INDEX(E1000_TDT)];

    if (!(tx_queue[idx].sta & E1000_TXD_STAT_DD))
        return -1;

    tx_queue[idx].length = len;
    memcpy(KADDR(tx_queue[idx].addr), (const void*)pkt, tx_queue[idx].length);
    tx_queue[idx].cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS;
    tx_queue[idx].sta = 0;

    // cprintf(
    //     "Transmit: %d %d %p %x %x\n", tx_queue[idx].length, idx,
    //     tx_queue[idx].addr,
    //     tx_queue[idx].cmd << 24 | tx_queue[idx].cso << 16 |
    //         tx_queue[idx].length,
    //     tx_queue[idx].special << 24 | tx_queue[idx].css << 16 |
    //         tx_queue[idx].sta);

    e1000[INDEX(E1000_TDT)] = (idx + 1) % TX_RING_SIZE;

    return 0;
}


/// @brief receive packet
/// @param pkt pointer to buffer for receiving data
/// @param len the length of data in bytes
/// @return    0  receive successfully
///           -1  receive failed
int e1000_receive(void* pkt, uint32_t * len)
{
    static int idx = 0;

    if (!(rx_queue[idx].sta & E1000_RXD_STAT_DD) || rx_queue[idx].errors)
    {
        cprintf("none\n");
        return -1;
    }
    // cprintf("idx : %d %d %d\n", idx,  *(uint32_t *)(KADDR(rx_queue[idx].addr)), rx_queue[idx].length);
    assert(rx_queue[idx].sta & E1000_RXD_STAT_EOP);
    memcpy(pkt, KADDR(rx_queue[idx].addr), rx_queue[idx].length);
    *len = rx_queue[idx].length - 4;

    rx_queue[idx].sta = 0;
    // cprintf(
    //     "Receive: %d %s(%d)\n", idx, rx_queue[idx].addr, rx_queue[idx].length);
    e1000[INDEX(E1000_RDT)] = idx;
    idx = (idx + 1) % RX_RING_SIZE;

    return 0;
}

int pci_e1000_attach(struct pci_func* pcif)
{
    pci_func_enable(pcif);

    e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    cprintf("status word: 0x%x\n", *(e1000 + 2));

    return 0;
}

int check_e1000_transmit()
{
    static char* msgs[] = {
        "hello world\n", "this is a test program\n",
        "which is running in kernel\n", NULL};

    for (int i = 0; i < 10; ++i) {
        e1000_transmit(msgs[i % 3], strlen(msgs[i % 3]));
    }
    return 0;
}
