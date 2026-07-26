# I2C drivers

`yi_i2c.h` defines message-based transfers and convenience helpers for master
write, read, and combined write/read operations. Addresses passed to the API
are seven-bit addresses and must not include the R/W bit.

`yi_soft_i2c` implements a GPIO bit-banged controller with configurable bus
frequency and clock-stretch timeout. SCL and SDA require suitable pull-ups;
open-drain GPIO configuration is recommended.
