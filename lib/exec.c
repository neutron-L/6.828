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

}

int execlp(const char *program, const char **arg0, ...)
{

}

int execv(const char *pathname, const char **argv)
{

}

int execvp(const char *program, const char **argv)
{

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

    struct Trapframe * tf = &thisenv->env_tf.tf_esp;
    tf->tf_esp = (uintptr_t)(argv_store - 2);

    return 0;
}