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

Configure and build with the WCH GCC15 toolchain:

```powershell
cmake -S applications/ch32h417-yidap -B build/ch32h417-yidap -G Ninja `
  -DCMAKE_MAKE_PROGRAM=D:/toolchains/ninja-win/ninja.exe `
  -DWCH_TOOLCHAIN_PATH='D:/toolchains/WCH/Toolchain/RISC-V Embedded GCC15'
cmake --build build/ch32h417-yidap --parallel
```
