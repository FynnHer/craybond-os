#include "framebuffer.h"
#include "uart.h"

#include "virtio_gpu_pci/virtio_gpu_pci_driver.h"
#include "ramfb_driver/ramfb_driver.h"

typedef enum {
    NONE,
    VIRTIO_GPU_PCI,
    RAMFB
} SupportedGPU;

SupportedGPU chosen_gpu;

void gpu_init() {
    if (vgp_init())
        chosen_gpu = VIRTIO_GPU_PCI;
    if (rfb_init())
        chosen_gpu = RAMFB;
    uart_puts("Selected and initialized GPU ");
    uart_puthex(chosen_gpu);
    uart_putc('\n');
}

void gpu_flush() {
    switch (chosen_gpu) {
        case VIRTIO_GPU_PCI:
            vgp_transfer_to_host();
            vgp_flush();
            break;
        default:
            break;
    }
}
void gpu_clear(uint32_t color) {
    switch (chosen_gpu) {
        case VIRTIO_GPU_PCI:
            vgp_clear(color);
            break;
        case RAMFB:
            rfb_clear(color);
        default:
            break;
    }
}