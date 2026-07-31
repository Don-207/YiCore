# STM32F103xE MCUboot application

This bootloader uses only the STM32F103xE 256 KiB internal flash:

| Region | Offset | CPU address | Size |
| --- | ---: | ---: | ---: |
| MCUboot | `0x00000` | `0x08000000` | 48 KiB |
| Primary slot | `0x0C000` | `0x0800C000` | 96 KiB |
| Secondary slot | `0x24000` | `0x08024000` | 96 KiB |
| Update state | `0x3C000` | `0x0803C000` | 12 KiB |
| Swap scratch | `0x3F000` | `0x0803F000` | 4 KiB |

The final MCUboot erase page starts at `0x0800B800` and is reserved for the
fixed-address `.yi_build_info` record. Application code can read and validate
the bootloader metadata with:

```c
const yi_build_info_t *bootloader_info =
    yi_build_info_at(YI_MCUBOOT_BUILD_INFO_ADDRESS);
```

The application is linked at `0x0800C200`, leaving a 512-byte MCUboot image
header at the beginning of the primary slot. One 2 KiB erase page is reserved
for each MCUboot trailer, leaving 93.5 KiB for linked application content.

The MCUboot target must use `linker/armclang/mcuboot-stm32f103xe.sct`. The
application target must use `linker/armclang/app-slot0-stm32f103xe.sct` and set
its vector-table address to `0x0800C200`.

Before each bootloader build, regenerate its DeviceTree and build metadata:

```powershell
python scripts\yi_dts_gen.py `
  --dts applications\mcuboot-stm32f103\app.dts `
  --bindings dts\bindings `
  --output applications\mcuboot-stm32f103\generated

python scripts\yi_build_info_gen.py `
  --image bootloader `
  --version-file applications\mcuboot-stm32f103\VERSION `
  --output applications\mcuboot-stm32f103\generated\yi_build_info.c
```

Compile both `core/yi_build_info.c` and the generated `yi_build_info.c`. The
resulting `.yi_build_info` record contains the image name, version, build date,
and build time. Change `VERSION` before releasing a new bootloader. The
application uses its own `VERSION` file and generated metadata record.

Create the unsigned MCUboot image with its SHA-256 integrity TLV:

```powershell
python ..\bootloader\mcuboot\scripts\imgtool.py sign `
  --header-size 0x200 `
  --align 2 `
  --slot-size 0x18000 `
  --version 1.0.0 `
  --pad-header `
  app.bin app.mcuboot.bin
```

This configuration intentionally contains no public or private key. SHA-256
detects corruption but does not authenticate the firmware producer.

The bootloader uses scratch swapping. A downloaded image is requested as a
test image; the application confirms it after its health checks pass. Reset or
failure before confirmation causes MCUboot to restore the previous image.
