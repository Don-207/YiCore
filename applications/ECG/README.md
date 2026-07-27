# ECG

YiCore application target:

Vendor: `STMicroelectronics`
MCU: `stm32f103xe` (`stm32f1`)
Board: `ECG-Board`

## Build

1. Install Python 3.9 or newer and Keil MDK-ARM with STM32F1 support.
2. Open `MDK-ARM/ECG.uvprojx`.
3. Build the `ECG` target.

The Keil pre-build command generates this application's DeviceTree sources in
`generated/`.

- Keep the shared board files under `boards/ECG-Board/` unchanged.
- Edit `app-gpios.dtsi` for application-specific GPIO assignments.
- Edit `app-pinctrl.dtsi` for application-specific peripheral pinmux.
- Edit `app-devices.dtsi` for application-specific device and bus properties.
- Edit `app.dts` to enable or disable devices exposed by the board.

Create a new board directory only when the physical PCB wiring changes.

The project references shared YiCore sources, the STM32F1 SoC backend, CMSIS,
and STM32Cube HAL from the repository root. Do not copy vendor libraries into
this application.

## ECG upload

The application samples the ADS1298 at 500 SPS and uploads every fifth sample
over USART1 at 115200 8N1 (100 frames/s). Channels 1, 2 and 3 are Lead I,
Lead II and chest V. Each packet is 15 bytes:

`AA 55 0F 00 84 index I_LO I_HI II_LO II_HI V_LO V_HI status CRC_LO CRC_HI`

The CRC is CRC-16/MODBUS over bytes 0 through 12. Lead status bits are
RA, LA, LL and V in bits 0 through 3. Derived limb leads and BPM are calculated
by the PC tool under `Tools/`.
