// Test Conversation between parent and child environment
// Contributed by Varun Agrawal at Stony Brook

#include <inc/lib.h>


void
umain(int argc, char **argv)
{
	envid_t who;

	if ((who = fork()) == 0) {
		// Child
		int r = ipc_recv(&who, 0, 0);
		cprintf("%x got value: %x\n", who, r);
		
		// ipc_send(who, 0, TEMP_ADDR_CHILD, PTE_P | PTE_W | PTE_U);
		return;
	}

	// Parent
    int val = 0xdeadbeef;
	ipc_send(who, val, 0, 0);

	// ipc_recv(&who, TEMP_ADDR, 0);
	cprintf("%x sent value to %x: %d\n", thisenv->env_id, who, val);
	return;
}
