#include "mmu.h"
#include "console/serial/uart.h"
#include "ram_e.h"
#include "console/kio.h"
#include "gic.h"

#define MAIR_DEVICE_nGnRnE 0b00000000 // Device-nGnRnE is 0b00000000 | nGnRnE is "non-Gathering, non-Reordering, no Early write acknowledgment"
#define MAIR_NORMAL_NOCACHE 0b01000100 // Normal memory, Non-cacheable is 0b01000100
#define MAIR_IDX_DEVICE 0 // 0 for device memory
#define MAIR_IDX_NORMAL 1 // 1 for normal memory

#define PD_TABLE 0b11 // Table entry 
#define PD_BLOCK 0b01 // Block entry
#define PD_ACCESS (1 << 10) // Access flag
#define BOOT_PGD_ATTR PD_TABLE // | PD_ACCESS
#define BOOT_PUD_ATTR PD_TABLE // | PD_ACCESS
#define PTE_ATTR_DEVICE (PD_ACCESS | (MAIR_IDX_DEVICE << 2) | PD_TABLE) // Device memory
#define PTE_ATTR_NORMAL (PD_ACCESS | (MAIR_IDX_NORMAL << 2) | PD_TABLE) // Normal memory

#define PAGE_TABLE_ENTRIES 512 // 512 entries per page table
#define PAGE_SIZE PAGE_TABLE_ENTRIES * 8 // 4KB page size

uint64_t page_table_l1[PAGE_TABLE_ENTRIES] __attribute__((aligned(PAGE_SIZE))); // Level 1 page table is aligned to 4KB boundary

void mmu_map_2mb(uint64_t va, uint64_t pa, uint64_t attr_index) { // Map a 2MB page from virtual address va to physical address pa
    // Calculate indices for L1, L2, and L3 tables by shifting and masking the virtual address
    uint64_t l1_index = (va >> 39) & 0x1FF;
    uint64_t l2_index = (va >> 30) & 0x1FF;
    uint64_t l3_index = (va >> 21) & 0x1FF;

    printf("Addresses for %h: %i %i %i", va, l1_index, l2_index, l3_index);

    if (!(page_table_l1[l1_index] & 1)) { // Check if L2 table exists
        // Allocate L2 table
        uint64_t* pud = (uint64_t*)palloc(PAGE_SIZE);
        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) pud[i] = 0; // Initialize L2 table entries to 0
        page_table_l1[l1_index] = ((uint64_t)pud & 0xFFFFFFFFF000ULL) | PD_TABLE; // Set L1 entry to point to L2 table
    }

    uint64_t* pud = (uint64_t*)(page_table_l1[l1_index] & 0xFFFFFFFFF000ULL); // Get L2 table address from L1 entry

    if (!(pud[l2_index] & 1)) { // Check if L3 table exists
        // Allocate L3 table
        uint64_t* pmd = (uint64_t*)palloc(PAGE_SIZE);
        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) pmd[i] = 0; // Initialize L3 table entries to 0
        pud[l2_index] = ((uint64_t)pmd & 0xFFFFFFFFF000ULL) | PD_TABLE; // Set L2 entry to point to L3 table
    }

    uint64_t* pmd = (uint64_t*)(pud[l2_index] & 0xFFFFFFFFF000ULL); // Get L3 table address from L2 entry

    uint64_t attr = PD_ACCESS | (attr_index << 2) | PD_BLOCK; // Set attributes for the block entry
    pmd[l3_index] = (pa & 0xFFFFFFFFF000ULL) | attr; // Set L3 entry to map 2MB page
}

void mmu_map_4kb(uint64_t va, uint64_t pa, uint64_t attr_index) { // Map a 4KB page from virtual address va to physical address pa
    uint64_t l1_index = (va >> 39) & 0x1FF; // Level 1 index
    uint64_t l2_index = (va >> 30) & 0x1FF; // Level 2 index
    uint64_t l3_index = (va >> 21) & 0x1FF; // Level 3 index
    uint64_t l4_index = (va >> 12) & 0x1FF; // Level 4 index

    printf("Addresses for %h: %i %i %i %i", va, l1_index, l2_index, l3_index, l4_index);

    if (!(page_table_l1[l1_index] & 1)) { // Check if L2 table exists
        uint64_t* l2 = (uint64_t*)palloc(PAGE_SIZE); // Allocate L2 table
        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) l2[i] = 0; // Initialize L2 table entries to 0
        page_table_l1[l1_index] = ((uint64_t)l2 & 0xFFFFFFFFF000ULL) | PD_TABLE; // Set L1 entry to point to L2 table
    }

    uint64_t* l2 = (uint64_t*)(page_table_l1[l1_index] & 0xFFFFFFFFF000ULL); // Get L2 table address from L1 entry
    if (!(l2[l2_index] & 1)) {
        uint64_t* l3 = (uint64_t*)palloc(PAGE_SIZE); // Allocate L3 table
        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) l3[i] = 0; // Initialize L3 table entries to 0
        l2[l2_index] = ((uint64_t)l3 & 0xFFFFFFFFF000ULL) | PD_TABLE; // Set L2 entry to point to L3 table
    }

    uint64_t* l3 = (uint64_t*)(l2[l2_index] & 0xFFFFFFFFF000ULL); // Get L3 table address from L2 entry
    if (!(l3[l3_index] & 1)) {
        uint64_t* l4 = (uint64_t*)palloc(PAGE_SIZE);
        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) l4[i] = 0; // Initialize L4 table entries to 0
        l3[l3_index] = ((uint64_t)l4 & 0xFFFFFFFFF000ULL) | PD_TABLE; // Set L3 entry to point to L4 table
    }

    uint64_t* l4 = (uint64_t*)(l3[l3_index] & 0xFFFFFFFFF000ULL); // Get L4 table address from L3 entry
    uint64_t attr = PD_ACCESS | (attr_index << 2) | 0b11; // Set attributes for the page entry
    l4[l4_index] = (pa & 0xFFFFFFFFF000ULL) | attr; // Set L4 entry to map 4KB page
}

void mmu_init() {
    for (int i = 0; i < PAGE_TABLE_ENTRIES; i ++) {
        page_table_l1[i] = 0;
    }

    uint64_t start = mem_get_kmem_start();
    uint64_t end = mem_get_kmem_end();
    for (uint64_t addr = start; addr < end; addr += 0x200000)
        mmu_map_2mb(addr, addr, MAIR_IDX_NORMAL); // Map kernel memory as normal memory

    for (uint64_t addr = UART0_BASE; addr < UART0_BASE + 0x1000; addr += 0x200000)
        mmu_map_4kb(addr, addr, MAIR_IDX_DEVICE); // Map UART as device memory

    for (uint64_t addr = GICD_BASE; addr < GICD_BASE + 0x12000; addr += 0x1000)
        mmu_map_4kb(addr, addr, MAIR_IDX_DEVICE); // Map GIC as device memory

    uint64_t mair = (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE * 8)) | (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL * 8));
    asm volatile ("msr mair_el1, %0" :: "r"(mair)); // Set MAIR_EL1 register

    uint64_t tcr = ((64 - 48) << 0) | (64 - 48) << 16 | (0b00 << 14) | (0b10 << 30);
    asm volatile ("msr tcr_el1, %0" :: "r"(tcr)); // Set TCR_EL1 register

    asm volatile ("dsb ish"); // Data Synchronization Barrier
    asm volatile ("isb"); // Instruction Synchronization Barrier

    asm volatile ("msr ttbr0_el1, %0" :: "r"(page_table_l1)); // Set TTBR0_EL1 register

    uint64_t sctlr;
    asm volatile ("mrs %0, sctlr_el1" : "=r"(sctlr)); // Read SCTLR_EL1 register
    sctlr |= 1; // Enable MMU
    asm volatile ("msr sctlr_el1, %0" :: "r"(sctlr)); // Write back to SCTLR_EL1 register
    asm volatile ("isb"); // Instruction Synchronization Barrier

    printf("Finished MMU init");

}