# YiCore

YiCore is a lightweight DeviceTree-based embedded device framework for MCUs,
bootloaders, and bare-metal applications.

It separates application code from board configuration and vendor HAL details.
The current reference port targets STM32F103xE and keeps compatibility with
STM32CubeMX and Keil MDK-ARM.

## Features

- Device model with automatic registration and initialization ordering
- Dependency-aware DeviceTree parser, binding validation, and C generation
- GPIO, pinmux, clock, UART, SPI, I2C, CAN, timer, and flash abstractions
- Console and leveled logging with UART or SEGGER RTT backends
- STM32F1 reference port and Fire Mini STM32F103 board description
- Dependency-free Python generator with unit tests

## Repository layout

```text
boards/       Board-level DeviceTree descriptions
core/         YiCore device model and system API
drivers/      Platform-independent driver APIs
dts/          DeviceTree bindings
ports/        Platform and debug-backend implementations
soc/          SoC-level DeviceTree descriptions
subsys/       Console, logging, and other subsystems
scripts/      DeviceTree generator and unit tests
third_party/  Vendored third-party components
linker/       GCC and Arm linker fragments
docs/         Architecture and DeviceTree documentation
examples/     Buildable board/toolchain examples
```

## Quick start

Requirements:

- Python 3.9 or newer
- Keil MDK-ARM with STM32F1 device support for the reference example
- A Fire Mini STM32F103 board or another compatible STM32F103xE target

Generate the DeviceTree sources from the repository root:

```powershell
python scripts\yi_dts_gen.py `
  --dts boards\fire-mini-stm32f103\board.dts `
  --bindings dts\bindings `
  --output generated
```

Run the generator tests:

```powershell
python -m unittest discover -s scripts\tests -v
```

Open
`examples/stm32f103-dts-demo/MDK-ARM/stm32f103-dts-demo.uvprojx` in Keil.
The project regenerates the DeviceTree sources before each build.

## Documentation

- [Architecture roadmap](docs/architecture-roadmap.md)
- [DeviceTree guide](docs/devicetree.md)
- [STM32F103 example](examples/stm32f103-dts-demo/README.md)

## Project status

YiCore is under active development. Interfaces may evolve before the first
stable release. See the architecture roadmap for implemented and planned work.

## License

YiCore is licensed under the Apache License 2.0. Third-party components remain
under their respective licenses.

