#include "ns.h"
#define debug 0

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
    uint32_t req, whom;
    int perm, r;

    while (1)
    {
        req = ipc_recv((int32_t *)&whom, &nsipcbuf, &perm);

        if (debug) {
			cprintf("ns req %d from %08x\n", req, whom);
		}
        
        if (!(perm & PTE_P)) {
			cprintf("Invalid request from %08x: no argument page\n", whom);
			continue; // just leave it hanging...
		}

        if (req != NSREQ_OUTPUT)
        {
            cprintf("Invalid request %d to output env\n", req);
			continue; // just leave it hanging...
        }
        // cprintf("Start!");
        sys_transmit(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);
        // cprintf("Done!");
    }

}
