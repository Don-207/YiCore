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

    def test_facade_is_backend_neutral(self):
        """YiDAP delegates through operations without CherryDAP dependencies."""

        source = (
            self.repo_root / "subsys" / "dap" / "yi_dap.c"
        ).read_text(encoding="utf-8")
        self.assertIn("backend_api->init", source)
        self.assertIn("backend_api->process", source)
        self.assertNotIn("chry_dap_", source)

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

    def test_facade_exposes_initialization_diagnostics(self):
        """YiDAP distinguishes progress, readiness, and initialization errors."""

        header = (
            self.repo_root / "subsys" / "dap" / "yi_dap.h"
        ).read_text(encoding="utf-8")
        source = (
            self.repo_root / "subsys" / "dap" / "yi_dap.c"
        ).read_text(encoding="utf-8")
        for state in (
            "YI_DAP_STATE_UNINITIALIZED",
            "YI_DAP_STATE_INITIALIZING",
            "YI_DAP_STATE_READY",
            "YI_DAP_STATE_ERROR",
        ):
            self.assertIn(state, header)
        self.assertIn("yi_dap_get_state", header)
        self.assertIn("yi_dap_get_last_error", header)
        self.assertIn("yi_dap_state = YI_DAP_STATE_ERROR", source)
        self.assertIn("yi_dap_state != YI_DAP_STATE_READY", source)

    def test_ch32h417_product_uses_yidap_and_schematic_pin_order(self):
        """CH32H417 binds YiDAP to USBFS and the HPM-style PA4..PA8 signals."""

        app_root = self.repo_root / "applications" / "ch32h417-yidap"
        cmake = (app_root / "CMakeLists.txt").read_text(encoding="utf-8")
        main = (app_root / "src" / "main.c").read_text(encoding="utf-8")
        config = (app_root / "src" / "DAP_config.h").read_text(
            encoding="utf-8"
        )
        self.assertIn("yi_dap_init", main)
        self.assertIn("yi_dap_process", main)
        self.assertIn("usb_dc_usbfs.c", cmake)
        self.assertIn("#define YIDAP_PIN_TDO GPIO_Pin_4", config)
        self.assertIn("#define YIDAP_PIN_TDI GPIO_Pin_5", config)
        self.assertIn("#define YIDAP_PIN_TCK GPIO_Pin_6", config)
        self.assertIn("#define YIDAP_PIN_TMS GPIO_Pin_7", config)
        self.assertIn("#define YIDAP_PIN_RESET GPIO_Pin_8", config)
        self.assertIn("#define DAP_PACKET_SIZE 64U", config)


if __name__ == "__main__":
    unittest.main()
