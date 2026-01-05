/*
kernel/ram_e.c
This file implements basic RAM access functions including reading and writing
8, 16, 32, and 64-bit values. It also provides simple temporary and permanent
memory allocation functions. The memory allocators use a bump pointer strategy
to allocate memory sequentially from predefined regions.
*/
#include "ram_e.h"
#include "types.h"
#include "exception_handler.h"
#include "console/kio.h"
#include "dtb.h"
#include "console/serial/uart.h"

static uint64_t total_ram_size = 0; // in bytes
static uint64_t total_ram_start = 0; // start address of total RAM
static uint64_t calculated_ram_size = 0; // in bytes
static uint64_t calculated_ram_start = 0; // start address of calculated RAM
static uint64_t calculated_ram_end = 0; // end address of calculated RAM

/*
Free spaces in temporary memory allocator are tracked using a linked list of FreeBlock structures.
This allows for efficient allocation and deallocation of temporary memory blocks. It is a common
technique in memory management to keep track of free memory regions.
*/
typedef struct FreeBlock {
    struct FreeBlock* next;
    uint64_t size;
} FreeBlock;

FreeBlock* temp_free_list = 0;

uint8_t read8(uintptr_t addr) {
    return *(volatile uint8_t*)addr;
}

void write8(uintptr_t addr, uint8_t value) {
    *(volatile uint8_t*)addr = value;
}

uint16_t read16(uintptr_t addr) {
    return *(volatile uint16_t*)addr;
}

void write16(uintptr_t addr, uint16_t value) {
    *(volatile uint16_t*)addr = value;
}

uint32_t read32(uintptr_t addr) {
    return *(volatile uint32_t*)addr;
}

void write32(uintptr_t addr, uint32_t value) {
    *(volatile uint32_t*)addr = value;
}

uint64_t read64(uintptr_t addr) {
    return *(volatile uint64_t*)addr;
}

void write64(uintptr_t addr, uint64_t value) {
    *(volatile uint64_t*)addr = value;
}

void write(uint64_t addr, uint64_t value) {
    write64(addr, value);
}

uint64_t read(uint64_t addr) {
    return read64(addr);
}

int memcmp(const void *s1, const void *s2, unsigned long n) {
    /*
    This function compares two memory blocks byte by byte for a given length n.
    It returns 0 if the blocks are equal, a negative value if the first block
    is less than the second, and a positive value if the first block is greater.
    */
    const unsigned char *a = s1;
    const unsigned char *b = s2;
    for (unsigned long i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
    }
    return 0;
}

#define temp_start (uint64_t)&heap_bottom + 0x500000 // 5 MB after heap bottom

extern uint64_t kernel_start; // defined in linker script
extern uint64_t heap_bottom; // defined in linker script
extern uint64_t heap_limit; // defined in linker script
extern uint64_t kcode_end; // defined in linker script
extern uint64_t kfull_end; // defined in linker script
extern uint64_t shared_start; // defined in linker script
extern uint64_t shared_end; // defined in linker script
uint64_t next_free_temp_memory = (uint64_t)&heap_bottom; // Start of temporary memory, implements a bump pointer allocator
uint64_t next_free_perm_memory = temp_start; // Start of permanent memory, implements a bump pointer allocator

// Well need to use a table indicating which sections of memory are available
// so we can talloc and free dynamically

static bool talloc_verbose = false;

uint64_t talloc(uint64_t size) {
    /*
    This function allocates temporary memory of the given size using a bump pointer allocator.
    It aligns the allocation to a 4KB boundary and checks for overflow against permanent memory.
    */
    size = (size + 0xFFF) & ~0xFFF;

    if (talloc_verbose) {
        uart_raw_puts("[talloc] Requested size: ");
        uart_puthex(size);
        uart_raw_puts("\n");
    }

    FreeBlock** curr = &temp_free_list;
    while (*curr) {

        if ((*curr)->size >= size) {
            if (talloc_verbose) {
                uart_raw_puts("[talloc] Reusing free block at ");
                uart_puthex((uint64_t)*curr);
                uart_raw_putc('\n');
            }

            uint64_t result = (uint64_t)*curr;
            *curr = (*curr)->next;
            return result;
        }
        curr = &(*curr)->next;
    }

    if (next_free_temp_memory + size > temp_start) {
        panic_with_info("Temporary allocator overflow", next_free_temp_memory);
    }

    uint64_t result = next_free_temp_memory;
    next_free_temp_memory += size;
    return result;
}

void temp_free(void* ptr, uint64_t size) {
    /*
    This function frees temporary memory by adding it to the free list.
    It creates a FreeBlock structure at the given pointer and links it into the free list.
    */
   size = (size + 0xFFF) & ~0xFFF;

    if (talloc_verbose) {
        uart_raw_puts("[temp_free] Freeing block at ");
        uart_puthex((uint64_t)ptr);
        uart_raw_puts(" of size ");
        uart_puthex(size);
        uart_raw_putc('\n');
    }

    FreeBlock* block = (FreeBlock*)ptr;
    block->size = size;
    block->next = temp_free_list;
    temp_free_list = block;
}

void enable_talloc_verbose() {
    talloc_verbose = true;
}

uint64_t palloc(uint64_t size) {
    /*
    This function allocates permanent memory of the given size using a bump pointer allocator.
    It aligns the allocation to a 4KB boundary and checks for overflow against the heap limit.
    */
    uint64_t aligned_size = (size + 0xFFF) & ~0xFFF;
    next_free_perm_memory = (next_free_perm_memory + 0xFFF) & ~0xFFF;
    if (next_free_perm_memory + aligned_size > (uint64_t)&heap_limit)
        panic_with_info("Permanent allocator overflow", (uint64_t)&heap_limit);
    uint64_t result = next_free_perm_memory;
    next_free_perm_memory += aligned_size;
    return result;
}

uint64_t mem_get_kmem_start(){
    /*
    This function returns the start address of the kernel memory region.
    It is defined by the kernel_start symbol from the linker script.
    */
    return (uint64_t)&kernel_start;
}

uint64_t mem_get_kmem_end(){
    /*
    This function returns the end address of the kernel memory region.
    It is defined by the heap_limit symbol from the linker script.
    */
    return (uint64_t)&kcode_end;
}

void calc_ram() {
    /*
    This function calculates the total and user RAM sizes and start addresses
    by reading the device tree blob (DTB). It sets the global variables accordingly.
    */
    if (get_memory_region(&total_ram_start, &total_ram_size)) {
        calculated_ram_end = total_ram_start + total_ram_size;
        calculated_ram_start = ((uint64_t)&kfull_end) + 0x1;
        calculated_ram_start = ((calculated_ram_start) & ~((1ULL << 21) - 1)); // Align to 2MB
        calculated_ram_end = ((calculated_ram_end) & ~((1ULL << 21) - 1)); // Align to 2MB

        calculated_ram_size = calculated_ram_end - calculated_ram_start;
        kprintf("Device has %h memory starting at %h. %h for users starting at %h  ", total_ram_size, total_ram_start, calculated_ram_size, calculated_ram_start);
    }
}

#define calcvar(var)\
    if (var == 0)\
        calc_ram();\
    return var;

uint64_t get_total_ram() {
    calcvar(total_ram_size);
}

uint64_t get_total_user_ram() {
    calcvar(calculated_ram_size);
}

uint64_t get_user_ram_start() {
    calcvar(calculated_ram_start);
}

uint64_t get_user_ram_end() {
    calcvar(calculated_ram_end);
}

uint64_t get_shared_start() {
    return (uint64_t)&shared_start;
}

uint64_t get_shared_end() {
    return (uint64_t)&shared_end;
}