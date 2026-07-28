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


if __name__ == "__main__":
    unittest.main()
