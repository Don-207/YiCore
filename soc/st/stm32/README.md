# STM32 SoC backends

This directory contains YiCore-owned STM32 implementations. It is the only
layer that may depend directly on STM32 CMSIS Device, HAL, or LL interfaces.

```text
stm32/
├── common/       Shared code that is genuinely identical across STM32 series
└── stm32f1/      STM32F1-specific backends
```

Do not move code into `common/` merely because filenames are similar. Clock
trees, GPIO configuration, DMA routing, interrupt behavior, Flash geometry,
and peripheral register layouts must be verified across every supported
series first.

Public application-facing interfaces belong in `drivers/` or `core/`.
Board-specific clocks, pins, and device instances belong in `boards/`.
