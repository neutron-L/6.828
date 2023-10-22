// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#include <kern/pmap.h>
#include <kern/env.h>

#define CMDBUF_SIZE 80 // enough for one VGA text line

struct Command
{
    const char *name;
    const char *desc;
    // return -1 to force monitor to exit
    int (*func)(int argc, char **argv, struct Trapframe *tf);
};

static struct Command commands[] = {
    {"help", "Display this list of commands", mon_help},
    {"kerninfo", "Display information about the kernel", mon_kerninfo},
    {"backtrace", "Displays all frames on the stack", mon_backtrace},
    {"showmappings", "Display the physical page mappings"
                     "(or lack thereof) that apply to a particular range of virtual/linear addresses"
                     " in the currently active address space.",
     mon_showmappings},
    {"update", "Update perm of the specified virtual page", mon_update},
    {"dp", "Update contents of the specified physical pages", mon_dump_ppages},
    {"dv", "Update contents of the specified virtual pages", mon_dump_vpages},
    {"continue", " Single-stepping", mon_single_stepping},
};

/***** Implementations of basic kernel monitor commands *****/

int mon_help(int argc, char **argv, struct Trapframe *tf)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(commands); i++)
        cprintf("%s - %s\n", commands[i].name, commands[i].desc);
    return 0;
}

int mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
    extern char _start[], entry[], etext[], edata[], end[];

    cprintf("Special kernel symbols:\n");
    cprintf("  _start                  %08x (phys)\n", _start);
    cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
    cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
    cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
    cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
    cprintf("Kernel executable memory footprint: %dKB\n",
            ROUNDUP(end - entry, 1024) / 1024);
    return 0;
}

int mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
    // Your code here.
    uint32_t ebp, eip;
    uint32_t *args;
    struct Eipdebuginfo info;

    cprintf("Stack backtrace:\n");
    ebp = read_ebp();
    while (ebp)
    {
        eip = *((uint32_t *)ebp + 1);
        args = (uint32_t *)ebp + 2;
        cprintf("ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n", ebp, eip, args[0], args[1], args[2], args[3], args[4]);
        if (debuginfo_eip(eip, &info) == -1)
            return -1;
        cprintf("         %s:%d: %.*s+%d\n", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, eip - info.eip_fn_addr);
        ebp = *(uint32_t *)ebp;
    }
    return 0;
}

static uint32_t stoi(char *str, uintptr_t *res)
{
    int base = 10;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
    {
        str += 2;
        base = 16;
    }
    uintptr_t temp = 0;
    int32_t delta;
    while (*str)
    {
        delta = -1;
        if (*str >= '0' && *str <= '9')
            delta = *str - '0';
        else if (base == 16)
        {
            if (*str >= 'a' && *str <= 'f')
                delta = *str - 'a' + 10;
            else if (*str >= 'A' && *str <= 'F')
                delta = *str - 'A' + 10;
        }
        if (delta == -1)
        {
            cprintf("%c %d\n", *str, base);
            return -1;
        }
        temp = temp * base + delta;
        ++str;
    }

    *res = temp;
    return 0;
}

int mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
    if (argc != 2 && argc != 3)
    {
        cprintf("usage: showmappings start_va <end_va>\n");
        return 1;
    }
    uintptr_t va1;
    uintptr_t va2;
    if (stoi(argv[1], &va1) || (argc == 3 && stoi(argv[2], &va2)))
    {
        cprintf("stoi: invalide number string %s %s", argv[1], argc == 3 ? argv[2] : "");
        return -1;
    }
    if (argc == 2)
        va2 = va1;
    showmappings(kern_pgdir, va1, va2);
    return 0;
}

int mon_update(int argc, char **argv, struct Trapframe *tf)
{
    if (argc != 3)
    {
        cprintf("usage: update va perm\n");
        return 1;
    }
    uintptr_t va;
    uintptr_t perm;
    if (stoi(argv[1], &va) || stoi(argv[2], &perm))
    {
        cprintf("stoi: invalide number string %s %s", argv[1], argv[2]);
        return -1;
    }
    update_perm(kern_pgdir, va, perm);

    return 0;
}

int mon_dump_ppages(int argc, char **argv, struct Trapframe *tf)
{
    if (argc != 2 && argc != 3)
    {
        cprintf("usage: dp start_va <end_va>\n");
        return 1;
    }
    uintptr_t addr1;
    uintptr_t addr2;
    if (stoi(argv[1], &addr1) || (argc == 3 && stoi(argv[2], &addr2)))
    {
        cprintf("stoi: invalide number string %s %s", argv[1], argc == 3 ? argv[2] : "");
        return -1;
    }
    if (argc == 2)
        addr2 = addr1;
    dump_pages(kern_pgdir, addr1, addr2, 0);
    return 0;
}

int mon_dump_vpages(int argc, char **argv, struct Trapframe *tf)
{
    if (argc != 2 && argc != 3)
    {
        cprintf("usage: dv start_va <end_va>\n");
        return 1;
    }
    uintptr_t addr1;
    uintptr_t addr2;
    if (stoi(argv[1], &addr1) || (argc == 3 && stoi(argv[2], &addr2)))
    {
        cprintf("stoi: invalide number string %s %s", argv[1], argc == 3 ? argv[2] : "");
        return -1;
    }
    if (argc == 2)
        addr2 = addr1;
    dump_pages(kern_pgdir, addr1, addr2, 1);
    return 0;
}

int mon_single_stepping(int argc, char **argv, struct Trapframe *tf)
{
    cprintf("ip: %x\n", tf->tf_eip);
    env_pop_tf(tf);
    return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
    int argc;
    char *argv[MAXARGS];
    int i;

    // Parse the command buffer into whitespace-separated arguments
    argc = 0;
    argv[argc] = 0;
    while (1)
    {
        // gobble whitespace
        while (*buf && strchr(WHITESPACE, *buf))
            *buf++ = 0;
        if (*buf == 0)
            break;

        // save and scan past next arg
        if (argc == MAXARGS - 1)
        {
            cprintf("Too many arguments (max %d)\n", MAXARGS);
            return 0;
        }
        argv[argc++] = buf;
        while (*buf && !strchr(WHITESPACE, *buf))
            buf++;
    }
    argv[argc] = 0;

    // Lookup and invoke the command
    if (argc == 0)
        return 0;
    for (i = 0; i < ARRAY_SIZE(commands); i++)
    {
        if (strcmp(argv[0], commands[i].name) == 0)
            return commands[i].func(argc, argv, tf);
    }
    cprintf("Unknown command '%s'\n", argv[0]);
    return 0;
}

void monitor(struct Trapframe *tf)
{
    char *buf;

    cprintf("Welcome to the JOS kernel monitor!\n");
    cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
