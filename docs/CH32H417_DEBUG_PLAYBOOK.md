<!-- File: CH32H417_DEBUG_PLAYBOOK.md | Description: Verified CH32H417 WCH-Link debug workflow. | Author: Don | Date: 2026-08-02 | Version: 1.1.0 -->

# CH32H417 调试链路经验

本流程已使用 WCH-LinkRV 和 CH32H417 V3F 开发板验证，PC2/PC3 GPIO 翻转程序可以编译、下载并运行。

## 固定环境

```text
芯片：CH32H417，V3F 主核
调试器：WCH-LinkRV
虚拟串口：COM10（UART，不是 SWD 下载通道；端口号可能随主机变化）
GCC：D:\toolchains\Yi\wch-riscv-gcc\15.2.0
Ninja：D:\toolchains\ninja-win\ninja.exe
OpenOCD：D:\toolchains\Yi\wch-riscv-gcc\OpenOCD\OpenOCD\bin\openocd.exe
配置：wch-riscv.cfg；GDB 3333；Telnet 4444
```

MounRiver 调试器选择 `WCH-Link`、`V3F (Master)`。COM4 仅用于串口；下载调试使用 SWCLK、SWDIO、GND。

## 构建配置

```powershell
$env:WCH_RISCV_TOOLCHAIN_PATH = "D:\toolchains\Yi\wch-riscv-gcc\15.2.0"
cmake -S . -B build-h417-2 -G Ninja -DCMAKE_MAKE_PROGRAM="D:\toolchains\ninja-win\ninja.exe"
cmake --build build-h417-2
```

必须使用 H417 SDK、`startup_ch32h417_v3f.S`、`Link_v3f.ld`、编译宏 `CH32H417;Core_V3F` 和 `rv32imac_zaamo_zalrsc_xw/ilp32`。独立工程需提供 `ch32h417_it.h`。GPIO 相关工程还需链接 `ch32h417_fmc.c`、`ch32h417_flash.c`，使用 WCH `debug.c` 时还需 `ch32h417_usart.c`。

## GPIO 翻转关键点

```c
SystemInit();
SystemAndCoreClockUpdate();
Delay_Init();
RCC_HB2PeriphClockCmd(RCC_HB2Periph_GPIOC, ENABLE);
gpio.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
gpio.GPIO_Speed = GPIO_Speed_Very_High;
gpio.GPIO_Mode = GPIO_Mode_Out_PP;
GPIO_Init(GPIOC, &gpio);
for (;;) {
    Delay_Ms(1000);
    state = (state == Bit_RESET) ? Bit_SET : Bit_RESET;
    GPIO_WriteBit(GPIOC, GPIO_Pin_2 | GPIO_Pin_3, state);
}
```

必须定义 `Core_V3F`，否则 WCH `debug.c` 中的 `Delay_Ms()` 实现不会启用；必须调用 `SystemAndCoreClockUpdate()`，否则延时基准可能不正确。

## OpenOCD 下载运行

启动 MounRiver 自带的 WCH OpenOCD，监听 `3333/4444`。通过 Telnet 4444 下载时要选择 V3F 主核，并在下载后显式运行：

```text
program D:/code/mcu/YiLink/ch32h417_pc2_led/build-h417-2/ch32h417_pc2_led reset
targets wch_riscv.cpu.0
reset run
resume
```

`program ... reset` 后目标有时仍处于暂停状态，必须 `reset run`/`resume`；否则会出现“下载后 LED 不亮，按开发板复位后才运行”。

## V3F/V5F 双核镜像烧录

CH32H417 双核工程不能依次把 V3F ELF 和 V5F ELF 当作两个独立镜像烧录。
实测第二次 `program` 可能按 Flash 擦除粒度破坏第一次写入的 V3F 区域，现象为：

- V3F 和 V5F 单独烧录都报告 `Verified OK`；
- 最后复位后 PC2、PC3 均不工作；
- GPIOC 配置寄存器保持为零；
- 再次执行 `verify_image` 时，地址 `0x00000000` 与 V3F ELF 大量不一致。

正确方法是构建一个合并 BIN：

```text
0x00000000  V3F binary
...         以 0xFF 填充
0x00010000  V5F binary
```

只对合并镜像执行一次擦除、写入和校验：

```text
targets wch_riscv.cpu.0
reset halt
program D:/path/to/CH32H417DualCoreGPIO.bin verify 0x00000000
reset run
resume
```

该流程已使用 `CH32H417DualCoreGPIO.bin` 实测验证。执行 `reset run` 和
`resume` 后 PC2、PC3 会直接开始闪烁，不需要再按开发板 RESET。物理复位只作为
目标未正常恢复运行或 WCH-Link 无法重新连接时的恢复手段。

## 调试检查

```text
targets wch_riscv.cpu.0
reset halt
reg pc
mdw 0x40011000 4
resume
halt
```

GPIOC 基址为 `0x40011000`；`+0x00` 为 CFGLR，`+0x04` 为 CFGHR，`+0x08` 为 INDR，`+0x0C` 为 OUTDR。PC2/PC3 翻转应观察 OUTDR 的 bit2/bit3。

## 常见问题

| 现象 | 处理 |
| --- | --- |
| 找不到 Ninja | 指定 `D:\toolchains\ninja-win\ninja.exe` |
| RISC-V 被当作 Windows 编译器 | 设置 `CMAKE_SYSTEM_NAME Generic` 和 `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY` |
| `ch32h417_it.h` 缺失 | 在工程 `src` 提供最小中断头文件 |
| `FLASH_BOOT_GetMode` 未定义 | 加入 `ch32h417_flash.c` |
| `Delay_Ms()` 不工作 | 定义 `Core_V3F` 并调用 `SystemAndCoreClockUpdate()` |
| 下载后不运行 | 选择 `wch_riscv.cpu.0`，执行 `reset run`、`resume` |
| 双核镜像分别校验成功但复位后均不运行 | 合并 V3F/V5F 为一个 BIN，从 `0x00000000` 单次烧录 |
| COM4 无法下载 | COM4 是 UART，SWD 下载使用 SWCLK/SWDIO/GND |
| GDB 卡住 | 关闭 MounRiver，确保只有一个 OpenOCD/GDB 会话占用 WCH-Link |

已验证工程：[ch32h417_pc2_led](../ch32h417_pc2_led/)，当前为 PC2/PC3 同步输出、1 秒翻转。
