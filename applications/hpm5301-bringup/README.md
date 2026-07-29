# HPM5301 architecture bring-up

This minimum image validates YiCore's RISC-V architecture boundary on the
official HPM5301EVKLite. The pinned HPM SDK owns reset entry, trap dispatch,
clock initialization, pinmux, UART console, boot header and Flash XIP linker
layout. YiCore contributes only its vendor-neutral interrupt primitives and
the thin test application.

Prerequisites:

- YiHAL-HPMicro with `hpm_sdk` at
  `88b01b43900d8c30844a1e5cdd3f3b7aff6db40e`;
- HPMicro GNU RISC-V toolchain with B-extension multilib;
- CMake and Ninja;
- HPM5301EVKLite plus CMSIS-DAP or J-Link.

PowerShell build:

```powershell
$env:HPM_SDK_BASE = 'D:\code\mcu\YiHAL-HPMicro\hpm_sdk'
$env:GNURISCV_TOOLCHAIN_PATH = `
    'D:\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win'
cmake -S . -B build -G Ninja -DBOARD=hpm5301evklite
cmake --build build
```

Run these commands from this directory. The default image is `flash_xip` and
uses the official 1 MiB QSPI NOR layout beginning at `0x80003000`.
The project selects the portable `rv32imac_zicsr_zifencei` baseline with the
`ilp32` ABI. The official GCC 13.2 toolchain is detected by the SDK and extends
that baseline with HPM5301's `zba/zbb/zbc/zbs` multilib automatically.

xPack GCC 12.4 remains usable as a compatibility build with
`-DCUSTOM_TARGET_TRIPLET=riscv-none-elf`, but it does not pass the SDK's
official-toolchain detection and encounters an internal compiler error when
the B extensions are forced.

Connect UART0 TX at J3.36 (SoC PA00) and UART0 RX at J3.38 (SoC PA01)
through a 3.3 V USB-to-serial adapter. At 115200 8N1, the console should first
print:

```text
YiCore HPM5301 architecture bring-up ready
heartbeat 0
```

One heartbeat follows every second. No output indicates startup/flash/pinmux
failure; a banner without heartbeats points to the application or delay path.
