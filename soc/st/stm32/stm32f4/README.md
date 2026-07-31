<!--
File: README.md
Description: Define the STM32F4 SoC backend integration boundary.
Author: Don
Date: 2026-07-31
Version: 1.0.0
-->

# STM32F4 backend

This reserved backend targets STM32F407xx with the YiHAL-ST STM32F4 CMSIS
Device and HAL packages.

Implemented:

- HAL configuration baseline;
- HSI PLL system clock at 168 MHz;
- DWT time services and SysTick runtime;
- GCC Flash/main-SRAM linker layout.

Still required before changing the platform status to `ready`: pinmux, GPIO,
UART, SPI, I2C, timer, internal flash, DeviceTree and GCC build adapter.
