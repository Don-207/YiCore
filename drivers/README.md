# YiCore drivers

This directory contains platform-independent driver APIs and external-device
drivers. MCU register and vendor HAL code belongs under `soc/`; applications
should access devices through YiCore APIs and generated DeviceTree instances.

Driver categories:

- `adc`, `dac`, and `dds`: analog conversion and waveform generation
- `eeprom` and `flash`: nonvolatile storage
- `gpio`, `i2c`, `spi`, `uart`, and `can`: buses and communication
- `clock`, `pinmux`, and `timer`: MCU infrastructure interfaces
- `sensor`: temperature and measurement devices
- `led`: GPIO LED convenience driver
- `onewire`: 1-Wire bus implementation

Each category and concrete device directory contains a README with its API,
DeviceTree settings, units, limitations, and usage examples.
