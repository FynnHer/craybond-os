/*
kernel/process/syscall.c
This file implements basic syscall handling for the kernel. It provides functions to handle
syscalls invoked from user space (EL0) to perform operations such as printing to the console.
Syscalls are invoked using the SVC (Supervisor Call) instruction, which triggers an exception
that is handled by the kernel.
*/
#include "syscall.h"
#include "console/kio.h"
#include "exception_handler.h"
#include "console/serial/uart.h"

void sync_el0_handler_c() {
    // We nned to inspect this call is what we think it is
    // we need to check if this is allowed, since not all syscalls are allowed from processes

    const char* x0;
    asm volatile ("mov %0, x0" : "=r"(x0));
    uint64_t *x1;
    asm volatile ("mov %0, x1" : "=r"(x1));
    uint64_t x2;
    asm volatile ("mov %0, x2" : "=r"(x2));
    uint64_t x8;
    asm volatile ("mov %0, x8" : "=r"(x8));

    uint64_t esr, elr, far;

    asm volatile ("mrs %0, elr_el1" : "=r"(elr));
    asm volatile ("mrs %0, daif" : "=r"(far));
    
    if (x8 == 3){
        printf_args_raw(x0,x1,x2);
    } else {
        handle_exception("UNEXPECTED EL0 EXCEPTION");
    }

    asm volatile ("eret");
}