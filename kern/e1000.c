#include <kern/e1000.h>
#include <kern/pmap.h>


#include <inc/string.h>


// LAB 6: Your driver code here

static volatile uint32_t *e1000;

static uint8_t tx_buffer[RING_SIZE * BUFFER_SIZE];
static struct tx_desc tx_queue[RING_SIZE]__attribute__((aligned(16)));


int e1000_init()
{
    assert((sizeof(tx_queue) & 0x7F) == 0);

    /* Init tx descriptor queue */ 
    for (int i = 0; i < RING_SIZE; ++i)
    {
        tx_queue[i].addr = PADDR(&tx_buffer[i * BUFFER_SIZE]);
        tx_queue[i].sta = E1000_TXD_STAT_DD;
    }

    e1000[INDEX(E1000_TDBAL)] = PADDR(tx_queue);
    e1000[INDEX(E1000_TDBAH)] = 0;
    e1000[INDEX(E1000_TDLEN)] = RING_SIZE << 7;
    e1000[INDEX(E1000_TDH)] = e1000[INDEX(E1000_TDT)] = 0;

    e1000[INDEX(E1000_TCTL)] = E1000_TCTL_EN;
    e1000[INDEX(E1000_TCTL)] |= E1000_TCTL_PSP;

    e1000[INDEX(E1000_TCTL)] |= 0x10 << 4;
    e1000[INDEX(E1000_TCTL)] |= 0x40 << 12;

    e1000[INDEX(E1000_TIPG)] = 10;
    e1000[INDEX(E1000_TIPG)] |= 8<<10;
    e1000[INDEX(E1000_TIPG)] |= 6<<20;

    check_e1000_transmit();

}

int e1000_transmit(void * pkt, uint32_t len)
{
    int idx = e1000[INDEX(E1000_TDT)];

    if (!(tx_queue[idx].sta & E1000_TXD_STAT_DD))
        return -1;

    tx_queue[idx].length = len;
    memcpy(KADDR(tx_queue[idx].addr), (const void *)pkt, tx_queue[idx].length);
    cprintf("%s", KADDR(tx_queue[idx].addr));
    tx_queue[idx].cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS;
    tx_queue[idx].sta &= ~E1000_TXD_STAT_DD;

    cprintf("%d %p %x %x\n", idx, tx_queue[idx].addr, 
    tx_queue[idx].cmd << 24 | tx_queue[idx].cso << 16 | tx_queue[idx].length,
    tx_queue[idx].special << 24 | tx_queue[idx].css << 16 | tx_queue[idx].sta);
    
    e1000[INDEX(E1000_TDT)] = (idx + 1) % RING_SIZE;

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
    static char * msgs[]=
    {
        "hello world\n",
        "this is a test program\n",
        "which is running in kernel\n",
        NULL
    };

    for (int i = 0; i < 40; ++i)
    {
        e1000_transmit(msgs[i % 3], strlen(msgs[i%3]));
    }
    return 0;
}