<!--
File: README.md
Description: Document the Yi HPM5301 board contract and SDK adaptation.
Author: Don
Date: 2026-07-31
Version: 1.0.0
-->

# Yi HPM5301

This board is the reusable YiCore definition for the HPM5301 hardware shared
by YiLink-class products. It uses the official `hpm5301evklite` power, 24 MHz
clock, startup and XIP flash baseline.

Fixed routing:

- PB08: UART2_TXD
- PB09: UART2_RXD
- PB12: active-low status LED
- PA02: I2C0_SCL
- PA03: I2C0_SDA
- PA26: SPI1_CS0
- PA27: SPI1_SCLK
- PA28: SPI1_MISO
- PA29: SPI1_MOSI

The I2C connector requires external pull-ups. The SPI routing is shared with
the YiLinkPro HPM5301-to-FPGA transport. Product-specific USB, JTAG/reset and
FPGA control signals remain in their product repositories.
