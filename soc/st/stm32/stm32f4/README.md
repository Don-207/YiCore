<!--
File: README.md
Description: Define the STM32F4 SoC backend integration boundary.
Author: Don
Date: 2026-07-31
Version: 1.0.0
-->

# STM32F4 backend

This reserved backend targets STM32F407xx with the YiHAL-ST STM32F4 CMSIS
Device and HAL packages. Before changing its platform status to `ready`, add
the HAL configuration, system clock, pinmux, GPIO, UART, SPI, I2C, timer,
interrupt/runtime and internal-flash implementations plus a GCC build adapter.
