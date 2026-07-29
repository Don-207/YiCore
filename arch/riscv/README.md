# RISC-V architecture boundary

Status: reserved, not buildable.

This directory owns architecture-wide RISC-V startup contracts, trap entry,
interrupt state primitives and toolchain definitions shared by future YiCore
RISC-V platforms. SoC register definitions and vendor runtime code do not
belong here.

The first consumer is HPMicro HPM5300. Its exact ISA and ABI flags must be
copied from the pinned HPM5301 SDK configuration and validated with the
selected toolchain before architecture sources are added.
