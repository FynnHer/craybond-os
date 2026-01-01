#pragma once

#include "types.h"

typedef struct {
    uint64_t regs[31];  // General-purpose registers x0-x30
    uint64_t sp;        // Stack pointer
    uint64_t pc;        // Program counter
    uint64_t spsr;      // Saved program status register
    uint64_t id;        // Process ID
    enum { READY, RUNNING, BLOCKED } state; // Process state
} process_t;