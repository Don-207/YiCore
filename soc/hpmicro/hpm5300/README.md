# HPM5300 backend reservation

Status: minimum official-SDK bring-up application available; platform adapter
and YiCore peripheral backends remain reserved.

This directory is the YiCore backend boundary for the HPM5300 family and the
HPM5301 model. It will adapt the pinned official HPM SDK to existing `yi_*`
APIs without exposing SDK types to applications.

Before changing the platform to `ready`, validate all of the following against
the HPM5301EVKLite SDK target:

- RISC-V ISA, ABI, code model and toolchain prefix;
- reset entry, trap vector and interrupt-controller initialization;
- on-chip RAM and external XIP flash regions used by the linker;
- boot header and image conversion requirements;
- system clock, machine timer and polling console UART;
- pinmux, GPIO and peripheral clock ownership.

Bring-up proceeds through startup/linker and traps, clock/timebase, polling
UART, then GPIO/pinmux. Interrupt-driven peripherals and DMA follow only after
the minimum image runs on hardware.

The first build target is `applications/hpm5301-bringup`. It deliberately uses
the SDK's `hpm5301evklite` board sources, startup, trap handler, boot header and
`flash_xip.ld`. This validates the architecture boundary before YiCore replaces
any board-facing service.
