"""Test the standard YiCore product and optional-image creators.

Author: Don
Date: 2026-07-28
Version: 1.0.0
"""

import sys
import tempfile
import unittest
from pathlib import Path


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_create_product import add_image, create_product  # noqa: E402
from yi_create_project import (  # noqa: E402
    load_supported_boards,
    load_supported_targets,
    resolve_board,
    resolve_target,
)


class ProductCreationTests(unittest.TestCase):
    """Verify the non-duplicating product layout and optional images."""

    @classmethod
    def setUpClass(cls):
        """Record the YiCore repository used as the source template."""
        cls.yicore = SCRIPTS_DIR.parent

    def test_application_is_default_and_images_are_optional(self):
        """Create application first, then add bootloader and test explicitly."""
        targets = load_supported_targets(self.yicore)
        boards = load_supported_boards(self.yicore)
        target = resolve_target(targets, model="stm32f103xe")
        board = resolve_board(boards, target, "fire-mini-stm32f103")

        with tempfile.TemporaryDirectory() as temporary:
            product = create_product(
                self.yicore,
                "ProductTest",
                board,
                target,
                Path(temporary),
            )
            images = product / "firmware" / "images"
            self.assertTrue((images / "application" / "main.c").is_file())
            self.assertFalse((images / "bootloader").exists())
            self.assertFalse((images / "test").exists())
            self.assertTrue(
                (product / "firmware/projects/keil/ProductTest.uvprojx")
                .is_file()
            )
            self.assertTrue(
                (product / "firmware/projects/gcc/CMakeLists.txt").is_file()
            )
            west = (product / "west.yml").read_text(encoding="utf-8")
            self.assertIn("+hal-st", west)
            self.assertIn("-bootloader", west)
            self.assertIn("name: YiCore", west)
            self.assertIn("import:", west)
            self.assertFalse((product / ".gitmodules").exists())
            self.assertIn(
                "/YiCore/",
                (product / ".gitignore").read_text(encoding="utf-8"),
            )

            add_image(product, "bootloader")
            add_image(product, "test")
            self.assertTrue((images / "bootloader" / "main.c").is_file())
            self.assertTrue((images / "test" / "main.c").is_file())
            self.assertTrue(
                (
                    product
                    / "firmware/projects/keil/"
                    "ProductTest-bootloader.uvprojx"
                ).is_file()
            )
            self.assertTrue(
                (
                    product
                    / "firmware/projects/keil/ProductTest-test.uvprojx"
                ).is_file()
            )
            self.assertIn(
                "+bootloader",
                (product / "west.yml").read_text(encoding="utf-8"),
            )

    def test_hpm5301_product_uses_official_sdk_build(self):
        """Generate an HPM5301 product without STM32 or Keil artifacts."""

        targets = load_supported_targets(self.yicore)
        boards = load_supported_boards(self.yicore)
        target = resolve_target(targets, model="hpm5301")
        board = resolve_board(
            boards, target, "hpm5301evklite"
        )

        with tempfile.TemporaryDirectory() as temporary:
            product = create_product(
                self.yicore,
                "HpmProduct",
                board,
                target,
                Path(temporary),
            )
            cmake = (
                product / "firmware/projects/gcc/CMakeLists.txt"
            ).read_text(encoding="utf-8")
            main_source = (
                product / "firmware/images/application/main.c"
            ).read_text(encoding="utf-8")
            manifest = (
                product / "yi-manifest.yml"
            ).read_text(encoding="utf-8")
            west = (product / "west.yml").read_text(encoding="utf-8")

            self.assertIn("find_package(hpm-sdk REQUIRED", cmake)
            self.assertIn('BOARD "hpm5301evklite"', cmake)
            self.assertIn("yi_riscv_irq.c", cmake)
            self.assertIn("board_init()", main_source)
            self.assertIn("YiHAL-HPMicro", manifest)
            self.assertIn("+hal-hpmicro", west)
            self.assertIn("-hal-st", west)
            self.assertIn("name: YiCore", west)
            self.assertFalse((product / ".gitmodules").exists())
            self.assertFalse(
                (product / "firmware/projects/keil").exists()
            )


if __name__ == "__main__":
    unittest.main()
