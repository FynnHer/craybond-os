/*
kernel/process/kprocess_loader.c
This file implements the kernel process loader, which is responsible for creating
and managing kernel processes. It provides functions to create kernel processes,
relocate code, and set up their initial stack and registers.
*/
#include "kprocess_loader.h"
#include "console/kio.h"
#include "scheduler.h"
#include "proc_allocator.h"

process_t *create_kernel_process(void (*func)(), uint64_t code_size) {
    /*
    This function creates a new kernel process by allocating memory for its code segment,
    setting up its initial stack and registers. It returns a pointer to the newly created
    kernel process structure.
    Example usage: process_t* kproc = create_kernel_process(kernel_function, code_size);
    */
    process_t* proc = init_process();

    uint64_t stack_size = 0x1000;

    uint64_t stack = (uint64_t)alloc_proc_mem(stack_size, true);
    kprintf_raw("Stack size %h. Start %h", stack_size,stack);
    if (!stack) return 0;

    proc->sp = (stack + stack_size);

    proc->pc = (uint64_t)func;
    kprintf_raw("Kernel Process allocated with address at %h, stack at %h",proc->pc, proc->sp);
    proc->spsr = 0x3C5; // Set appropriate SPSR for EL1
    proc->state = READY;
    
    return proc;
}