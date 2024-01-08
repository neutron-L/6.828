#include "ns.h"
#define debug 0

extern union Nsipc nsipcbuf;
extern volatile pte_t uvpt[];
extern volatile pde_t uvpd[];

void sleep(int ms)
{
    int start = sys_time_msec();
    while (start + ms > sys_time_msec())
        continue;
    return;
}

void input(envid_t ns_envid)
{
    binaryname = "ns_input";

    // LAB 6: Your code here:
    // 	- read a packet from the device driver
    //	- send it to the network server
    // Hint: When you IPC a page to the network server, it will be
    // reading from it for a while, so don't immediately receive
    // another packet in to the same physical page.

    nsipcbuf.pkt.jp_len = 0;
    while (1) {
        while (sys_receive(nsipcbuf.pkt.jp_data, &nsipcbuf.pkt.jp_len) == -1)
            continue;

        ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P | PTE_W | PTE_U);
        sleep(10);
    }
}
