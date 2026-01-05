#include "default_process.h"
#include "types.h"
#include "process_loader.h"
#include "console/kio.h"
#include "syscalls/syscalls.h"

__attribute__((section(".rodata.proc1"))) // Read-only data section for process 1
static const char fmt[] = "Process %i";
__attribute__((section(".data.proc1"))) // Writable data section for process 1
static uint64_t j = 1; // Counter variable for process 1

__attribute__((section(".text.proc1"))) // Code section for process 1
void proc_func() {
    while (1) {
        printf(fmt, j++);
    }
}

void default_processes() {
    extern uint8_t proc_1_start;
    extern uint8_t proc_1_end;
    extern uint8_t proc_1_rodata_start;
    extern uint8_t proc_1_rodata_end;

    kprintf("Proc starts at %h and ends at %h", (uint64_t)&proc_1_start, (uint64_t)&proc_1_end);
    kprintf("Data starts at %h and ends at %h", (uint64_t)&proc_1_rodata_start, (uint64_t)&proc_1_rodata_end);

    create_process(proc_func, (uint64_t)&proc_1_end - (uint64_t)&proc_1_start, (uint64_t)&proc_1_start, (void*)&fmt, (uint64_t)&proc_1_rodata_end - (uint64_t)&proc_1_rodata_start);
    create_process(proc_func, (uint64_t)&proc_1_end - (uint64_t)&proc_1_start, (uint64_t)&proc_1_start, (void*)&fmt, (uint64_t)&proc_1_rodata_end - (uint64_t)&proc_1_rodata_start);
}