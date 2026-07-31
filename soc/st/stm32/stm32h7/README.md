<!--
File: README.md
Description: Define the STM32H7 SoC backend integration boundary.
Author: Don
Date: 2026-07-31
Version: 1.0.0
-->

# STM32H7 backend

This reserved backend targets STM32H743xx with the YiHAL-ST STM32H7 CMSIS
Device and HAL packages. Before changing its platform status to `ready`, add
the HAL configuration, system clock, cache/MPU policy, pinmux, GPIO, UART,
SPI, I2C, timer, interrupt/runtime and internal-flash implementations plus a
GCC build adapter.
