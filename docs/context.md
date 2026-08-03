<!-- File: context.md | Description: Continuation context for CH32H417 dual-core YiCore development. | Author: Don | Date: 2026-08-03 | Version: 1.0.0 -->

# CH32H417 双核开发上下文

## 当前目标

在 CH32H417QEU 上运行两个独立固件：

- V3F 使用 PC2，每 500 ms 翻转一次；
- V5F 使用 PC3，每 1 s 翻转一次；
- application 层通过 YiCore 的 system、device 和 GPIO API 实现；
- V3F 负责共享时钟初始化并从 `0x00010000` 唤醒 V5F。

正式产品工程位于工作区根目录：

```text
CH32H417DualCoreGPIO/
├── west.yml
├── boards/ch32h417-dual-core-gpio/
├── firmware/images/v3f/
├── firmware/images/v5f/
├── firmware/projects/gcc/
└── scripts/merge_dual_core_images.py
```

该结构参考 `YiLink/`，属于独立产品工程，不是
`applications/ch32h417-pc2-led` 下的单应用。

## YiCore 分支与依赖

YiCore 当前位于：

```text
branch: agent/add-ch32h417-yidap
upstream: gitee/agent/add-ch32h417-yidap
```

工作区已按 `YiCore/yi-modules.yml` 同步 YiHAL-WCH：

```text
modules/hal/wch
revision: 1a8132b1813ecda1ade48fb07fd85e2b286e8337
```

工具链与调试器：

```text
GCC: D:\toolchains\Yi\wch-riscv-gcc\15.2.0
Ninja: D:\toolchains\ninja-win\ninja.exe
OpenOCD: D:\toolchains\Yi\wch-riscv-gcc\OpenOCD\OpenOCD\bin\openocd.exe
OpenOCD config: 同目录 wch-riscv.cfg
WCH-Link: WCH-LinkRV 2.20
UART: COM10（端口号可能变化，不用于下载）
```

## 产品实现

两个 image 都使用独立 Devicetree overlay：

- V3F overlay 只启用 `v3f_pc2_gpio` 和 `v3f_led`，禁用 PC3；
- V5F overlay 只启用 `v5f_pc3_gpio` 和 `v5f_led`，禁用 PC2；
- 每个 target 使用独立的生成目录，避免双 application 的 DTS 产物互相覆盖。

application 入口使用：

```c
yi_system_init();
yi_device_init_all();
DEVICE_DT_GET(...);
yi_gpio_set(...);
yi_system_delay_ms(...);
yi_gpio_toggle(...);
```

V3F 仍需调用 CH32H417 专属 `NVIC_WakeUp_V5F(0x00010000)`。YiCore
目前没有通用的 secondary-core 启动 API，因此这是 application 中仅保留的 SoC
专属操作。

## YiCore 改动

### application/board 支持

`cmake/YiCoreApplication.cmake`：

- `yi_application()` 新增 `CORE` 参数；
- 支持通过 `YI_BOARD_ROOT` 使用产品自有 board。

`cmake/platforms/wch-ch32h4xx.cmake`：

- 支持 `CORE V3F` 和 `CORE V5F`；
- 分别选择 `startup_ch32h417_v3f.S`/`startup_ch32h417_v5f.S`；
- 分别选择 `Link_v3f.ld`/`Link_v5f.ld`；
- 分别定义 `Core_V3F`/`Core_V5F`；
- DTS 生成目录按 target 名称隔离。

### system API

`soc/wch/ch32h4xx/yi_ch32h417_system.c` 已扩展为双核后端：

- V3F 的 `yi_system_init()` 调用 `SystemInit()`，然后更新时钟变量；
- V5F 的 `yi_system_init()` 不重复配置共享时钟，只更新本内核时钟变量；
- `yi_system_delay_us()` 和 `yi_system_delay_ms()` 使用各内核本地 SysTick；
- V3F 使用 SysTick0 状态 bit0，V5F 使用 SysTick1 和状态 bit1；
- application 不再调用 `Delay_Init()`、`Delay_Ms()` 或
  `SystemAndCoreClockUpdate()`。

## 构建与合并镜像

构建命令：

```powershell
$env:WCH_RISCV_TOOLCHAIN_PATH = 'D:\toolchains\Yi\wch-riscv-gcc\15.2.0'
cmake -S CH32H417DualCoreGPIO\firmware\projects\gcc `
  -B CH32H417DualCoreGPIO\build -G Ninja `
  -DCMAKE_MAKE_PROGRAM='D:\toolchains\ninja-win\ninja.exe'
cmake --build CH32H417DualCoreGPIO\build
```

构建生成：

```text
CH32H417DualCoreGPIO-v3f.elf/bin/hex
CH32H417DualCoreGPIO-v5f.elf/bin/hex
CH32H417DualCoreGPIO.bin
```

`CH32H417DualCoreGPIO.bin` 是必须用于烧录的合并镜像：

```text
0x00000000  V3F binary
...         0xFF padding
0x00010000  V5F binary
```

## 已验证的烧录经验

不要分别烧录 V3F ELF 和 V5F ELF。实测第二次 `program` 可能按 Flash
擦除粒度破坏第一次写入的 V3F 区域，即使两个操作各自曾显示 `Verified OK`。
故障时两个 LED 均灭、GPIOC 配置寄存器为零，并且 `verify_image` 显示
`0x00000000` 与 V3F ELF 大量不一致。

正确流程是对合并 BIN 只执行一次写入：

```powershell
openocd.exe -f wch-riscv.cfg `
  -c 'init' `
  -c 'targets wch_riscv.cpu.0' `
  -c 'reset halt' `
  -c 'program D:/code/mcu/YiLink/YiLinkWorkspace/CH32H417DualCoreGPIO/build/CH32H417DualCoreGPIO.bin verify 0x00000000' `
  -c 'reset run' `
  -c 'resume' `
  -c 'shutdown'
```

该流程已在连接的 CH32H417 + WCH-LinkRV 上重复执行，OpenOCD 报告：

```text
Programming Finished
Verified OK
```

执行 `reset run`/`resume` 后程序会直接运行，无需人工按 RESET。物理复位用于
WCH-Link 无法重连或目标未恢复运行时的恢复。

OpenOCD 校验结束时可能报告无法恢复 `0x20000000` working area，但随后仍报告
`Verified OK`。当前实测不影响 Flash 内容和目标运行，应保留日志并继续观察。

## 已执行验证

- V3F、V5F ELF 均可用 WCH GCC 15.2.0 构建；
- 合并 BIN 自动生成；
- `YiCore/scripts/tests/test_yi_ch32h417_arch.py` 两项测试通过；
- 生成代码确认 V3F 只注册 PC2，V5F 只注册 PC3；
- 合并镜像已通过 WCH OpenOCD 写入与校验；
- application 源码中已无 `Delay_Ms`、`Delay_Init`、`debug.h`、
  `SystemInit` 或 `SystemAndCoreClockUpdate` 调用。

## 当前工作区注意事项

YiCore 中当前未提交的相关修改包括：

```text
cmake/YiCoreApplication.cmake
cmake/platforms/wch-ch32h4xx.cmake
soc/wch/ch32h4xx/yi_ch32h417_system.c
docs/CH32H417_DEBUG_PLAYBOOK.md
docs/context.md
```

`samples/boards/ch32h417/dual-core-gpio/` 是早期误放在 YiCore 内的验证工程，正式
实现迁移到 `CH32H417DualCoreGPIO/` 后已删除，避免两套实现并存。

`docs/CH32H417_DEBUG_PLAYBOOK.md` 原先也是未跟踪文件，已经补充双核合并镜像、
OpenOCD 直接运行、实际工具路径和故障诊断经验。
