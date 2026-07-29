# YiCore

YiCore is a lightweight DeviceTree-based embedded device framework for MCUs,
bootloaders, and bare-metal applications.

It separates application code from board configuration and vendor HAL details.
The reference ports target the STM32F103xE high-density SoC group and the
HPM5301 RISC-V MCU. STM32 keeps compatibility with STM32CubeMX and Keil
MDK-ARM, while HPM5301 uses HPMicro's official SDK and GNU toolchain. Exact
orderable parts, packages, and memory sizes belong to board manifests.

## Features

- Device model with automatic registration and initialization ordering
- Dependency-aware DeviceTree parser, binding validation, and C generation
- GPIO, pinmux, clock, UART, SPI, I2C, CAN, timer, and flash abstractions
- Console and leveled logging with UART or SEGGER RTT backends
- STM32F1 reference port and Fire Mini STM32F103 board description
- HPM5301 official-SDK bring-up application and HPM5301EVKLite board
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

Create an independent product repository, add YiCore, then run every creation
operation through the unified Zephyr-style command:

```powershell
mkdir ProductName
cd ProductName
git init -b main
git submodule add https://github.com/Don-207/YiCore.git YiCore
git submodule update --init --recursive

.\YiCore\yi.cmd board create product-name-stm32f103 `
  --model stm32f103xe `
  --from-board fire-mini-stm32f103

.\YiCore\yi.cmd product create --board product-name-stm32f103
```

Following Zephyr's hardware layering, `stm32f103xe` identifies the shared SoC
and CMSIS compatibility group. A board using STM32F103ZCT6 keeps
`"model": "stm32f103xe"` and records its exact `"part"`, package, Flash, and
RAM in `board.json`.

The board is generated below the product root at
`boards/product-name-stm32f103`. The product command then creates shared
`firmware/common`, the default `firmware/images/application`, flat Keil and
GCC project directories, and linker descriptions. It does not create
bootloader or test images by default.

Add optional images from the product root:

```powershell
.\YiCore\yi.cmd image add bootloader
.\YiCore\yi.cmd image add test
```

Adding the bootloader initializes the pinned MCUboot dependency. Both commands
refuse to overwrite an existing image. Keil metadata stays flat under
`firmware/projects/keil`. From the product root,
`.\YiCore\yi.cmd build` builds the GCC application; use
`--image bootloader|test` for optional images.

After creation, edit the new directory under `boards/` to match the physical
PCB before running `yi product create`. The creator refuses to overwrite an
existing board.

For HPM5301, select the `hpm5301` target and copy the reference EVK:

```powershell
.\YiCore\yi.cmd board create product-name-hpm5301 `
  --model hpm5301 `
  --from-board hpm5301evklite
.\YiCore\yi.cmd product create --board product-name-hpm5301
.\YiCore\yi.cmd update
```

The generated GCC/CMake project uses the official HPM SDK. Set
`GNURISCV_TOOLCHAIN_PATH` to the toolchain root (not its `bin` directory), then
configure and build `firmware/projects/gcc`.

## Documentation

- [Product project creation workflow](docs/product-project-creation.md)
- [Architecture roadmap](docs/architecture-roadmap.md)
- [DeviceTree guide](docs/devicetree.md)
- [STM32F103 example](examples/stm32f103-dts-demo/README.md)

## Thin applications

New applications follow an application-centric layout and select hardware at
build time through the unified command:

```text
yi app create MyApp
yi boards
yi sdk list
yi sdk verify
yi update -m yi-manifest.yml
yi manifest freeze -m yi-manifest.yml
yi build -b fire-mini-stm32f103 applications/MyApp
yi build -p always -b fire-mini-stm32f103 applications/MyApp
```

This creates:

```text
applications/MyApp/
├── CMakeLists.txt
├── app.conf
├── app.overlay
├── VERSION
└── src/main.c
```

The application contains no startup, linker, board, SoC or vendor-library
copies. Configure with `-DBOARD=<board-id>` and `-DYICORE_ROOT=<path>`.

The STM32F103xE GCC adapter is available for
`-DBOARD=fire-mini-stm32f103`. On Windows run `yi.cmd` when the repository
directory is not on `PATH`.

The legacy `create-app.cmd`, `create-board.cmd`, `create-product.cmd`,
`create-boot.cmd`, and `create-test.cmd` entry points have been removed.
There are no compatibility aliases; use the corresponding nested `yi`
commands.

`yi build` prefers external workspace modules at `modules/hal/st` and
`modules/lib/cmsis`. If they are absent it uses the compatible packages below
`YiCore/vendor/`, allowing SDK repositories to be extracted without breaking
existing products.

The optional multi-repository manifest format is demonstrated by
`yi-manifest.example.yml`. `yi update` refuses to switch a repository with
uncommitted changes. `yi manifest freeze` records checked-out full commit SHAs
in `yi-manifest.lock.yml`.

Install CLI dependencies with:

```text
python -m pip install -r YiCore/scripts/requirements.txt
```

## Project status

YiCore is under active development. Interfaces may evolve before the first
stable release. See the architecture roadmap for implemented and planned work.

## License

YiCore is licensed under the Apache License 2.0. Third-party components remain
under their respective licenses.
