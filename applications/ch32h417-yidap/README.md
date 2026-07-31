# CH32H417 YiDAP

This application runs the shared YiDAP lifecycle with CherryDAP on the
CH32H417 V3F core. USBFS uses the dedicated PA11/PA12 pins and exposes the
CMSIS-DAP bulk interface plus CherryDAP's CDC interface.

Target debug routing follows the HPM5301 YiLink signal convention:

- PA4: TDO
- PA5: TDI
- PA6: SWCLK/TCK
- PA7: SWDIO/TMS
- PA8: active-low nRESET

SWD and JTAG currently use direct GPIO signaling. CH32H417 SPI1 maps SCK,
MISO, and MOSI to PA5, PA6, and PA7, which does not match the required debug
clock/data roles, so the firmware deliberately does not enable SPI emulation.

The same USBFS CMSIS-DAP interface also exposes peripheral control:

- `0x80..0x82`: I2C2 on PC0=SCL and PC1=SDA (AF9).
- `0x83..0x85`: SPI3 on PA14=SCK, PA15=MISO, PA13=MOSI and PB12=nCS.
- `0x86..0x87`: FPGA capability and raw full-duplex exchange at 12 MHz,
  mode 0, MSB first.

Each transaction is limited to 48 data bytes so the command and response fit
one 64-byte CMSIS-DAP USBFS packet. All hardware polling has a timeout so a
disconnected target cannot permanently stall USB processing.

Configure and build with the WCH GCC15 toolchain:

```powershell
cmake -S applications/ch32h417-yidap -B build/ch32h417-yidap -G Ninja `
  -DCMAKE_MAKE_PROGRAM=D:/toolchains/ninja-win/ninja.exe `
  -DWCH_RISCV_TOOLCHAIN_PATH='D:/toolchains/WCH/Toolchain/RISC-V Embedded GCC15'
cmake --build build/ch32h417-yidap --parallel
```
