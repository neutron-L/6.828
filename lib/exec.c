#include <inc/lib.h>
#include <inc/elf.h>

extern volatile pte_t uvpt[];
extern volatile pde_t uvpd[];

// Helper functions for spawn.
static int reset_stack(const char **argv);
static int map_segment(uintptr_t va, size_t memsz,
                       int fd, size_t filesz, off_t fileoffset, int perm);

int execl(const char *pathname, const char *arg0, ...)
{
    int argc = 0;
    va_list vl;
    va_start(vl, arg0);
    while (va_arg(vl, void *))
        ++argc;
    va_end(vl);

    const char *argv[argc + 2];
    va_start(vl, arg0);
    argv[0] = arg0;
    argv[argc + 1] = NULL;
    for (int i = 1; i <= argc; ++i)
        argv[i] = va_arg(vl, const char *);
    va_end(vl);

    return execv(pathname, argv);
}

int execlp(const char *program, const char **arg0, ...)
{
    int argc = 0;
    va_list vl;
    va_start(vl, arg0);
    while (va_arg(vl, void *))
        ++argc;
    va_end(vl);

    const char *argv[argc + 2];
    va_start(vl, arg0);
    argv[0] = arg0;
    argv[argc + 1] = NULL;
    for (int i = 1; i <= argc; ++i)
        argv[i] = va_arg(vl, const char *);
    va_end(vl);

    return execvp(program, argv);
}

int execv(const char *pathname, const char **argv)
{
    int r, fd;
    struct Stat stat;
    // 文件最大为MAXFILESIZE，比4MB稍大，假设文件不大于4MB
    // 将文件内容读取并映射到UTEMP起始处
    if ((r = open(pathname, O_RDONLY)) < 0)
        return r;
    fd = r;

    if ((r = fstat(fd, &stat)) < 0)
        return r;
    if ((r = sys_page_alloc(thisenv->env_id, (void *)UTEMP, PTE_P | PTE_U)) < 0)
        return r;
    
    // 读取
    if ((r = readn(fd, (void *)UTEMP, stat.st_size)) < 0)
        return r;
    sys_execv(argv);
    
    return -1;
}

int execvp(const char *program, const char **argv)
{
    // 根据进程当前路径，构造可执行文件的完整路径

    // 调用execv
}

//
// 重置栈的内容，重新设置参数
//
static int reset_stack(const char **argv)
{
    size_t string_size;
    int argc, i, r;
    char *string_store;
    uintptr_t *argv_store;

    // 1. 计算参数字符串所占空间大小
    string_size = 0;
    for (argc = 0; argv[argc] != NULL; ++argc)
        string_size += strlen(argv[argc]) + 1;

    string_store = USTACKTOP - string_size;
    argv_store = (uintptr_t *)ROUNDDOWN(string_store, 4) - 4 * (argc + 1);
    // 判断栈的空间是否足够容纳参数
    if ((uintptr_t)(argv_store - 2) <= USTACKTOP - PGSIZE)
        return -E_NO_MEM;

    // 2. 拷贝argv参数到用户栈顶
    for (int i = 0; i < argc; ++i)
    {
        argv_store[i] = string_store;
        strcpy(string_store, argv[i]);
        string_store += strlen(argv[i]) + 1;
    }
    argv_store[argc] = NULL;
    argv_store[-1] = argv_store;
    argv_store[-2] = argc;

    struct Trapframe *tf = &thisenv->env_tf.tf_esp;
    tf->tf_esp = (uintptr_t)(argv_store - 2);

    return 0;
}