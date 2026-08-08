<!--
File: README.md
Description: Record the integration state of the Yi STM32H743 board.
Author: Don
Date: 2026-07-31
Version: 1.0.0
-->

# Yi STM32H743

Reference MCU: STM32H743ZIT6, LQFP144, 2 MiB Flash and 1,040 KiB total SRAM.

The board identity is reserved. It becomes buildable after the STM32H7 SoC
backend, DeviceTree description and `st-stm32h7.cmake` adapter are added.
Board-specific clocks, cache/MPU policy and pin assignments must be confirmed
against the PCB.
