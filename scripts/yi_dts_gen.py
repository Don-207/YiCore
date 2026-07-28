#!/usr/bin/env python3
"""Generate YiCore device registration sources from DTS and bindings.

Author: Don
Date: 2026-07-26
Version: 1.0.0
"""

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


def _generate_max31856(item: ValidatedNode) -> str:
    label = item.node.label
    types = {name: f"YI_MAX31856_TC_{name.upper()}" for name in "bejknrst"}
    tc_name = item.properties["thermocouple-type"]
    if tc_name not in types:
        raise BindingError(f"{item.node.path}: invalid thermocouple-type")
    frequency = item.properties["spi-frequency"]
    filter_hz = item.properties["filter-hz"]
    average = item.properties["average-samples"]
    open_ms = item.properties["open-circuit-ms"]
    timeout = item.properties["transfer-timeout-ms"]
    if not 0 < frequency <= 5000000:
        raise BindingError(f"{item.node.path}: spi-frequency must be in range 1..5000000")
    if filter_hz not in {50, 60} or average not in {1, 2, 4, 8, 16}:
        raise BindingError(f"{item.node.path}: invalid filter or averaging setting")
    if open_ms not in {0, 10, 32, 100} or timeout <= 0:
        raise BindingError(f"{item.node.path}: invalid fault timing or timeout")
    return f"""static yi_device_t {label};
static const yi_max31856_config_t {label}_cfg =
{{
    .self = &{label},
    .spi = &{item.properties['bus'].label},
    .spi_config = {{ .frequency = {frequency}U,
        .cs_gpio = &{item.properties['cs-gpio'].label},
        .mode = 1U, .cs_active_high = false }},
    .thermocouple_type = {types[tc_name]},
    .transfer_timeout_ms = {timeout}U,
    .average_samples = {average}U,
    .filter_hz = {filter_hz}U,
    .open_circuit_ms = {open_ms}U
}};
static yi_max31856_data_t {label}_data;
YI_MAX31856_DEFINE_LEVEL({label}, {_level(item)},
    {item.properties['init-priority']}, {label}_cfg, {label}_data);"""


def _generate_ad9834(item: ValidatedNode) -> str:
    label = item.node.label
    frequency = item.properties["spi-frequency"]
    mclk = item.properties["mclk-frequency"]
    timeout = item.properties["transfer-timeout-ms"]
    if not 0 < frequency <= 40000000:
        raise BindingError(f"{item.node.path}: spi-frequency must be in range 1..40000000")
    if mclk <= 0 or timeout <= 0:
        raise BindingError(f"{item.node.path}: mclk-frequency and timeout must be positive")
    return f"""static yi_device_t {label};
static const yi_ad9834_config_t {label}_cfg =
{{
    .self = &{label},
    .spi = &{item.properties['bus'].label},
    .spi_config = {{ .frequency = {frequency}U,
        .cs_gpio = &{item.properties['cs-gpio'].label},
        .mode = 2U, .cs_active_high = false }},
    .mclk_frequency = {mclk}U,
    .transfer_timeout_ms = {timeout}U
}};
static yi_ad9834_data_t {label}_data;
YI_AD9834_DEFINE_LEVEL({label}, {_level(item)},
    {item.properties['init-priority']}, {label}_cfg, {label}_data);"""


def _generate_ad9851(item: ValidatedNode) -> str:
    label = item.node.label
    reference = item.properties["reference-clock-frequency"]
    multiplier = item.properties["clock-multiplier"]
    delay = item.properties["pulse-delay-us"]
    system_clock = reference * (6 if multiplier else 1)
    if reference <= 0 or system_clock > 180000000 or (multiplier and reference > 30000000):
        raise BindingError(f"{item.node.path}: resulting system clock must be in range 1..180000000")
    if delay < 0:
        raise BindingError(f"{item.node.path}: pulse-delay-us must not be negative")
    return f"""static yi_device_t {label};
static const yi_ad9851_config_t {label}_cfg =
{{
    .self = &{label},
    .w_clk_gpio = &{item.properties['w-clk-gpio'].label},
    .fq_ud_gpio = &{item.properties['fq-ud-gpio'].label},
    .data_gpio = &{item.properties['data-gpio'].label},
    .reset_gpio = &{item.properties['reset-gpio'].label},
    .reference_clock_frequency = {reference}U,
    .pulse_delay_us = {delay}U,
    .clock_multiplier = {str(multiplier).lower()}
}};
static yi_ad9851_data_t {label}_data;
YI_AD9851_DEFINE_LEVEL({label}, {_level(item)},
    {item.properties['init-priority']}, {label}_cfg, {label}_data);"""


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


def _generate_adc(item: ValidatedNode) -> str:
    label = item.node.label
    reg, bus, mask = _stm32_peripheral(item)
    channel = item.properties["channel"]
    sample_cycles = item.properties["sample-cycles"]
    divider = item.properties["clock-divider"]
    if not 0 <= channel <= 15:
        raise BindingError(f"{item.node.path}: channel must be in range 0..15")
    if not 0 <= sample_cycles <= 7:
        raise BindingError(f"{item.node.path}: sample-cycles must be in range 0..7")
    if divider not in {2, 4, 6, 8}:
        raise BindingError(f"{item.node.path}: clock-divider must be 2, 4, 6 or 8")
    return f"""static yi_device_t {label};
static const yi_adc_stm32f1_config_t {label}_cfg =
{{
    .self = &{label},
    .instance = (ADC_TypeDef *)0x{reg:08X}U,
    .clock = {{ .bus = {bus}, .enable_mask = 0x{mask:08X}U }},
    .input_pin = &{item.properties['input-pin'].label},
    .channel = {channel}U,
    .sample_cycles = {sample_cycles}U,
    .clock_divider = {divider}U
}};
static yi_adc_stm32f1_data_t {label}_data;
YI_ADC_STM32F1_DEFINE_LEVEL(
    {label}, {_level(item)}, {item.properties['init-priority']},
    {label}_cfg, {label}_data
);"""


def _generate_ads7830(item: ValidatedNode) -> str:
    label = item.node.label
    address = item.properties["address"]
    channel = item.properties["default-channel"]
    reference_mv = item.properties["reference-mv"]
    timeout = item.properties["transfer-timeout-ms"]
    if not 0x48 <= address <= 0x4B:
        raise BindingError(f"{item.node.path}: address must be in range 0x48..0x4b")
    if not 0 <= channel <= 7:
        raise BindingError(f"{item.node.path}: default-channel must be in range 0..7")
    if not 0 < reference_mv <= 65535:
        raise BindingError(f"{item.node.path}: reference-mv must be in range 1..65535")
    if timeout <= 0:
        raise BindingError(f"{item.node.path}: transfer-timeout-ms must be positive")
    internal_reference = 1 if item.properties["internal-reference"] else 0
    return f"""static yi_device_t {label};

static const yi_ads7830_config_t {label}_cfg =
{{
    .self = &{label},
    .i2c = &{item.properties['bus'].label},
    .address = 0x{address:02X}U,
    .default_channel = {channel}U,
    .internal_reference = {internal_reference},
    .reference_mv = {reference_mv}U,
    .transfer_timeout_ms = {timeout}U
}};

static yi_ads7830_data_t {label}_data;

YI_ADS7830_DEFINE_LEVEL(
    {label}, {_level(item)}, {item.properties['init-priority']},
    {label}_cfg, {label}_data
);"""


def _generate_adc081c02(item: ValidatedNode) -> str:
    label = item.node.label
    address = item.properties["address"]
    reference = item.properties["reference-mv"]
    configuration = item.properties["configuration"]
    low = item.properties["low-limit"]
    high = item.properties["high-limit"]
    hysteresis = item.properties["hysteresis"]
    timeout = item.properties["transfer-timeout-ms"]
    if not 0 <= address <= 0x7f:
        raise BindingError(f"{item.node.path}: address must be a 7-bit value")
    if not 0 < reference <= 65535 or timeout <= 0:
        raise BindingError(f"{item.node.path}: invalid reference or timeout")
    if not all(0 <= value <= 255 for value in (configuration, low, high, hysteresis)):
        raise BindingError(f"{item.node.path}: register values must be in range 0..255")
    if low > high:
        raise BindingError(f"{item.node.path}: low-limit must not exceed high-limit")
    return f"""static yi_device_t {label};
static const yi_adc081c02_config_t {label}_cfg =
{{
    .self = &{label},
    .i2c = &{item.properties['bus'].label},
    .address = 0x{address:02X}U,
    .reference_mv = {reference}U,
    .transfer_timeout_ms = {timeout}U,
    .configuration = 0x{configuration:02X}U,
    .low_limit = {low}U,
    .high_limit = {high}U,
    .hysteresis = {hysteresis}U
}};
static yi_adc081c02_data_t {label}_data;
YI_ADC081C02_DEFINE_LEVEL({label}, {_level(item)},
    {item.properties['init-priority']}, {label}_cfg, {label}_data);"""


def _generate_ads1258(item: ValidatedNode) -> str:
    label = item.node.label
    frequency = item.properties["spi-frequency"]
    timeout = item.properties["transfer-timeout-ms"]
    byte_fields = ["config0", "config1", "muxsch", "muxdif", "system-readings"]
    if not 0 < frequency <= 2000000 or timeout <= 0:
        raise BindingError(f"{item.node.path}: invalid SPI frequency or timeout")
    if any(not 0 <= item.properties[name] <= 255 for name in byte_fields):
        raise BindingError(f"{item.node.path}: register values must be in range 0..255")
    mask = item.properties["single-ended-mask"]
    if not 0 <= mask <= 0xffff:
        raise BindingError(f"{item.node.path}: single-ended-mask must be in range 0..65535")
    optional = lambda name: (f"&{item.properties[name].label}" if item.properties.get(name) else "NULL")
    return f"""static yi_device_t {label};
static const yi_ads1258_config_t {label}_cfg = {{
    .self = &{label}, .spi = &{item.properties['bus'].label},
    .spi_config = {{ .frequency = {frequency}U,
        .cs_gpio = &{item.properties['cs-gpio'].label}, .mode = 1U, .cs_active_high = false }},
    .reset_gpio = {optional('reset-gpio')}, .start_gpio = &{item.properties['start-gpio'].label},
    .drdy_gpio = {optional('drdy-gpio')}, .transfer_timeout_ms = {timeout}U,
    .config0 = 0x{item.properties['config0']:02X}U, .config1 = 0x{item.properties['config1']:02X}U,
    .muxsch = 0x{item.properties['muxsch']:02X}U, .muxdif = 0x{item.properties['muxdif']:02X}U,
    .single_ended_mask = 0x{mask:04X}U, .system_readings = 0x{item.properties['system-readings']:02X}U
}};
static yi_ads1258_data_t {label}_data;
YI_ADS1258_DEFINE_LEVEL({label}, {_level(item)}, {item.properties['init-priority']},
    {label}_cfg, {label}_data);"""


def _generate_ads1298(item: ValidatedNode) -> str:
    label = item.node.label
    frequency = item.properties["spi-frequency"]
    master_clock = item.properties["master-clock-hz"]
    timeout = item.properties["transfer-timeout-ms"]
    data_rate = item.properties["data-rate"]
    power_mask = item.properties["channel-power-down-mask"]
    channel_gains = [item.properties[f"channel{i}-gain"] for i in range(8)]
    channel_muxes = [item.properties[f"channel{i}-mux"] for i in range(8)]
    gains = {
        1: "YI_ADS1298_GAIN_1", 2: "YI_ADS1298_GAIN_2",
        3: "YI_ADS1298_GAIN_3", 4: "YI_ADS1298_GAIN_4",
        6: "YI_ADS1298_GAIN_6", 8: "YI_ADS1298_GAIN_8",
        12: "YI_ADS1298_GAIN_12",
    }
    muxes = {
        "normal": "YI_ADS1298_MUX_NORMAL",
        "input-short": "YI_ADS1298_MUX_INPUT_SHORT",
        "rld-measure": "YI_ADS1298_MUX_RLD_MEASURE",
        "mvdd": "YI_ADS1298_MUX_MVDD",
        "temperature": "YI_ADS1298_MUX_TEMPERATURE",
        "test-signal": "YI_ADS1298_MUX_TEST_SIGNAL",
        "rld-drp": "YI_ADS1298_MUX_RLD_DRP",
        "rld-drn": "YI_ADS1298_MUX_RLD_DRN",
    }
    if (frequency <= 0 or master_clock <= 0 or
            frequency > master_clock * 2 or timeout <= 0):
        raise BindingError(
            f"{item.node.path}: invalid SPI, master clock or timeout setting"
        )
    if not 0 <= data_rate <= 6 or not 0 <= power_mask <= 0xff:
        raise BindingError(
            f"{item.node.path}: invalid data rate or power-down mask"
        )
    if (any(gain not in gains for gain in channel_gains) or
            any(mux not in muxes for mux in channel_muxes)):
        raise BindingError(
            f"{item.node.path}: invalid channel gain or mux"
        )
    register_names = (
        "config2", "config3-extra", "loff", "rld-sensp", "rld-sensn",
        "loff-sensp", "loff-sensn", "loff-flip", "gpio-register",
        "pace", "resp", "config4", "wct1", "wct2",
    )
    if any(not 0 <= item.properties[name] <= 0xff
           for name in register_names):
        raise BindingError(
            f"{item.node.path}: register values must be in range 0..255"
        )
    def optional(name: str) -> str:
        reference = item.properties.get(name)
        return f"&{reference.label}" if reference else "NULL"
    channels = ",\n        ".join(
        f"{{ .power_down = {'true' if power_mask & (1 << channel) else 'false'}, "
        f".gain = {gains[channel_gains[channel]]}, "
        f".mux = {muxes[channel_muxes[channel]]} }}"
        for channel in range(8)
    )
    high_resolution = str(item.properties["high-resolution"]).lower()
    internal_reference = str(item.properties["internal-reference"]).lower()
    reference_4v = str(item.properties["reference-4v"]).lower()
    enable_active_low = str(item.properties["enable-active-low"]).lower()
    return f"""static yi_device_t {label};
static const yi_ads1298_config_t {label}_cfg = {{
    .self = &{label}, .spi = &{item.properties['bus'].label},
    .spi_config = {{ .frequency = {frequency}U,
        .cs_gpio = &{item.properties['cs-gpio'].label},
        .mode = 1U, .cs_active_high = false }},
    .enable_gpio = {optional('enable-gpio')},
    .enable_active_low = {enable_active_low},
    .reset_gpio = {optional('reset-gpio')},
    .start_gpio = {optional('start-gpio')},
    .drdy_gpio = {optional('drdy-gpio')},
    .master_clock_hz = {master_clock}U,
    .transfer_timeout_ms = {timeout}U,
    .data_rate = YI_ADS1298_DATA_RATE_{data_rate},
    .high_resolution = {high_resolution},
    .internal_reference = {internal_reference},
    .reference_4v = {reference_4v},
    .config2 = 0x{item.properties['config2']:02X}U,
    .config3_extra = 0x{item.properties['config3-extra']:02X}U,
    .loff = 0x{item.properties['loff']:02X}U,
    .rld_sensp = 0x{item.properties['rld-sensp']:02X}U,
    .rld_sensn = 0x{item.properties['rld-sensn']:02X}U,
    .loff_sensp = 0x{item.properties['loff-sensp']:02X}U,
    .loff_sensn = 0x{item.properties['loff-sensn']:02X}U,
    .loff_flip = 0x{item.properties['loff-flip']:02X}U,
    .gpio = 0x{item.properties['gpio-register']:02X}U,
    .pace = 0x{item.properties['pace']:02X}U,
    .resp = 0x{item.properties['resp']:02X}U,
    .config4 = 0x{item.properties['config4']:02X}U,
    .wct1 = 0x{item.properties['wct1']:02X}U,
    .wct2 = 0x{item.properties['wct2']:02X}U,
    .channels = {{
        {channels}
    }}
}};
static yi_ads1298_data_t {label}_data;
YI_ADS1298_DEFINE_LEVEL({label}, {_level(item)},
    {item.properties['init-priority']}, {label}_cfg, {label}_data);"""


def _generate_ads8688(item: ValidatedNode) -> str:
    label = item.node.label
    frequency = item.properties["spi-frequency"]
    timeout = item.properties["transfer-timeout-ms"]
    channel = item.properties["default-channel"]
    auto_mask = item.properties["auto-sequence-mask"]
    power_mask = item.properties["power-down-mask"]
    ranges = [item.properties[f"channel{i}-range"] for i in range(8)]
    if not 0 < frequency <= 16000000 or timeout <= 0 or not 0 <= channel < 8:
        raise BindingError(f"{item.node.path}: invalid SPI, timeout or channel setting")
    if not 0 <= auto_mask <= 255 or not 0 <= power_mask <= 255:
        raise BindingError(f"{item.node.path}: channel masks must be in range 0..255")
    if any(value not in {0, 1, 2, 5, 6} for value in ranges):
        raise BindingError(f"{item.node.path}: invalid channel input range")
    range_values = ", ".join(f"(yi_ads8688_range_t){value}U" for value in ranges)
    return f"""static yi_device_t {label};
static const yi_ads8688_config_t {label}_cfg = {{
    .self = &{label}, .spi = &{item.properties['bus'].label},
    .spi_config = {{ .frequency = {frequency}U,
        .cs_gpio = &{item.properties['cs-gpio'].label}, .mode = 1U, .cs_active_high = false }},
    .transfer_timeout_ms = {timeout}U, .default_channel = {channel}U,
    .auto_sequence_mask = 0x{auto_mask:02X}U, .power_down_mask = 0x{power_mask:02X}U,
    .range = {{ {range_values} }}
}};
static yi_ads8688_data_t {label}_data;
YI_ADS8688_DEFINE_LEVEL({label}, {_level(item)}, {item.properties['init-priority']},
    {label}_cfg, {label}_data);"""


def _generate_mcp4725(item: ValidatedNode) -> str:
    label = item.node.label
    address = item.properties["address"]
    reference = item.properties["reference-mv"]
    default = item.properties["default-value"]
    timeout = item.properties["transfer-timeout-ms"]
    eeprom_timeout = item.properties["eeprom-timeout-ms"]
    if not 0 <= address <= 0x7f:
        raise BindingError(f"{item.node.path}: address must be a 7-bit value")
    if not 0 < reference <= 65535 or not 0 <= default <= 4095:
        raise BindingError(f"{item.node.path}: invalid reference or default value")
    if timeout <= 0 or eeprom_timeout <= 0:
        raise BindingError(f"{item.node.path}: timeouts must be positive")
    return f"""static yi_device_t {label};
static const yi_mcp4725_config_t {label}_cfg = {{
    .self = &{label}, .i2c = &{item.properties['bus'].label},
    .address = 0x{address:02X}U, .reference_mv = {reference}U,
    .default_value = {default}U, .transfer_timeout_ms = {timeout}U,
    .eeprom_timeout_ms = {eeprom_timeout}U
}};
static yi_mcp4725_data_t {label}_data;
YI_MCP4725_DEFINE_LEVEL({label}, {_level(item)}, {item.properties['init-priority']},
    {label}_cfg, {label}_data);"""


def _generate_mcp4728(item: ValidatedNode) -> str:
    label = item.node.label
    address = item.properties["address"]
    default_channel = item.properties["default-channel"]
    vdd = item.properties["vdd-mv"]
    timeout = item.properties["transfer-timeout-ms"]
    eeprom_timeout = item.properties["eeprom-timeout-ms"]
    values = [item.properties[f"channel{i}-value"] for i in range(4)]
    if not 0x60 <= address <= 0x67 or not 0 <= default_channel < 4:
        raise BindingError(f"{item.node.path}: invalid address or default channel")
    if vdd <= 0 or timeout <= 0 or eeprom_timeout <= 0:
        raise BindingError(f"{item.node.path}: supply and timeouts must be positive")
    if any(not 0 <= value <= 4095 for value in values):
        raise BindingError(f"{item.node.path}: channel values must be in range 0..4095")
    channels = []
    for i, value in enumerate(values):
        internal = str(item.properties[f"channel{i}-internal-reference"]).lower()
        gain = str(item.properties[f"channel{i}-gain-2x"]).lower()
        channels.append(f"{{ .value = {value}U, .internal_reference = {internal}, "
                        f".gain_2x = {gain}, .power = YI_MCP4728_POWER_NORMAL }}")
    return f"""static yi_device_t {label};
static const yi_mcp4728_config_t {label}_cfg = {{
    .self = &{label}, .i2c = &{item.properties['bus'].label},
    .address = 0x{address:02X}U, .default_channel = {default_channel}U,
    .vdd_mv = {vdd}U, .transfer_timeout_ms = {timeout}U,
    .eeprom_timeout_ms = {eeprom_timeout}U,
    .channel = {{ {', '.join(channels)} }}
}};
static yi_mcp4728_data_t {label}_data;
YI_MCP4728_DEFINE_LEVEL({label}, {_level(item)}, {item.properties['init-priority']},
    {label}_cfg, {label}_data);"""


def _generate_gp8210s(item: ValidatedNode) -> str:
    label=item.node.label; address=item.properties["address"]
    channel=item.properties["default-channel"]; output=item.properties["output-range-mv"]
    values=[item.properties["channel0-value"],item.properties["channel1-value"]]
    timeout=item.properties["transfer-timeout-ms"]
    if not 0<=address<=0x7f or channel not in {0,1} or output not in {5000,10000}:
        raise BindingError(f"{item.node.path}: invalid address, channel or output range")
    if any(not 0<=value<=32767 for value in values) or timeout<=0:
        raise BindingError(f"{item.node.path}: invalid DAC value or timeout")
    range_name="YI_GP8210S_RANGE_10V" if output==10000 else "YI_GP8210S_RANGE_5V"
    return f"""static yi_device_t {label};
static const yi_gp8210s_config_t {label}_cfg = {{
    .self=&{label}, .i2c=&{item.properties['bus'].label}, .address=0x{address:02X}U,
    .default_channel={channel}U, .range={range_name},
    .default_value={{ {values[0]}U, {values[1]}U }}, .transfer_timeout_ms={timeout}U
}};
static yi_gp8210s_data_t {label}_data;
YI_GP8210S_DEFINE_LEVEL({label}, {_level(item)}, {item.properties['init-priority']},
    {label}_cfg, {label}_data);"""


def _generate_tsys01(item: ValidatedNode) -> str:
    label = item.node.label
    address = item.properties["address"]
    transfer_timeout = item.properties["transfer-timeout-ms"]
    conversion_delay = item.properties["conversion-delay-ms"]
    reset_delay = item.properties["reset-delay-ms"]
    checksum = 1 if item.properties["validate-prom-checksum"] else 0
    if not 0 <= address <= 0x7F:
        raise BindingError(f"{item.node.path}: address must be a 7-bit value")
    if transfer_timeout <= 0:
        raise BindingError(f"{item.node.path}: transfer-timeout-ms must be positive")
    if conversion_delay <= 0:
        raise BindingError(f"{item.node.path}: conversion-delay-ms must be positive")
    if reset_delay <= 0:
        raise BindingError(f"{item.node.path}: reset-delay-ms must be positive")
    return f"""static yi_device_t {label};

static const yi_tsys01_config_t {label}_cfg =
{{
    .self = &{label},
    .i2c = &{item.properties['bus'].label},
    .address = 0x{address:02X}U,
    .transfer_timeout_ms = {transfer_timeout}U,
    .conversion_delay_ms = {conversion_delay}U,
    .reset_delay_ms = {reset_delay}U,
    .validate_prom_checksum = {checksum}
}};

static yi_tsys01_data_t {label}_data;

YI_TSYS01_DEFINE_LEVEL(
    {label}, {_level(item)}, {item.properties['init-priority']},
    {label}_cfg, {label}_data
);"""


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
    "max31856": _generate_max31856,
    "ad9834": _generate_ad9834,
    "ad9851": _generate_ad9851,
    "ads7830": _generate_ads7830,
    "adc081c02": _generate_adc081c02,
    "ads1258": _generate_ads1258,
    "ads1298": _generate_ads1298,
    "ads8688": _generate_ads8688,
    "mcp4725": _generate_mcp4725,
    "mcp4728": _generate_mcp4728,
    "gp8210s": _generate_gp8210s,
    "tsys01": _generate_tsys01,
    "timer": _generate_timer,
    "spi": _generate_spi,
    "adc": _generate_adc,
    "i2c": _generate_i2c,
    "soft-i2c": _generate_soft_i2c,
    "soft-spi": _generate_soft_spi,
    "can": _generate_can,
}


def generate_sources(nodes: list[ValidatedNode], source_name: str,
                     bootloader_enabled: bool = False) -> tuple[str, str]:
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
    if bootloader_enabled:
        boot_config = """#define YI_BOOTLOADER_ENABLED 1
#define YI_APP_FLASH_OFFSET 0x0000C000U
#define YI_APP_HEADER_SIZE 0x00000200U
#define YI_APP_VECTOR_ADDRESS 0x0800C200U
#define YI_APP_SLOT_SIZE 0x00018000U"""
    else:
        boot_config = """#define YI_BOOTLOADER_ENABLED 0
#define YI_APP_FLASH_OFFSET 0x00000000U
#define YI_APP_HEADER_SIZE 0x00000000U
#define YI_APP_VECTOR_ADDRESS 0x08000000U
#define YI_APP_SLOT_SIZE 0x00040000U"""

    h_source = f"""/*
 * Generated by yi_dts_gen.py from {source_name}.
 * Do not edit manually.
 */
#ifndef YI_GENERATED_H
#define YI_GENERATED_H

#include "yi_device.h"

{boot_config}

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


def _app_scatter(bootloader_enabled: bool) -> str:
    if bootloader_enabled:
        address = 0x0800C200
        size = 0x00017600
        description = "MCUboot primary slot, excluding header and trailer"
    else:
        address = 0x08000000
        size = 0x00040000
        description = "standalone application using the complete internal flash"

    return f"""; Generated by yi_dts_gen.py: {description}.
LR_IROM1 0x{address:08X} 0x{size:08X} {{
  ER_IROM1 0x{address:08X} 0x{size:08X} {{
    *.o (RESET, +First)
    *(InRoot$$Sections)
    .ANY (+RO)
    .ANY (+XO)
  }}

  yi_device +0 ALIGN 4 {{
    * (.yi_device)
  }}

  yi_build_info +0 ALIGN 4 {{
    * (.yi_build_info)
  }}

  RW_IRAM1 0x20000000 0x0000C000 {{
    .ANY (+RW +ZI)
  }}
}}
"""


def generate(dts: Path, bindings_dir: Path, output_dir: Path) -> tuple[Path, Path]:
    tree = parse_file(dts)
    nodes = validate_tree(tree, load_bindings(bindings_dir))
    bootloader_enabled = False
    bootloader = tree.labels.get("bootloader")
    if bootloader is not None:
        status = bootloader.properties.get("status", "disable")
        if status not in ("okay", "disable"):
            raise BindingError(
                f"{bootloader.path}: bootloader status must be 'okay' or 'disable'"
            )
        bootloader_enabled = status == "okay"
    c_source, h_source = generate_sources(
        nodes, dts.name, bootloader_enabled=bootloader_enabled
    )
    c_path = output_dir / "yi_generated.c"
    h_path = output_dir / "yi_generated.h"
    scatter_path = output_dir / "yi_app.sct"
    _write_if_changed(c_path, c_source)
    _write_if_changed(h_path, h_source)
    _write_if_changed(scatter_path, _app_scatter(bootloader_enabled))
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
