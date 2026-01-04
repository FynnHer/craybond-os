#include "scheduler.h"
#include "console/kio.h"
#include "ram_e.h"
#include "proc_allocator.h"
#include "gic.h"
#include "console/serial/uart.h"

extern void save_context(process_t* proc);
extern void save_pc_interrupt(process_t* proc);
extern void restore_context(process_t* proc);

#define MAX_PROCS 16
process_t processes[MAX_PROCS];
int current_proc = 0;
int proc_count = 0;

void save_context_registers() {
    save_context(&processes[current_proc]);
}

void save_return_address_interrupt() {
    save_pc_interrupt(&processes[current_proc]);
}

void switch_proc(ProcSwitchReason reason) {
    /*
    This function switches the currently running process to the next one in a round-robin fashion.
    It saves the context of the current process, updates the current process index,
    and restores the context of the next process to run.
    The reason for the switch (e.g., timer interrupt, blocking) is logged for debugging purposes.
    */
    if (proc_count == 0)
        return;
    int next_proc = (current_proc + 1) % proc_count;
    while (processes[next_proc].state != READY) {
        next_proc = (next_proc + 1) % proc_count;
        if (next_proc == current_proc)
            return;
    }
    
    current_proc = next_proc;
    restore_context(&processes[current_proc]);
}

void relocate_code(void* dst, void* src, uint32_t size, uint64_t src_data_base, 
    uint64_t dst_data_base, uint32_t data_size) {
    /*
    This function relocates code from a source address to a destination address,
    adjusting branch instructions to ensure they point to the correct targets
    after relocation. It handles unconditional branches, conditional branches,
    and ADRP instructions, modifying their offsets as necessary.
    Example usage: relocate_code(dest_code, source_code, code_size, source_data_base, dest_data_base, data_size);
    */
    uint32_t* src32 = (uint32_t*)src;
    uint32_t* dst32 = (uint32_t*)dst;
    uint64_t src_base = (uint64_t)src32;
    uint64_t dst_base = (uint64_t)dst32;
    uint32_t count = size / 4;

    printf("Beginning translation from base address %h to new address %h", src_base, dst_base);

    for (uint32_t i = 0; i < count; i++) {
        /*
        This loop processes each instruction in the source code, checking if it is a branch or ADRP instruction
        that needs to be relocated. If so, it calculates the new target address and modifies the instruction accordingly.
        */
        uint32_t instr = src32[i];
        uint32_t op = instr >> 26;

        if (op == 5 || op == 37) { // op means B or BL which are 0b000101 and 0b100101. these commands branch unconditionally
            /*
            This block handles unconditional branch instructions (B and BL).
            */
            int32_t offset = ((int32_t)instr << 6) >> 6;
            offset *= 4;
            printf("Offset %i",offset);
            printf("Address %h",(uint64_t)src_base+(i*4));
            uint64_t target = src_base + (i * 4) + offset;
            bool internal = (target >= src_base) && (target < src_base + size);
        
            if (!internal) {
                int64_t rel = (int64_t)(target - (dst_base + (i * 4))) >> 2;
                instr = (instr & 0xFC000000) | (rel & 0x03FFFFFF);
            }
        
            printf("Branch op %i to %h (%s)", op, target, (uint64_t)(internal ? "internal" : "external"));
        } else if ((instr >> 24) == 84) { // B.cond (untested)
            int32_t offset = ((int32_t)(instr >> 5) & 0x7FFFF);
            offset = (offset << 6) >> 6;
            offset *= 4;
            uint64_t target = src_base + (i * 4) + offset;
            bool internal = (target >= src_base) && (target < src_base + size);
            if (!internal) {
                int32_t rel = (int32_t)(target - (dst_base + (i * 4))) >> 2;
                instr = (instr & ~0xFFFFE0) | ((rel & 0x7FFFF) << 5);
                printf("Relocated conditional branch to %h\n", target);
            } else {
                printf("Preserved internal conditional branch to %h\n", target);
            }
        } else if ((instr & 0x9F000000) == 0x90000000) {
            /*
            This block handles ADRP instructions, which load a page-aligned address into a register.
            */
            uint64_t immlo = (instr >> 29) & 0x3;
            uint64_t immhi = (instr >> 5) & 0x7FFFF;
            int64_t offset = ((int64_t)((immhi << 14) | (immlo << 12)) << 43) >> 43;

            uint64_t pc_page = (src_base + i * 4) & ~0xFFFULL;
            uint64_t target = pc_page + offset;

            printf("Was at offset %i of original code, so at address %h and data started at %h",offset,target,src_data_base);
            
            // uint64_t target = (src_base & ~0xFFFULL) + ((i * 4 + offset) & ~0xFFFULL);
            bool internal = (target >= src_data_base) && (target < src_data_base + data_size);

            if (internal) {
                /*
                This branch targets data within the data segment, so we need to adjust 
                the address to point to the new data location.
                */
                                uint64_t data_offset = target - src_data_base;
                uint64_t new_target = dst_data_base + data_offset;

                uint64_t dst_pc_page = (dst_base + i * 4) & ~0xFFFULL;
                int64_t new_offset = (int64_t)(new_target - dst_pc_page);
                
                uint64_t new_immhi = (new_offset >> 14) & 0x7FFFF;
                uint64_t new_immlo = (new_offset >> 12) & 0x3;
                
                instr = (instr & ~0x60000000) | (new_immlo << 29);
                instr = (instr & ~(0x7FFFF << 5)) | (new_immhi << 5);

                printf("We're inside data stack, so new address is: %i",data_offset);

                immlo = (instr >> 29) & 0x3;
                immhi = (instr >> 5) & 0x7FFFF;
                offset = ((int64_t)((immhi << 14) | (immlo << 12)) << 43) >> 43;

                pc_page = (dst_base + i * 4) & ~0xFFFULL;
                target = pc_page + offset;

                printf("Confirmation: New address is %h compared to calculated one %h",target, new_target);
            } else {
                printf("We dont support this type of symbol yet");
            }
        }

        dst32[i] = instr;
    }

    printf("Finished translation");
}

process_t* create_process(void (*func)(), uint64_t code_size, uint64_t func_base, void* data, uint64_t data_size) {
    /*
    This function creates a new process by allocating memory for its code and data segments,
    relocating the provided function code, and setting up its initial stack and registers.
    It returns a pointer to the newly created process structure.
    Example usage: process_t* proc = create_process(my_function, code_size, func_base, my_data, data_size);
    */
    if (proc_count >= MAX_PROCS) return 0;

    process_t* proc = &processes[proc_count];

    printf("Code size %h. Data size %h", code_size, data_size);
    
    uint8_t* data_dest = (uint8_t*)alloc_proc_mem(data_size);
    if (!data_dest) return 0;

    for (uint64_t i = 0; i < data_size; i++){
        data_dest[i] = ((uint8_t *)data)[i];
    }

    uint64_t* code_dest = (uint64_t*)alloc_proc_mem(code_size);
    if (!code_dest) return 0;

    // We need to relocate the code to the new memory location because the original code might be in a different memory region
    relocate_code(code_dest, func, code_size, (uint64_t)&data[0], (uint64_t)&data_dest[0], data_size);
    
    printf("Code copied to %h", (uint64_t)code_dest);
    uint64_t stack_size = 0x1000;

    uint64_t stack = (uint64_t)alloc_proc_mem(stack_size);
    printf("Stack size %h. Start %h", stack_size,stack);
    if (!stack) return 0;

    proc->sp = (stack + stack_size);
    
    proc->pc = (uint64_t)code_dest;
    printf("Process allocated with address at %h, stack at %h",proc->pc, proc->sp);
    proc->spsr = 0;
    proc->state = READY;
    proc->id = proc_count++;
    
    return proc;
}

void start_scheduler() {
    /*
    This function starts the process scheduler by initializing the timer interrupt
    to trigger every 1 millisecond. This allows the scheduler to perform context
    switches at regular intervals.
    Example usage: start_scheduler(); would begin the scheduling of processes.
    */
    disable_interrupt();
    timer_init(10);
    switch_proc(YIELD);
}

int get_current_proc() {
    /*
    This function returns the currently running process ID.
    Example usage: int pid = get_current_proc(); would retrieve the current process ID.
    */
    return current_proc;
}

/*
Attributes for the sample process code and data segments.
These attributes place the code and data in specific sections of the binary,
which are defined in the linker script for proper memory layout.
*/

__attribute__((section(".rodata.proc1"))) // Read-only data section for process 1
static const char fmt[] = "Process %i";
__attribute__((section(".data.proc1"))) // Writable data section for process 1
static uint64_t j = 12;

__attribute__((section(".text.proc1"))) // Code section for process 1
void proc_func() {
    /*
    This is a sample process function that runs in user mode (EL0). It demonstrates
    how a process can use system calls to print messages. The function enters an
    infinite loop, printing its process ID and a counter value.
    Example usage: This function would be executed as part of a created process.
    */
    //const char *msg = "hi from EL0\n";
    while (1) {
        register uint64_t x0 asm("x0") = (uint64_t)&fmt;
        register uint64_t x1 asm("x1") = (uint64_t)&j;
        register uint64_t x2 asm("x2") = 1;
        register uint64_t x8 asm("x8") = 3;

        asm volatile(
            "svc #3"
            :
            : "r"(x0), "r"(x1), "r"(x2), "r"(x8)
            : "memory"
        );
        j++;
    }
}

void default_processes(){
    extern uint8_t proc_1_start;
    extern uint8_t proc_1_end;
    extern uint8_t proc_1_rodata_start;
    extern uint8_t proc_1_rodata_end;

    printf("DATA STARTS AT %h ENDS AT %h",(uint64_t)&proc_1_rodata_start,(uint64_t)&proc_1_rodata_end);

    create_process(proc_func, (uint64_t)&proc_1_end - (uint64_t)&proc_1_start, (uint64_t)&proc_1_start, (void*)&fmt, (uint64_t)&proc_1_rodata_end - (uint64_t)&proc_1_rodata_start);
    create_process(proc_func, (uint64_t)&proc_1_end - (uint64_t)&proc_1_start, (uint64_t)&proc_1_start, (void*)&fmt, (uint64_t)&proc_1_rodata_end - (uint64_t)&proc_1_rodata_start);
}