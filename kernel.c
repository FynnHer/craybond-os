#include "uart.h"
#include "gpu/gpu.h"
#include "pci.h"


void kernel_main() {

  enable_uart();
  
  uart_puts("Hello craybond!\n");
  uart_puts("Preparing for drawing...\n");

  gpu_init();

  gpu_clear(0x00FF00); // Clear screen to green

  uart_puts("Square drawn\n");

}
