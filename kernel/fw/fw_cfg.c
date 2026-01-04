/*
kernel/fw/fw_cfg.c
This file provides functions to interact with the firmware configuration interface (fw_cfg).
It allows reading and writing configuration data via DMA operations. 
It also includes functionality to search for specific configuration files.
*/
#include "fw_cfg.h"
#include "console/kio.h"
#include "ram_e.h"
#include "string.h"

#define FW_CFG_DATA  0x09020000
#define FW_CFG_CTL   (FW_CFG_DATA + 0x8)
#define FW_CFG_DMA   (FW_CFG_DATA + 0x10)

#define FW_CFG_DMA_READ 0x2
#define FW_CFG_DMA_SELECT 0x8
#define FW_CFG_DMA_WRITE 0x10
#define FW_CFG_DMA_ERROR 0x1

#define FW_LIST_DIRECTORY 0x19

static bool checked = false;

struct fw_cfg_dma_access {
    /*
    This structure represents a DMA access operation for the firmware configuration interface.
    It includes fields for control flags, data length, and the memory address for the operation.
    */
    uint32_t control;
    uint32_t length;
    uint64_t address;
}__attribute__((packed));

bool fw_cfg_check(){
    /*
    This function checks if the firmware configuration interface is available.
    It reads a specific signature value from the FW_CFG_DATA address and verifies it. If the signature matches, it indicates that the fw_cfg interface is present.
    The result is cached for subsequent calls.
    Example usage: bool available = fw_cfg_check(); would check if fw_cfg is available.
    */
    if (checked) return true;
    checked = read64(FW_CFG_DATA) == 0x554D4551;
    return checked;
}

void fw_cfg_dma_operation(void* dest, uint32_t size, uint32_t ctrl) {
    /*
    This function performs a DMA operation for the firmware configuration interface.
    It sets up a DMA access structure with the specified destination address, size, and control flags.
    It initiates the DMA operation and waits for its completion.
    Example usage: fw_cfg_dma_operation(buffer, 256, FW_CFG_DMA_READ); would read 256 bytes into buffer.
    */
    struct fw_cfg_dma_access access = {
        .address = __builtin_bswap64((uint64_t)dest),
        .length = __builtin_bswap32(size),
        .control = __builtin_bswap32(ctrl),
    };

    write64(FW_CFG_DMA, __builtin_bswap64((uint64_t)&access));

    __asm__("ISB");

    while (__builtin_bswap32(access.control) & ~0x1) {}
    
}

void fw_cfg_dma_read(void* dest, uint32_t size, uint32_t ctrl){
    /*
    This function performs a DMA read operation from the firmware configuration interface.
    It calls the generic DMA operation function with the read control flag.
    Example usage: fw_cfg_dma_read(buffer, 256, FW_LIST_DIRECTORY); would read 256 bytes into buffer from the directory listing.
    */
    if (!fw_cfg_check())
        return;

    fw_cfg_dma_operation(dest, size, (ctrl << 16) | FW_CFG_DMA_SELECT | FW_CFG_DMA_READ);
}

void fw_cfg_dma_write(void* dest, uint32_t size, uint32_t ctrl){
    /*
    This function performs a DMA write operation to the firmware configuration interface.
    It calls the generic DMA operation function with the write control flag.
    Example usage: fw_cfg_dma_write(buffer, 256, FW_LIST_DIRECTORY); would write 256 bytes from buffer to the directory listing.
    */
    if (!fw_cfg_check())
        return;

    fw_cfg_dma_operation(dest, size, (ctrl << 16) | FW_CFG_DMA_SELECT | FW_CFG_DMA_WRITE);
}

bool fw_find_file(string search, struct fw_cfg_file *file) {
    /*
    This function searches for a specific file in the firmware configuration interface.
    It reads the directory listing and compares each file's name with the provided search string.
    If a matching file is found, it populates the provided fw_cfg_file structure with the file's details and returns true.
    Example usage: struct fw_cfg_file file; bool found = fw_find_file(string_l("etc/ramfb"), &file); would search for the "etc/ramfb" file.
    */

    if (!fw_cfg_check())
        return false;

    uint32_t count;
    fw_cfg_dma_read(&count, sizeof(count), FW_LIST_DIRECTORY);

    count = __builtin_bswap32(count);

    for (uint32_t i = 0; i < count; i++) {

        fw_cfg_dma_operation(file, sizeof(struct fw_cfg_file), FW_CFG_DMA_READ);

        file->size = __builtin_bswap32(file->size);
        file->selector = __builtin_bswap16(file->selector);

        string filename = string_ca_max(file->name, 56);
        if (string_equals(filename, search)){
            kprintf("Found device at selector %h", file->selector);
            return true;
        }
    }

    return false;
}