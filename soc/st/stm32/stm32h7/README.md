<!--
File: README.md
Description: Define the STM32H7 SoC backend integration boundary.
Author: Don
Date: 2026-07-31
Version: 1.0.0
-->

# STM32H7 backend

This reserved backend targets STM32H743xx with the YiHAL-ST STM32H7 CMSIS
Device and HAL packages.

Implemented:

- HAL configuration baseline;
- LDO supply and HSI PLL system clock at 400 MHz;
- instruction/data cache enablement;
- DWT time services and SysTick runtime;
- GCC Flash/AXI-SRAM linker layout.

Still required before changing the platform status to `ready`: MPU policy,
pinmux, GPIO, UART, SPI, I2C, timer, internal flash, DeviceTree and GCC build
adapter.
