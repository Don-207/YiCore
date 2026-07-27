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
