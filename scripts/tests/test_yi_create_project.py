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
    _relative_path,
    create_project,
    load_supported_targets,
    resolve_target,
    select_target_interactive,
)


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
        self.assertEqual(target["board"], "fire-mini-stm32f103")
        self.assertEqual(target["template"], "stm32f103-dts-demo")

    def test_interactive_target_selection_prompts_vendor_then_model(self):
        targets = load_supported_targets(self.yicore)

        with patch("builtins.input", side_effect=["1", "1"]):
            target = select_target_interactive(targets)

        self.assertEqual(target["vendor"], "st")
        self.assertEqual(target["model"], "stm32f103xe")


if __name__ == "__main__":
    unittest.main()
