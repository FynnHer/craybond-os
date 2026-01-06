/*
kernel/filesystem/disk.c
This file implements basic disk functionality for the kernel. It provides functions to
initialize disks, read and write sectors, and manage disk operations.
*/
#include "disk.h"
#include "dtb.h"
#include "kstring.h"
#include "ram_e.h"
#include "mmu.h"
#include "console/kio.h"

static uint64_t disk_device_address;
static uint64_t disk_device_size;
static uint32_t disk_device_interrupt;

int handle_virito_node(const char *name, const char *propname, const void *prop, uint32_t len, dtb_match_t *match) {
    /*
    This function handles properties of the "virtio,mmio" DTB node.
    It extracts the "reg" and "interrupts" properties to populate
    the match structure with the device's base address, size, and IRQ. 
    */
   if (strcmp(propname, "reg") == 0 && len >= 16) {
    uint32_t *p = (uint32_t *)prop;
    match->reg_base = ((uint64_t)__builtin_bswap32(p[0]) << 32) | __builtin_bswap32(p[1]);
    match->reg_size = ((uint64_t)__builtin_bswap32(p[2]) << 32) | __builtin_bswap32(p[3]);
    uint8_t device_id = read32(match->reg_base + 0x008);
    match->found = device_id == 2;
    return 1;
   }
   else if (match->found && strcmp(propname, "interrupts") == 0 && len >= 4) {
        match->irq = __builtin_bswap32(*(uint32_t *)prop);
        return 1;
   }
   return 0;
}

int find_virtio_blk(uint64_t *out_base, uint64_t *out_size, uint32_t *out_irq) {
    /*
    This function searches the DTB for a "virtio,mmio" node
    and retrieves its base address, size, and IRQ using the
    handle_virito_node callback.
    Returns 1 on success, 0 on failure.
    */
    dtb_match_t match = {0};
    if (dtb_scan("virtio_mmio", handle_virito_node, &match)) {
        *out_base = match.reg_base;
        *out_size = match.reg_size;
        *out_irq = match.irq;
        return 1;
    }
    return 0;
}

void init_disk() {
   find_virtio_blk(&disk_device_address, &disk_device_size, &disk_device_interrupt); 
}

uint64_t get_disk_address() {
    return disk_device_address;
}

uint64_t get_disk_size() {
    return disk_device_size;
}