/*
kernel/dma.c
This file provides basic DMA (Direct Memory Access) read and write operations.
It allows reading from and writing to specific memory addresses directly,
bypassing the CPU for improved performance in certain scenarios.
Example scenarios include transferring data to/from hardware devices.
*/
#include "dma.h"
#include "ram_e.h"

void dma_read(void* dest, uint32_t size, uint64_t pointer) {
    /*
    This function performs a DMA read operation by copying data
    from a specified memory address (pointer) to a destination buffer (dest).
    It reads 'size' bytes of data from the source address to the destination.
    */
    uint8_t* d = (uint8_t*)dest;
    for (uint32_t i = 0; i < size; i++) {
        d[i] = *(volatile uint8_t*)(uintptr_t)(pointer + i);
    }
}

void dma_write(void* data, uint32_t size, uint64_t pointer) {
    
}