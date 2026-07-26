# Flash drivers

The YiCore flash API exposes read, program, and erase operations plus device
geometry. Erase and write alignment rules are backend-specific and must be
checked before use.

Backends include STM32 internal flash and W25Q64 SPI NOR. Flash programming
can only clear bits; an erase operation restores bits to one.
