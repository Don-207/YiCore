# GPIO interface

`yi_gpio.h` defines input/output values, pull and drive modes, interrupts, and
callback registration. GPIO devices are also used as dependencies by SPI chip
selects and external-device reset, start, and data-ready signals.

Pin direction and electrical mode are configured in DeviceTree. Open-drain
buses such as I2C require external or board-provided pull-ups.
