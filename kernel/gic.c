#include "gic.h"
#include "console/kio.h"
#include "ram_e.h"

#define IRQ_TIMER 30

static uint64_t secs;

void gic_init() {
    /*
    Initialize the GIC Distributor and CPU Interface by disabling them,
    configuring the timer interrupt, and then enabling them.

    GICD - GIC Distributor -> This component is responsible for receiving interrupts from peripherals
    GICC - GIC CPU Interface -> This component is responsible for delivering interrupts to the CPU cores

    */
    uint64_t current_el; // Current Exception Level
    asm volatile ("mrs %0, CurrentEL" : "=r"(current_el)); // Read CurrentEL system register | mrs = Move Register from System
    printf("[GIC INIT] CurrentEL: %h\n", current_el);

    write8(GICD_BASE, 0); // Disable Distributor
    write8(GICC_BASE, 0); // Disable CPU Interface

    uint32_t flag = read32(GICD_BASE + 0x100 + (IRQ_TIMER / 32) * 4); // Read current enable state
    write32(GICD_BASE + 0x100 + (IRQ_TIMER / 32) * 4, flag | (1 << (IRQ_TIMER % 32))); // Enable timer interrupt
    write32(GICD_BASE + 0x800 + (IRQ_TIMER / 4) * 4, 1 << ((IRQ_TIMER % 4) * 8)); // Target CPU 0
    write32(GICD_BASE + 0x400 + (IRQ_TIMER / 4) * 4, 0); // Priority 0

    write16(GICC_BASE + 0x004, 0xF0); // Priority mask register

    write8(GICC_BASE, 1); // Enable CPU Interface
    write8(GICD_BASE, 1); // Enable Distributor

    printf("[GIC INIT] GIC Initialized\n");
}

void timer_reset() {
    /*
    Reset the timer interval based on the configured seconds.
    */
    uint64_t freq;
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
    uint64_t interval = freq * secs;
    printf("[TIMER] Resetting interval to: %h\n", interval);
    asm volatile ("msr cntp_tval_el0, %0" :: "r"(interval));
}

void timer_enable() {
    /*
    Enable the physical timer by setting the appropriate control bits.
    */
    uint64_t val = 1;
    asm volatile ("msr cntp_ctl_el0, %0" :: "r"(val));
    asm volatile ("msr cntkctl_el1, %0" :: "r"(val));
    printf("[TIMER] Timer enabled\n");
}

void timer_init(uint64_t seconds) {
    /*
    Initialize the timer with the specified interval in seconds.
    */
    secs = seconds;
    timer_reset();
    timer_enable();
}

void enable_interrupt() {
    /*
    Enable global interrupts by clearing the interrupt mask.
    */
    asm volatile ("msr daifclr, #2"); // Enable IRQs
    printf("[INTERRUPTS] Global interrupts enabled\n");
}

void irq_el1_handler() {
    /*
    Handle IRQ exceptions by checking the interrupt ID and responding accordingly.
    1. Read the interrupt ID from the GICC.
    2. If it's the timer interrupt, reset the timer and signal end of interrupt.
    3. If it's an unhandled interrupt, log the interrupt ID.
    */
    uint32_t irq = read32(GICC_BASE + 0xC);

    if (irq == IRQ_TIMER) {
        printf(">>> Timer IRQ fired\n");
        timer_reset();
        write32(GICC_BASE + 0x10, irq); // End of Interrupt
        return;
    }

    printf(">>> Unhandled IRQ: %h\n", irq);
}