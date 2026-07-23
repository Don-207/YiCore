import sys
import unittest
from pathlib import Path


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_dts_parser import (  # noqa: E402
    DtsCells,
    DtsParseError,
    DtsReference,
    parse_file,
    parse_text,
)


class DtsParserTests(unittest.TestCase):
    def test_current_app(self):
        app = SCRIPTS_DIR.parent / "examples" / "stm32f103-dts-demo" / "app.dts"
        tree = parse_file(app)

        self.assertEqual(
            set(tree.labels),
            {"flash0", "clk_gpioa", "clk_gpiob", "clk_gpioc", "timers1", "timers2",
             "timers3", "timers4", "timers5", "timers6", "timers7", "timers8", "gpio_led0", "led0",
             "usart1", "usart2", "usart3", "uart4", "uart5", "spi1", "spi2", "spi3",
             "i2c1", "i2c2", "can1", "key0", "gpio_led1", "led1",
             "soft_i2c0_scl", "soft_i2c0_sda", "soft_i2c0", "at24c02",
             "spi1_sck", "spi1_miso", "spi1_mosi", "spi1_cs", "w25q64",
             "uart0_tx", "uart0_rx", "rtt0", "console0"},
        )
        self.assertEqual(tree.node_by_label("gpio_led0").properties["pin"], DtsCells((2,)))
        self.assertEqual(
            tree.node_by_label("led0").properties["gpios"],
            DtsCells((DtsReference("gpio_led0"),)),
        )
        self.assertTrue(tree.node_by_label("led0").properties["active-low"])
        self.assertEqual(
            tree.node_by_label("usart1").properties["current-speed"],
            DtsCells((115200,)),
        )

    def test_comments_directive_and_value_lists(self):
        tree = parse_text(
            '''
            /dts-v1/;
            / {
                /* block comment */
                n0: node@0 {
                    compatible = "yi,test", "yi,fallback";
                    values = <0x10 20>;
                    enabled; // boolean property
                };
            };
            '''
        )
        node = tree.node_by_label("n0")
        self.assertEqual(node.properties["compatible"], ["yi,test", "yi,fallback"])
        self.assertEqual(node.properties["values"], DtsCells((16, 20)))
        self.assertIs(node.properties["enabled"], True)

    def test_unknown_reference_is_rejected(self):
        with self.assertRaisesRegex(DtsParseError, "unknown label 'missing'"):
            parse_text("/ { node { target = <&missing>; }; };")

    def test_duplicate_label_is_rejected(self):
        with self.assertRaisesRegex(DtsParseError, "duplicate label 'same'"):
            parse_text("/ { same: a {}; same: b {}; };")

    def test_duplicate_property_is_rejected(self):
        with self.assertRaisesRegex(DtsParseError, "duplicate property 'status'"):
            parse_text('/ { node { status = "okay"; status = "disabled"; }; };')

    def test_label_overlay_overrides_properties(self):
        tree = parse_text('''/ {
            timer7: timer@40001400 { status = "disabled"; value = <1>; };
        };
        &timer7 { status = "okay"; value = <2>; };
        ''')
        timer = tree.node_by_label("timer7")
        self.assertEqual(timer.properties["status"], "okay")
        self.assertEqual(timer.properties["value"], DtsCells((2,)))

    def test_unknown_overlay_is_rejected(self):
        with self.assertRaisesRegex(DtsParseError, "overlay references unknown label"):
            parse_text('/ {}; &missing { status = "okay"; };')


if __name__ == "__main__":
    unittest.main()
