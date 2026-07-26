# STM32F103xE MCUboot application

This bootloader uses only the STM32F103xE 256 KiB internal flash:

| Region | Offset | CPU address | Size |
| --- | ---: | ---: | ---: |
| MCUboot | `0x00000` | `0x08000000` | 48 KiB |
| Primary slot | `0x0C000` | `0x0800C000` | 104 KiB |
| Secondary slot | `0x26000` | `0x08026000` | 104 KiB |

The application is linked at `0x0800C200`, leaving a 512-byte MCUboot image
header at the beginning of the primary slot. Both slots contain MCUboot image
trailers and therefore the usable application payload is smaller than 104 KiB.

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

Generate a project-owned key outside version control, then export only its
public half into the bootloader sources:

```powershell
python third_party\mcuboot-2.4.0\scripts\imgtool.py keygen `
  --key keys\root-ec-p256.pem --type ecdsa-p256

python third_party\mcuboot-2.4.0\scripts\imgtool.py getpub `
  --key keys\root-ec-p256.pem `
  --output applications\mcuboot-stm32f103\Core\Src\mcuboot_public_key.c
```

`keys.c` references the generated `ecdsa_pub_key` symbols. Add both files to
the bootloader target, but never add the private PEM file to the repository.

Sign the application using the same private key:

```powershell
python third_party\mcuboot-2.4.0\scripts\imgtool.py sign `
  --key keys\root-ec-p256.pem `
  --header-size 0x200 `
  --align 2 `
  --slot-size 0x1A000 `
  --version 1.0.0 `
  --pad-header `
  app.bin app.signed.bin
```

Do not use MCUboot's repository example keys for production firmware.
