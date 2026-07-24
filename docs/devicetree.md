# YiCore DeviceTree

## 构建流程

```text
app.dts
    -> board.dts
    -> board-*.dtsi
    -> soc.dtsi
    -> yi_dts_parser.py
    -> yi_dts_bindings.py
    -> yi_dts_gen.py
    -> generated/yi_generated.c
    -> generated/yi_generated.h
```

Keil工程已经配置 Before Build 命令，会在每次构建前自动执行生成器。
生成失败时，生成文件会包含 `#error`，防止工程继续链接旧的设备配置。

## 手动生成

在工程根目录执行：

```powershell
python scripts\yi_dts_gen.py `
  --dts examples\stm32f103-dts-demo\app.dts `
  --bindings dts\bindings `
  --output generated
```

生成器仅依赖Python标准库。

## DTS分层

当前工程按职责拆分：

```text
dts/arm/st/stm32f103xe/          芯片控制器、地址、IRQ和RCC能力
boards/fire-mini-stm32f103/      PCB固定连接和板载设备
examples/stm32f103-dts-demo/     应用启用策略
```

`board.dts`只负责包含SoC和板级片段：

```text
board-clocks.dtsi
board-gpios.dtsi
board-pinctrl.dtsi
board-software-buses.dtsi
board-devices.dtsi
board-console.dtsi
```

`board-gpios.dtsi`只放GPIO资源，`board-pinctrl.dtsi`只放硬件复用引脚，
`board-software-buses.dtsi`只放GPIO模拟总线，`board-devices.dtsi`放EEPROM、
Flash和LED等板级逻辑设备。硬件控制器的固定引脚连接在`board.dts`中配置。

应用只启用逻辑设备。生成器沿phandle依赖递归带入对应总线、GPIO、PinCtrl和Clock，
因此切换设备总线时不需要同时维护一组引脚节点开关。

## 应用访问

```c
#include "yi_generated.h"

yi_device_t *led = YI_DT_GET(LED0);
yi_device_t *uart = YI_DT_GET(UART0);
```

应用不应包含板级配置文件，也不应直接定义DTS设备。

## 当前支持的DTS语法

- 根节点和嵌套节点
- 节点label
- 字符串、整数和布尔属性
- `<...>` cells
- `&label` phandle
- `/dts-v1/;`
- `//` 和 `/* ... */` 注释
- `status = "okay"`、`status = "available"` 和 `status = "disabled"`

三种状态含义：

- `okay`：应用主动启用的根设备。
- `available`：板级可用资源，仅在被启用设备引用时生成。
- `disabled`：不可用；启用设备引用它会直接报错。

## Binding

Binding文件位于 `dts/bindings`，支持：

- `compatible`
- `driver`
- `header`
- `define`
- `properties`
- `type`
- `required`
- `default`

属性类型包括 `string`、`identifier`、`int`、`boolean` 和 `phandle`。

## PinMux

每个 PinMux 节点描述并独占一个物理引脚。外设使用 phandle 声明依赖：

```dts
usart1_tx_pin: usart1-tx-pin {
    compatible = "yi,stm32-pinmux";
    port = "GPIOA";
    pin = <9>;
    function = "uart-tx";
    mode = "alternate-push-pull";
    speed = "high";
    status = "available";
};

&usart1 {
    current-speed = <115200>;
    tx-pin = <&usart1_tx_pin>;
    rx-pin = <&usart1_rx_pin>;
    status = "available";
};
```

`function` 支持 `gpio`、`uart-tx`、`uart-rx`、`spi-clock`、
`spi-mosi`、`spi-miso`、`i2c-scl`、`i2c-sda`、`can-tx`、`can-rx`、
`pwm` 和 `adc`。生成器会拒绝同一端口同一引脚被多个节点占用。

## Clock

时钟作为独立设备声明，外设通过 `clocks` phandle 引用：

```dts
clk_gpioa: clock-gpioa {
    compatible = "yi,stm32-clock";
    clock-id = "gpioa";
};

usart1_tx_pin: usart1-tx-pin {
    compatible = "yi,stm32-pinmux";
    port = "GPIOA";
    pin = <9>;
    function = "uart-tx";
    mode = "alternate-push-pull";
    clocks = <&clk_gpioa>;
};
```

当前支持 `gpioa` 至 `gpioe`、`usart1` 至 `usart3`、`spi1` 和 `spi2`。
Clock驱动使用引用计数，因此多个引脚可以安全共享同一个GPIO端口时钟。

## 硬件定时器

硬件定时器采用SoC描述与板级启用分离的方式。STM32F103xE的Timer实例、
寄存器地址、RCC位和IRQ定义在：

```text
dts/arm/st/stm32f103xe/timers.dtsi
```

SoC中的Timer节点默认均为`disabled`。板级DTS包含SoC描述：

```dts
/ {
    /include/ "../../dts/arm/st/stm32f103xe/soc.dtsi"

    /* board devices */
};
```

应用入口`app.dts`包含板级描述，然后通过label overlay只启用实际需要的实例：

```dts
&timers7 {
    tick-frequency = <1000>;
    irq-priority = <10>;
    init-level = "post-kernel";
    init-priority = <10>;
    status = "okay";
};
```

生成器只为`status = "okay"`的Timer生成设备对象和IRQ Handler。Timer驱动直接使用
SoC节点提供的寄存器地址、总线类型和RCC位，不依赖CubeMX的`htimN`句柄，也没有
固定数量的运行时Timer数组。换用其他芯片时应提供对应的SoC Timer描述文件，板级
DTS只选择需要启用的实例。

## UART、SPI、I2C和CAN控制器

STM32F103xE的控制器能力分别位于`uart.dtsi`、`spi.dtsi`、`i2c.dtsi`和
`can.dtsi`。节点默认禁用，板级配置通过overlay提供速率、引脚和优先级。例如：

```dts
&usart1 {
    current-speed = <115200>;
    tx-pin = <&usart1_tx_pin>;
    rx-pin = <&usart1_rx_pin>;
    irq-priority = <10>;
    status = "available";
};
```

SPI控制器需要`sck-pin`、`miso-pin`、`mosi-pin`和`max-frequency`上限；I2C需要
`scl-pin`、`sda-pin`和不超过400kHz的`clock-frequency`；CAN需要TX/RX引脚、
`bitrate`和可选的千分比`sample-point`。每个启用实例都会生成独立HAL handle和
直接IRQ Handler，不依赖CubeMX的`huartN`、`hspiN`、`hi2cN`或`hcanN`对象。
控制器在板级通常标记为`available`，由引用它的逻辑设备自动带入。

UART节点可带DMA通道。STM32F103xE的SoC描述已经为USART1/2/3填入固定DMA映射：

```text
USART1_TX -> DMA1_Channel4, USART1_RX -> DMA1_Channel5
USART2_TX -> DMA1_Channel7, USART2_RX -> DMA1_Channel6
USART3_TX -> DMA1_Channel2, USART3_RX -> DMA1_Channel3
```

启用UART后，生成器会同时生成USART IRQ和DMA IRQ入口。阻塞收发仍可使用
`yi_uart_write()`和`yi_uart_read()`；DMA发送使用`yi_uart_write_dma()`。
变长接收可直接使用`yi_uart_rx_start_dma()`启动循环DMA，然后通过
`yi_uart_rx_dma_pos()`和`yi_uart_rx_idle(dev, true)`自行管理DMA缓冲。
如果应用需要字节流环形缓冲，可使用可选的
`yi_uart_dma_lwrb_start()`适配层；它在主循环中通过
`yi_uart_dma_lwrb_poll()`同步DMA当前位置，再用
`yi_uart_dma_lwrb_available()`和`yi_uart_dma_lwrb_read()`消费数据。

GPIO模拟SPI使用`yi,soft-spi`，通过`sck-gpio`、`miso-gpio`和`mosi-gpio`
引用三个GPIO设备，`max-frequency`支持1kHz至500kHz。模拟SPI与硬件SPI共用
`yi_spi_transceive()`；频率、模式0至3和可选片选GPIO由每次传输使用
`yi_spi_transfer_config_t`指定，因此同一总线可以连接配置不同的从设备。

W25Q64使用`winbond,w25q64`节点引用SPI控制器和片选GPIO。生成的设备实现统一
Flash API，应用通过`YI_DT_GET(W25Q64)`取得设备，不直接发送SPI NOR命令。

AT24C02使用`atmel,24c02`节点引用I2C控制器。生成的设备实现统一EEPROM API，
应用通过`YI_DT_GET(AT24C02)`取得设备，页写拆分与ACK polling由驱动处理。

## Console与日志

Console通过任意支持Device流式读写API的设备作为后端。下面是UART后端示例；
当前板级配置使用RTT：

```dts
console0: console0 {
    compatible = "yi,console";
    backend = <&uart0>;
    default-console;
    init-level = "application";
    init-priority = <0>;
};
```

启用Console时必须且只能有一个节点包含`default-console`。应用可直接使用：

```c
yi_log_info("application started");
yi_log_error("image validation failed");
```

日志接口支持DEBUG、INFO、WARN、ERROR等级及`yi_log_set_level()`过滤。
输出行为由后端决定：UART为阻塞输出，RTT可配置为非阻塞。日志接口不应在中断中调用。

### SEGGER RTT后端

RTT通过`ports/rtt`适配为标准YiCore流设备，Console和日志层不直接依赖第三方接口：

```dts
rtt0: rtt0 {
    compatible = "yi,segger-rtt";
    up-buffer = <0>;
    mode = "no-block-skip";
};

console0: console0 {
    compatible = "yi,console";
    backend = <&rtt0>;
    default-console;
};
```

将`backend`改为`<&uart0>`即可切回UART，不需要修改日志代码。RTT模式支持：

- `no-block-skip`：空间不足时整条丢弃，默认且不会因调试器未连接而阻塞
- `no-block-trim`：写入可容纳的部分，日志API会报告写入失败
- `block`：等待空间，可能在没有RTT主机读取时永久阻塞

## 测试

```powershell
python -m unittest discover -s scripts\tests -v
```

## GCC

GCC工程在编译前执行相同的生成命令，并编译
`generated/yi_generated.c`。链接时继续加载：

```text
-Tlinker/gcc/yi_device.ld
```
