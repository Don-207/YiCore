import sys
import unittest
from pathlib import Path


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_dts_bindings import BindingError, load_bindings, validate_tree  # noqa: E402
from yi_dts_gen import dependency_order, generate_sources  # noqa: E402
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
        self.assertLess(source.index("soft_i2c0_cfg"), source.index("at24c02_cfg"))
        self.assertLess(source.index("soft_spi0_cfg"), source.index("w25q64_cfg"))
        self.assertNotIn('#define YI_DT_USART1_NAME "usart1"', header)
        self.assertIn("#define YI_DT_GET(_label)", header)
        self.assertIn(".instance = (TIM_TypeDef *)0x40001400U", source)
        self.assertIn(".enable_mask = 0x00000020U", source)
        self.assertIn("void TIM7_IRQHandler(void)", source)
        self.assertIn("yi_timer_irq_handler(&timers7);", source)
        self.assertIn(".direction = YI_GPIO_DIRECTION_OUTPUT", source)
        self.assertIn('#define YI_DT_TIMERS7_NAME "timers7"', header)

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

    def test_dependency_cycle_is_rejected(self):
        tree = parse_text('''/ {
            a: a { compatible = "yi,gpio-led"; gpios = <&b>; };
            b: b { compatible = "yi,gpio-led"; gpios = <&a>; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "dependency cycle"):
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

    def test_multiple_default_consoles_are_rejected(self):
        tree = parse_text('''/ {
            backend: backend { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            a: a { compatible = "yi,console"; backend = <&backend>; default-console; };
            b: b { compatible = "yi,console"; backend = <&backend>; default-console; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        with self.assertRaisesRegex(BindingError, "exactly one enabled console"):
            generate_sources(nodes, "consoles.dts")

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
