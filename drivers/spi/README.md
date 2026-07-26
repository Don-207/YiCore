# SPI drivers

`yi_spi.h` defines full-duplex transfers with per-device frequency, mode,
chip-select GPIO, polarity, and timeout. The wrapper asserts and releases the
configured software chip select around each transaction.

`yi_soft_spi` supplies a GPIO implementation supporting modes 0 through 3.
Its practical maximum frequency is limited by GPIO calls and microsecond
delay resolution; use a SoC hardware SPI backend for high-rate converters.
