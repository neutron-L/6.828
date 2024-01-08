#include "ns.h"
#define debug 0

extern union Nsipc nsipcbuf;
extern volatile pte_t uvpt[];
extern volatile pde_t uvpd[];

void input(envid_t ns_envid)
{
    binaryname = "ns_input";

    // LAB 6: Your code here:
    // 	- read a packet from the device driver
    //	- send it to the network server
    // Hint: When you IPC a page to the network server, it will be
    // reading from it for a while, so don't immediately receive
    // another packet in to the same physical page.

    // cprintf("PTE %x %x\n", uvpd[PDX(&nsipcbuf)], uvpt[PTX(&nsipcbuf)]);
    nsipcbuf.pkt.jp_len = 0;
    // cprintf("PTE %x %x\n", uvpd[PDX(&nsipcbuf)], uvpt[PTX(&nsipcbuf)]);
    while (1) {
        // cprintf("Start!");
        // cprintf("Input: %p %p\n", input_pkt.pkt.jp_data, &input_pkt.pkt.jp_len);

        while (sys_receive(nsipcbuf.pkt.jp_data, &nsipcbuf.pkt.jp_len) == -1)
            continue;
        // cprintf("Done!");
    // cprintf("PTE %x %x\n", uvpd[PDX(&nsipcbuf)], uvpt[PTX(&nsipcbuf)]);
        ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P | PTE_W | PTE_U);
    }
}
