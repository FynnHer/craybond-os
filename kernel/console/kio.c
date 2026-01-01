/*
kernel/console/kio.c
This file provides kernel I/O functions that utilize UART for serial communication.
It includes functions to output characters, strings, hexadecimal values,
and formatted strings to the console via UART.
*/
#include "kio.h"
#include "serial/uart.h"
#include "string.h"
#include "kconsole/kconsole.h"

void puts(const char *s){
    uart_raw_puts(s);
    kconsole_puts(s);
}

void putc(const char c){
    uart_putc(c);
    kconsole_putc(c);
}

void printf_args(const char *fmt, const uint64_t *args, uint32_t arg_count){
    asm volatile ("msr daifset, #2"); // Disable IRQs
    string s = string_format_args(fmt, args, arg_count);
    puts(s.data);
    putc('\n');
    asm volatile ("msr daifclr, #2"); // Enable IRQs
}