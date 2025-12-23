#include "uart.h"
#include "framebuffer.h"
#include "pci.h"


void kernel_main() {

  enable_uart();
  
  uart_puts("Hello craybond!\n");
  uart_puts("Preparing for drawing...\n");

  gpu_init();

  gpu_clear(0x00FF00FF); // ARGB - Blue screen

  uart_puts("Square drawn\n");

}
