/*
kernel/exception_handler.c
This file implements basic exception handling for the kernel. It sets up exception vectors,
handles synchronous exceptions, FIQs, and errors by reading system registers and outputting relevant information.
It also provides panic functions to halt the system in case of critical errors.
FIQ stands for Fast Interrupt Request.
*/
#include "exception_handler.h"
#include "console/serial/uart.h"
#include "kstring.h"
#include "console/kio.h"
#include "mmu.h"
#include "graph/graphics.h"

void set_exception_vectors(){
    /*
    This function sets the exception vector base address by writing to the VBAR_EL1 system register.
    The exception vectors are defined in the exception_vectors array.
    VBAR_EL1 - Vector Base Address Register for Exception Level 1
    Vectors are used to handle different types of exceptions and interrupts.
    */
    extern char exception_vectors[];
    kprintf("Exception vectors setup at %h", (uint64_t)&exception_vectors);
    asm volatile ("msr vbar_el1, %0" :: "r"(exception_vectors));
}

void handle_exception(const char* type) {
    /*
    This function handles exceptions by reading relevant system registers
    such as ESR_EL1 (Exception Syndrome Register), ELR_EL1 (Exception Link Register),
    and FAR_EL1 (Fault Address Register). It then formats this information into a string
    and triggers a kernel panic with the details.
    ESR_EL1 - Provides information about the cause of the exception.
    ELR_EL1 - Contains the address of the instruction that caused the exception.
    FAR_EL1 - Contains the address that caused a data abort exception.
    */
    uint64_t esr, elr, far;
    asm volatile ("mrs %0, esr_el1" : "=r"(esr));
    asm volatile ("mrs %0, elr_el1" : "=r"(elr));
    asm volatile ("mrs %0, far_el1" : "=r"(far));

    disable_visual();

    kstring s = string_format("%s \nESR_EL1: %h\nELR_EL1: %h\n,FAR_EL1: %h",(uint64_t)string_l(type).data,esr,elr,far);
    panic(s.data);
}

void sync_el1_handler(){ handle_exception("SYNC EXCEPTION"); }

void fiq_el1_handler(){ handle_exception("FIQ EXCEPTION\n"); }

void error_el1_handler(){ handle_exception("ERROR EXCEPTION\n"); }

void panic(const char* panic_msg) {
    uart_raw_puts("*** CRAYON DOESN'T DRAW ANYMORE ***");
    uart_raw_puts(panic_msg);
    uart_raw_puts("System Halted");
    while (1);
}

void panic_with_info(const char* msg, uint64_t info) {
    gpu_clear(0x0000FF);
    uint32_t scale = 3;
    uint32_t size = gpu_get_char_size(scale);
    kstring s = string_format("CRAYON NOT CRAYING\n%s\nError code: %h\nSystem Halted", (uint64_t)msg, info);
    gpu_draw_string(s, (point){10,10}, scale, 0xFFFFFFFF);
    uart_raw_puts("*** CRAYON DOESN'T DRAW ANYMORE ***");
    uart_raw_puts(msg);
    uart_raw_putc('\n');
    uart_raw_puts("Additional info: ");
    uart_puthex(info);
    uart_raw_putc('\n');
    uart_raw_puts("System Halted");
    while (1);
}