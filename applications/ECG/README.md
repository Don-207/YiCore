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

## ECG protocol

All packets use the common `AA 55` header, a little-endian total length, ECG
slave ID `0x04`, an 8-bit message index, and a trailing CRC-16/MODBUS.
USART1 runs at 115200 8N1.

The host can send these four-byte command payloads:

- `0x00 00 00 00`: heartbeat query; the board replies with type `0x00`.
- `0x01 00 00 00`: stop unsolicited ECG upload.
- `0x01 01 00 00`: start unsolicited ECG upload.
- `0xFF 00 00 00`: query bootloader/app version and build date/time.

ECG upload is disabled after reset. ADS1298 acquisition continues internally,
but the board sends no ECG data until it receives the start command.

The ADS1298 runs at 500 SPS. After upload is enabled, the application sends
every fifth sample over USART1 (100 frames/s). Channels 1, 2 and 3 are Lead I,
Lead II and chest V. Each ECG data response is 16 bytes:

`AA 55 10 00 84 index 02 I_LO I_HI II_LO II_HI V_LO V_HI status CRC_LO CRC_HI`

The CRC covers every byte except the final two CRC bytes. Lead status bits are
RA, LA, LL and V in bits 0 through 3. Derived limb leads and BPM are calculated
by the PC tool under `Tools/`.
