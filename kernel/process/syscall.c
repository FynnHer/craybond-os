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
    asm volatile ("mov %0, x0" : "=r"(x0)); // Get syscall argument from x0
    uint64_t* x1;
    asm volatile ("mov %0, x1" : "=r"(x1)); // Get syscall argument from x1
    uint64_t x2;
    asm volatile ("mov %0, x2" : "=r"(x2)); // Get syscall argument from x2
    uint64_t x8;
    asm volatile ("mov %0, x8" : "=r"(x8)); // Get syscall number from x8

    uint64_t esr, elr, far;

    //asm volatile ("mrs %0, esr_el1" : "=r"(esr));
    asm volatile ("mrs %0, elr_el1" : "=r"(elr));
    
    //uint32_t ec = (esr >> 26) & 0x3F; // Exception Class

    //asm volatile ("mrs %0, far_el1" : "=r"(far));
    //elr -= 4; // Adjust ELR to point to the instruction after SVC
    //uint32_t instr = *(uint32_t *)elr;

    //uint8_t svc_num = (instr >> 5) & 0xFFFF; // Extract SVC number from instruction
    uart_raw_puts("Hello");
    // uart_puts("Hello");

    if (x8 == 3) { // Syscall number 3: Print
        // x0: format string
        // x1: first argument
        // x2: number of arguments
        printf_args_raw(x0, x1, x2);
    }

    asm volatile ("eret"); // Return from exception

    //while (1) {}

    //asm volatile ("mrs %0, elr_el1" : "=r"(elr));
    //asm volatile ("mrs %0, far_el1" : "=r"(far));
    //elr -= 4;
    //uint32_t instr = *(uint32_t *)elr;
    //uint8_t svc_num = (instr >> 5) & 0xFFFF; // Extract SVC number from instruction
    //printf("Call is %i, from %h [%h]. X0: %h" , svc_num, elr, instr, x0);
    //if (svc_num == 3) {
    //    register const char *msg_asm("x0");
    //    uart_puts(msg);
    //    printf("Well print here");
    //}
    //while (1) {}
    

}