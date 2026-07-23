# YiCore DeviceTree

## 构建流程

```text
board.dts
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
  --dts boards\fire-mini-stm32f103\board.dts `
  --bindings dts\bindings `
  --output generated
```

生成器仅依赖Python标准库。

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
- `status = "okay"` 和 `status = "disabled"`

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
uart0_tx: uart0-tx {
    compatible = "yi,stm32-pinmux";
    port = "GPIOA";
    pin = <9>;
    function = "uart-tx";
    mode = "alternate-push-pull";
    speed = "high";
};

&usart1 {
    current-speed = <115200>;
    tx-pin = <&uart0_tx>;
    rx-pin = <&uart0_rx>;
    status = "okay";
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

uart0_tx: uart0-tx {
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
soc/st/stm32f103xe/timers.dtsi
```

SoC中的Timer节点默认均为`disabled`。板级DTS包含SoC描述：

```dts
/ {
    /include/ "../../soc/st/stm32f103xe/soc.dtsi"

    /* board devices */
};
```

然后通过label overlay只启用实际需要的实例：

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
    tx-pin = <&uart0_tx>;
    rx-pin = <&uart0_rx>;
    irq-priority = <10>;
    status = "okay";
};
```

SPI需要`sck-pin`、`miso-pin`、`mosi-pin`、`max-frequency`和模式0至3；I2C需要
`scl-pin`、`sda-pin`和不超过400kHz的`clock-frequency`；CAN需要TX/RX引脚、
`bitrate`和可选的千分比`sample-point`。每个启用实例都会生成独立HAL handle和
直接IRQ Handler，不依赖CubeMX的`huartN`、`hspiN`、`hi2cN`或`hcanN`对象。

GPIO模拟SPI使用`yi,soft-spi`，通过`sck-gpio`、`miso-gpio`和`mosi-gpio`
引用三个GPIO设备。`max-frequency`支持1kHz至500kHz，`mode`可设为0、1、2或3，
分别选择对应的CPOL/CPHA组合。模拟SPI与硬件SPI共用`yi_spi_transceive()`；片选
由应用使用独立GPIO控制。

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
