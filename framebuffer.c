#include "framebuffer.h"
#include "uart.h"
#include "mmio.h"
#include "pci.h"

#define VIRTIO_MMIO_STATUS 0x70


#define VIRTIO_STATUS_RESET 0x0
#define VIRTIO_STATUS_ACKNOWLEDGE   0x01
#define VIRTIO_STATUS_DRIVER       0x02
#define VIRTIO_STATUS_DRIVER_OK     0x04
#define VIRTIO_STATUS_FEATURES_OK   0x08
#define VIRTIO_STATUS_FAILED       0x80

#define VIRTIO_GPU_CMD_GET_DISPLAY_INFO 0x100


#define PCI_BAR4 0x20
#define PCI_BAR5 0x24

uint64_t setup_gpu_bars(uint64_t base) {
  uart_puts("Setting up GPU BAR4 and BAR5...\n");

  uart_puts("Writing BAR4 and BAR5...\n");

  mmio_write(base + 0x20, 0xFFFFFFFF);

  uint64_t bar4 = mmio_read(base + PCI_BAR4);
  uint64_t bar5 = mmio_read(base + PCI_BAR5);

  if (bar4 == 0 || bar4 == 0xFFFFFFFF) {
    uart_puts("BAR4 size probing failed\n");
    return 0x0;
  }

  uint64_t size = ((uint64_t)(~((bar5 << 32) | (bar4 & ~0xF))) + 1);
  uart_puts("Calculated BAR size: ");
  uart_puthex(size);
  uart_putc('\n');

  uint64_t mmio_base = 0x10010000;
  mmio_write(base + 0x20, mmio_base & 0xFFFFFFFF);
  mmio_write(base + 0x24, (mmio_base >> 32) & 0xFFFFFFFF);

  // confirme the setup 
  bar4 = mmio_read(base + PCI_BAR4);
  bar5 = mmio_read(base + PCI_BAR5);

  return ((uint64_t)bar5 << 32) | (bar4 & ~0xF);
}

void virtio_gpu_display_on(uint64_t base_addr) {
  // setup virtqueue (assuming queue 0 for simplicity)
  mmio_write(base_addr + 0x30, 0); // queue selected
  mmio_write(base_addr + 0x38, 128); // queue size
  mmio_write(base_addr + 0x3C, 1); // queue enabled


  // send display on command
  mmio_write(base_addr + 0x20, VIRTIO_GPU_CMD_GET_DISPLAY_INFO);

  mmio_write(base_addr + 0x14, 4); // set the driver ok status bit

  uint64_t status = mmio_read(base_addr + 0x14);

  if (status & 4) {
    uart_puts("Display activated\n");
  } else {
    uart_puts("Display activation failed\n");
  }
}

void virtio_gpu_init(uint64_t base_addr) {
  mmio_write(base_addr + 0x14, 0); // Reset the device

  while (mmio_read(base_addr + 0x14) != 0);

  mmio_write(base_addr + 0x14, 1); // Acknowledge the device
  mmio_write(base_addr + 0x14, 2); // driver

  mmio_write(base_addr + 0x0, 0); // select feature bits 0-31
  uint64_t features = mmio_read(base_addr + 0x4); // Read features
  mmio_write(base_addr + 0x8, 0); // seletc driver feeatures 0-31
  mmio_write(base_addr + 0xC, features); // write feaatures
  //
  mmio_write(base_addr + 0x14, 8); // features OK 
  if (!(mmio_read(base_addr + 0x14) & 8)) {
    uart_puts("Features OK not set, device unsuable\n");
    return;
  }

  mmio_write(base_addr + 0x14, 4); // driver ok 
  uart_puts("GPU initialization complete\n");
}

void gpu_init() {

  uint64_t mmio_base;

  uint64_t address = find_pci_device(0x1AF4, 0x1050, &mmio_base);

  if (address > 0) {
    uart_puts("Virtio GPU detected at ");
    uart_puthex(address);

    uart_puts("Initializinjg GPU... \n");

    mmio_base = setup_gpu_bars(address);
    pci_enable_device(address);

    if (mmio_base == 0) {
      uart_puts("Failed to read GPU MMIO base.\n");
      return;
    }
    uart_puts("MMIO base: ");
    uart_puthex(mmio_base);
    uart_putc('\n');

    virtio_gpu_init(mmio_base);

    virtio_gpu_display_on(mmio_base);
  }
}
