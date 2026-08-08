# CH32H417 V3F bring-up

This image validates the CH32H417QEU V3F startup, vendor 25 MHz HSE clock
profile, YiCore interrupt/system boundary, and the evaluation-board PB1 GPIO.

Build with the WCH GNU RISC-V toolchain shipped with MounRiver Studio:

```powershell
$env:WCH_RISCV_TOOLCHAIN_PATH = `
    'D:\toolchains\WCH\Toolchain\RISC-V Embedded GCC15\bin'
cmake -S . -B build -G Ninja `
    -DCMAKE_MAKE_PROGRAM='D:\toolchains\ninja-win\ninja.exe' `
    -DBOARD=ch32h417-evt
cmake --build build
```

The build produces ELF, HEX, and BIN images. Program the V3F image with
WCH-Link. PB1 should produce a 500 ms period square wave (250 ms high and
250 ms low). No transition indicates a startup, flash layout, clock, or GPIO
failure. An incorrect period points to the provisional busy-wait delay and not
necessarily to clock-tree failure.
