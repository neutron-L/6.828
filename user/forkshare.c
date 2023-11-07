// Test sfork()
// child environment and grandchild environment write the same data segment
// but own private stack

#include <inc/string.h>
#include <inc/lib.h>


int global = 100;

void umain(int argc, char **argv)
{
    envid_t child;
    int autovar = 100;
    int oldglobal = global;
    cprintf("&global = %p\n", &global);

    child = sfork();
    
    if (child < 0)
        panic("umain: %e", child);
    else if (child == 0)
    {
        cprintf("Child process\n");
        
        int newglobal = global + 1;
        cprintf("Child modifies global variable: %d -> %d\n", global, newglobal);
        global = newglobal;
        cprintf("Parent cannot modify child auto variable: %d\n", autovar);
        while (global == newglobal)
            ;
        cprintf("Parent modifies global variable: %d -> %d\n", newglobal, global);
    }
    else
    {
        cprintf("Parent process\n");
        int newautovar = autovar - 1;
        cprintf("Parent modifies own auto variable: %d -> %d\n", autovar, newautovar);
        autovar = newautovar;
        while (global == oldglobal)
            ;
        cprintf("Child modifies global variable: %d -> %d\n", oldglobal, global);
        global--;
        cprintf("Child cannot modify parent auto variable: %d\n", autovar);
    }
}