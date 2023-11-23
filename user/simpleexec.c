// Fork and exec in child process.

#include <inc/lib.h>

const char * args[] = {
    "echo",
    "I call execv to change my program image\n",
    "this is a new executable file\n",
    "if I print these messages,",
    "it means you are successful",
    NULL
};

void umain(int argc, char **argv)
{
    envid_t child;
    cprintf("parent: %p\n", thisenv->env_id);
    cprintf("now call exec echo\n");

    int r = execv("echo", args);
    // int r = execl("hello", "hello", NULL);
    cprintf("should not be here: %d\n", r);
    // child = fork();

    // if (child < 0)
    //     panic("umain: %e", child);
    // else if (child == 0)
    // {
    //     cprintf("Child: %p\n", thisenv->env_id);
    //     cprintf("Child call exec %s\n", argv[1]);
    //     // int r = execv(argv[0], NULL);
    //     cprintf("Should not be here: %d\n", 0);
    // }
    // else
    // {
    //     cprintf("Parent process continue execute...\n");
    // }
}
