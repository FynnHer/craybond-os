
# Craybond: AArch64 Bare-Metal Kernel

A lightweight, feature-rich bare-metal kernel for AArch64 architecture, designed to run on QEMU's virt machine with graphics, process management, and interrupt handling.

##  Features

- ** Bare-Metal AArch64**: Full assembly-level CPU initialization and exception handling
- ** Memory Management**: MMU with page tables, DMA, and RAM error handling
- ** Hardware Abstraction**: GIC interrupt controller, PCI enumeration, Device Tree parsing
- ** Process Management**: Task scheduling, context switching, and system calls
- ** Graphics Stack**: Dual graphics support (RAM framebuffer + Virtio GPU)
- ** Console I/O**: Serial UART driver with kernel I/O abstraction
- ** Modular Design**: Clean separation of concerns across subsystems

## Project Structure

See [STRUCTURE.md](STRUCTURE.md) for detailed architecture and component overview.

### Quick Layout
```
kernel/          → Core kernel, memory, hardware, process management
├── console/     → I/O and serial communication
├── graph/       → Graphics engines and drivers
└── process/     → Scheduler and system calls

shared/          → Shared libraries (future)
user/            → User-space programs (future)
```

## Build & Run

```bash
make              # Build kernel.elf
make clean        # Clean artifacts
./run             # Build and run in QEMU
./run debug       # Run with GDB debugging
```

## System Requirements

- **Cross-compiler**: aarch64-linux-gnu toolchain
- **Emulator**: QEMU with AArch64 support
- **Build tools**: Make

## Architecture Diagram

See STRUCTURE.md for detailed mermaid diagram showing subsystem dependencies and data flow.

