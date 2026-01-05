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
    // kprintf_raw("Resumiong execution of process %i at %h", current_proc, processes[current_proc].pc);
    restore_context(&processes[current_proc]);
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

process_t* init_process() {
    if (proc_count >= MAX_PROCS) return 0;

    process_t* proc = &processes[proc_count];
    proc->id = proc_count++;
}