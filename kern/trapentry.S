/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
#define HANDLER(num) num ## _handler

TRAPHANDLER_NOEC(HANDLER(divide), T_DIVIDE)
TRAPHANDLER_NOEC(HANDLER(debug), T_DEBUG)
TRAPHANDLER_NOEC(HANDLER(nmi), T_NMI)
TRAPHANDLER_NOEC(HANDLER(breakpoint), T_BRKPT)

TRAPHANDLER_NOEC(HANDLER(overflow), T_OFLOW)
TRAPHANDLER_NOEC(HANDLER(bound), T_BOUND)
TRAPHANDLER_NOEC(HANDLER(illegal), T_ILLOP)
TRAPHANDLER_NOEC(HANDLER(device), T_DEVICE)

TRAPHANDLER(HANDLER(dbfault), T_DBLFLT)

TRAPHANDLER_NOEC(HANDLER(tss), T_TSS)
TRAPHANDLER_NOEC(HANDLER(segment), T_SEGNP)

TRAPHANDLER_NOEC(HANDLER(stack), T_STACK)
TRAPHANDLER(HANDLER(gpflt), T_GPFLT)
TRAPHANDLER(HANDLER(pgflt), T_PGFLT)


TRAPHANDLER_NOEC(HANDLER(fperr), T_FPERR)
TRAPHANDLER(HANDLER(align), T_ALIGN)
TRAPHANDLER_NOEC(HANDLER(mchk), T_MCHK)
TRAPHANDLER_NOEC(HANDLER(smiderr), T_SIMDERR)

.data
    .align 4
    .global trap_table
trap_table:
    .long HANDLER(divide)
    .long HANDLER(debug)
    .long HANDLER(nmi)
    .long HANDLER(breakpoint)

    .long HANDLER(overflow)
    .long HANDLER(bound)
    .long HANDLER(illegal)
    .long HANDLER(device)

    .long HANDLER(dbfault)
    .long 0
    .long HANDLER(tss)
    .long HANDLER(segment)
    
    .long HANDLER(stack)
    .long HANDLER(gpflt)
    .long HANDLER(pgflt)
    .long 0

    .long HANDLER(fperr)
    .long HANDLER(align)
    .long HANDLER(mchk)
    .long HANDLER(smiderr)


/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
    pushl %ds
    pushl %es
    pushal
    movw $(GD_KD), %ax
    movw %ax, %ds
    movw %ax, %es
    pushl %esp
    call trap
