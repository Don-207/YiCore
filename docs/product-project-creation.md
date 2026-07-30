<!--
File: product-project-creation.md
Description: YiCore independent product project creation and build workflow.
Author: Don
Date: 2026-07-29
Version: 1.1.0
-->

# YiCore 新建产品工程流程

本文说明如何使用统一的 `yi` 命令创建一个独立的 YiCore 产品仓库。新流程不再使用
`create-app.cmd`、`create-board.cmd`、`create-product.cmd`、
`create-boot.cmd` 或 `create-test.cmd`。

## 1. 环境准备

Windows 开发环境需要：

- Git；
- Python 3；
- Keil MDK（使用 Keil 工程时）；
- CMake、Ninja 和 Arm GNU Toolchain（使用 GCC 工程时）。
- HPMicro 推荐的 RISC-V GNU Toolchain（构建 HPM5301 时）。

可先确认基础工具：

```powershell
git --version
python --version
cmake --version
ninja --version
arm-none-eabi-gcc --version
```

HPM5301 推荐工具链已安装在下列位置时，配置根目录而不是 `bin` 目录：

```powershell
$env:GNURISCV_TOOLCHAIN_PATH = `
  "D:\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win"
& "$env:GNURISCV_TOOLCHAIN_PATH\bin\riscv32-unknown-elf-gcc.exe" --version
```

## 2. 创建独立产品仓库

以下示例创建名为 `ProductName` 的产品。建议产品目录使用有意义且稳定的名称，因为
生成的 Keil 工程名会使用该目录名。

```powershell
mkdir ProductName
cd ProductName
git init -b main

git clone https://github.com/Don-207/YiCore.git YiCore
python -m pip install west
```

此时目录至少包含：

```text
ProductName/
├── .git/
└── YiCore/
```

## 3. 创建产品本地板级配置

直接运行以下命令可进入提示模式：

```powershell
.\YiCore\yi.cmd board create
```

命令会依次提示选择 MCU 厂商/型号、兼容参考板，并输入新板卡 ID、显示名称和描述。
直接按 Enter 可接受提示中的默认值。

也可以从命令行一次性提供参数，适用于脚本和自动化：

从兼容的参考板复制一份产品专用板级配置：

```powershell
.\YiCore\yi.cmd board create product-name-stm32f103 `
  --model stm32f103xe `
  --from-board fire-mini-stm32f103 `
  --display-name "Product Name STM32F103" `
  --description "Product Name controller board"
```

默认输出位置为当前产品根目录下的：

```text
boards/product-name-stm32f103/
```

其中 `board.json` 用于描述板卡标识和 MCU 型号，`board.dts` 及相关 DTSI 文件用于描述
板级硬件。

创建完成后，必须按实际 PCB 检查并修改以下内容：

- 时钟源和系统时钟；
- GPIO 引脚及电气模式；
- UART、SPI、I2C、CAN 等外设引脚；
- 中断和 DMA 配置；
- Flash 容量、RAM 容量及链接布局；
- 板载 LED、按键、传感器和外部存储器。

板级 ID 必须唯一。目标目录已存在时，创建命令会拒绝覆盖。

可用以下命令查看 YiCore 已支持的参考板：

```powershell
.\YiCore\yi.cmd boards
```

YiCore 按 Zephyr 的硬件分层区分 SoC 兼容组和具体料号。例如 STM32F103ZCT6
使用 `stm32f103xe` SoC/CMSIS 兼容组，具体料号、封装和内存属于产品板卡：

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

板卡还必须根据原理图配置时钟、GPIO、复用引脚和外设节点。

HPM5301 日常创建推荐直接使用交互模式：

```powershell
.\YiCore\yi.cmd board create
```

依次选择 `HPMicro`、`hpm5301` 和 `HPM5301EVKLite`，然后输入产品板卡 ID、显示名称
和描述。完整参数形式主要用于脚本和自动化：

```powershell
.\YiCore\yi.cmd board create product-name-hpm5301 `
  --model hpm5301 `
  --from-board hpm5301evklite `
  --display-name "Product Name HPM5301" `
  --description "Product Name HPM5301 controller board"
```

首版工程沿用官方 SDK 的 `hpm5301evklite` 底层板级启动配置。若产品 PCB 的晶振、
Flash、调试串口或引脚与 EVK 不同，应先在 YiHAL-HPMicro 中增加产品专用 SDK board，
再修改生成工程中的 `BOARD`，不能只改 YiCore 的 `board.dts`。

## 4. 生成默认 application 工程

在产品根目录执行以下命令进入板卡选择提示：

```powershell
.\YiCore\yi.cmd product create
```

命令会列出当前产品 `boards/` 下注册的板卡，可输入序号或板卡 ID。也可以直接通过
参数指定板卡，适用于自动化：

```powershell
.\YiCore\yi.cmd product create --board product-name-stm32f103
```

该命令在当前仓库中原地创建 application，不会再创建一层同名产品目录。生成后的主要
结构如下：

```text
ProductName/
├── boards/
│   └── product-name-stm32f103/
├── firmware/
│   ├── common/
│   │   ├── Core/
│   │   └── cubemx/
│   ├── images/
│   │   └── application/
│   │       ├── generated/
│   │       ├── VERSION
│   │       ├── app.dts
│   │       ├── app-devices.dtsi
│   │       ├── app-gpios.dtsi
│   │       ├── app-pinctrl.dtsi
│   │       └── main.c
│   ├── linker/
│   │   └── gcc/
│   └── projects/
│       ├── gcc/
│       │   ├── CMakeLists.txt
│       │   └── arm-none-eabi.cmake
│       └── keil/
│           └── ProductName.uvprojx
└── YiCore/
```

其中：

- `firmware/common` 保存多个镜像共享的 STM32/CubeMX 文件；
- `firmware/images/application` 保存 application 独有的入口、版本和设备树；
- `firmware/projects/keil` 保存扁平化的 Keil 工程；
- `firmware/projects/gcc` 是 application、bootloader 和 test 共用的 GCC 入口；
- `boards` 保存产品自己的板级配置；
- `YiCore` 继续作为独立 Git 子模块维护。

如果 application 已经存在，命令会拒绝覆盖。

HPM5301 生成的结构更精简，入口和 CMake 工程分别位于：

```text
firmware/images/application/main.c
firmware/projects/gcc/CMakeLists.txt
```

同时生成 `yi-manifest.yml`，其中固定 YiHAL-HPMicro 版本。执行以下命令会下载该模块并
递归初始化其中固定版本的官方 `hpm_sdk`：

```powershell
.\YiCore\yi.cmd update
```

## 5. 添加可选固件镜像

产品默认只生成 application。需要 bootloader 或板级测试镜像时，在产品根目录显式
添加：

```powershell
.\YiCore\yi.cmd image add bootloader
.\YiCore\yi.cmd image add test
```

添加 bootloader 时会初始化 YiCore 固定版本的 MCUboot 子模块。每个新镜像会得到独立
的 `main.c`、`VERSION`、设备树和生成目录，同时生成对应的 Keil 工程：

```text
firmware/projects/keil/ProductName-bootloader.uvprojx
firmware/projects/keil/ProductName-test.uvprojx
```

镜像目录已经存在时，命令会拒绝覆盖。

## 6. 使用 Keil 构建

根据需要打开对应工程：

```text
firmware/projects/keil/ProductName.uvprojx
firmware/projects/keil/ProductName-bootloader.uvprojx
firmware/projects/keil/ProductName-test.uvprojx
```

构建前会根据当前镜像的 `app.dts` 和 `VERSION` 重新生成设备树及构建信息。首次构建前
应检查：

- Keil Device Family Pack 是否安装；
- 编译器版本是否与工程配置兼容；
- 调试器类型和接口是否正确；
- Flash 下载算法是否匹配目标 MCU；
- scatter 文件和实际 Flash 分区是否一致。

## 7. 使用 GCC 构建

在产品根目录运行统一命令即可构建 application：

```powershell
.\YiCore\yi.cmd build
```

默认构建目录为 `build/application`。需要删除该镜像的旧构建目录并重新配置时：

```powershell
.\YiCore\yi.cmd build -p always
```

构建已添加的可选镜像：

```powershell
.\YiCore\yi.cmd build --image bootloader
.\YiCore\yi.cmd build --image test
```

构建成功后，对应构建目录中会生成 `.elf`、`.hex`、`.bin` 和 `.map` 文件。

HPM5301 当前使用官方 SDK 的 CMake 入口，构建命令如下：

```powershell
$env:GNURISCV_TOOLCHAIN_PATH = `
  "D:\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win"
$env:Path = "C:\Qt\Tools\Ninja;$env:Path"

cmake -S firmware\projects\gcc -B build\application -G Ninja
cmake --build build\application --parallel
```

输出位于 `build/application/output/`。当前 HPM5301 路径只生成 application；STM32
专用的 Keil、MCUboot 和 `image add bootloader|test` 尚未接入 HPM5301。

## 8. 首次提交建议

完成板级修改并确认至少一个工具链能够构建后，再提交产品仓库：

```powershell
git status
git add .gitmodules boards firmware YiCore
git commit -m "feat: initialize ProductName firmware"
```

`YiCore` 在产品仓库中记录的是固定提交，不应把 YiCore 内部文件直接提交到产品仓库。
更新 YiCore 后应重新构建全部镜像，并在产品仓库中提交新的子模块指针。

## 9. 推荐验证顺序

1. 检查 `boards/<board-id>/board.json` 中的 MCU 型号；
2. 检查 `board.dts`、引脚、时钟和存储布局；
3. 先构建 application；
4. 查看 `.map` 文件，确认 Flash/RAM 没有溢出；
5. 烧录并检查复位、时钟、调试串口和基础 GPIO；
6. 再添加并验证 bootloader；
7. 最后添加 test 镜像完成外设自检。

执行硬件验证时，至少观察复位脚、系统时钟、调试串口和一个已知 GPIO。若启动失败，
优先检查复位原因、Fault 寄存器、向量表地址、链接地址和时钟配置。

## 10. 常见问题

### 找不到 YiCore west 项目

首次创建时可临时克隆 YiCore；生成 `west.yml` 后由 west 统一管理：

```powershell
git clone https://github.com/Don-207/YiCore.git YiCore
.\YiCore\yi.cmd update
```

### 找不到产品板卡

确认板卡位于：

```text
boards/<board-id>/board.json
```

并且 `board.json` 中的 `id` 与目录名完全一致。

### 目标目录或镜像已存在

创建命令默认禁止覆盖，以避免损坏已有工程。请检查命令是否在正确目录执行；不要通过
删除已有目录来绕过保护，除非已经确认其中没有需要保留的修改。

### 切换镜像后仍使用旧配置

不同镜像应使用不同构建目录。必要时删除对应的 `build/<image>`，然后重新执行 CMake
配置。

### 更新 YiCore 后提交历史发生变化

如果 YiCore 上游进行过历史重写，旧克隆不应直接普通拉取。建议重新克隆 YiCore
子模块，或在确认没有 YiCore 内部修改后，将子模块重置到产品仓库记录的新提交。
