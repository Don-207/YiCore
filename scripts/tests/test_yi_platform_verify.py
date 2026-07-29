"""Test YiCore platform registry validation.

Author: Don
Date: 2026-07-28
Version: 1.0.0
"""

import sys
import unittest
from pathlib import Path


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_platform_verify import (  # noqa: E402
    load_platforms,
    verify_platforms,
)
from yi_vendor_verify import load_packages  # noqa: E402


class PlatformRegistryTests(unittest.TestCase):
    """Verify ready and reserved platform integration boundaries."""

    @classmethod
    def setUpClass(cls):
        """Load the repository registries shared by all checks."""

        cls.repo_root = SCRIPTS_DIR.parent
        cls.platforms = load_platforms(
            SCRIPTS_DIR / "yi_platforms.json"
        )
        cls.packages = load_packages(
            SCRIPTS_DIR / "yi_vendor_packages.json"
        )

    def test_repository_platforms_are_consistent(self):
        """Every platform must have a backend and known vendor package."""

        self.assertEqual(
            verify_platforms(
                self.repo_root, self.platforms, self.packages
            ),
            [],
        )

    def test_stm32f1_remains_ready(self):
        """The existing production platform remains buildable."""

        platform = next(
            item for item in self.platforms
            if item["id"] == "st-stm32f1"
        )
        self.assertEqual(platform["status"], "ready")
        self.assertTrue(
            (
                self.repo_root
                / "cmake"
                / "platforms"
                / "st-stm32f1.cmake"
            ).is_file()
        )

    def test_gd32f30x_is_reserved(self):
        """GD has an extension point without a false support claim."""

        platform = next(
            item for item in self.platforms
            if item["id"] == "gigadevice-gd32f30x"
        )
        self.assertEqual(platform["status"], "reserved")
        self.assertEqual(
            platform["backend"], "soc/gigadevice/gd32f30x"
        )

    def test_hpm5300_is_reserved_for_riscv(self):
        """HPM5300 has an explicit RISC-V boundary before build enablement."""

        platform = next(
            item for item in self.platforms
            if item["id"] == "hpmicro-hpm5300"
        )
        self.assertEqual(platform["status"], "reserved")
        self.assertEqual(platform["arch"], "riscv")
        self.assertEqual(platform["backend"], "soc/hpmicro/hpm5300")


if __name__ == "__main__":
    unittest.main()
