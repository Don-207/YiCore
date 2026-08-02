<!--
File: product-project-creation.md
Description: YiCore pure-west product creation and build workflow.
Author: Don
Date: 2026-07-30
Version: 2.0.0
-->

# YiCore 新建产品工程流程

本文说明如何创建独立的 YiCore 产品仓库。YiCore、厂商 HAL、第三方库和 bootloader
全部由 west 管理，产品仓库不使用 Git submodule，也不包含 `.gitmodules`。

## 1. 环境准备

基础工具：

- Git；
- Python 3；
- west；
- Keil MDK（使用 Keil 工程时）；
- CMake、Ninja 和 Arm GNU Toolchain（使用 STM32 GCC 工程时）；
- HPMicro RISC-V GNU Toolchain（构建 HPM5301 时）。

安装并检查：

```powershell
python -m pip install west
git --version
python -m west --version
cmake --version
ninja --version
arm-none-eabi-gcc --version
```

YiCore 使用统一工具链根目录自动选择各芯片所需的编译器：

```powershell
$env:YI_TOOLCHAIN_HOME = "D:\toolchains\Yi"
```

默认目录结构为：

```text
D:\toolchains\Yi
├── arm-gnu-toolchain\15.3.rel1
├── hpmicro-riscv-gcc\13.2.0
└── wch-riscv-gcc\15.2.0
```

`yi build` 读取产品 `board.json` 和 YiCore `environments/` 清单，自动设置
`ARM_GCC_ROOT`、`GNURISCV_TOOLCHAIN_PATH` 或 `WCH_RISCV_TOOLCHAIN_PATH`。
用户不再需要逐个设置厂家环境变量。可执行文件必须位于对应版本目录的
`bin/` 下。

将变量持久化到当前用户：

```powershell
[Environment]::SetEnvironmentVariable(
  "YI_TOOLCHAIN_HOME", "D:\toolchains\Yi", "User"
)
```

## 2. 创建产品仓库并引导 YiCore

首次创建时，需要临时克隆 YiCore 来运行生成命令：

```powershell
mkdir ProductName
cd ProductName
git init -b main
git clone https://github.com/Don-207/YiCore.git YiCore
```

这里的 `YiCore/` 不是 submodule。产品生成器会把它加入 `.gitignore`，并在
`west.yml` 中记录其精确提交；之后由 west 接管。

## 3. 创建产品板卡

交互式创建：

```powershell
.\YiCore\yi.cmd board create
```

STM32F103 命令行示例：

```powershell
.\YiCore\yi.cmd board create product-name-stm32f103 `
  --model stm32f103xe `
  --from-board fire-mini-stm32f103 `
  --display-name "Product Name STM32F103" `
  --description "Product Name controller board"
```

HPM5301 示例：

```powershell
.\YiCore\yi.cmd board create product-name-hpm5301 `
  --model hpm5301 `
  --from-board hpm5301evklite `
  --display-name "Product Name HPM5301" `
  --description "Product Name HPM5301 controller board"
```

板卡生成在：

```text
boards/<board-id>/
```

提交前必须按照实际 PCB 检查：

- MCU 具体料号、封装、Flash 和 RAM；
- 晶振和系统时钟；
- GPIO 电气模式；
- UART、SPI、I2C、CAN 等复用引脚；
- 中断与 DMA；
- Flash 分区和链接地址；
- LED、按键、传感器及外部存储器。

`model` 表示 SoC/CMSIS 兼容组，`part` 表示具体芯片。例如 STM32F103ZCT6 可使用
`stm32f103xe` model，同时在 `board.json` 中记录准确料号和内存：

```json
{
  "model": "stm32f103xe",
  "part": "stm32f103zct6",
  "keil_device": "STM32F103ZC",
  "cube_package": "LQFP144",
  "flash_size": 262144,
  "ram_size": 49152
}
```

## 4. 生成默认 application

交互式选择产品板卡：

```powershell
.\YiCore\yi.cmd product create
```

或明确指定：

```powershell
.\YiCore\yi.cmd product create --board product-name-stm32f103
```

主要结构：

```text
ProductName/
├── .gitignore
├── west.yml
├── boards/
├── firmware/
│   ├── common/
│   ├── images/
│   │   └── application/
│   ├── linker/
│   └── projects/
│       ├── gcc/
│       └── keil/
└── YiCore/                 west 管理，产品 Git 忽略
```

生成的 `west.yml`：

- 固定当前 YiCore 提交；
- 从 YiCore 导入 `yi-modules.yml`；
- 根据 MCU 平台启用 `hal-st` 或 `hal-hpmicro`；
- 根据工程功能选择 `serial`、`debug` 和 `bootloader`；
- 将所有 west project 放在产品目录内部。

## 5. 初始化并下载 west 工作区

产品生成后，在产品根目录执行：

```powershell
.\YiCore\yi.cmd update
```

该命令会自动初始化本地 west 工作区并调用 `west update`。也可以直接使用 west：

```powershell
python -m west init -l .
python -m west update
```

对于已经发布的产品，新用户不需要先克隆 YiCore：

```powershell
git clone https://github.com/example/ProductName.git
cd ProductName
python -m pip install west
python -m west init -l .
python -m west update
```

west 会下载产品 `west.yml` 中固定的 YiCore，再通过 YiCore 清单下载 active 模块。

临时改变下载组：

```powershell
.\YiCore\yi.cmd update --group-filter=+bootloader,-debug
```

过滤只影响后续 west 操作，不会自动删除已经存在的 checkout。

## 6. 添加可选镜像

```powershell
.\YiCore\yi.cmd image add bootloader
.\YiCore\yi.cmd image add test
```

添加 bootloader 时，生成器会把产品 `west.yml` 中的 `bootloader` 组改为启用状态；
下一次 `yi update` 会下载 MCUboot。MCUboot 是 west project，不是 submodule。

每个镜像拥有独立的入口、版本、设备树和生成目录，并生成对应 Keil 工程：

```text
firmware/projects/keil/ProductName-bootloader.uvprojx
firmware/projects/keil/ProductName-test.uvprojx
```

HPM5301 当前只生成 application，尚未接入 STM32 专用的 Keil 和 MCUboot 镜像流程。

## 7. 构建

GCC application：

```powershell
.\YiCore\yi.cmd build
```

强制清理后构建：

```powershell
.\YiCore\yi.cmd build -p always
```

其他镜像：

```powershell
.\YiCore\yi.cmd build --image bootloader
.\YiCore\yi.cmd build --image test
```

Keil 工程位于：

```text
firmware/projects/keil/
```

首次构建应检查 Device Family Pack、编译器版本、调试器、Flash 算法、链接脚本和实际
Flash 分区是否一致。

HPM5301 使用官方 SDK CMake 入口：

```powershell
cmake -S firmware\projects\gcc -B build\application -G Ninja
cmake --build build\application --parallel
```

## 8. 锁定依赖

产品 `west.yml` 是可复现依赖的权威来源。需要记录当前全部 active project 的解析结果时：

```powershell
.\YiCore\yi.cmd manifest freeze
```

默认生成：

```text
west.lock.yml
```

更新 YiCore 或模块时，应修改 `west.yml` 或 YiCore 的 `yi-modules.yml`，执行
`yi update`，完成构建与硬件验证后再更新锁文件。

## 9. 首次提交

west 下载的目录已被 `.gitignore` 排除。产品首次提交只包含产品源码和 manifest：

```powershell
git status
git add .gitignore west.yml boards firmware
git commit -m "feat: initialize ProductName firmware"
```

不要提交 `YiCore/`、`modules/`、`bootloader/` 或 `.west/`。产品仓库中不应出现
mode `160000` gitlink。

## 10. 推荐验证顺序

1. 执行 `west list`，确认只有目标产品需要的模块为 active；
2. 检查 `board.json`、设备树、时钟、引脚和存储布局；
3. 构建 application；
4. 查看 map 文件，确认 Flash/RAM 没有溢出；
5. 烧录并检查复位、系统时钟、调试串口和基础 GPIO；
6. 添加并验证 bootloader；
7. 最后添加 test 镜像完成外设自检。

硬件启动失败时，优先检查复位原因、Fault 寄存器、向量表、链接地址和时钟配置。

## 11. 常见问题

### 找不到 `west`

```powershell
python -m pip install west
python -m west --version
```

### 找不到 YiCore

新产品首次生成前可以临时执行：

```powershell
git clone https://github.com/Don-207/YiCore.git YiCore
```

已有产品应执行 `west init` 和 `west update`，不要添加 submodule。

### west 提示 manifest import 不可用

先更新导入清单所属项目：

```powershell
python -m west update YiCore
python -m west update
```

### 不需要某个模块

在产品 `west.yml` 的 `group-filter` 中禁用对应组，例如：

```yaml
manifest:
  group-filter:
    - -debug
    - -bootloader
```

已下载目录不会自动删除，但会变为 inactive。

### 切换镜像后仍使用旧配置

使用不同的 `build/<image>` 目录，或执行：

```powershell
.\YiCore\yi.cmd build --image <image> -p always
```
