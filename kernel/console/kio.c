/*
kernel/console/kio.c
This file provides kernel I/O functions that utilize UART for serial communication.
It includes functions to output characters, strings, hexadecimal values,
and formatted strings to the console via UART.
*/
#include "kio.h"
#include "serial/uart.h"
#include "kstring.h"
#include "gic.h"
#include "ram_e.h"
#include "kconsole/kconsole.h"

static bool use_visual = true;

void puts(const char *s){
    uart_raw_puts(s);
    if (use_visual)
        kconsole_puts(s);
}

void putc(const char c){
    uart_raw_putc(c);
    if (use_visual)
        kconsole_putc(c);
}

void kprintf_args(const char *fmt, const uint64_t *args, uint32_t arg_count){
    /*
    IRQs are disabled during the execution of this function to prevent
    context switches that could interfere with the formatted output.
    This ensures that the output remains consistent and uninterrupted.
    Example usage: printf_args("Value: %h", &value, 1); would print the hexadecimal value.
    IRQ stands for Interrupt Request. This is a hardware signal sent to the processor to gain its attention.
    When an IRQ is received, the processor temporarily halts its current activities
    to execute a function called an interrupt handler or interrupt service routine (ISR).
    */
    disable_interrupt();
    kprintf_args_raw(fmt, args, arg_count);
    enable_interrupt();
}

void kprintf_args_raw(const char *fmt, const uint64_t *args, uint32_t arg_count) {
    kstring s = string_format_args(fmt, args, arg_count);
    puts(s.data);
    putc('\n');
    temp_free(s.data, 256);
}

void disable_visual() {
    use_visual = false;
}

void enable_visual() {
    use_visual = true;
}