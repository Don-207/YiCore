#!/usr/bin/env python3
"""Generate YiCore device registration sources from DTS and bindings."""

from __future__ import annotations

import argparse
import re
from pathlib import Path
from typing import Any, Iterable

from yi_dts_bindings import (
    BindingError,
    ValidatedNode,
    load_bindings,
    validate_tree,
)
from yi_dts_parser import DtsCells, DtsReference, parse_file


_LEVEL_MAP = {
    "pre-kernel": "YI_INIT_PRE_KERNEL",
    "post-kernel": "YI_INIT_POST_KERNEL",
    "application": "YI_INIT_APPLICATION",
}

_PINMUX_FUNCTION_MAP = {
    "gpio": "YI_PINMUX_FUNCTION_GPIO",
    "uart-tx": "YI_PINMUX_FUNCTION_UART_TX",
    "uart-rx": "YI_PINMUX_FUNCTION_UART_RX",
    "spi-clock": "YI_PINMUX_FUNCTION_SPI_CLOCK",
    "spi-mosi": "YI_PINMUX_FUNCTION_SPI_MOSI",
    "spi-miso": "YI_PINMUX_FUNCTION_SPI_MISO",
    "i2c-scl": "YI_PINMUX_FUNCTION_I2C_SCL",
    "i2c-sda": "YI_PINMUX_FUNCTION_I2C_SDA",
    "can-tx": "YI_PINMUX_FUNCTION_CAN_TX",
    "can-rx": "YI_PINMUX_FUNCTION_CAN_RX",
    "pwm": "YI_PINMUX_FUNCTION_PWM",
    "adc": "YI_PINMUX_FUNCTION_ADC",
}

_PINMUX_MODE_MAP = {
    "input": "YI_PINMUX_MODE_INPUT",
    "output-push-pull": "YI_PINMUX_MODE_OUTPUT_PUSH_PULL",
    "output-open-drain": "YI_PINMUX_MODE_OUTPUT_OPEN_DRAIN",
    "alternate-push-pull": "YI_PINMUX_MODE_ALTERNATE_PUSH_PULL",
    "alternate-open-drain": "YI_PINMUX_MODE_ALTERNATE_OPEN_DRAIN",
    "analog": "YI_PINMUX_MODE_ANALOG",
}

_PINMUX_PULL_MAP = {
    "none": "YI_PINMUX_PULL_NONE",
    "up": "YI_PINMUX_PULL_UP",
    "down": "YI_PINMUX_PULL_DOWN",
}

_PINMUX_SPEED_MAP = {
    "low": "YI_PINMUX_SPEED_LOW",
    "medium": "YI_PINMUX_SPEED_MEDIUM",
    "high": "YI_PINMUX_SPEED_HIGH",
}

_CLOCK_ID_MAP = {
    "gpioa": "YI_STM32_CLOCK_GPIOA",
    "gpiob": "YI_STM32_CLOCK_GPIOB",
    "gpioc": "YI_STM32_CLOCK_GPIOC",
    "gpiod": "YI_STM32_CLOCK_GPIOD",
    "gpioe": "YI_STM32_CLOCK_GPIOE",
    "usart1": "YI_STM32_CLOCK_USART1",
    "usart2": "YI_STM32_CLOCK_USART2",
    "usart3": "YI_STM32_CLOCK_USART3",
    "spi1": "YI_STM32_CLOCK_SPI1",
    "spi2": "YI_STM32_CLOCK_SPI2",
}

_STM32_BUS_MAP = {
    "apb1": "YI_STM32_BUS_APB1",
    "apb2": "YI_STM32_BUS_APB2",
    "apb1-timer": "YI_STM32_BUS_APB1_TIMER",
    "apb2-timer": "YI_STM32_BUS_APB2_TIMER",
}

_RTT_MODE_MAP = {
    "no-block-skip": "YI_RTT_MODE_NO_BLOCK_SKIP",
    "no-block-trim": "YI_RTT_MODE_NO_BLOCK_TRIM",
    "block": "YI_RTT_MODE_BLOCK",
}


def _references(value: Any) -> Iterable[DtsReference]:
    if isinstance(value, DtsReference):
        yield value
    elif isinstance(value, DtsCells):
        for item in value.values:
            yield from _references(item)
    elif isinstance(value, list):
        for item in value:
            yield from _references(item)


def dependency_order(nodes: list[ValidatedNode]) -> list[ValidatedNode]:
    by_label: dict[str, ValidatedNode] = {}
    for item in nodes:
        label = item.node.label
        if label is None or not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", label):
            raise BindingError(f"{item.node.path}: enabled device requires a C identifier label")
        by_label[label] = item

    dependencies: dict[str, set[str]] = {}
    for label, item in by_label.items():
        refs = {
            reference.label
            for value in item.properties.values()
            for reference in _references(value)
        }
        missing = refs - by_label.keys()
        if missing:
            raise BindingError(
                f"{item.node.path}: references nodes that do not generate devices: "
                f"{', '.join(sorted(missing))}"
            )
        dependencies[label] = refs

    result: list[ValidatedNode] = []
    emitted: set[str] = set()
    while len(result) < len(nodes):
        ready = [
            item for item in nodes
            if item.node.label not in emitted
            and dependencies[item.node.label].issubset(emitted)
        ]
        if not ready:
            cycle = sorted(set(by_label) - emitted)
            raise BindingError(f"device dependency cycle: {' -> '.join(cycle)}")
        for item in ready:
            result.append(item)
            emitted.add(item.node.label)
    return result


def _level(item: ValidatedNode) -> str:
    return _LEVEL_MAP[item.properties["init-level"]]


def _stm32_peripheral(item: ValidatedNode) -> tuple[int, str, int]:
    reg = item.properties["reg"]
    bus_value = item.properties["clock-bus"]
    mask = item.properties["clock-enable-mask"]
    try:
        bus = _STM32_BUS_MAP[bus_value]
    except KeyError as exc:
        raise BindingError(
            f"{item.node.path}: invalid clock-bus {bus_value!r}; expected one of "
            f"{', '.join(sorted(_STM32_BUS_MAP))}"
        ) from exc
    if (reg < 0x40000000) or (reg > 0x5FFFFFFF) or ((reg & 0x3) != 0):
        raise BindingError(f"{item.node.path}: reg must be an aligned peripheral address")
    if mask <= 0:
        raise BindingError(f"{item.node.path}: clock-enable-mask must be positive")
    return reg, bus, mask


def _irq(item: ValidatedNode, name: str) -> str:
    value = item.properties[name]
    if not value.endswith("_IRQn"):
        raise BindingError(f"{item.node.path}: {name} must name a CMSIS IRQn value")
    return value


def _irq_handler(irq: str) -> str:
    return irq.removesuffix("_IRQn") + "_IRQHandler"


def _optional_irq(item: ValidatedNode, name: str) -> str | None:
    if name not in item.properties:
        return None
    return _irq(item, name)


def _irq_priority(item: ValidatedNode) -> int:
    priority = item.properties["irq-priority"]
    if not 0 <= priority <= 15:
        raise BindingError(f"{item.node.path}: irq-priority must be in range 0..15")
    return priority


def _generate_gpio(item: ValidatedNode) -> str:
    label = item.node.label
    port = item.properties["port"]
    pin = item.properties["pin"]
    clock = item.properties["clocks"].label
    direction_map = {
        "input": "YI_GPIO_DIRECTION_INPUT",
        "output": "YI_GPIO_DIRECTION_OUTPUT",
    }
    drive_map = {
        "push-pull": "YI_GPIO_DRIVE_PUSH_PULL",
        "open-drain": "YI_GPIO_DRIVE_OPEN_DRAIN",
    }
    pull_map = {"none": "YI_GPIO_PULL_NONE", "up": "YI_GPIO_PULL_UP", "down": "YI_GPIO_PULL_DOWN"}
    interrupt_map = {
        "none": "YI_GPIO_INTERRUPT_NONE",
        "rising": "YI_GPIO_INTERRUPT_RISING",
        "falling": "YI_GPIO_INTERRUPT_FALLING",
        "both": "YI_GPIO_INTERRUPT_BOTH",
    }
    try:
        direction = direction_map[item.properties["direction"]]
        drive = drive_map[item.properties["drive"]]
        pull = pull_map[item.properties["pull"]]
        interrupt = interrupt_map[item.properties["interrupt"]]
    except KeyError as exc:
        raise BindingError(f"{item.node.path}: invalid GPIO setting {exc.args[0]!r}") from exc
    irq_priority = _irq_priority(item)
    if not 0 <= pin <= 15:
        raise BindingError(f"{item.node.path}: pin must be in range 0..15")
    if interrupt != "YI_GPIO_INTERRUPT_NONE" and direction != "YI_GPIO_DIRECTION_INPUT":
        raise BindingError(f"{item.node.path}: interrupt GPIO must use direction = input")
    return f"""static yi_device_t {label};

static const yi_gpio_config_t {label}_cfg =
{{
    .self = &{label},
    .port = {port},
    .pin = YI_GPIO_PIN({pin}),
    .clock = &{clock},
    .direction = {direction},
    .drive = {drive},
    .pull = {pull},
    .interrupt = {interrupt},
    .irq_priority = {irq_priority}U
}};

static yi_gpio_data_t {label}_data;

YI_GPIO_DEFINE_LEVEL(
    {label},
    {_level(item)},
    {item.properties['init-priority']},
    {label}_cfg,
    {label}_data
);"""


def _generate_led(item: ValidatedNode) -> str:
    label = item.node.label
    gpio = item.properties["gpios"].label
    active_low = 1 if item.properties["active-low"] else 0
    return f"""static const yi_led_config_t {label}_cfg =
{{
    .gpio = &{gpio},
    .active_low = {active_low}
}};

YI_LED_DEFINE_LEVEL(
    {label},
    {_level(item)},
    {item.properties['init-priority']},
    {label}_cfg
);"""


def _generate_uart(item: ValidatedNode) -> str:
    label = item.node.label
    reg, clock_bus, clock_mask = _stm32_peripheral(item)
    speed = item.properties["current-speed"]
    tx_pin = item.properties["tx-pin"].label
    rx_pin = item.properties["rx-pin"].label
    interrupt = _irq(item, "interrupts")
    handler = _irq_handler(interrupt)
    priority = _irq_priority(item)
    tx_dma_channel = item.properties.get("tx-dma-channel")
    rx_dma_channel = item.properties.get("rx-dma-channel")
    tx_dma_irq = _optional_irq(item, "tx-dma-interrupt")
    rx_dma_irq = _optional_irq(item, "rx-dma-interrupt")
    dma_priority = item.properties["dma-irq-priority"]

    if (tx_dma_channel is None) != (tx_dma_irq is None):
        raise BindingError(
            f"{item.node.path}: tx-dma-channel and tx-dma-interrupt must be set together"
        )
    if (rx_dma_channel is None) != (rx_dma_irq is None):
        raise BindingError(
            f"{item.node.path}: rx-dma-channel and rx-dma-interrupt must be set together"
        )
    if not 0 <= dma_priority <= 15:
        raise BindingError(f"{item.node.path}: dma-irq-priority must be in range 0..15")
    if speed <= 0:
        raise BindingError(f"{item.node.path}: current-speed must be positive")
    tx_dma_channel_expr = f"{tx_dma_channel}" if tx_dma_channel else "NULL"
    rx_dma_channel_expr = f"{rx_dma_channel}" if rx_dma_channel else "NULL"
    tx_dma_irq_expr = tx_dma_irq if tx_dma_irq else "0"
    rx_dma_irq_expr = rx_dma_irq if rx_dma_irq else "0"
    dma_handlers = []
    if tx_dma_irq is not None:
        dma_handlers.append(
            f"""void {_irq_handler(tx_dma_irq)}(void)
{{
    yi_uart_stm32_dma_tx_irq_handler(&{label});
}}"""
        )
    if rx_dma_irq is not None:
        dma_handlers.append(
            f"""void {_irq_handler(rx_dma_irq)}(void)
{{
    yi_uart_stm32_dma_rx_irq_handler(&{label});
}}"""
        )
    dma_handler_text = "\n\n" + "\n\n".join(dma_handlers) if dma_handlers else ""
    return f"""static yi_device_t {label};

static const yi_uart_stm32_config_t {label}_cfg =
{{
    .self = &{label},
    .instance = (USART_TypeDef *)0x{reg:08X}U,
    .clock = {{ .bus = {clock_bus}, .enable_mask = 0x{clock_mask:08X}U }},
    .baudrate = {speed}U,
    .tx_pin = &{tx_pin},
    .rx_pin = &{rx_pin},
    .irqn = {interrupt},
    .irq_priority = {priority}U,
    .tx_dma_channel = {tx_dma_channel_expr},
    .rx_dma_channel = {rx_dma_channel_expr},
    .tx_dma_irqn = {tx_dma_irq_expr},
    .rx_dma_irqn = {rx_dma_irq_expr},
    .dma_irq_priority = {dma_priority}U
}};

static yi_uart_stm32_data_t {label}_data;

YI_UART_STM32_DEFINE_LEVEL(
    {label},
    {_level(item)},
    {item.properties['init-priority']},
    {label}_cfg,
    {label}_data
);

void {handler}(void)
{{
    yi_uart_stm32_irq_handler(&{label});
}}{dma_handler_text}"""


def _mapped_pinmux_value(item: ValidatedNode, name: str, mapping: dict[str, str]) -> str:
    value = item.properties[name]
    try:
        return mapping[value]
    except KeyError as exc:
        raise BindingError(
            f"{item.node.path}: invalid {name} {value!r}; expected one of "
            f"{', '.join(sorted(mapping))}"
        ) from exc


def _generate_pinmux(item: ValidatedNode) -> str:
    label = item.node.label
    pin = item.properties["pin"]
    if not 0 <= pin <= 15:
        raise BindingError(f"{item.node.path}: pin must be in range 0..15")
    function = _mapped_pinmux_value(item, "function", _PINMUX_FUNCTION_MAP)
    mode = _mapped_pinmux_value(item, "mode", _PINMUX_MODE_MAP)
    pull = _mapped_pinmux_value(item, "pull", _PINMUX_PULL_MAP)
    speed = _mapped_pinmux_value(item, "speed", _PINMUX_SPEED_MAP)
    port = item.properties["port"]
    clock = item.properties["clocks"].label
    return f"""static const yi_pinmux_config_t {label}_cfg =
{{
    .port = {port},
    .pin = YI_GPIO_PIN({pin}),
    .mode = {mode},
    .pull = {pull},
    .speed = {speed},
    .function = {function},
    .clock = &{clock}
}};

YI_PINMUX_DEFINE_LEVEL(
    {label},
    {_level(item)},
    {item.properties['init-priority']},
    {label}_cfg
);"""


def _generate_clock(item: ValidatedNode) -> str:
    label = item.node.label
    clock_id = _mapped_pinmux_value(item, "clock-id", _CLOCK_ID_MAP)
    return f"""static const yi_clock_config_t {label}_cfg =
{{
    .id = {clock_id}
}};

static yi_clock_data_t {label}_data;

YI_CLOCK_DEFINE_LEVEL(
    {label},
    {_level(item)},
    {item.properties['init-priority']},
    {label}_cfg,
    {label}_data
);"""


def _generate_console(item: ValidatedNode) -> str:
    label = item.node.label
    backend = item.properties["backend"].label
    is_default = 1 if item.properties["default-console"] else 0
    return f"""static yi_device_t {label};

static const yi_console_config_t {label}_cfg =
{{
    .self = &{label},
    .backend = &{backend},
    .default_console = {is_default}
}};

YI_CONSOLE_DEFINE_LEVEL(
    {label},
    {_level(item)},
    {item.properties['init-priority']},
    {label}_cfg
);"""


def _generate_rtt(item: ValidatedNode) -> str:
    label = item.node.label
    up_buffer = item.properties["up-buffer"]
    if up_buffer < 0:
        raise BindingError(f"{item.node.path}: up-buffer must not be negative")
    mode = _mapped_pinmux_value(item, "mode", _RTT_MODE_MAP)
    return f"""static const yi_rtt_config_t {label}_cfg =
{{
    .up_buffer = {up_buffer}U,
    .mode = {mode}
}};

YI_RTT_DEFINE_LEVEL(
    {label},
    {_level(item)},
    {item.properties['init-priority']},
    {label}_cfg
);"""


def _generate_flash(item: ValidatedNode) -> str:
    label = item.node.label
    base = item.properties["base-address"]
    size = item.properties["size"]
    erase_size = item.properties["erase-block-size"]
    write_size = item.properties["write-block-size"]
    if base < 0 or size <= 0 or erase_size <= 0 or write_size <= 0:
        raise BindingError(f"{item.node.path}: flash geometry values must be positive")
    if size % erase_size != 0:
        raise BindingError(f"{item.node.path}: size must be a multiple of erase-block-size")
    return f"""static const yi_flash_config_t {label}_cfg =
{{
    .base_address = 0x{base:08X}U,
    .size = {size}U,
    .erase_block_size = {erase_size}U,
    .write_block_size = {write_size}U
}};

YI_STM32_FLASH_DEFINE_LEVEL(
    {label},
    {_level(item)},
    {item.properties['init-priority']},
    {label}_cfg
);"""


def _generate_w25q64(item: ValidatedNode) -> str:
    label = item.node.label
    frequency = item.properties["spi-frequency"]
    mode = item.properties["spi-mode"]
    size = item.properties["size"]
    erase_size = item.properties["erase-block-size"]
    write_size = item.properties["write-block-size"]
    transfer_timeout = item.properties["transfer-timeout-ms"]
    program_timeout = item.properties["program-timeout-ms"]
    erase_timeout = item.properties["erase-timeout-ms"]

    if frequency <= 0 or mode not in {0, 1, 2, 3}:
        raise BindingError(f"{item.node.path}: invalid SPI frequency or mode")
    if size != 0x00800000 or erase_size != 4096 or write_size != 1:
        raise BindingError(f"{item.node.path}: invalid W25Q64 geometry")
    if transfer_timeout <= 0 or program_timeout <= 0 or erase_timeout <= 0:
        raise BindingError(f"{item.node.path}: timeouts must be positive")

    return f"""static yi_device_t {label};

static const yi_w25q64_config_t {label}_cfg =
{{
    .flash = {{
        .base_address = 0U,
        .size = {size}U,
        .erase_block_size = {erase_size}U,
        .write_block_size = {write_size}U
    }},
    .self = &{label},
    .spi = &{item.properties['bus'].label},
    .spi_config = {{
        .frequency = {frequency}U,
        .cs_gpio = &{item.properties['cs-gpio'].label},
        .mode = {mode}U,
        .cs_active_high = false
    }},
    .transfer_timeout_ms = {transfer_timeout}U,
    .program_timeout_ms = {program_timeout}U,
    .erase_timeout_ms = {erase_timeout}U
}};

static yi_w25q64_data_t {label}_data;

YI_W25Q64_DEFINE_LEVEL(
    {label},
    {_level(item)},
    {item.properties['init-priority']},
    {label}_cfg,
    {label}_data
);"""


def _generate_at24c02(item: ValidatedNode) -> str:
    label = item.node.label
    address = item.properties["address"]
    size = item.properties["size"]
    page_size = item.properties["page-size"]
    transfer_timeout = item.properties["transfer-timeout-ms"]
    write_timeout = item.properties["write-timeout-ms"]

    if not 0 <= address <= 0x7F:
        raise BindingError(f"{item.node.path}: address must be a 7-bit value")
    if size != 256 or page_size != 8:
        raise BindingError(f"{item.node.path}: invalid AT24C02 geometry")
    if transfer_timeout <= 0 or write_timeout <= 0:
        raise BindingError(f"{item.node.path}: timeouts must be positive")

    return f"""static yi_device_t {label};

static const yi_at24c02_config_t {label}_cfg =
{{
    .eeprom = {{
        .size = {size}U,
        .page_size = {page_size}U
    }},
    .self = &{label},
    .i2c = &{item.properties['bus'].label},
    .address = 0x{address:02X}U,
    .transfer_timeout_ms = {transfer_timeout}U,
    .write_timeout_ms = {write_timeout}U
}};

static yi_at24c02_data_t {label}_data;

YI_AT24C02_DEFINE_LEVEL(
    {label},
    {_level(item)},
    {item.properties['init-priority']},
    {label}_cfg,
    {label}_data
);"""


def _generate_timer(item: ValidatedNode) -> str:
    label = item.node.label
    reg, clock_bus, clock_enable_mask = _stm32_peripheral(item)
    interrupt = _irq(item, "interrupts")
    counter_bits = item.properties["counter-bits"]
    tick_frequency = item.properties["tick-frequency"]
    irq_priority = item.properties["irq-priority"]

    if counter_bits not in {16, 32}:
        raise BindingError(f"{item.node.path}: counter-bits must be 16 or 32")
    if tick_frequency <= 0:
        raise BindingError(f"{item.node.path}: tick-frequency must be positive")
    if not 0 <= irq_priority <= 15:
        raise BindingError(f"{item.node.path}: irq-priority must be in range 0..15")

    handler = _irq_handler(interrupt)
    return f"""static yi_device_t {label};

static const yi_timer_config_t {label}_cfg =
{{
    .self = &{label},
    .instance = (TIM_TypeDef *)0x{reg:08X}U,
    .clock = {{ .bus = {clock_bus}, .enable_mask = 0x{clock_enable_mask:08X}U }},
    .counter_bits = {counter_bits}U,
    .tick_frequency = {tick_frequency}U,
    .irqn = {interrupt},
    .irq_priority = {irq_priority}U
}};

static yi_timer_data_t {label}_data;

YI_TIMER_DEFINE_LEVEL(
    {label},
    {_level(item)},
    {item.properties['init-priority']},
    {label}_cfg,
    {label}_data
);

void {handler}(void)
{{
    yi_timer_irq_handler(&{label});
}}"""


def _generate_spi(item: ValidatedNode) -> str:
    label = item.node.label
    reg, bus, mask = _stm32_peripheral(item)
    irq = _irq(item, "interrupts")
    frequency = item.properties["max-frequency"]
    if frequency <= 0:
        raise BindingError(f"{item.node.path}: invalid SPI frequency")
    return f"""static yi_device_t {label};
static const yi_spi_stm32_config_t {label}_cfg =
{{
    .self = &{label},
    .instance = (SPI_TypeDef *)0x{reg:08X}U,
    .clock = {{ .bus = {bus}, .enable_mask = 0x{mask:08X}U }},
    .sck_pin = &{item.properties['sck-pin'].label},
    .miso_pin = &{item.properties['miso-pin'].label},
    .mosi_pin = &{item.properties['mosi-pin'].label},
    .max_frequency = {frequency}U,
    .irqn = {irq},
    .irq_priority = {_irq_priority(item)}U
}};
static yi_spi_stm32_data_t {label}_data;
YI_SPI_STM32_DEFINE_LEVEL(
    {label}, {_level(item)}, {item.properties['init-priority']},
    {label}_cfg, {label}_data
);
void {_irq_handler(irq)}(void) {{ yi_spi_stm32_irq_handler(&{label}); }}"""


def _generate_i2c(item: ValidatedNode) -> str:
    label = item.node.label
    reg, bus, mask = _stm32_peripheral(item)
    event_irq = _irq(item, "event-interrupt")
    error_irq = _irq(item, "error-interrupt")
    frequency = item.properties["clock-frequency"]
    if not 0 < frequency <= 400000:
        raise BindingError(f"{item.node.path}: clock-frequency must be in range 1..400000")
    return f"""static yi_device_t {label};
static const yi_i2c_stm32_config_t {label}_cfg =
{{
    .self = &{label},
    .instance = (I2C_TypeDef *)0x{reg:08X}U,
    .clock = {{ .bus = {bus}, .enable_mask = 0x{mask:08X}U }},
    .scl_pin = &{item.properties['scl-pin'].label},
    .sda_pin = &{item.properties['sda-pin'].label},
    .clock_frequency = {frequency}U,
    .event_irqn = {event_irq},
    .error_irqn = {error_irq},
    .irq_priority = {_irq_priority(item)}U
}};
static yi_i2c_stm32_data_t {label}_data;
YI_I2C_STM32_DEFINE_LEVEL({label}, {_level(item)}, {item.properties['init-priority']},
                    {label}_cfg, {label}_data);
void {_irq_handler(event_irq)}(void) {{ yi_i2c_stm32_event_irq_handler(&{label}); }}
void {_irq_handler(error_irq)}(void) {{ yi_i2c_stm32_error_irq_handler(&{label}); }}"""


def _generate_soft_i2c(item: ValidatedNode) -> str:
    label = item.node.label
    frequency = item.properties["clock-frequency"]
    stretch_timeout = item.properties["stretch-timeout-us"]
    recovery_clocks = item.properties["recovery-clocks"]
    if not 1000 <= frequency <= 100000:
        raise BindingError(f"{item.node.path}: clock-frequency must be in range 1000..100000")
    if stretch_timeout <= 0 or not 1 <= recovery_clocks <= 32:
        raise BindingError(f"{item.node.path}: invalid Soft-I2C timeout or recovery clocks")
    half_period_us = max(1, 500000 // frequency)
    return f"""static const yi_soft_i2c_config_t {label}_cfg =
{{
    .scl_gpio = &{item.properties['scl-gpio'].label},
    .sda_gpio = &{item.properties['sda-gpio'].label},
    .clock_frequency = {frequency}U,
    .half_period_us = {half_period_us}U,
    .stretch_timeout_us = {stretch_timeout}U,
    .recovery_clocks = {recovery_clocks}U
}};
static yi_soft_i2c_data_t {label}_data;
YI_SOFT_I2C_DEFINE_LEVEL({label}, {_level(item)}, {item.properties['init-priority']},
                         {label}_cfg, {label}_data);"""


def _generate_soft_spi(item: ValidatedNode) -> str:
    label = item.node.label
    frequency = item.properties["max-frequency"]
    if not 1000 <= frequency <= 500000:
        raise BindingError(f"{item.node.path}: max-frequency must be in range 1000..500000")
    return f"""static const yi_soft_spi_config_t {label}_cfg =
{{
    .sck_gpio = &{item.properties['sck-gpio'].label},
    .miso_gpio = &{item.properties['miso-gpio'].label},
    .mosi_gpio = &{item.properties['mosi-gpio'].label},
    .max_frequency = {frequency}U
}};
static yi_soft_spi_data_t {label}_data;
YI_SOFT_SPI_DEFINE_LEVEL({label}, {_level(item)}, {item.properties['init-priority']},
                         {label}_cfg, {label}_data);"""


def _generate_can(item: ValidatedNode) -> str:
    label = item.node.label
    reg, bus, mask = _stm32_peripheral(item)
    irq_names = ["tx-interrupt", "rx0-interrupt", "rx1-interrupt", "sce-interrupt"]
    irqs = [_irq(item, name) for name in irq_names]
    bitrate = item.properties["bitrate"]
    sample_point = item.properties["sample-point"]
    if bitrate <= 0 or not 0 < sample_point < 1000:
        raise BindingError(f"{item.node.path}: invalid CAN bitrate or sample-point")
    handlers = "\n".join(
        f"void {_irq_handler(irq)}(void) {{ yi_can_irq_handler(&{label}); }}"
        for irq in irqs
    )
    return f"""static yi_device_t {label};
static const yi_can_config_t {label}_cfg =
{{
    .self = &{label},
    .instance = (CAN_TypeDef *)0x{reg:08X}U,
    .clock = {{ .bus = {bus}, .enable_mask = 0x{mask:08X}U }},
    .tx_pin = &{item.properties['tx-pin'].label},
    .rx_pin = &{item.properties['rx-pin'].label},
    .bitrate = {bitrate}U,
    .sample_point = {sample_point}U,
    .tx_irqn = {irqs[0]}, .rx0_irqn = {irqs[1]},
    .rx1_irqn = {irqs[2]}, .sce_irqn = {irqs[3]},
    .irq_priority = {_irq_priority(item)}U
}};
static yi_can_data_t {label}_data;
YI_CAN_DEFINE_LEVEL({label}, {_level(item)}, {item.properties['init-priority']},
                    {label}_cfg, {label}_data);
{handlers}"""


_GENERATORS = {
    "gpio": _generate_gpio,
    "led": _generate_led,
    "uart": _generate_uart,
    "pinmux": _generate_pinmux,
    "clock": _generate_clock,
    "console": _generate_console,
    "rtt": _generate_rtt,
    "flash": _generate_flash,
    "w25q64": _generate_w25q64,
    "at24c02": _generate_at24c02,
    "timer": _generate_timer,
    "spi": _generate_spi,
    "i2c": _generate_i2c,
    "soft-i2c": _generate_soft_i2c,
    "soft-spi": _generate_soft_spi,
    "can": _generate_can,
}


def generate_sources(nodes: list[ValidatedNode], source_name: str) -> tuple[str, str]:
    ordered = dependency_order(nodes)
    occupied_pins: dict[tuple[str, int], str] = {}
    for item in ordered:
        if item.binding.driver != "pinmux":
            continue
        key = (item.properties["port"], item.properties["pin"])
        if key in occupied_pins:
            raise BindingError(
                f"{item.node.path}: pin {key[0]}:{key[1]} is already owned by "
                f"{occupied_pins[key]}"
            )
        occupied_pins[key] = item.node.path
    default_consoles = [
        item for item in ordered
        if item.binding.driver == "console" and item.properties["default-console"]
    ]
    consoles = [item for item in ordered if item.binding.driver == "console"]
    if consoles and len(default_consoles) != 1:
        raise BindingError(
            "exactly one enabled console must have the default-console property"
        )
    unsupported = sorted({item.binding.driver for item in ordered} - _GENERATORS.keys())
    if unsupported:
        raise BindingError(f"no code generator for drivers: {', '.join(unsupported)}")

    headers = sorted({item.binding.header for item in ordered})
    includes = "\n".join(f'#include "{header}"' for header in headers)
    bodies = "\n\n".join(_GENERATORS[item.binding.driver](item) for item in ordered)
    c_source = f"""/*
 * Generated by yi_dts_gen.py from {source_name}.
 * Do not edit manually.
 */

#include <stddef.h>
{includes}

{bodies}
"""

    aliases = []
    for item in ordered:
        macro = item.node.label.upper()
        aliases.append(f'#define YI_DT_{macro}_NAME "{item.node.label}"')
    alias_text = "\n".join(aliases)
    h_source = f"""/*
 * Generated by yi_dts_gen.py from {source_name}.
 * Do not edit manually.
 */
#ifndef YI_GENERATED_H
#define YI_GENERATED_H

#include "yi_device.h"

{alias_text}

#define YI_DT_GET(_label) yi_device_get(YI_DT_##_label##_NAME)

#endif
"""
    return c_source, h_source


def _write_if_changed(path: Path, content: str) -> None:
    if path.exists() and path.read_text(encoding="utf-8") == content:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8", newline="\n")


def generate(dts: Path, bindings_dir: Path, output_dir: Path) -> tuple[Path, Path]:
    nodes = validate_tree(parse_file(dts), load_bindings(bindings_dir))
    c_source, h_source = generate_sources(nodes, dts.name)
    c_path = output_dir / "yi_generated.c"
    h_path = output_dir / "yi_generated.h"
    _write_if_changed(c_path, c_source)
    _write_if_changed(h_path, h_source)
    return c_path, h_path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dts", type=Path, required=True)
    parser.add_argument("--bindings", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        c_path, h_path = generate(args.dts, args.bindings, args.output)
    except (OSError, ValueError) as exc:
        failure = (
            "/* DTS generation failed. See the pre-build log. */\n"
            '#error "YiCore DTS generation failed"\n'
        )
        _write_if_changed(args.output / "yi_generated.c", failure)
        _write_if_changed(args.output / "yi_generated.h", failure)
        parser.exit(1, f"error: {exc}\n")
    print(c_path)
    print(h_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
