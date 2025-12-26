#include "uart.h"
#include "console/serial/uart.h"
#include "ram_e.h"

#define UART0_DR   (UART0_BASE + 0x00)  
#define UART0_FR   (UART0_BASE + 0x18)
#define UART0_IBRD (UART0_BASE + 0x24)
#define UART0_FBRD (UART0_BASE + 0x28)
#define UART0_LCRH (UART0_BASE + 0x2C)
#define UART0_CR   (UART0_BASE + 0x30)

uint64_t get_uart_base() {
  return UART0_BASE;
}

void enable_uart() {
  write32(UART0_CR, 0x0); // Disable UART

  write32(UART0_IBRD, 1); // Set integer baud rate divisor
  write32(UART0_FBRD, 40); // Set fractional baud rate divisor

  write32(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6)); // 8 bits, no parity, one stop bit, FIFOs enabled

  write32(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9)); // Enable UART, TX and RX
}

void uart_raw_putc(const char c) {
  while (read32(UART0_FR) & (1 << 5)); // Wait until TX FIFO is not full
  write32(UART0_DR, c);
}

void uart_putc(const char c) {
  asm volatile ("msr daifset, #2"); // Disable IRQs
  uart_raw_putc(c);
  asm volatile ("msr daifclr, #2"); // Enable IRQs
}

void uart_puts(const char *s){
  asm volatile ("msr daifset, #2"); // Disable IRQs
  while (*s != '\0') {
    uart_raw_putc(*s);
    s++;
  }
  asm volatile ("msr daifclr, #2"); // Enable IRQs
}

void uart_puthex(uint64_t value) {
  asm volatile ("msr daifset, #2"); // Disable IRQs
    const char hex_chars[] = "0123456789ABCDEF";
    bool started = false;
    uart_raw_putc('0');
    uart_raw_putc('x');
    for (int i = 60; i >= 0; i -= 4) {
        char curr_char = hex_chars[(value >> i) & 0xF];
        if (started || curr_char != '0' || i == 0) {
            started = true;
            uart_putc(curr_char);
        }
    }
    asm volatile ("msr daifclr, #2"); // Enable IRQs
}
