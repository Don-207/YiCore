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

## YiLink / YiDap 最终实现状态

YiLink 已完全移除旧 HPM5301、CherryDAP 和 CherryUSB 依赖。当前实现由两个内核
分工：

- V3F：运行独立实现的 YiDap CMSIS-DAP 命令引擎和 GPIO bit-bang SWD；
- V5F：运行 WCH 官方 CH32H417 USBHS Device 寄存器驱动，通过共享 mailbox
  将 512 字节 CMSIS-DAP 请求交给 V3F；
- 合并镜像布局为 V3F `0x00000000`、V5F `0x00010000`；
- V3F 的 PC2 每 200 ms 翻转，V5F 的 PC3 每 800 ms 翻转；
- V5F 启动后延迟 3 秒再关闭 SWJ 并启用 USBHS，保留 WCH-Link 恢复窗口。

目标调试接口引脚固定为：

```text
PA5  SWCLK/TCK
PA6  SWDIO/TMS
PC4  TDI
PC5  TDO
PB0  nRESET（开漏）
```

板载 USBHS 的 D+/D- 使用 PB8/PB9，它们同时是 CH32H417 本机下载调试口的
SWCLK/SWDIO。程序启动 USBHS 前必须执行 `GPIO_Remap_SWJ_Disable` 释放这两个
引脚；需要 WCH-Link 恢复时，应利用上述 3 秒窗口或按住复位后连接。

### YiDap 协议覆盖

YiDap 不依赖 CherryDAP，当前独立支持 Keil 建链和下载所需的命令：

```text
DAP_Info                 0x00
DAP_HostStatus           0x01
DAP_Connect              0x02
DAP_Disconnect           0x03
DAP_TransferConfigure    0x04
DAP_Transfer             0x05
DAP_TransferBlock        0x06
DAP_TransferAbort        0x07
DAP_WriteABORT           0x08
DAP_Delay                0x09
DAP_ResetTarget          0x0A
DAP_SWJ_Pins             0x10
DAP_SWJ_Clock            0x11
DAP_SWJ_Sequence         0x12
DAP_SWD_Configure        0x13
DAP_SWD_Sequence         0x1D
```

传输层已实现 WAIT 重试、value match、match mask、transfer idle clocks、可配置
turnaround，以及 WAIT/FAULT 可选 data phase。`DAP_Info(0x04)` 返回 CMSIS-DAP
协议版本 `2.1.0`，产品固件信息返回 `YiDap 0.3.1`。当前只声明 SWD 能力，未声明
尚未实现的 JTAG、SWO 或 UART 能力。

### USBHS 与 Keil 发现条件

最终 USB 标识和接口如下：

```text
VID:          0x1A86
PID:          0xD41A
bcdDevice:    2.05
Product:      CMSIS-DAP v2
Interface:    CMSIS-DAP v2
Class:        0xFF / 0x00 / 0x00
Endpoint 1:   Bulk OUT, HS 512 bytes / FS 64 bytes
Endpoint 2:   Bulk IN,  HS 512 bytes / FS 64 bytes
Driver:       WinUSB
```

单接口 CMSIS-DAP v2 的 Product String 必须包含 `CMSIS-DAP`。只把该文本放在
Interface String 中会导致 Windows 能枚举 WinUSB，但 Keil 不显示探针。

Microsoft OS 1.0 描述符包括：

- Extended Compat ID（`wIndex=0x0004`），将接口 0 绑定到 WinUSB；
- Extended Properties（`wIndex=0x0005`），注册 CMSIS-DAP 标准接口 GUID
  `{CDB3B5AD-293B-4663-AA36-1AAE46463776}`。

Windows 会按 VID/PID 缓存 Microsoft OS 描述符。加入接口 GUID 后曾将 PID 从
`0xD417` 更新到 `0xD418`；加入 CDC-ACM 后进一步更新到 `0xD41A`，确保系统按
新复合设备重新查询描述符，而不沿用旧缓存。
设备管理器的 FriendlyName 可能仍短暂显示旧的 `YiDap V2 HS`；判断固件是否更新
应读取 `DEVPKEY_Device_BusReportedDeviceDesc` 和 Hardware ID，而不是只看缓存名称。

### 已完成实机验证

- Windows 正常枚举 `VID_1A86&PID_D41A`，设备无黄色感叹号；
- Microsoft WinUSB 驱动正常绑定；
- USB 总线上报名称为 `CMSIS-DAP v2`；
- Keil 已能发现并选择该 CMSIS-DAP v2 探针；
- WCH GCC 15.2.0 已成功构建 V3F、V5F 和合并镜像；
- 已验证合并镜像输出为 `YiLink/build/verify-ch32h417/YiLink.bin`。

后续目标侧联调建议先在 Keil 中选择 100--500 kHz SWD 时钟，依次验证 IDCODE、
复位、单字读写、块传输和 Flash 下载，再逐步提高 SWD 时钟。

## YiLink V3F GPIO、SPI 与 I2C 扩展

V3F 的产品协议层已经改为只依赖 YiCore 公共 API，默认不包含 WCH GPIO、SPI、
I2C 类型或寄存器实现。CH32H417 的时钟、GPIO 复用、轮询传输和错误标志处理集中在
`YiCore/soc/wch/ch32h4xx/`，YiLink 板级文件只负责把固定引脚绑定为 YiCore 设备。

YiCore 本轮新增或补齐：

- CH32H4xx 硬件 SPI 后端，并在平台 CMake 中默认接入；
- `yi_spi_configure()` 和 `yi_spi_get_frequency()`，用于配置并返回实际分频时钟；
- CH32H4xx 硬件 I2C 主机后端，并在平台 CMake 中默认接入；
- `yi_i2c_get_frequency()`，用于协议层查询当前总线速率；
- CH32H4xx GPIO 后端增加 GPIOF 时钟识别；
- V3F PC2 心跳灯和 SPI 软件片选均通过 `yi_gpio` 操作。

### SPI3

固定引脚为：

```text
PC10  SPI3_SCK
PC11  SPI3_MISO
PC12  SPI3_MOSI
PD0   软件 CS，低有效
```

主机命令沿用 CMSIS-DAP v2 Bulk 接口：`0x83` 查询、`0x84` 配置、`0x85`
全双工传输。支持 Mode 0--3、MSB/LSB、1--255 字节传输和 100 kHz--20 MHz
请求范围。硬件采用 2 的幂次分频，响应返回实际频率；整个事务期间由 YiCore SPI
公共层保持 PD0 为低。

### I2C4

根据 CH32H417 引脚复用表，固定引脚为：

```text
PF12  I2C4_SCL，AF2
PF13  I2C4_SDA，AF2
```

两根线配置为复用开漏，板上或外部目标必须提供与总线电压、速率和电容匹配的上拉
电阻。主机命令为 `0x80` 查询、`0x81` 配置、`0x82` 传输，支持 100 kHz、
400 kHz 和 1 MHz，单阶段写、单阶段读以及无 STOP 间隔的重复 START 写后读。
读写长度各为 1--255 字节，拒绝 `0x08` 以下和 `0x77` 以上的保留地址，并将
NACK、超时和其他总线错误映射为稳定协议状态码。

协议文档已经同步到 `YiLink/doc/SPI_PROTOCOL.md` 和
`YiLink/doc/I2C_PROTOCOL.md`。WCH GCC 15.2.0 完整构建通过，最终合并镜像为
`YiLink/build/verify-ch32h417/YiLink.bin`，大小 76148 字节，SHA256 为
`82276AA25EAB74299BA2F5B9DDAF01B998FA060C0A78CC046DAA0B1C3C728889`。

## USB CDC-ACM 虚拟串口

V5F USBHS 已扩展为四接口复合设备：

```text
Interface 0  CMSIS-DAP v2，WinUSB，EP1 OUT / EP2 IN
Interface 1  Logic Analyzer，WinUSB，EP3 OUT/IN / EP4 IN
Interface 2  CDC-ACM Control，EP5 Interrupt IN
Interface 3  CDC-ACM Data，EP6 Bulk OUT / EP7 Bulk IN
```

设备类采用复合设备 `0xEF/0x02/0x01`，CDC 描述符包含 IAD、Header、Call
Management、ACM 和 Union。V5F 支持 `GET_LINE_CODING`、`SET_LINE_CODING` 和
`SET_CONTROL_LINE_STATE`，默认 Line Coding 为 115200、8N1。CDC OUT 使用
1024 字节中断生产/主循环消费的环形队列，并通过
`yilink_usbhs_cdc_read()`/`yilink_usbhs_cdc_write()` 暴露固件接口。

在尚未指定 V3F 物理 USART 引脚前，V5F 主循环提供 64 字节 CDC 回环，便于使用
串口工具立即验证 COM 口双向传输。后续接入实际 UART 时，应以 YiCore `yi_uart`
设备替换回环，不需要修改 USB 描述符。

加入 CDC 后，高速和全速配置描述符均为 128 字节，恰好是 EP0 64 字节最大包长的
整数倍。Windows 以大于实际描述符的长度请求配置描述符时，控制 IN 传输必须在
128 字节后发送零长度包（ZLP）。缺少该 ZLP 时，设备管理器显示“未知 USB 设备
（配置描述符请求失败）”，并退化为 `VID_0000&PID_0003`。USBHS 驱动现已记录
packet-aligned short descriptor 状态并自动发送终止 ZLP。

修复后的合并镜像 `YiLink.bin` 大小为 76148 字节，SHA256 为
`7A2AD8A221E94F4A10C584D1ADE9D40D43266F8888EAE5A4326316434AAD7443`。
该镜像已经完成实机烧录和重新插拔验证，Windows 能成功识别新的复合设备及
CDC-ACM 虚拟串口。

## 当前工作区注意事项

`YiLink/doc/nanoCH32H417.pdf` 是完整电路图，仅用于核对 USB 部分，目前保持未跟踪，
不得随固件提交。`samples/boards/ch32h417/dual-core-gpio/` 是早期验证工程，正式实现
已迁移到独立产品工程，避免维护两套实现。
