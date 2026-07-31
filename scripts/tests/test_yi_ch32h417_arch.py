"""Test the CH32H417 V3F architecture bring-up contract.

Author: Don
Date: 2026-07-31
Version: 1.0.0
"""

import json
import unittest
from pathlib import Path


class Ch32h417ArchitectureTests(unittest.TestCase):
    """Keep the first WCH target tied to its pinned vendor SDK boundary."""

    @classmethod
    def setUpClass(cls):
        """Resolve repository paths shared by static contract checks."""

        cls.repo_root = Path(__file__).resolve().parents[2]
        cls.application = cls.repo_root / "applications" / "ch32h417-bringup"

    def test_vendor_package_is_pinned_and_complete(self):
        """The registry pins all startup, system, and GPIO vendor inputs."""

        registry_path = (
            self.repo_root / "scripts" / "yi_vendor_packages.json"
        )
        packages = json.loads(registry_path.read_text(encoding="utf-8"))
        package = next(item for item in packages if item["id"] == "wch-ch32h4xx")
        self.assertEqual(
            package["version"], "1a8132b1813ecda1ade48fb07fd85e2b286e8337"
        )
        self.assertTrue(registry_path.is_file())
        self.assertIn("Startup/startup_ch32h417_v3f.S", package["required"])
        self.assertIn("System/system_ch32h417.c", package["required"])

    def test_bringup_is_v3f_only(self):
        """The minimum image selects V3F startup and does not wake V5F."""

        adapter = (
            self.repo_root / "cmake" / "platforms" / "wch-ch32h4xx.cmake"
        ).read_text(encoding="utf-8")
        main = (self.application / "src" / "main.c").read_text(encoding="utf-8")
        self.assertIn("startup_ch32h417_v3f.S", adapter)
        self.assertIn("Ld/V3F/Link_v3f.ld", adapter)
        self.assertIn("Core_V3F", adapter)
        self.assertNotIn("WakeUp_V5F", main)
        self.assertIn("GPIO_Pin_1", main)


if __name__ == "__main__":
    unittest.main()
