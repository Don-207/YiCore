# YiCore 嵌入式框架演进路线

## 项目目标

YiCore 是一个面向长期维护型嵌入式产品的轻量设备框架。

设计目标：

-   类似 Zephyr 的 Device Model 思想
-   保留 STM32CubeMX 底层支持
-   支持裸机、Bootloader、应用统一架构
-   支持 STM32、FPGA、Linux 控制节点等多平台扩展

------------------------------------------------------------------------

# 阶段 1：YiDevice 基础设备模型

## 目标

建立类似 Zephyr `struct device` 的设备抽象。

## 已实现内容

-   yi_device_t
-   设备注册
-   自动设备发现
-   init 函数
-   config/data 分离
-   `.yi_device` 链接段

结构：

    Application
        |
     yi_device_get()
        |
     yi_device_t
        |
     driver init
        |
     Hardware

支持：

-   LED
-   UART

------------------------------------------------------------------------

# 阶段 2：Device API 抽象

## 目标

让应用不直接调用具体驱动函数。

当前：

    yi_uart_write()
    yi_led_on()

升级：

    device->api->write()
    device->api->on()

增加：

-   yi_device_api_t
-   统一设备接口
-   handle抽象

目标：

``` c
yi_device_t *uart;

uart->api->write();
```

------------------------------------------------------------------------

# 阶段 3：初始化等级管理

## 目标

解决设备依赖关系。

例如：

    GPIO
     |
    UART
     |
    Console
     |
    Application

增加：

-   PRE_KERNEL
-   POST_KERNEL
-   APPLICATION

示例：

``` c
YI_DEVICE_DEFINE(
    gpio0,
    PRE_KERNEL,
    yi_gpio_init,
    &gpio_cfg
);
```

------------------------------------------------------------------------

# 阶段 4：DeviceTree硬件描述

## 状态

已实现：DTS解析、Binding校验、依赖排序、C代码生成及Keil预构建集成。

## 目标

硬件配置和驱动分离。

结构：

    board.dts

        |

    yi_dts_gen.py

        |

    yi_generated.c

描述：

-   GPIO
-   UART
-   SPI
-   Flash
-   FPGA

示例：

``` dts
led0 {
    compatible="yi,gpio-led";
    pin=<13>;
};
```

------------------------------------------------------------------------

# 阶段 5：PinMux框架

## 状态

已实现：PinMux设备抽象、STM32F1 HAL后端、DTS/Binding/代码生成、引脚占用冲突检查及UART依赖集成。

## 目标

统一管理引脚复用。

解决：

-   UART TX/RX
-   SPI CLK/MOSI/MISO
-   PWM
-   ADC

结构：

    Device
     |
    PinMux
     |
    GPIO Hardware

实现要点：

- 每个 `yi,stm32-pinmux` 节点独占一个物理引脚
- 支持 GPIO、UART、SPI、PWM、ADC 功能标记
- 支持输入、模拟、推挽/开漏输出和复用模式
- 外设通过 phandle 引用 PinMux，初始化前检查 PinMux 已就绪
- 生成阶段拒绝重复的 `port + pin` 占用和非法配置值

------------------------------------------------------------------------

# 阶段 6：Clock Framework

## 状态

已实现：Clock设备抽象、STM32F1 RCC后端、引用计数、频率查询、DTS生成及GPIO/PinMux/UART依赖集成。

## 目标

统一管理外设时钟。

移除：

``` c
__HAL_RCC_xxx_CLK_ENABLE()
```

统一：

    Driver
     |
    Clock API
     |
    RCC

支持：

-   GPIO Clock
-   UART Clock
-   SPI Clock

实现要点：

- 驱动通过 `yi_clock_enable()` / `yi_clock_disable()` 管理时钟
- 引用计数保护多个设备共享的GPIO端口时钟
- `yi_clock_get_rate()` 返回HCLK、PCLK1或PCLK2域频率
- 支持GPIOA-E、USART1-3及SPI1-2时钟ID
- GPIO、PinMux和UART通过DTS `clocks` phandle声明依赖

------------------------------------------------------------------------

# 阶段 7：Console与日志系统

## 状态

已实现：Console设备抽象、UART后端适配、默认控制台DTS选择及分级日志接口。

## 目标

建立统一调试接口。

提供：

``` c
yi_log_info()
yi_log_error()
```

结构：

    Log
     |
    Console
     |
    UART

支持：

-   Bootloader日志
-   APP日志
-   调试信息

实现要点：

- Console通过通用Device API委托到底层UART读写
- DTS `default-console` 选择唯一默认日志输出设备
- 支持DEBUG、INFO、WARN、ERROR等级和运行时等级过滤
- 日志帧使用固定栈缓冲区，不依赖堆和`printf`
- 每条日志以单次Console写入输出，格式为`[LEVEL] message\r\n`

------------------------------------------------------------------------

# 阶段 8：Flash Framework

## 状态

已实现：统一Flash API、STM32F1内部Flash后端、容量/擦除/写入粒度查询、DTS/Binding/代码生成及参数校验。

## 目标

抽象不同Flash设备。

统一接口：

``` c
flash_read()

flash_write()

flash_erase()
```

支持：

-   STM32内部Flash
-   SPI Flash
-   QSPI Flash

实现要点：

- 应用统一使用 `yi_flash_read()` / `yi_flash_write()` / `yi_flash_erase()`，地址参数为相对Flash起始地址的偏移
- 写入与擦除操作执行越界和粒度检查，STM32后端保证操作完成后重新锁定Flash控制器
- DTS描述Flash基地址、容量、擦除块大小和写入块大小
- SPI Flash与QSPI Flash可通过实现同一 `yi_flash_api_t` 接口接入

------------------------------------------------------------------------

# 阶段 9：Partition Manager

## 目标

支持固件升级体系。

类似 Zephyr Partition Manager。

描述：

    flash

    bootloader

    slot0

    slot1

    storage

生成：

``` c
BOOT_ADDRESS
APP_A_ADDRESS
APP_B_ADDRESS
```

支持：

-   MCUboot
-   OTA
-   参数存储

------------------------------------------------------------------------

# 阶段 10：FPGA设备模型

## 目标

将FPGA纳入YiCore设备体系。

示例：

``` dts
fpga0 {

compatible="yi,fpga";

bus="spi1";

};
```

提供：

``` c
fpga_read_reg()

fpga_write_reg()

fpga_update()
```

支持：

-   FPGA控制
-   bitstream升级
-   SPI寄存器访问

------------------------------------------------------------------------

# 最终架构

                        YiCore

                           |

                  -----------------

                  |               |

              YiDevice        DeviceTree


                  |

            -----------------

            |       |       |

          GPIO    UART    SPI


                  |

          STM32 SoC Backend

                  |

              Vendor HAL


                  |

            STM32 / FPGA / Hardware

------------------------------------------------------------------------

# 推荐开发顺序

1.  YiDevice
2.  Device API
3.  Init Level
4.  DeviceTree
5.  PinMux
6.  Clock
7.  Console
8.  Flash
9.  Partition
10. MCUboot
11. FPGA设备模型

最终形成：

**YiCore：面向长期维护的嵌入式设备平台**

------------------------------------------------------------------------

# STM32CubeMX边界

当前保留CubeMX生成的启动文件、CMSIS设备文件、HAL配置和工程文件，但运行时初始化由YiCore接管：

- 应用通过平台无关的 `yi_system_init()` 完成HAL及系统时钟初始化
- GPIO与PinMux配置完全来自DTS，不再调用 `MX_GPIO_Init()`
- 公共GPIO/PinMux API使用YiCore枚举，不向应用暴露HAL配置宏
- UART公共API位于 `drivers/uart/yi_uart.h`，STM32句柄和IRQ配置隔离在
  `soc/st/stm32/stm32f1/yi_uart_stm32.h`
- STM32 HAL仍是当前backend；替换芯片平台时新增对应SoC backend，不改应用接口
- CMSIS和STM32Cube原厂代码集中在 `vendor/st/`，保持上游文件不修改
- STM32 HAL、LL和CMSIS Device调用只允许出现在 `soc/st/stm32/` 后端

边界结构：

    Application / Subsystem
             |
       YiCore public API
             |
       STM32 SoC backend
             |
      vendor STM32 HAL / CMSIS
