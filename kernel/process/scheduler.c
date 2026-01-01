#include "scheduler.h"
#include "console/kio.h"
#include "gic.h"

extern void save_context(process_t* proc);
extern void save_pc_interrupt(process_t* proc);
extern void restore_context(process_t* proc);
extern void restore_context(process_t* proc);
extern void restore_pc_interrupt(process_t* proc);

#define MAX_PROCS 16
process_t processes[MAX_PROCS];
int current_proc = 0;
int proc_count = MAX_PROCS;

void save_context_registers() {
    save_context(&processes[current_proc]);
}

void save_return_address_interrupt() {
    save_pc_interrupt(&processes[current_proc]);
}

void restore_return_address_interrupt() {
    restore_pc_interrupt(&processes[current_proc]);
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
    int next_proc = current_proc; // (current_proc + 1) % proc_count;
    // while (processes[next_proc].state != READY) {
    //     next_proc = (next_proc + 1) % proc_count;
    //     if (next_proc == current_proc)
    //         return; // No other ready process found
    // }

    current_proc = next_proc;
    restore_context(&processes[current_proc]);
}

void start_scheduler() {
    /*
    This function starts the process scheduler by initializing the timer interrupt
    to trigger every 1 millisecond. This allows the scheduler to perform context
    switches at regular intervals.
    Example usage: start_scheduler(); would begin the scheduling of processes.
    */
    timer_init(1);
}

int get_current_proc() {
    /*
    This function returns the currently running process ID.
    Example usage: int pid = get_current_proc(); would retrieve the current process ID.
    */
    return current_proc;
}