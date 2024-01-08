/* See COPYRIGHT for copyright information. */

#include <inc/assert.h>
#include <inc/elf.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/sched.h>
#include <kern/syscall.h>
#include <kern/time.h>
#include <kern/trap.h>
#include <kern/e1000.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void sys_cputs(const char* s, size_t len)
{
    // Check that the user has permission to read memory [s, s+len).
    // Destroy the environment if not.

    // LAB 3: Your code here.
    user_mem_assert(curenv, s, len, PTE_U);

    // Print the string supplied by the user.
    cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int sys_cgetc(void)
{
    return cons_getc();
}

// Returns the current environment's envid.
static envid_t sys_getenvid(void)
{
    return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int sys_env_destroy(envid_t envid)
{
    int         r;
    struct Env* e;

    if ((r = envid2env(envid, &e, 1)) < 0)
        return r;
    if (e == curenv)
        cprintf("[%08x] exiting gracefully\n", curenv->env_id);
    else
        cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
    env_destroy(e);
    return 0;
}

// Deschedule current environment and pick a different one to run.
static void sys_yield(void)
{
    sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t sys_exofork(void)
{
    // Create the new environment with env_alloc(), from kern/env.c.
    // It should be left as env_alloc created it, except that
    // status is set to ENV_NOT_RUNNABLE, and the register set is copied
    // from the current environment -- but tweaked so sys_exofork
    // will appear to return 0.

    // LAB 4: Your code here.
    struct Env* env = NULL;
    int         ret;

    ret = env_alloc(&env, curenv->env_id);
    if (ret != 0)
        return ret;
    memcpy(&env->env_tf, &curenv->env_tf, sizeof(struct Trapframe));
    env->env_status             = ENV_NOT_RUNNABLE;
    env->env_tf.tf_regs.reg_eax = 0;

    return env->env_id;
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int sys_env_set_status(envid_t envid, int status)
{
    // Hint: Use the 'envid2env' function from kern/env.c to translate an
    // envid to a struct Env.
    // You should set envid2env's third argument to 1, which will
    // check whether the current environment has permission to set
    // envid's status.

    // LAB 4: Your code here.
    struct Env* env;
    int         ret;

    ret = envid2env(envid, &env, 1);
    if (ret)
        return ret;
    env->env_status = status;
    return 0;
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3), interrupts enabled, and IOPL of 0.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int sys_env_set_trapframe(envid_t envid, struct Trapframe* tf)
{
    // LAB 5: Your code here.
    // Remember to check whether the user has supplied us with a good
    // address!
    // panic("sys_env_set_trapframe not implemented");
    struct Env* env;

    if (envid2env(envid, &env, 1))
        return -E_BAD_ENV;
    user_mem_assert(
        curenv, (const void*)tf, sizeof(struct Trapframe), PTE_P | PTE_U);

    env->env_tf = *tf;
    env->env_tf.tf_cs |= 0x3;
    env->env_tf.tf_eflags &= ~FL_IOPL_MASK;
    env->env_tf.tf_eflags |= FL_IOPL_0 | FL_IF;

    return 0;
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int sys_env_set_pgfault_upcall(envid_t envid, void* func)
{
    // LAB 4: Your code here.
    struct Env* env;
    if (envid2env(envid, &env, 1))
        return -E_BAD_ENV;
    user_mem_assert(env, func, sizeof(uintptr_t), PTE_P | PTE_U);
    env->env_pgfault_upcall = func;
    return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int sys_page_alloc(envid_t envid, void* va, int perm)
{
    // Hint: This function is a wrapper around page_alloc() and
    //   page_insert() from kern/pmap.c.
    //   Most of the new code you write should be to check the
    //   parameters for correctness.
    //   If page_insert() fails, remember to free the page you
    //   allocated!

    // LAB 4: Your code here.
    struct Env* env;
    if (envid2env(envid, &env, 1))
        return E_BAD_ENV;
    page_remove(env->env_pgdir, va);
    if ((uintptr_t)va >= UTOP || ((uintptr_t)va % PGSIZE))
        return -E_INVAL;
    if (!(perm & PTE_U) || !(perm & PTE_P) ||
        (perm & ~(PTE_U | PTE_P | PTE_W | PTE_AVAIL)))
        return -E_INVAL;

    struct PageInfo* pp = page_alloc(ALLOC_ZERO);
    if (!pp || page_insert(env->env_pgdir, pp, va, perm))
        return -E_NO_MEM;
    return 0;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int sys_page_map(
    envid_t srcenvid,
    void*   srcva,
    envid_t dstenvid,
    void*   dstva,
    int     perm)
{
    // Hint: This function is a wrapper around page_lookup() and
    //   page_insert() from kern/pmap.c.
    //   Again, most of the new code you write should be to check the
    //   parameters for correctness.
    //   Use the third argument to page_lookup() to
    //   check the current permissions on the page.

    // LAB 4: Your code here.
    struct Env *srcenv, *dstenv;
    if (envid2env(srcenvid, &srcenv, 1) || envid2env(dstenvid, &dstenv, 1))
        return -E_BAD_ENV;

    if ((uintptr_t)srcva >= UTOP || ((uintptr_t)srcva % PGSIZE) ||
        (uintptr_t)dstva >= UTOP || ((uintptr_t)dstva % PGSIZE))
        return -E_INVAL;

    struct PageInfo* pp;
    pte_t*           srcpte;

    pp = page_lookup(srcenv->env_pgdir, srcva, &srcpte);
    if (!pp)
        return -E_INVAL;
    if (!(perm & PTE_U) || !(perm & PTE_P) || (perm & ~PTE_SYSCALL))
        return -E_INVAL;

    if ((perm & PTE_W) && !((*srcpte) & PTE_W))
        return -E_INVAL;

    if (page_insert(dstenv->env_pgdir, pp, dstva, perm))
        return -E_NO_MEM;
    return 0;
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int sys_page_unmap(envid_t envid, void* va)
{
    // Hint: This function is a wrapper around page_remove().

    // LAB 4: Your code here.
    struct Env* env;
    int         ret;

    ret = envid2env(envid, &env, 1);
    if (ret)
        return ret;
    page_remove(env->env_pgdir, va);
    return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void* srcva, unsigned perm)
{
    // LAB 4: Your code here.
    // panic("sys_ipc_try_send not implemented");

    struct Env* env;
    int         r;
    uintptr_t   va = (uintptr_t)srcva;

    // check params
    if ((r = envid2env(envid, &env, 0)))
        return -E_BAD_ENV;
    if (!env->env_ipc_recving)
        return -E_IPC_NOT_RECV;

    if (va < UTOP) {
        pte_t* pte;

        if (PGOFF(srcva))
            return -E_INVAL;
        if (!(perm & PTE_U) || !(perm & PTE_P) || (perm & ~PTE_SYSCALL))
            return -E_INVAL;
        pte = pgdir_walk(curenv->env_pgdir, srcva, 0);

        if (!pte || !((*pte) & PTE_P))
            return -E_INVAL;
        if (!(*pte & PTE_W) && perm & PTE_W)
            return -E_INVAL;

        // transfer page
        if ((uintptr_t)env->env_ipc_dstva < UTOP) {
            assert(!PGOFF(env->env_ipc_dstva));
            struct PageInfo* pp;

            pp = page_lookup(curenv->env_pgdir, srcva, NULL);
            assert(pp);

            if (page_insert(env->env_pgdir, pp, env->env_ipc_dstva, perm))
                return -E_NO_MEM;

            env->env_ipc_perm = perm;
        }
    }

    env->env_ipc_recving = 0;
    env->env_ipc_from    = curenv->env_id;
    env->env_ipc_value   = value;

    env->env_status = ENV_RUNNABLE;

    return 0;
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int sys_ipc_recv(void* dstva)
{
    // LAB 4: Your code here.
    // panic("sys_ipc_recv not implemented");
    if ((uintptr_t)dstva < UTOP && PGOFF(dstva))
        return -E_INVAL;
    curenv->env_ipc_dstva = dstva;

    curenv->env_ipc_recving = 1;
    curenv->env_status      = ENV_NOT_RUNNABLE;

    return 0;
}

//
// 重置栈的内容，重新设置参数
//
static int reset_user_stack(const char** argv)
{
    size_t     string_size;
    int        argc, i, r;
    char*      string_store;
    uintptr_t* argv_store;

    // 1. 计算参数字符串所占空间大小
    string_size = 0;
    for (argc = 0; argv[argc] != NULL; ++argc)
        string_size += strlen(argv[argc]) + 1;

    string_store = USTACKTOP - string_size;
    argv_store   = (uintptr_t*)ROUNDDOWN(string_store, 4) - 4 * (argc + 1);
    // 判断栈的空间是否足够容纳参数
    if ((uintptr_t)(argv_store - 2) <= USTACKTOP - PGSIZE)
        return -E_NO_MEM;

    // 2. 拷贝argv参数到用户栈顶
    for (int i = 0; i < argc; ++i) {
        argv_store[i] = string_store;
        strcpy(string_store, argv[i]);
        string_store += strlen(argv[i]) + 1;
    }
    argv_store[argc] = NULL;
    argv_store[-1]   = argv_store;
    argv_store[-2]   = argc;

    curenv->env_tf.tf_esp = (uintptr_t)(argv_store - 2);

    return 0;
}

static int sys_execv(const char** argv)
{
    int   r;
    char* binary = (char*)UTEMP;

    // 设置当前程序状态为可运行
    curenv->env_status = ENV_RUNNABLE;

    // 读取新的程序段
    struct Proghdr *ph, *eph;
    struct Elf*     elfhdr;

    elfhdr = (struct Elf*)binary;

    if (elfhdr->e_magic != ELF_MAGIC)
        panic("sys_execv: %e", -E_INVAL);

    ph  = (struct Proghdr*)((uint8_t*)elfhdr + elfhdr->e_phoff);
    eph = ph + elfhdr->e_phnum;

    // 重置用户栈
    reset_user_stack(argv);
    // 重置ip
    curenv->env_tf.tf_eip = elfhdr->e_entry;

    // unmap old memory
    // 保留文件相关的内存区
    uintptr_t        va = USTABDATA;
    struct PageInfo* pp;
    pte_t*           pte;
    while (va < 0xD0000000) {
        if (va < UTEMP || va >= UTEXT) {
            pp = page_lookup(curenv->env_pgdir, va, &pte);
            if (pp && ((*pte) & PTE_U) && (r = sys_page_unmap(0, (void*)va)))
                return r;
        }

        va += PGSIZE;
    }

    uintptr_t pva;
    size_t    memsz;
    int       off, perm, pages;

    for (; ph < eph; ++ph) {
        if (ph->p_type != ELF_PROG_LOAD)
            continue;
        pva   = ph->p_va;
        va    = ROUNDDOWN(pva, PGSIZE);
        memsz = ph->p_memsz;
        perm  = PTE_P | PTE_U;
        if (ph->p_flags & ELF_PROG_FLAG_WRITE)
            perm |= PTE_W;

        // 分配内存 拷贝二进制文件内容
        // 必须映射为可写，即使是代码段，否则无法写入初始内容
        pages = ROUNDUP(memsz + PGOFF(pva), PGSIZE) >> PGSHIFT;

        for (int i = 0; i < pages; ++i) {
            if ((r = sys_page_alloc(
                     0, (void*)(va + (i << PGSHIFT)), PTE_P | PTE_U | PTE_W)))
                panic("sys_execv: %e", r);
        }
        memmove((void*)pva, binary + ph->p_offset, ph->p_filesz);

        // 修改该段的perm
        for (int i = 0; i < pages; ++i) {
            if ((r = sys_page_map(
                     0, (void*)(va + (i << PGSHIFT)), 0,
                     (void*)(va + (i << PGSHIFT)), perm)))
                panic("sys_execv: %e", r);
        }
    }

    // 清理binary文件内容
    va = UTEMP;
    while (va < UTEXT) {
        pp = page_lookup(curenv->env_pgdir, va, &pte);
        if (pp && ((*pte) & PTE_U) && (r = sys_page_unmap(0, (void*)va)))
            return r;
        va += PGSIZE;
    }

    return 0;
}

// Return the current time.
static int sys_time_msec(void)
{
    // LAB 6: Your code here.
    // panic("sys_time_msec not implemented");
    return time_msec();
}

static int sys_transmit(void* pkt, uint32_t len)
{
    // check memory
    // cprintf("check mem in transmit\n");
    user_mem_assert(curenv, pkt, len, PTE_U);
    // cprintf("done\n");

    while (e1000_transmit(pkt, len) == -1) {
        continue;
    }

    return 0;
}


static int sys_receive(void* pkt, uint32_t * len)
{
    // check memory
    // cprintf("check mem in receive\n");
 
    user_mem_assert(curenv, ROUNDDOWN(pkt, PGSIZE), PGSIZE, PTE_U | PTE_W);
    user_mem_assert(curenv, len, 4, PTE_U | PTE_W);
    // cprintf("done\n");

    return e1000_receive(pkt, len);
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t syscall(
    uint32_t syscallno,
    uint32_t a1,
    uint32_t a2,
    uint32_t a3,
    uint32_t a4,
    uint32_t a5)
{
    // Call the function corresponding to the 'syscallno' parameter.
    // Return any appropriate return value.
    // LAB 3: Your code here.

    // panic("syscall not implemented");

    switch (syscallno) {
        case SYS_cputs: sys_cputs((const char*)a1, a2); return a2;
        case SYS_cgetc: return sys_cgetc();
        case SYS_getenvid: return sys_getenvid();
        case SYS_env_destroy: sys_env_destroy(a1); return 0;

        case SYS_page_alloc: return sys_page_alloc(a1, (void*)a2, a3);
        case SYS_page_map:
            return sys_page_map(a1, (void*)a2, a3, (void*)a4, a5);
        case SYS_page_unmap: return sys_page_unmap(a1, (void*)a2);
        case SYS_exofork: return sys_exofork();
        case SYS_execv: return sys_execv((const char**)a1);
        case SYS_env_set_status: return sys_env_set_status(a1, a2);
        case SYS_env_set_trapframe: return sys_env_set_trapframe(a1, (void*)a2);
        case SYS_env_set_pgfault_upcall:
            return sys_env_set_pgfault_upcall(a1, (void*)a2);
        case SYS_yield: sys_yield(); return 0;
        case SYS_ipc_try_send: return sys_ipc_try_send(a1, a2, (void*)a3, a4);
        case SYS_ipc_recv: return sys_ipc_recv((void*)a1);
        case SYS_time_msec: return sys_time_msec();
        case SYS_transmit: return sys_transmit((void*)a1, a2);
        case SYS_receive: return sys_receive((void*)a1, a2);
        default: return -E_INVAL;
    }
}
