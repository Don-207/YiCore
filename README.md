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
applications/ Framework-owned utility images such as the reference bootloader
```

The dependency direction is:

```text
application -> core/subsys/drivers -> soc backend -> vendor library -> hardware
```

## Product repositories

Independent products should live in separate repositories and pin YiCore as a
Git submodule. Product firmware, PCB descriptions, host tools, captures, and
release binaries stay with the product; reusable drivers and framework changes
stay in YiCore.

```text
product/
  applications/<product>/
  boards/<product-board>/
  Tools/<product-tool>/
  YiCore/                    Git submodule pinned to a tested commit
```

This prevents a YiCore update from changing released products automatically.
Each product advances its `YiCore` submodule pointer only after its own build
and hardware validation. YiECG is the first product split this way:
`https://github.com/Don-207/YiECG`.

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

Create a standalone product with the application image only:

```powershell
.\create-app ProductName --board fire-mini-stm32f103
```

The default output is `<current-directory>/ProductName`. The generated product
contains shared `firmware/common`, one `firmware/images/application`, flat Keil
and GCC project directories, linker descriptions, a product-local board copy,
and a pinned local YiCore checkout. It does not create bootloader or test
images by default.

Add optional images from the product root:

```powershell
YiCore\create-boot --product-root .
YiCore\create-test --product-root .
```

`create-boot` initializes the pinned MCUboot dependency. Both commands refuse
to overwrite an existing image. Keil metadata stays flat under
`firmware/projects/keil`; GCC uses one CMake entry selected with
`YI_PRODUCT_IMAGE=application|bootloader|test`.
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
PCB, then pass its id to `create-app --board`. The creator refuses to
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
