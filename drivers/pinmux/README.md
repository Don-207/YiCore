# Pinmux interface

Pinmux devices configure pins for peripheral alternate functions before UART,
SPI, I2C, CAN, timer, or ADC backends initialize. The generated configuration
contains the SoC-specific mode and remap information. A physical pin must not
be assigned to multiple enabled owners.
