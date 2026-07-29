"""YiCore test yi create project utility.

Author: Don
Date: 2026-07-26
Version: 1.0.0
"""

import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
from unittest.mock import patch


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_create_project import (  # noqa: E402
    ProjectCreationError,
    _prune_optional_drivers,
    _relative_path,
    create_project,
    load_supported_boards,
    load_supported_targets,
    resolve_board,
    resolve_target,
    select_board_interactive,
    select_board_any_interactive,
    select_target_interactive,
)
from yi_dts_parser import DtsCells, parse_file  # noqa: E402


class ProjectCreationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.yicore = SCRIPTS_DIR.parent

    def test_project_name_is_parameterized(self):
        with tempfile.TemporaryDirectory() as temporary:
            output_root = Path(temporary)
            project = create_project(
                self.yicore,
                "product-bootloader",
                output_root=output_root,
            )

            self.assertEqual(project, output_root / "product-bootloader")
            self.assertTrue(
                (project / "MDK-ARM" / "product-bootloader.uvprojx").is_file()
            )
            self.assertTrue(
                (project / "product-bootloader.ioc").is_file()
            )
            self.assertTrue((project / "generated" / "yi_generated.c").is_file())
            self.assertTrue((project / "app-gpios.dtsi").is_file())
            self.assertTrue((project / "app-pinctrl.dtsi").is_file())
            self.assertTrue((project / "app-devices.dtsi").is_file())
            main_source = (
                project / "Core" / "Src" / "main.c"
            ).read_text(encoding="utf-8")
            self.assertIn("yi_system_init()", main_source)
            self.assertIn("yi_device_init_all()", main_source)
            self.assertNotIn("UART_STRESS", main_source)
            self.assertNotIn("yi_uart_dma_lwrb", main_source)
            self.assertNotIn("report_progress", main_source)

            project_text = (
                project / "MDK-ARM" / "product-bootloader.uvprojx"
            ).read_text(encoding="utf-8")
            self.assertIn(
                "<TargetName>product-bootloader</TargetName>", project_text
            )
            self.assertIn(
                "<OutputName>product-bootloader</OutputName>", project_text
            )
            self.assertIn("..\\generated\\yi_generated.c", project_text)
            self.assertNotIn("stm32f103-dts-demo", project_text)

            ioc_text = (project / "product-bootloader.ioc").read_text(
                encoding="utf-8"
            )
            self.assertIn(
                "ProjectManager.ProjectName=product-bootloader", ioc_text
            )

            app_dts = (project / "app.dts").read_text(encoding="utf-8")
            expected_board = _relative_path(
                project,
                self.yicore
                / "boards"
                / "fire-mini-stm32f103"
                / "board.dts",
                "/",
            )
            self.assertTrue(
                app_dts.startswith(f'/include/ "{expected_board}"')
            )
            self.assertIn('/include/ "app-gpios.dtsi"', app_dts)
            self.assertIn('/include/ "app-pinctrl.dtsi"', app_dts)
            self.assertIn('/include/ "app-devices.dtsi"', app_dts)
            self.assertNotIn("&w25q64", app_dts)

            readme = (project / "README.md").read_text(encoding="utf-8")
            self.assertIn("Keep the shared board files", readme)
            self.assertIn("app-gpios.dtsi", readme)
            self.assertIn("app-pinctrl.dtsi", readme)
            self.assertIn("app-devices.dtsi", readme)

            project_xml = ET.parse(
                project / "MDK-ARM" / "product-bootloader.uvprojx"
            )
            mdk_dir = project / "MDK-ARM"
            missing_files = [
                node.text
                for node in project_xml.findall(".//FilePath")
                if node.text and not (mdk_dir / node.text).exists()
            ]
            self.assertEqual(missing_files, [])

            include_path = project_xml.find(
                ".//Target/TargetOption/TargetArmAds/Cads/"
                "VariousControls/IncludePath"
            )
            self.assertIsNotNone(include_path)
            missing_includes = [
                entry
                for entry in (include_path.text or "").split(";")
                if entry and not (mdk_dir / entry).is_dir()
            ]
            self.assertEqual(missing_includes, [])

    def test_optional_driver_pruning_keeps_only_selected_devices(self):
        template = (
            self.yicore
            / "examples"
            / "stm32f103-dts-demo"
            / "MDK-ARM"
            / "stm32f103-dts-demo.uvprojx"
        ).read_text(encoding="utf-8")

        project_text = _prune_optional_drivers(template, {"ads1298"})

        self.assertIn("yi_ads1298.c", project_text)
        self.assertIn("drivers\\adc\\ads1298", project_text)
        self.assertNotIn("yi_ads7830.c", project_text)
        self.assertNotIn("yi_max31856.c", project_text)
        self.assertNotIn("yi_tsys01.c", project_text)
        self.assertNotIn("drivers\\adc\\ads7830", project_text)
        self.assertNotIn("drivers\\sensor\\max31856", project_text)

    def test_existing_destination_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            output_root = Path(temporary)
            (output_root / "existing").mkdir()
            with self.assertRaisesRegex(
                ProjectCreationError, "destination already exists"
            ):
                create_project(
                    self.yicore, "existing", output_root=output_root
                )

    def test_project_local_gpio_overrides_shared_board(self):
        with tempfile.TemporaryDirectory() as temporary:
            board_dts = (
                self.yicore
                / "boards"
                / "fire-mini-stm32f103"
                / "board-pinctrl.dtsi"
            )
            board_before = board_dts.read_text(encoding="utf-8")
            project = create_project(
                self.yicore,
                "local-pinout",
                output_root=Path(temporary),
            )

            (project / "app-gpios.dtsi").write_text(
                """&soft_i2c0_scl_gpio {
    port = "GPIOB";
    pin = <8>;
    clocks = <&clk_gpiob>;
};
""",
                encoding="utf-8",
            )

            tree = parse_file(project / "app.dts")
            scl_pin = tree.node_by_label("soft_i2c0_scl_gpio")
            self.assertEqual(scl_pin.properties["port"], "GPIOB")
            self.assertEqual(scl_pin.properties["pin"], DtsCells((8,)))
            self.assertEqual(
                board_dts.read_text(encoding="utf-8"), board_before
            )

    def test_invalid_project_name_is_rejected(self):
        with self.assertRaisesRegex(ProjectCreationError, "project name"):
            create_project(self.yicore, "../unsafe")

    def test_unknown_board_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(ProjectCreationError, "board not found"):
                create_project(
                    self.yicore,
                    "demo",
                    "missing-board",
                    Path(temporary),
                )

    def test_supported_targets_can_be_resolved(self):
        targets = load_supported_targets(self.yicore)
        target = resolve_target(
            targets,
            vendor="st",
            series="stm32f1",
            model="stm32f103xe",
        )

        self.assertEqual(target["vendor_name"], "STMicroelectronics")
        self.assertEqual(target["template"], "stm32f103-dts-demo")

    def test_supported_board_is_resolved_for_target(self):
        targets = load_supported_targets(self.yicore)
        boards = load_supported_boards(self.yicore)
        target = resolve_target(targets, model="stm32f103xe")
        board = resolve_board(boards, target, "fire-mini-stm32f103")

        self.assertEqual(board["name"], "Fire Mini STM32F103")
        self.assertEqual(board["model"], target["model"])

    def test_boards_are_discovered_from_local_manifests(self):
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)
            board_dir = repo / "boards" / "custom-stm32f103"
            board_dir.mkdir(parents=True)
            (board_dir / "board.dts").write_text("/ {};\n", encoding="utf-8")
            (board_dir / "board.json").write_text(
                """{
  "id": "custom-stm32f103",
  "name": "Custom STM32F103",
  "vendor": "st",
  "series": "stm32f1",
  "model": "stm32f103xe",
  "description": "Test board"
}
""",
                encoding="utf-8",
            )

            boards = load_supported_boards(repo)

            self.assertEqual([board["id"] for board in boards],
                             ["custom-stm32f103"])

    def test_board_manifest_id_must_match_directory(self):
        with tempfile.TemporaryDirectory() as temporary:
            repo = Path(temporary)
            board_dir = repo / "boards" / "directory-name"
            board_dir.mkdir(parents=True)
            (board_dir / "board.dts").write_text("/ {};\n", encoding="utf-8")
            (board_dir / "board.json").write_text(
                """{
  "id": "different-name",
  "name": "Different Name",
  "vendor": "st",
  "series": "stm32f1",
  "model": "stm32f103xe",
  "description": "Test board"
}
""",
                encoding="utf-8",
            )

            with self.assertRaisesRegex(
                ProjectCreationError, "must match directory"
            ):
                load_supported_boards(repo)

    def test_board_must_match_selected_target(self):
        boards = load_supported_boards(self.yicore)
        other_target = {
            "vendor": "st",
            "series": "stm32f4",
            "model": "stm32f407xx",
        }
        with self.assertRaisesRegex(
            ProjectCreationError, "does not support"
        ):
            resolve_board(
                boards, other_target, "fire-mini-stm32f103"
            )

    def test_exact_part_metadata_is_owned_by_board(self):
        target = resolve_target(
            load_supported_targets(self.yicore), model="stm32f103xe"
        )
        with tempfile.TemporaryDirectory() as temporary:
            product_root = Path(temporary)
            board_dir = product_root / "boards" / "zc-board"
            board_dir.mkdir(parents=True)
            (board_dir / "board.dts").write_text(
                '/include/ "'
                + (self.yicore / "boards/fire-mini-stm32f103/board.dts")
                .as_posix()
                + '"\n',
                encoding="utf-8",
            )
            (board_dir / "board.json").write_text(
                """{
  "id": "zc-board",
  "name": "ZC Board",
  "vendor": "st",
  "series": "stm32f1",
  "model": "stm32f103xe",
  "part": "stm32f103zct6",
  "keil_device": "STM32F103ZC",
  "cube_cpn": "STM32F103ZCT6",
  "cube_name": "STM32F103Z(C-D-E)Tx",
  "cube_package": "LQFP144",
  "cube_user_name": "STM32F103ZCTx",
  "flash_size": 262144,
  "ram_size": 49152,
  "description": "Test exact-part board"
}
""",
                encoding="utf-8",
            )
            project = create_project(
                self.yicore,
                "zc-product",
                board="zc-board",
                output_root=product_root / "output",
                target=target,
                board_root=product_root,
            )

            project_text = (
                project / "MDK-ARM" / "zc-product.uvprojx"
            ).read_text(encoding="utf-8")
            ioc_text = (project / "zc-product.ioc").read_text(
                encoding="utf-8"
            )

            self.assertIn("<Device>STM32F103ZC</Device>", project_text)
            self.assertIn(
                "IRAM(0x20000000-0x2000BFFF)", project_text
            )
            self.assertIn("IROM(0x8000000-0x803FFFF)", project_text)
            self.assertIn("<IRAM>", project_text)
            self.assertIn("<Size>0xC000</Size>", project_text)
            self.assertIn("<Size>0x40000</Size>", project_text)
            self.assertIn("Mcu.CPN=STM32F103ZCT6", ioc_text)
            self.assertIn("Mcu.Package=LQFP144", ioc_text)
            self.assertIn("Mcu.UserName=STM32F103ZCTx", ioc_text)
            board = resolve_board(
                load_supported_boards(product_root), target, "zc-board"
            )
            self.assertEqual(board["model"], "stm32f103xe")
            self.assertEqual(board["part"], "stm32f103zct6")
            self.assertEqual(board["flash_size"], 262144)

    def test_interactive_target_selection_prompts_vendor_then_model(self):
        targets = load_supported_targets(self.yicore)

        with patch("builtins.input", side_effect=["1", "1"]):
            target = select_target_interactive(targets)

        self.assertEqual(target["vendor"], "st")
        self.assertEqual(target["model"], "stm32f103xe")

    def test_interactive_board_selection_is_constrained_to_target(self):
        target = resolve_target(
            load_supported_targets(self.yicore), model="stm32f103xe"
        )
        boards = [
            {
                "id": "board-a",
                "name": "Board A",
                "vendor": "st",
                "series": "stm32f1",
                "model": "stm32f103xe",
            },
            {
                "id": "board-b",
                "name": "Board B",
                "vendor": "st",
                "series": "stm32f1",
                "model": "stm32f103xe",
            },
            {
                "id": "other-mcu-board",
                "name": "Other MCU Board",
                "vendor": "st",
                "series": "stm32f4",
                "model": "stm32f407xx",
            },
        ]

        with patch("builtins.input", return_value="2"):
            board = select_board_interactive(boards, target)

        self.assertEqual(board["id"], "board-b")

    def test_interactive_product_board_selection_accepts_board_id(self):
        """Product board selection accepts a stable board identifier."""

        boards = load_supported_boards(self.yicore)
        with patch(
            "builtins.input", return_value="fire-mini-stm32f103"
        ):
            board = select_board_any_interactive(boards)

        self.assertEqual(board["id"], "fire-mini-stm32f103")


if __name__ == "__main__":
    unittest.main()
