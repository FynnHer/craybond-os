/*
kernel/console/serial/uart.c
UART stands for Universal Asynchronous Receiver/Transmitter.
This file contains functions to initialize and use the UART for serial communication.
By writing to and reading from specific memory-mapped registers,
we can send and receive data over a serial interface.
*/
#include "console/serial/uart.h"
#include "ram_e.h"
#include "gic.h"

#define UART0_DR   (UART0_BASE + 0x00)
#define UART0_FR   (UART0_BASE + 0x18)
#define UART0_IBRD (UART0_BASE + 0x24)
#define UART0_FBRD (UART0_BASE + 0x28)
#define UART0_LCRH (UART0_BASE + 0x2C)
#define UART0_CR   (UART0_BASE + 0x30)

uint64_t get_uart_base(){
    return UART0_BASE;
}

void enable_uart() {
  /*
  This function initializes the UART by configuring its baud rate,
  line control settings, and enabling the transmitter and receiver.
  It writes specific values to the UART's memory-mapped registers
  to set it up for serial communication.
  Example usage: enable_uart(); would initialize the UART for use.
  */
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
  disable_interrupt();
  uart_raw_putc(c);
  enable_interrupt();
}

void uart_puts(const char *s) {
  disable_interrupt();
  uart_raw_puts(s);
  enable_interrupt();
}

void uart_raw_puts(const char *s){
  while (*s != '\0') {
    uart_raw_putc(*s);
    s++;
  }
}

void uart_puthex(uint64_t value) {
  disable_interrupt();
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
  enable_interrupt();
}
