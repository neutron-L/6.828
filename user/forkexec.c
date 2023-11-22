// Fork and exec in child process.

#include <inc/lib.h>

void umain(int argc, char **argv)
{
    envid_t child;
    cprintf("Parent: %p\n", thisenv->env_id);

    child = fork();

    if (child < 0)
        panic("umain: %e", child);
    else if (child == 0)
    {
        cprintf("Child: %p\n", thisenv->env_id);
        cprintf("Child call exec hello\n");
        int r = execl("hello", "hello", NULL);
        cprintf("should not be here: %d\n", r);
    }
    else
    {
        cprintf("Parent process continue execute...\n");
    }
}
