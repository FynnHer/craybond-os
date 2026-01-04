#include "syscalls.h"

void printf_args(const char *fmt, const uint64_t *args, uint32_t arg_count) {
    /*
    This function handles formatted output from user space by invoking the kernel's
    kprintf_args function. It takes a format string and an array of arguments,
    along with the count of arguments, and passes them to the kernel for printing.
    Example usage: printf_args("Value: %h", &value, 1); would print the hexadecimal value.
    */
    register uint64_t x8 asm("x8") = PRINTF_SYSCALL;

    asm volatile (
        "svc #3"
        :
        : "r"(x8)
        : "memory"
    );
}