# STM32F103 DeviceTree demo

This example integrates YiCore with an STM32CubeMX-generated STM32F103xE
project and Keil MDK-ARM. The application DeviceTree entry is `app.dts`; it
includes the reference board description from
`boards/fire-mini-stm32f103/board.dts`.

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

The board DeviceTree enables hardware `spi1` on PA5/PA6/PA7 as SCK/MISO/MOSI
with an 18 MHz controller limit and uses PA4 as the W25Q64 chip select.
Obtain the bus with `YI_DT_GET(SPI1)`. Each `yi_spi_transceive()` supplies a
`yi_spi_transfer_config_t` containing the device frequency, mode, and optional
chip-select GPIO, allowing devices with different requirements to share a bus.

The W25Q64 is registered as an independent Flash device. Application code gets
it with `YI_DT_GET(W25Q64)` and uses `yi_flash_read()`, `yi_flash_write()`, and
`yi_flash_erase()`; SPI NOR commands and busy polling remain inside the driver.

The AT24C02 is registered as an independent EEPROM device on `soft_i2c0`.
Application code gets it with `YI_DT_GET(AT24C02)` and uses
`yi_eeprom_read()` and `yi_eeprom_write()`; page splitting and ACK polling
remain inside the driver.

The example also starts a 500 ms periodic `yi_soft_timer`. Software timer
callbacks are dispatched by `yi_soft_timer_process()` in the main loop, so
callbacks execute outside interrupt context. Long blocking operations delay
callback dispatch even though the SysTick time base continues advancing.
