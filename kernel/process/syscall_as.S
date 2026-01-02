.global sync_el0_handler_as

sync_el0_handler_as: // EL0_64 sync exception handler
    cmp x8, #3 // Check if exception is from syscall (EC=0x3)
    b.ne .error // If not, branch to error handler
    ldr x0, =my_message // Load address of message
    mov x1, #0 // Length 0 for null-terminated string
    mov x2, #0 // File descriptor 0 (stdout)
    bl uart_puts // Call uart_puts to print the message
    b .done // Branch to done
.error: // Error handler
    adr x0, .  // Get current PC
    bl uart_puthex // Print PC value
    mrs x0, elr_el1 // Get ELR_EL1 (return address)
    bl uart_puthex // Print ELR_EL1 value
    ldr x0, =error_message // Load address of error message
    bl uart_puts // Print error message
.done:
    // Acknowledge the interrupt and return from exception
    mov x8, #0 // clear the instruction
    eret

.section .rodata
my_message:
    .asciz "hi from kernel syscall handler!\n"
error_message:
    .asciz " <- Wrong instruction\n"