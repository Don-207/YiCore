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
core/         Platform-independent device model and system API
drivers/      Platform-independent driver APIs and external-device drivers
dts/          Bindings and SoC-level DeviceTree descriptions
soc/          Vendor/family MCU backends implemented by YiCore
subsys/       Console, logging, timer, and other subsystems
ports/        Architecture-independent debug and transport backends
vendor/       Unmodified vendor CMSIS and STM32Cube dependencies
third_party/  Other vendored components such as SEGGER RTT
scripts/      DeviceTree generator and unit tests
linker/       GCC and Arm linker fragments
docs/         Architecture and DeviceTree documentation
examples/     Buildable board/toolchain examples
```

The dependency direction is:

```text
application -> core/subsys/drivers -> soc backend -> vendor library -> hardware
```

Files below `vendor/` retain their upstream licenses and should not be edited
locally. Platform-specific HAL types and calls belong below `soc/`, while
public driver APIs remain below `drivers/`.

## Quick start

Requirements:

- Python 3.9 or newer
- Keil MDK-ARM with STM32F1 device support for the reference example
- A Fire Mini STM32F103 board or another compatible STM32F103xE target

Generate the DeviceTree sources from the repository root:

```powershell
python scripts\yi_dts_gen.py `
  --dts examples\stm32f103-dts-demo\app.dts `
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

Create another MCU application interactively:

```powershell
.\create-project
```

The creator asks for the project name, then lists supported vendors, MCU
series/models, and compatible boards. MCU targets are recorded in
`scripts/yi_supported_targets.json`. Boards are discovered automatically from
`boards/*/board.json`.

You can also pass the project name directly:

```powershell
.\create-project product-bootloader
```

The default destination is `applications/<project-name>/`. Select a target and
destination non-interactively with:

```powershell
.\create-project controller-app `
  --vendor st `
  --series stm32f1 `
  --model stm32f103xe `
  --board fire-mini-stm32f103 `
  --output-root applications
```

One MCU may have multiple boards. Give each physical board its own
`boards/<board-id>/board.json` manifest and DTS files; no central board
registry needs to be edited. When more than one board supports the selected
MCU, choose one interactively or pass `--board`; the creator rejects a board
that does not match the selected MCU.

`create-project.cmd` is a thin launcher for `scripts/yi_create_project.py`. If
the repository root is on `PATH`, it can also be called as
`create-project product-bootloader`.

The creator refuses to overwrite an existing project directory. It gives each
application a private `generated/` directory while sharing YiCore, SoC, CMSIS,
and HAL sources from the repository root.

Create a new board interactively:

```powershell
.\create-board
```

The creator asks for a new board id, MCU target, and compatible reference
board. It copies the reference board and writes a board-local `board.json`
manifest. A non-interactive example is:

```powershell
.\create-board product-a-stm32f103 `
  --model stm32f103xe `
  --from-board fire-mini-stm32f103 `
  --display-name "Product A STM32F103" `
  --description "Product A controller board"
```

After creation, edit the new directory under `boards/` to match the physical
PCB, then pass its id to `create-project --board`. The creator refuses to
overwrite an existing board.

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
