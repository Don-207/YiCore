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

    def test_current_app_validates(self):
        tree = parse_file(
            self.yicore / "examples" / "stm32f103-dts-demo" / "app.dts"
        )
        nodes = validate_tree(tree, self.bindings)
        self.assertEqual(
            [item.binding.driver for item in nodes],
            ["timer", "flash", "clock", "clock",
             "gpio", "gpio", "gpio", "gpio", "gpio", "gpio", "gpio", "gpio", "gpio",
             "soft-i2c", "soft-spi",
             "at24c02", "w25q64", "led", "led",
             "rtt", "console"],
        )
        self.assertEqual(nodes[0].properties["reg"], 0x40001400)
        self.assertEqual(nodes[0].properties["interrupts"], "TIM7_IRQn")
        self.assertEqual(nodes[1].properties["base-address"], 0x08000000)
        self.assertEqual(nodes[2].properties["clock-id"], "gpioa")
        self.assertEqual(nodes[6].properties["pin"], 13)
        self.assertEqual(nodes[6].properties["interrupt"], "falling")
        self.assertEqual(nodes[12].properties["pin"], 4)
        self.assertEqual(nodes[13].properties["clock-frequency"], 50000)
        self.assertEqual(nodes[14].properties["max-frequency"], 100000)
        self.assertEqual(nodes[15].properties["bus"], DtsReference("soft_i2c0"))
        self.assertEqual(nodes[16].properties["bus"], DtsReference("soft_spi0"))
        self.assertEqual(nodes[17].properties["gpios"], DtsReference("led0_gpio"))
        self.assertEqual(nodes[19].properties["mode"], "no-block-skip")
        self.assertEqual(nodes[20].properties["backend"], DtsReference("rtt0"))
        self.assertTrue(nodes[20].properties["default-console"])

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

    def test_available_dependency_is_selected(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; clock-id = "gpioa";
                status = "available"; };
            gpio: gpio { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <1>;
                clocks = <&clk>; status = "okay"; };
        };''')
        self.assertEqual(
            [item.node.label for item in validate_tree(tree, self.bindings)],
            ["clk", "gpio"],
        )

    def test_disabled_dependency_is_rejected(self):
        tree = parse_text('''/ {
            clk: clk { compatible = "yi,stm32-clock"; status = "disabled"; };
            gpio: gpio { compatible = "yi,stm32-gpio"; port = "GPIOA"; pin = <1>;
                clocks = <&clk>; status = "okay"; };
        };''')
        with self.assertRaisesRegex(BindingError, "references disabled node 'clk'"):
            validate_tree(tree, self.bindings)

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
