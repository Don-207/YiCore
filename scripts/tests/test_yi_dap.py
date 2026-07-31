"""Test the YiDAP lifecycle facade contract.

Author: Don
Date: 2026-08-01
Version: 1.0.0
"""

import unittest
from pathlib import Path


class YiDapTests(unittest.TestCase):
    """Keep CherryDAP lifecycle coupling inside the YiDAP subsystem."""

    @classmethod
    def setUpClass(cls):
        """Resolve the YiCore and sibling YiLink repositories."""

        cls.repo_root = Path(__file__).resolve().parents[2]
        cls.yilink_root = cls.repo_root.parent / "YiLink"

    def test_facade_wraps_cherrydap_lifecycle(self):
        """YiDAP owns initialization and foreground CherryDAP calls."""

        source = (
            self.repo_root / "subsys" / "dap" / "yi_dap.c"
        ).read_text(encoding="utf-8")
        self.assertIn("chry_dap_init", source)
        self.assertIn("chry_dap_handle", source)
        self.assertIn("chry_dap_usb2uart_handle", source)
        self.assertIn("enable_cdc_uart", source)

    def test_product_uses_only_yidap_lifecycle(self):
        """YiLink application code does not bypass the facade lifecycle."""

        source_path = (
            self.yilink_root
            / "firmware"
            / "images"
            / "application"
            / "yilink.c"
        )
        if not source_path.is_file():
            self.skipTest("sibling YiLink product is not present")
        source = source_path.read_text(encoding="utf-8")
        self.assertIn("yi_dap_init", source)
        self.assertIn("yi_dap_process", source)
        self.assertNotIn("chry_dap_", source)


if __name__ == "__main__":
    unittest.main()
