# I2C drivers

`yi_i2c.h` defines message-based transfers and convenience helpers for master
write, read, and combined write/read operations. Addresses passed to the API
are seven-bit addresses and must not include the R/W bit.

Controllers that support runtime clock changes implement `yi_i2c_configure()`.
The HPMicro HPM5300 backend supports 100 kHz, 400 kHz, and 1 MHz operation and
preserves repeated START semantics for combined write/read transfers.

`yi_soft_i2c` implements a GPIO bit-banged controller with configurable bus
frequency and clock-stretch timeout. SCL and SDA require suitable pull-ups;
open-drain GPIO configuration is recommended.
