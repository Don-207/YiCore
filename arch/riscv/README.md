# RISC-V architecture boundary

Status: architecture primitives available; hardware build pending.

This directory owns architecture-wide RISC-V startup contracts, trap entry,
interrupt state primitives and toolchain definitions shared by future YiCore
RISC-V platforms. SoC register definitions and vendor runtime code do not
belong here.

`yi_riscv_irq.h` provides machine-mode interrupt save/restore and memory
barriers without depending on a silicon vendor. The first consumer is HPMicro
HPM5300; startup and trap entry continue to come from the pinned official SDK.
