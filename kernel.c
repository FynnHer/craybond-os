#include "uart.h"
#include "framebuffer.h"
#include "pci.h"


void kernel_main() {
  
  uart_puts("Hello craybond!\n");
  gpu_init();

}
