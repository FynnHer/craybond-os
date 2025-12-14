volatile unsigned int * const UART0DR = (unsigned int *) 0x9000000;

void put_uart(const char *s) {
  while(*s != '\0') {
    *UART0DR = (unsigned int)(*s);
    s++;
  }
}

void kernel_main() {
  put_uart("Hello craybond!\n");
}
