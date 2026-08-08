"""YiCore test yi dts gen utility.

Author: Don
Date: 2026-07-26
Version: 1.0.0
"""

import sys
import tempfile
import unittest
from pathlib import Path


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_dts_bindings import BindingError, load_bindings, validate_tree  # noqa: E402
from yi_dts_gen import dependency_order, generate, generate_sources  # noqa: E402
from yi_dts_parser import parse_file, parse_text  # noqa: E402


class DtsGeneratorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.yicore = SCRIPTS_DIR.parent
        cls.bindings = load_bindings(cls.yicore / "dts" / "bindings")

    def test_current_app_generation(self):
        dts = self.yicore / "examples" / "stm32f103-dts-demo" / "app.dts"
        nodes = validate_tree(parse_file(dts), self.bindings)
        source, header = generate_sources(nodes, dts.name)
        self.assertLess(source.index("led0_gpio_cfg"), source.index("led0_cfg"))
        self.assertIn("YI_INIT_POST_KERNEL", source)
        self.assertIn(".id = YI_STM32_CLOCK_GPIOA", source)
        self.assertIn(".backend = &rtt0", source)
        self.assertIn(".default_console = 1", source)
        self.assertIn(".mode = YI_RTT_MODE_NO_BLOCK_SKIP", source)
        self.assertIn(".base_address = 0x08000000U", source)
        self.assertIn(".erase_block_size = 2048U", source)
        self.assertIn('#define YI_DT_FLASH0_NAME "flash0"', header)
        self.assertLess(source.index("rtt0_cfg"), source.index("console0_cfg"))
        self.assertLess(source.index("usart1_tx_pin_cfg"), source.index("usart1_cfg"))
        self.assertLess(source.index("usart1_rx_pin_cfg"), source.index("usart1_cfg"))
        self.assertIn('#define YI_DT_USART1_NAME "usart1"', header)
        self.assertIn(".baudrate = 921600U", source)
        self.assertIn("void DMA1_Channel5_IRQHandler(void)", source)
        self.assertIn("yi_uart_stm32_dma_rx_irq_handler(&usart1);", source)
        self.assertIn("#define YI_DT_GET(_label)", header)
        self.assertIn("#define DT_NODELABEL(label) label", header)
        self.assertIn("#define DEVICE_DT_GET(node_id)", header)
        self.assertIn("#define DT_PROP(node_id, prop)", header)
        self.assertIn("#define DT_PHANDLE(node_id, prop)", header)
        self.assertIn("#define YI_DT_led0_STATUS_okay 1", header)
        self.assertIn("#define YI_DT_usart1_P_current_speed 921600", header)
        self.assertIn(".instance = (TIM_TypeDef *)0x40001400U", source)
        self.assertIn(".enable_mask = 0x00000020U", source)
        self.assertIn("void TIM7_IRQHandler(void)", source)
        self.assertIn("yi_timer_irq_handler(&timers7);", source)
        self.assertIn(".direction = YI_GPIO_DIRECTION_OUTPUT", source)
        self.assertLess(source.index("adc1_in0_pin_cfg"), source.index("adc1_cfg"))
        self.assertIn(".instance = (ADC_TypeDef *)0x40012400U", source)
        self.assertIn(".channel = 0U", source)
        self.assertIn(".sample_cycles = 7U", source)
        self.assertIn(".clock_divider = 6U", source)
        self.assertIn('#define YI_DT_ADC1_NAME "adc1"', header)
        self.assertLess(source.index("i2c1_cfg"), source.index("ads7830_cfg"))
        self.assertIn(".address = 0x48U", source)
        self.assertIn(".default_channel = 0U", source)
        self.assertIn('#define YI_DT_ADS7830_NAME "ads7830"', header)
        self.assertIn('#define YI_DT_TIMERS7_NAME "timers7"', header)

    def test_bootloader_switch_generates_slot_linker_and_vector(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            dts = root / "app.dts"
            dts.write_text('''/ {
                bootloader: bootloader { status = "okay"; };
            };''', encoding="utf-8")
            output = root / "generated"
            _, header_path = generate(dts, self.yicore / "dts" / "bindings", output)
            header = header_path.read_text(encoding="utf-8")
            scatter = (output / "yi_app.sct").read_text(encoding="utf-8")

        self.assertIn("#define YI_BOOTLOADER_ENABLED 1", header)
        self.assertIn("#define YI_APP_VECTOR_ADDRESS 0x0800C200U", header)
        self.assertIn("LR_IROM1 0x0800C200 0x00017600", scatter)

    def test_bootloader_switch_rejects_unknown_status(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            dts = root / "app.dts"
            dts.write_text('''/ {
                bootloader: bootloader { status = "enabled"; };
            };''', encoding="utf-8")
            with self.assertRaisesRegex(BindingError, "'okay' or 'disable'"):
                generate(dts, self.yicore / "dts" / "bindings", root / "generated")

    def test_aliases_and_chosen_generate_zephyr_style_tokens(self):
        """Expose phandle aliases and chosen nodes through familiar macros."""

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            dts = root / "app.dts"
            dts.write_text('''/ {
                aliases { status-led = <&led>; };
                chosen { yi,console = <&led>; };
                clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
                pin: pin { compatible = "yi,stm32-gpio"; port = "GPIOA";
                    pin = <1>; clocks = <&clk>; };
                led: led { compatible = "yi,gpio-led"; gpios = <&pin>; };
            };''', encoding="utf-8")
            _, header_path = generate(
                dts, self.yicore / "dts" / "bindings", root / "generated"
            )
            header = header_path.read_text(encoding="utf-8")

        self.assertIn("#define YI_DT_ALIAS_status_led led", header)
        self.assertIn("#define YI_DT_CHOSEN_yi_console led", header)
        self.assertIn("#define DT_ALIAS(alias)", header)
        self.assertIn("#define DT_PROP_OR(node_id, prop, default_value)", header)

    def test_gpio_interrupt_generation(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            key: key { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <0>;
                clocks = <&clk>; direction = "input"; pull = "up";
                interrupt = "falling"; irq-priority = <7>; };
        };''')
        source, _ = generate_sources(validate_tree(tree, self.bindings), "gpio-irq.dts")
        self.assertIn(".direction = YI_GPIO_DIRECTION_INPUT", source)
        self.assertIn(".pull = YI_GPIO_PULL_UP", source)
        self.assertIn(".interrupt = YI_GPIO_INTERRUPT_FALLING", source)
        self.assertIn(".irq_priority = 7U", source)

    def test_gpio_interrupt_requires_input(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            key: key { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <0>;
                clocks = <&clk>; interrupt = "rising"; };
        };''')
        with self.assertRaisesRegex(BindingError, "must use direction"):
            generate_sources(validate_tree(tree, self.bindings), "gpio-irq-bad.dts")

    def test_wch_gpio_generation_owns_its_port_clock(self):
        """CH32 GPIO nodes omit STM32-specific clock-provider dependencies."""

        tree = parse_text('''/ {
            led: led { compatible = "yi,wch-gpio"; port = "GPIOC";
                pin = <2>; direction = "output"; };
        };''')
        source, header = generate_sources(
            validate_tree(tree, self.bindings), "wch-gpio.dts",
            soc_header="ch32h417.h"
        )
        self.assertIn(".port = GPIOC", source)
        self.assertIn(".pin = YI_GPIO_PIN(2)", source)
        self.assertIn(".clock = NULL", source)
        self.assertIn('#define YI_DT_LED_NAME "led"', header)

    def test_soft_i2c_generation(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpiob"; };
            scl: scl { compatible = "yi,stm32-gpio"; port = "GPIOB"; pin = <6>;
                clocks = <&clk>; drive = "open-drain"; pull = "up"; };
            sda: sda { compatible = "yi,stm32-gpio"; port = "GPIOB"; pin = <7>;
                clocks = <&clk>; drive = "open-drain"; pull = "up"; };
            bus: bus { compatible = "yi,soft-i2c"; scl-gpio = <&scl>;
                sda-gpio = <&sda>; clock-frequency = <50000>; };
        };''')
        source, header = generate_sources(validate_tree(tree, self.bindings), "soft-i2c.dts")
        self.assertIn(".drive = YI_GPIO_DRIVE_OPEN_DRAIN", source)
        self.assertIn(".half_period_us = 10U", source)
        self.assertIn("YI_SOFT_I2C_DEFINE_LEVEL", source)
        self.assertIn('#define YI_DT_BUS_NAME "bus"', header)

    def test_soft_spi_generation(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpiob"; };
            sck: sck { compatible = "yi,stm32-gpio"; port = "GPIOB"; pin = <12>;
                clocks = <&clk>; direction = "output"; };
            miso: miso { compatible = "yi,stm32-gpio"; port = "GPIOB"; pin = <13>;
                clocks = <&clk>; direction = "input"; };
            mosi: mosi { compatible = "yi,stm32-gpio"; port = "GPIOB"; pin = <14>;
                clocks = <&clk>; direction = "output"; };
            bus: bus { compatible = "yi,soft-spi"; sck-gpio = <&sck>;
                miso-gpio = <&miso>; mosi-gpio = <&mosi>;
                max-frequency = <100000>; };
        };''')
        source, header = generate_sources(validate_tree(tree, self.bindings), "soft-spi.dts")
        self.assertIn(".max_frequency = 100000U", source)
        self.assertNotIn(".half_period_us", source)
        self.assertIn("YI_SOFT_SPI_DEFINE_LEVEL", source)
        self.assertIn('#define YI_DT_BUS_NAME "bus"', header)

    def test_tsys01_generation(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpiob"; };
            scl: scl { compatible = "yi,stm32-gpio"; port = "GPIOB"; pin = <6>;
                clocks = <&clk>; drive = "open-drain"; pull = "up"; };
            sda: sda { compatible = "yi,stm32-gpio"; port = "GPIOB"; pin = <7>;
                clocks = <&clk>; drive = "open-drain"; pull = "up"; };
            bus: bus { compatible = "yi,soft-i2c"; scl-gpio = <&scl>;
                sda-gpio = <&sda>; clock-frequency = <100000>; };
            tsys01: temp { compatible = "te,tsys01"; bus = <&bus>;
                address = <0x77>; conversion-delay-ms = <10>;
                reset-delay-ms = <3>; validate-prom-checksum; };
        };''')
        source, header = generate_sources(validate_tree(tree, self.bindings), "tsys01.dts")
        self.assertLess(source.index("bus_cfg"), source.index("tsys01_cfg"))
        self.assertIn('#include "yi_tsys01.h"', source)
        self.assertIn(".address = 0x77U", source)
        self.assertIn(".conversion_delay_ms = 10U", source)
        self.assertIn(".reset_delay_ms = 3U", source)
        self.assertIn(".validate_prom_checksum = 1", source)
        self.assertIn("YI_TSYS01_DEFINE_LEVEL", source)
        self.assertIn('#define YI_DT_TSYS01_NAME "tsys01"', header)

    def test_dependency_cycle_is_rejected(self):
        tree = parse_text('''/ {
            a: a { compatible = "yi,gpio-led"; gpios = <&b>; };
            b: b { compatible = "yi,gpio-led"; gpios = <&a>; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "dependency cycle"):
            dependency_order(nodes)

    def test_dependency_cannot_initialize_after_consumer(self):
        """Reject priority metadata that violates device dependencies."""

        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa";
                init-priority = <80>; };
            led: led { compatible = "yi,stm32-gpio"; port = "GPIOA";
                pin = <1>; clocks = <&clk>; init-priority = <20>; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "initializes after"):
            dependency_order(nodes)

    def test_duplicate_pin_ownership_is_rejected(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            a: a { compatible = "yi,stm32-pinmux"; port = "GPIOA"; pin = <9>;
                   function = "uart-tx"; mode = "alternate-push-pull"; clocks = <&clk>; };
            b: b { compatible = "yi,stm32-pinmux"; port = "GPIOA"; pin = <9>;
                   function = "pwm"; mode = "alternate-push-pull"; clocks = <&clk>; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "already owned"):
            generate_sources(nodes, "duplicate.dts")

    def test_invalid_pinmux_mode_is_rejected(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            a: a { compatible = "yi,stm32-pinmux"; port = "GPIOA"; pin = <9>;
                   function = "uart-tx"; mode = "magic"; clocks = <&clk>; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "invalid mode"):
            generate_sources(nodes, "invalid.dts")

    def test_invalid_clock_id_is_rejected(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "timer99"; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "invalid clock-id"):
            generate_sources(nodes, "invalid-clock.dts")

    def test_timer_generation(self):
        tree = parse_text('''/ {
            timer2: timer2 { compatible = "yi,stm32-timer";
                reg = <0x40000000>; interrupts = "TIM2_IRQn";
                clock-bus = "apb1-timer"; clock-enable-mask = <1>;
                counter-bits = <16>; tick-frequency = <1000>; irq-priority = <10>; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        source, header = generate_sources(nodes, "timer.dts")
        self.assertIn(".instance = (TIM_TypeDef *)0x40000000U", source)
        self.assertIn(".bus = YI_STM32_BUS_APB1_TIMER", source)
        self.assertIn(".enable_mask = 0x00000001U", source)
        self.assertIn(".tick_frequency = 1000U", source)
        self.assertIn(".irqn = TIM2_IRQn", source)
        self.assertIn(".irq_priority = 10U", source)
        self.assertIn("void TIM2_IRQHandler(void)", source)
        self.assertIn("yi_timer_irq_handler(&timer2);", source)
        self.assertIn('#define YI_DT_TIMER2_NAME "timer2"', header)

    def test_timer_interrupt_must_be_irqn(self):
        tree = parse_text('''/ {
            timer2: timer2 { compatible = "yi,stm32-timer";
                reg = <0x40000000>; interrupts = "not_an_irq";
                clock-bus = "apb1-timer"; clock-enable-mask = <1>;
                counter-bits = <16>; tick-frequency = <1000>; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "CMSIS IRQn"):
            generate_sources(nodes, "timer-invalid-irq.dts")

    def test_timer_ranges_are_rejected(self):
        tree = parse_text('''/ {
            timer2: timer2 { compatible = "yi,stm32-timer";
                reg = <0x40000000>; interrupts = "TIM2_IRQn";
                clock-bus = "apb1-timer"; clock-enable-mask = <1>;
                counter-bits = <16>; tick-frequency = <0>; irq-priority = <16>; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "tick-frequency must be positive"):
            generate_sources(nodes, "timer-invalid-range.dts")

    def test_timer_irq_priority_is_rejected(self):
        tree = parse_text('''/ {
            timer2: timer2 { compatible = "yi,stm32-timer";
                reg = <0x40000000>; interrupts = "TIM2_IRQn";
                clock-bus = "apb1-timer"; clock-enable-mask = <1>;
                counter-bits = <16>; tick-frequency = <1000>; irq-priority = <16>; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "irq-priority must be in range 0..15"):
            generate_sources(nodes, "timer-invalid-priority.dts")

    def test_timer_clock_bus_is_rejected(self):
        tree = parse_text('''/ {
            timer2: timer2 { compatible = "yi,stm32-timer";
                reg = <0x40000000>; interrupts = "TIM2_IRQn";
                clock-bus = "ahb-timer"; clock-enable-mask = <1>;
                counter-bits = <16>; tick-frequency = <1000>; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "invalid clock-bus"):
            generate_sources(nodes, "timer-invalid-clock.dts")

    def test_spi_i2c_can_generation(self):
        tree = parse_text('''/ {
            p: pins { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            spi1: spi { compatible = "yi,stm32-spi"; reg = <0x40013000>;
                clock-bus = "apb2"; clock-enable-mask = <0x1000>;
                interrupts = "SPI1_IRQn"; sck-pin = <&p>; miso-pin = <&p>;
                mosi-pin = <&p>; max-frequency = <18000000>; };
            i2c1: i2c { compatible = "yi,stm32-i2c"; reg = <0x40005400>;
                clock-bus = "apb1"; clock-enable-mask = <0x200000>;
                event-interrupt = "I2C1_EV_IRQn"; error-interrupt = "I2C1_ER_IRQn";
                scl-pin = <&p>; sda-pin = <&p>; clock-frequency = <400000>; };
            can1: can { compatible = "yi,stm32-can"; reg = <0x40006400>;
                clock-bus = "apb1"; clock-enable-mask = <0x2000000>;
                tx-interrupt = "USB_HP_CAN1_TX_IRQn";
                rx0-interrupt = "USB_LP_CAN1_RX0_IRQn";
                rx1-interrupt = "CAN1_RX1_IRQn"; sce-interrupt = "CAN1_SCE_IRQn";
                tx-pin = <&p>; rx-pin = <&p>; bitrate = <500000>; };
        };''')
        source, _ = generate_sources(validate_tree(tree, self.bindings), "buses.dts")
        self.assertIn("void SPI1_IRQHandler(void)", source)
        self.assertIn("void I2C1_EV_IRQHandler(void)", source)
        self.assertIn("void I2C1_ER_IRQHandler(void)", source)
        self.assertIn("void USB_HP_CAN1_TX_IRQHandler(void)", source)
        self.assertIn(".bitrate = 500000U", source)

    def test_uart_dma_generation(self):
        tree = parse_text('''/ {
            p: pins { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            usart1: serial { compatible = "yi,stm32-uart";
                reg = <0x40013800>; clock-bus = "apb2";
                clock-enable-mask = <0x4000>; interrupts = "USART1_IRQn";
                current-speed = <115200>; tx-pin = <&p>; rx-pin = <&p>;
                tx-dma-channel = "DMA1_Channel4";
                tx-dma-interrupt = "DMA1_Channel4_IRQn";
                rx-dma-channel = "DMA1_Channel5";
                rx-dma-interrupt = "DMA1_Channel5_IRQn";
                dma-irq-priority = <9>; };
        };''')
        source, _ = generate_sources(validate_tree(tree, self.bindings), "uart-dma.dts")
        self.assertIn(".tx_dma_channel = DMA1_Channel4", source)
        self.assertIn(".rx_dma_channel = DMA1_Channel5", source)
        self.assertIn(".tx_dma_irqn = DMA1_Channel4_IRQn", source)
        self.assertIn(".rx_dma_irqn = DMA1_Channel5_IRQn", source)
        self.assertIn(".dma_irq_priority = 9U", source)
        self.assertIn("void DMA1_Channel4_IRQHandler(void)", source)
        self.assertIn("yi_uart_stm32_dma_tx_irq_handler(&usart1);", source)
        self.assertIn("void DMA1_Channel5_IRQHandler(void)", source)
        self.assertIn("yi_uart_stm32_dma_rx_irq_handler(&usart1);", source)

    def test_wch_uart_irq_ring_generation(self):
        tree = parse_text('''/ {
            usart1: serial { compatible = "yi,wch-uart";
                reg = <0x40013800>; current-speed = <115200>;
                tx-port = "GPIOA"; tx-pin = <9>;
                rx-port = "GPIOA"; rx-pin = <10>; alternate = <7>;
                interrupts = "USART1_IRQn"; irq-priority = <10>; };
        };''')
        source, _ = generate_sources(validate_tree(tree, self.bindings),
                                     "wch-uart.dts")
        self.assertIn("static yi_uart_wch_data_t usart1_data", source)
        self.assertIn(".irqn = USART1_IRQn", source)
        self.assertIn('interrupt("WCH-Interrupt-fast")', source)
        self.assertIn("yi_uart_wch_irq_handler(&usart1);", source)

    def test_multiple_default_consoles_are_rejected(self):
        tree = parse_text('''/ {
            backend: backend { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            a: a { compatible = "yi,console"; backend = <&backend>; default-console; };
            b: b { compatible = "yi,console"; backend = <&backend>; default-console; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "exactly one enabled console"):
            generate_sources(nodes, "consoles.dts")

    def test_max31856_generation(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            sck: sck { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <0>; clocks = <&clk>; };
            miso: miso { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <1>; clocks = <&clk>; };
            mosi: mosi { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <2>; clocks = <&clk>; };
            cs: cs { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <3>;
                clocks = <&clk>; direction = "output"; };
            spi: spi { compatible = "yi,soft-spi"; sck-gpio = <&sck>;
                miso-gpio = <&miso>; mosi-gpio = <&mosi>; max-frequency = <100000>; };
            sensor: sensor { compatible = "maxim,max31856"; bus = <&spi>;
                cs-gpio = <&cs>; spi-frequency = <100000>;
                thermocouple-type = "k"; filter-hz = <50>;
                average-samples = <4>; open-circuit-ms = <100>; };
        };''')
        source, header = generate_sources(validate_tree(tree, self.bindings), "max31856.dts")
        self.assertIn(".thermocouple_type = YI_MAX31856_TC_K", source)
        self.assertIn(".mode = 1U", source)
        self.assertIn(".average_samples = 4U", source)
        self.assertIn('#define YI_DT_SENSOR_NAME "sensor"', header)

    def test_ad9834_generation(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            sck: sck { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <0>; clocks = <&clk>; };
            miso: miso { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <1>; clocks = <&clk>; };
            mosi: mosi { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <2>; clocks = <&clk>; };
            cs: cs { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <3>;
                clocks = <&clk>; direction = "output"; };
            spi: spi { compatible = "yi,soft-spi"; sck-gpio = <&sck>;
                miso-gpio = <&miso>; mosi-gpio = <&mosi>; max-frequency = <500000>; };
            dds: dds { compatible = "adi,ad9834"; bus = <&spi>; cs-gpio = <&cs>;
                spi-frequency = <500000>; mclk-frequency = <75000000>; };
        };''')
        source, header = generate_sources(validate_tree(tree, self.bindings), "ad9834.dts")
        self.assertIn(".mclk_frequency = 75000000U", source)
        self.assertIn(".mode = 2U", source)
        self.assertIn('#define YI_DT_DDS_NAME "dds"', header)

    def test_ad9851_generation(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            wclk: wclk { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <0>;
                clocks = <&clk>; direction = "output"; };
            fqud: fqud { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <1>;
                clocks = <&clk>; direction = "output"; };
            data: data { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <2>;
                clocks = <&clk>; direction = "output"; };
            reset: reset { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <3>;
                clocks = <&clk>; direction = "output"; };
            dds: dds { compatible = "adi,ad9851"; w-clk-gpio = <&wclk>;
                fq-ud-gpio = <&fqud>; data-gpio = <&data>; reset-gpio = <&reset>;
                reference-clock-frequency = <30000000>; clock-multiplier; };
        };''')
        source, header = generate_sources(validate_tree(tree, self.bindings), "ad9851.dts")
        self.assertIn(".reference_clock_frequency = 30000000U", source)
        self.assertIn(".clock_multiplier = true", source)
        self.assertIn('#define YI_DT_DDS_NAME "dds"', header)

    def test_adc081c02_generation(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            scl: scl { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <0>; clocks = <&clk>; };
            sda: sda { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <1>; clocks = <&clk>; };
            i2c: i2c { compatible = "yi,soft-i2c"; scl-gpio = <&scl>; sda-gpio = <&sda>; };
            adc: adc { compatible = "ti,adc081c02"; bus = <&i2c>; address = <0x50>;
                reference-mv = <3300>; configuration = <0x20>;
                low-limit = <10>; high-limit = <240>; hysteresis = <4>; };
        };''')
        source, header = generate_sources(validate_tree(tree, self.bindings), "adc081c02.dts")
        self.assertIn(".address = 0x50U", source)
        self.assertIn(".configuration = 0x20U", source)
        self.assertIn(".high_limit = 240U", source)
        self.assertIn('#define YI_DT_ADC_NAME "adc"', header)

    def test_ads1258_generation(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            sck: sck { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <0>; clocks = <&clk>; };
            miso: miso { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <1>; clocks = <&clk>; };
            mosi: mosi { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <2>; clocks = <&clk>; };
            cs: cs { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <3>; clocks = <&clk>; direction = "output"; };
            start: start { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <4>; clocks = <&clk>; direction = "output"; };
            drdy: drdy { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <5>; clocks = <&clk>; direction = "input"; };
            spi: spi { compatible = "yi,soft-spi"; sck-gpio = <&sck>; miso-gpio = <&miso>;
                mosi-gpio = <&mosi>; max-frequency = <500000>; };
            adc: adc { compatible = "ti,ads1258"; bus = <&spi>; cs-gpio = <&cs>;
                start-gpio = <&start>; drdy-gpio = <&drdy>; spi-frequency = <500000>;
                single-ended-mask = <0xffff>; config1 = <0x01>; };
        };''')
        source, header = generate_sources(validate_tree(tree, self.bindings), "ads1258.dts")
        self.assertIn(".single_ended_mask = 0xFFFFU", source)
        self.assertIn(".config1 = 0x01U", source)
        self.assertIn(".mode = 1U", source)
        self.assertIn('#define YI_DT_ADC_NAME "adc"', header)

    def test_ads1298_generation(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            sck: sck { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <0>; clocks = <&clk>; };
            miso: miso { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <1>; clocks = <&clk>; };
            mosi: mosi { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <2>; clocks = <&clk>; };
            cs: cs { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <3>;
                clocks = <&clk>; direction = "output"; };
            reset: reset { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <4>;
                clocks = <&clk>; direction = "output"; };
            drdy: drdy { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <5>;
                clocks = <&clk>; direction = "input"; };
            spi: spi { compatible = "yi,soft-spi"; sck-gpio = <&sck>;
                miso-gpio = <&miso>; mosi-gpio = <&mosi>; max-frequency = <500000>; };
            adc: adc { compatible = "ti,ads1298"; bus = <&spi>; cs-gpio = <&cs>;
                reset-gpio = <&reset>; drdy-gpio = <&drdy>;
                spi-frequency = <500000>; master-clock-hz = <2048000>;
                data-rate = <6>; high-resolution; internal-reference;
                channel-power-down-mask = <0x80>; channel0-gain = <12>;
                channel7-mux = "input-short"; };
        };''')
        source, header = generate_sources(
            validate_tree(tree, self.bindings), "ads1298.dts"
        )
        self.assertIn(".mode = 1U", source)
        self.assertIn(".master_clock_hz = 2048000U", source)
        self.assertIn(".data_rate = YI_ADS1298_DATA_RATE_6", source)
        self.assertIn(".gain = YI_ADS1298_GAIN_12", source)
        self.assertIn(".power_down = true", source)
        self.assertIn(".mux = YI_ADS1298_MUX_INPUT_SHORT", source)
        self.assertIn(".drdy_gpio = &drdy", source)
        self.assertIn('#define YI_DT_ADC_NAME "adc"', header)

    def test_ads8688_generation(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            sck: sck { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <0>; clocks = <&clk>; };
            miso: miso { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <1>; clocks = <&clk>; };
            mosi: mosi { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <2>; clocks = <&clk>; };
            cs: cs { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <3>; clocks = <&clk>; direction = "output"; };
            spi: spi { compatible = "yi,soft-spi"; sck-gpio = <&sck>; miso-gpio = <&miso>;
                mosi-gpio = <&mosi>; max-frequency = <500000>; };
            adc: adc { compatible = "ti,ads8688"; bus = <&spi>; cs-gpio = <&cs>;
                spi-frequency = <500000>; default-channel = <2>; auto-sequence-mask = <0x0f>;
                channel0-range = <0>; channel1-range = <1>; channel2-range = <2>;
                channel3-range = <5>; channel4-range = <6>; };
        };''')
        source, header = generate_sources(validate_tree(tree, self.bindings), "ads8688.dts")
        self.assertIn(".default_channel = 2U", source)
        self.assertIn(".auto_sequence_mask = 0x0FU", source)
        self.assertIn("(yi_ads8688_range_t)6U", source)
        self.assertIn('#define YI_DT_ADC_NAME "adc"', header)

    def test_mcp4725_generation(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            scl: scl { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <0>; clocks = <&clk>; };
            sda: sda { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <1>; clocks = <&clk>; };
            i2c: i2c { compatible = "yi,soft-i2c"; scl-gpio = <&scl>; sda-gpio = <&sda>; };
            dac: dac { compatible = "microchip,mcp4725"; bus = <&i2c>; address = <0x60>;
                reference-mv = <3300>; default-value = <2048>; };
        };''')
        source, header = generate_sources(validate_tree(tree, self.bindings), "mcp4725.dts")
        self.assertIn(".address = 0x60U", source)
        self.assertIn(".default_value = 2048U", source)
        self.assertIn('#define YI_DT_DAC_NAME "dac"', header)

    def test_mcp4728_generation(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            scl: scl { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <0>; clocks = <&clk>; };
            sda: sda { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <1>; clocks = <&clk>; };
            i2c: i2c { compatible = "yi,soft-i2c"; scl-gpio = <&scl>; sda-gpio = <&sda>; };
            dac: dac { compatible = "microchip,mcp4728"; bus = <&i2c>; address = <0x60>;
                default-channel = <1>; channel0-value = <1000>;
                channel0-internal-reference; channel0-gain-2x; };
        };''')
        source, header = generate_sources(validate_tree(tree, self.bindings), "mcp4728.dts")
        self.assertIn(".default_channel = 1U", source)
        self.assertIn(".value = 1000U", source)
        self.assertIn(".internal_reference = true", source)
        self.assertIn('#define YI_DT_DAC_NAME "dac"', header)

    def test_gp8210s_generation(self):
        tree=parse_text('''/ {
            clk: clk { compatible="yi,stm32-clock"; clock-id="gpioa"; };
            scl: scl { compatible="yi,stm32-gpio"; port="GPIOA"; pin=<0>; clocks=<&clk>; };
            sda: sda { compatible="yi,stm32-gpio"; port="GPIOA"; pin=<1>; clocks=<&clk>; };
            i2c: i2c { compatible="yi,soft-i2c"; scl-gpio=<&scl>; sda-gpio=<&sda>; };
            dac: dac { compatible="guestgood,gp8210s"; bus=<&i2c>; address=<0x58>;
                output-range-mv=<10000>; channel0-value=<1000>; channel1-value=<2000>; };
        };''')
        source,header=generate_sources(validate_tree(tree,self.bindings),"gp8210s.dts")
        self.assertIn(".range=YI_GP8210S_RANGE_10V",source)
        self.assertIn(".default_value={ 1000U, 2000U }",source)
        self.assertIn('#define YI_DT_DAC_NAME "dac"',header)

    def test_invalid_rtt_mode_is_rejected(self):
        tree = parse_text('''/ {
            rtt: rtt { compatible = "yi,segger-rtt"; mode = "drop-sometimes"; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "invalid mode"):
            generate_sources(nodes, "invalid-rtt.dts")

    def test_invalid_flash_geometry_is_rejected(self):
        tree = parse_text('''/ {
            flash: flash { compatible = "yi,stm32-internal-flash";
                base-address = <0x08000000>; size = <4097>;
                erase-block-size = <1024>; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "multiple of erase-block-size"):
            generate_sources(nodes, "invalid-flash.dts")


if __name__ == "__main__":
    unittest.main()
