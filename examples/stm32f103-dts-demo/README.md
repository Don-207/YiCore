# STM32F103 DeviceTree demo

This example integrates YiCore with an STM32CubeMX-generated STM32F103xE
project and Keil MDK-ARM. The reference board description is
`boards/fire-mini-stm32f103/board.dts` at the repository root.

## Build

1. Install Python 3.9 or newer and Keil MDK-ARM with STM32F1 device support.
2. Open `MDK-ARM/stm32f103-dts-demo.uvprojx`.
3. Build the `stm32f103-dts-demo` target.

The Keil pre-build command regenerates `generated/yi_generated.c` and
`generated/yi_generated.h` at the repository root.

The `.ioc` file is retained for pin and clock inspection. Runtime peripheral
initialization is owned by YiCore and the board DeviceTree; regenerating code
with STM32CubeMX may overwrite application integration points, so review its
changes before committing them.

The board DeviceTree defines `soft_spi0` on PA5/PA6/PA7 as SCK/MISO/MOSI and
PA4 as the W25Q64 chip select.
Set its SPI mode with `mode = <0>` through `mode = <3>`, then obtain it with
`YI_DT_GET(SOFT_SPI0)` and transfer through `yi_spi_transceive()`. Chip select is
an application-controlled GPIO so one software bus can serve multiple devices.
