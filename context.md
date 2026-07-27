<!--
File: context.md
Function: Provide a compact project handoff context for subsequent Codex sessions.
Author: Don
Date: 2026-07-27
Version: 1.0.0
-->

# YiCore ECG 项目上下文

## 新对话使用方式

新对话开始时发送：

> 请先读取仓库根目录的 `context.md` 和当前 Git 状态，然后继续任务。以文件和 Git 为准，不要重新分析已经完成并验证的内容。

## 项目与环境

- 仓库路径：`D:\code\mcu\Bootloader\YiCore`
- GitHub：`https://github.com/Don-207/YiCore.git`
- 主线分支：`main`
- ECG 协议改造前基线提交：`1671ff6b851d5ff464181850e83561b199b53682`
- MCU：STM32F103RCT6
- 固件工具链：Keil MDK-ARM
- Qt：`C:\Qt\6.11.1\mingw_64`
- ECG 模拟器串口：`COM6`
- ECG 模拟器当前心率：60 BPM
- 实际硬件连接：5 导联 ECG

## 当前目标

跑通完整 ECG 采集链路：

1. ADS1298 采集 Lead I、Lead II 和胸导联 V。
2. MCU 按约定协议上传三路原始数据及导联状态。
3. 上电默认不上传 ECG，收到 ECG 启动控制命令后才主动上传。
4. Qt 上位机计算其余肢体导联和 BPM。
5. 使用 5 导联 ECG 模拟器检查波形、导联关系、心率和稳定性。

## 系统职责

### MCU

- 完成 ADS1298 初始化、DRDY 响应和原始采样。
- 解析心跳、ECG 启停和版本查询命令。
- 上电默认关闭 ECG 主动上传；收到启动命令后才上传。
- 上传 Lead I、Lead II、Chest V 和导联脱落状态。
- 不计算 BPM。
- 不计算 III、aVR、aVL、aVF。

### Qt 上位机

- 接收并解析 MCU 串口帧。
- 显示 I、II、III、aVR、aVL、aVF、V 共 7 路波形。
- 根据 Lead II 在上位机计算 BPM。
- 显示导联脱落状态和通信统计。
- 支持 CSV 记录。

## ECG 数据结构

```c
typedef struct
{
    int16_t lead_I;
    int16_t lead_II;
    int16_t chest_V;
    uint8_t lead_status;
} ECG_FRAME;
```

## 串口协议

- 通信参数：115200 baud、8N1、无流控。
- 控制、心跳和版本查询命令的完整帧长度：12 字节。
- 版本响应按字段索引 0–41 加尾部 CRC，完整帧长度为 44 字节。
- ECG 数据响应完整帧长度：16 字节。
- 多字节字段：小端。
- CRC：CRC-16/MODBUS，小端存放。

命令公共格式：

| 偏移 | 长度 | 内容 |
|---:|---:|---|
| 0 | 2 | 包头 `AA 55` |
| 2 | 2 | 总长度 `12` |
| 4 | 1 | 消息头 `0x04`，主机到 ECG 从机 |
| 5 | 1 | 消息索引，响应时原值返回 |
| 6 | 1 | 命令类型：心跳 `0x00`、控制 `0x01`、版本 `0xFF` |
| 7 | 1 | 控制值：停止 `0x00`、启动 `0x01`；其他命令为保留 |
| 8 | 2 | 保留 `0x0000` |
| 10 | 2 | CRC-16/MODBUS |

ECG 数据响应格式：

| 偏移 | 长度 | 内容 |
|---:|---:|---|
| 0 | 2 | 包头 `AA 55` |
| 2 | 2 | 总长度 `16` |
| 4 | 1 | 消息头 `0x84`，ECG 从机响应 |
| 5 | 1 | 主动上传消息索引，范围 0–255 |
| 6 | 1 | 响应类型 `0x02` |
| 7 | 2 | Lead I，`int16_t` |
| 9 | 2 | Lead II，`int16_t` |
| 11 | 2 | Chest V，`int16_t` |
| 13 | 1 | 导联脱落状态 |
| 14 | 2 | CRC-16/MODBUS |

`lead_status` 当前定义：

- bit0：RA
- bit1：LA
- bit2：LL
- bit3：V
- bit4：RL

## 采样率与导联计算

- ADS1298 采样率：500 SPS。
- MCU 每 5 个采样点上传一次。
- 上位机有效输入采样率：100 Hz。

上位机派生导联：

```text
III = II - I
aVR = -(I + II) / 2
aVL = I - II / 2
aVF = II - I / 2
```

BPM 基于 Lead II 的自适应 QRS 检测，仅用于工程调试，不用于医疗诊断。

## 当前固件架构

`main.c` 已只保留系统初始化、设备初始化、ECG 服务初始化和循环处理：

- `applications/ECG/Core/Src/main.c`

ECG 应用逻辑已经从 `main.c` 分离：

- `applications/ECG/App/ecg_service.c`
- `applications/ECG/App/ecg_service.h`
- `applications/ECG/App/ecg_protocol.c`
- `applications/ECG/App/ecg_protocol.h`

ADS1298 驱动：

- `drivers/adc/ads1298/yi_ads1298.c`
- `drivers/adc/ads1298/yi_ads1298.h`

设备树和板级配置：

- `applications/ECG/app.dts`
- `boards/ECG-Board/`

工程生成逻辑：

- `scripts/yi_create_project.py`
- `scripts/tests/test_yi_create_project.py`

## Qt 上位机

- 工具集合目录：`Tools/`
- ECG 上位机目录：`Tools/ECGMonitor/`
- 技术：Qt Widgets、CMake、Qt SerialPort。
- `Tools/` 采用多上位机目录结构，当前 ECG 工具位于独立子目录。
- 源码位于 `Tools/ECGMonitor/src/`。
- 协议测试位于 `Tools/ECGMonitor/tests/protocol_test.cpp`。
- 可运行发布包位于 `Tools/ECGMonitor/bin/`。
- 串口采集记录位于 `Tools/ECGMonitor/captures/`。
- 已记录真实串口采集文件，用于协议和波形检查。

## 当前验证结果

### 固件

- 最近一次 Keil 构建：0 Error、0 Warning。
- Code：36,010 B。
- RO-data：1,474 B。
- RW-data：468 B。
- ZI-data：3,220 B。
- Flash：37,732 B / 256 KiB，约 14.39%。
- RAM：3,688 B / 48 KiB，约 7.50%。

Flash 的主要占用来自 STM32 HAL 和 YiCore 通用驱动：

- HAL UART：约 4.3 KiB。
- HAL SPI：约 3.5 KiB。
- HAL DMA：约 3.3 KiB。
- HAL RCC：约 2.5 KiB。
- YiCore UART：约 2.4 KiB。
- ADS1298 驱动：约 2.2 KiB。
- ECG 协议与服务代码合计不到 1 KiB。

### 自动化测试

- `python -m unittest scripts.tests.test_yi_create_project`
- 结果：14 项通过。
- Qt `ctest` 曾因运行时 DLL 搜索路径缺失返回 `0xc0000135`；这不是协议断言失败。重新测试时应给测试程序配置 Qt/MinGW DLL 的 `PATH`。

### Git

- ECG 固件和生成脚本已提交并推送到 `origin/main`。
- 提交：`1671ff6 refactor ECG acquisition and protocol service`
- ECG 心跳、启停、版本查询、`0x02` 数据响应、LED 软定时器和
  `Tools/ECGMonitor/` 已进入本轮主线提交范围。

## 尚需继续确认

- 使用 COM6 持续采集，确认丢帧、CRC 错误和消息索引连续性。
- 确认 60 BPM 模拟信号下，上位机 BPM 长时间稳定在合理误差范围。
- 检查 I、II、III、aVR、aVL、aVF、V 的波形方向和派生关系。
- 检查各导联脱落位与实际拔线行为一致。
- 如需缩小固件，优先检查未使用的 Flash、LED、Console、RTT 设备，以及 UART/SPI 的 DMA、IT、阻塞式路径。

## 编码规范

所有新建的 MCU、Qt 和 FPGA 手写文件必须包含：

- 文件名
- 功能说明
- 作者 `Don`
- 日期，格式 `YYYY-MM-DD`
- 版本；无项目约定时从 `1.0.0` 开始

所有新增函数和变量都必须添加有意义的用途注释。注释应解释意图、约束、单位、范围、所有权、线程或中断上下文，而不是简单重复名称。

## 工作约束

- 开始工作前先读取本文件并执行 `git status -sb`。
- 以当前文件、构建产物和 Git 记录为准。
- 不覆盖或删除用户已有修改。
- 每个上位机放在 `Tools/` 下独立子目录，避免源码、测试、采集和发布包混用。
- 硬件操作前说明将运行的命令和预期检查项。
- 只保留关键日志、结论和复现命令，避免重复加载完整聊天记录。
- 完成重要阶段后更新本文件中的提交号、验证结果、未解决项和下一步。
