# GD32F30x backend reservation

Status: reserved, not buildable.

This directory is the integration boundary for future GD32F30x support. Add
implementation only after the official vendor package is imported and the
first exact MCU and board are selected.

The backend is expected to provide:

- architecture/system initialization and interrupt primitives;
- clock and peripheral-clock control;
- GPIO and pin multiplexing;
- UART with optional circular RX DMA and idle/event reporting;
- SPI, I2C, timer, ADC, CAN and internal flash as required by a board;
- startup source, linker memory description and toolchain definitions;
- SoC DeviceTree descriptions and generated IRQ dispatch integration.

Keep GD32 register and standard-peripheral-library types private to this
directory. Implement existing `yi_*` APIs instead of adding vendor calls to
applications or shared drivers.
