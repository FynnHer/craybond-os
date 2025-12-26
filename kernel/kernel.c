#include "console/kio.h"
#include "console/serial/uart.h"
#include "graph/graphics.h"
#include "pci.h"
#include "string.h"
#include "console/kconsole/kconsole.h"
#include "mmu.h"
#include "exception_handler.h"
#include "ram_e.h"
#include "gic.h"

void kernel_main() {

    printf("Kernel initializing...");

    enable_uart();

    printf("UART output enabled");

    size screen_size = {1024,768};

    printf("Preparing for draw");

    gpu_init(screen_size);

    string s = string_format("Hello. This is a test panic for %h", 0x0);

    printf("GPU initialized");

    printf("Device initialization finished");

    set_exception_vectors();

    printf("Exception vectors set");

    gic_init();

    printf("Interrupts init");

    timer_init(1000);

    printf("Test timer done");

    enable_interrupt();

    printf("Interrupts enabled");

    mmu_init();
    printf("MMU Mapped");

    printf("Kernel initialized successfully!");

    printf("Now we're writing a really long string, becuase why not? Let's see how the console handles it. This should wrap around multiple lines and still be perfectly readable. If everything works as expected, we should see this entire message displayed correctly on the screen without any issues. Let's add even more text to make sure we really test the limits of our console implementation. Here we go!");
}