#include <inc/lib.h>
#include <inc/elf.h>

extern volatile pte_t uvpt[];
extern volatile pde_t uvpd[];

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

int execlp(const char *program, const char *arg0, ...)
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

    int i;
    char * binary = (char *)UTEMP;
    for (i = 0; i < stat.st_size; i += PGSIZE)
    {
        if ((r = sys_page_alloc(thisenv->env_id, (void *)(binary + i), PTE_P | PTE_U | PTE_W)) < 0)
            goto bad;

        // 读取
        if ((r = readn(fd, (void *)(binary + i), MIN(PGSIZE, stat.st_size - i))) < 0)
            goto bad;
    }
    close(fd);
    sys_execv(argv);

    return -1;
bad:
    // unmap memory
    while (i)
    {
        i -= PGSIZE;
        sys_page_unmap(thisenv->env_id, binary + i);
    }
    close(fd);
    return -1;
}

int execvp(const char *program, const char **argv)
{
    // 根据进程当前路径，构造可执行文件的完整路径

    // 调用execv
}
