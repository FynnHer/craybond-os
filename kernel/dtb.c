/*
kernel/dtb.c

    Device Tree Blob (DTB) parsing and handling functions.
    This module provides functionality to read and interpret
    the DTB structure used for hardware description in ARM systems.
    The DTB is typically located at a predefined memory address.
*/
#include "dtb.h"
#include "console/kio.h"

#define DTB_ADDR 0x40000000UL
#define FDT_MAGIC 0xD00DFEED

#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

static inline uint32_t bswap32(uint32_t x) {
    /* 
    Swap byte order of a 32-bit integer.
    Example: 0xAABBCCDD becomes 0xDDCCBBAA
    */
    return __builtin_bswap32(x);
}

static inline uint64_t bswap64(uint64_t x) {
        /* 
    Swap byte order of a 64-bit integer.
    Example: 0xAABBCCDDEEFF0011 becomes 0x1100FFEEDDCCBBAA
    */
    return ((uint64_t)bswap32(x & 0xFFFFFFFF) << 32) | bswap32(x >> 32);
}

static int str_eq(const char *a, const char *b) {
    /*
    This function compares two null-terminated strings for equality.
    It returns 1 (true) if the strings are equal, and 0 (false) otherwise.
    */
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

int get_memory_region(uint64_t *out_base, uint64_t *out_size) {
    /*
    Gets the base address and size of the memory region from the DTB.
    It parses the DTB structure to find the "mem" node and retrieves
    the "reg" property which contains the memory region information.
    Returns 1 on success, 0 on failure.
    */
    struct fdt_header *hdr = (struct fdt_header *)DTB_ADDR; // Point to the DTB header
    if (bswap32(hdr->magic) != FDT_MAGIC) return 0; // Check for valid DTB magic number

    uint32_t *p = (uint32_t *)(DTB_ADDR + bswap32(hdr->off_dt_struct)); // Pointer to the structure block
    const char *string = (const char *)(DTB_ADDR + bswap32(hdr->off_dt_strings)); // Pointer to the strings block
    int in_mem = 0; // Flag to indicate if we are inside the "mem" node

    while (1) {
        /* 
        While loop to parse the DTB structure block.
        It reads tokens and processes nodes and properties accordingly.
        */
        uint32_t token = bswap32(*p++); // Read the next token
        if (token == FDT_END) {
            break;
        }
        if (token == FDT_BEGIN_NODE) {
            const char *name = (const char *)p;
            uint32_t skip = 0;
            while (((char *)p)[skip]) skip++;
            in_mem = (name[0] == 'm' && name[1] == 'e');
            skip = (skip + 4) & ~3;
            p += skip / 4;
        } else if (token == FDT_PROP && in_mem) {
            uint32_t len = bswap32(*p++);
            uint32_t nameoff = bswap32(*p++);
            const char *propname = string + nameoff;
            if (str_eq(propname, "reg") && len >= 16) {
                uint32_t hi_base = bswap32(p[0]);
                uint32_t lo_base = bswap32(p[1]);
                uint32_t hi_size = bswap32(p[2]);
                uint32_t lo_size = bswap32(p[3]);
                *out_base = ((uint64_t)hi_base << 32) | lo_base;
                *out_size = ((uint64_t)hi_size << 32) | lo_size;
                return 1;
            }
            p += (len + 3) / 4;
        } else if (token == FDT_END_NODE) {
            in_mem = 0;
        }
    }
    return 0;
}