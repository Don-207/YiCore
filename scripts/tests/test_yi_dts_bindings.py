import sys
import unittest
from pathlib import Path


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_dts_bindings import BindingError, load_bindings, validate_tree  # noqa: E402
from yi_dts_parser import DtsReference, parse_file, parse_text  # noqa: E402


class DtsBindingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.yicore = SCRIPTS_DIR.parent
        cls.bindings = load_bindings(cls.yicore / "dts" / "bindings")

    def test_current_board_validates(self):
        tree = parse_file(self.yicore / "boards" / "fire-mini-stm32f103" / "board.dts")
        nodes = validate_tree(tree, self.bindings)
        self.assertEqual(
            [item.binding.driver for item in nodes],
            ["timer", "uart", "flash", "clock", "clock", "clock",
             "gpio", "gpio", "gpio", "gpio", "gpio", "soft-i2c",
             "gpio", "gpio", "gpio", "gpio", "soft-spi",
             "led", "led", "pinmux", "pinmux", "rtt", "console"],
        )
        self.assertEqual(nodes[0].properties["reg"], 0x40001400)
        self.assertEqual(nodes[0].properties["interrupts"], "TIM7_IRQn")
        self.assertEqual(nodes[1].properties["current-speed"], 115200)
        self.assertEqual(nodes[1].properties["tx-pin"], DtsReference("uart0_tx"))
        self.assertEqual(nodes[2].properties["base-address"], 0x08000000)
        self.assertEqual(nodes[3].properties["clock-id"], "gpioa")
        self.assertEqual(nodes[6].properties["pin"], 2)
        self.assertEqual(nodes[8].properties["pin"], 13)
        self.assertEqual(nodes[8].properties["interrupt"], "falling")
        self.assertEqual(nodes[9].properties["drive"], "open-drain")
        self.assertEqual(nodes[11].properties["clock-frequency"], 50000)
        self.assertEqual(nodes[16].properties["mode"], 0)
        self.assertEqual(nodes[17].properties["gpios"], DtsReference("gpio_led0"))
        self.assertEqual(nodes[19].properties["function"], "uart-tx")
        self.assertEqual(nodes[21].properties["mode"], "no-block-skip")
        self.assertEqual(nodes[22].properties["backend"], DtsReference("rtt0"))
        self.assertTrue(nodes[22].properties["default-console"])

    def test_missing_required_property(self):
        tree = parse_text('''/ {
            p: p { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            n: node { compatible = "yi,stm32-uart"; reg = <0x40013800>;
                clock-bus = "apb2"; clock-enable-mask = <0x4000>;
                interrupts = "USART1_IRQn"; tx-pin = <&p>; rx-pin = <&p>; };
        };''')
        with self.assertRaisesRegex(BindingError, "missing required property 'current-speed'"):
            validate_tree(tree, self.bindings)

    def test_invalid_priority(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa"; };
            tx: tx { compatible = "yi,stm32-pinmux"; port = "GPIOA"; pin = <9>;
                     function = "uart-tx"; mode = "alternate-push-pull"; clocks = <&clk>; };
            rx: rx { compatible = "yi,stm32-pinmux"; port = "GPIOA"; pin = <10>;
                     function = "uart-rx"; mode = "input"; clocks = <&clk>; };
            n: node { compatible = "yi,stm32-uart"; reg = <0x40013800>;
                      clock-bus = "apb2"; clock-enable-mask = <0x4000>;
                      interrupts = "USART1_IRQn"; current-speed = <115200>;
                      tx-pin = <&tx>; rx-pin = <&rx>; init-priority = <100>; };
        };''')
        with self.assertRaisesRegex(BindingError, "range 0..99"):
            validate_tree(tree, self.bindings)

    def test_disabled_node_is_skipped(self):
        tree = parse_text('''/ { n: node { compatible = "yi,stm32-uart"; status = "disabled"; }; };''')
        self.assertEqual(validate_tree(tree, self.bindings), [])

    def test_unknown_property(self):
        tree = parse_text('''/ { n: node { compatible = "yi,stm32-uart"; handle = "huart1";
                           current-speed = <115200>; typo = <1>; }; };''')
        with self.assertRaisesRegex(BindingError, "properties not declared"):
            validate_tree(tree, self.bindings)

    def test_timer_binding_defaults(self):
        tree = parse_text('''/ {
            timer2: timer2 { compatible = "yi,stm32-timer";
                reg = <0x40000000>; interrupts = "TIM2_IRQn";
                clock-bus = "apb1-timer"; clock-enable-mask = <1>;
                counter-bits = <16>; tick-frequency = <1000>; };
        };''')
        nodes = validate_tree(tree, self.bindings)
        timer = nodes[0]
        self.assertEqual(timer.binding.driver, "timer")
        self.assertEqual(timer.properties["irq-priority"], 10)
        self.assertEqual(timer.properties["init-level"], "post-kernel")
        self.assertEqual(timer.properties["init-priority"], 10)


if __name__ == "__main__":
    unittest.main()
