"""Test the HPM5301 architecture bring-up contract.

Author: Don
Date: 2026-07-29
Version: 1.0.0
"""

import unittest
from pathlib import Path


class Hpm5301ArchitectureTests(unittest.TestCase):
    """Keep the minimum HPM5301 image tied to official SDK contracts."""

    @classmethod
    def setUpClass(cls):
        """Resolve repository paths shared by static contract checks."""

        cls.repo_root = Path(__file__).resolve().parents[2]
        cls.application = (
            cls.repo_root / "applications" / "hpm5301-bringup"
        )

    def test_bringup_uses_official_flash_xip_board(self):
        """Build metadata selects the exact SDK board and Flash XIP layout."""

        cmake = (self.application / "CMakeLists.txt").read_text(
            encoding="utf-8"
        )
        readme = (self.application / "README.md").read_text(
            encoding="utf-8"
        )

        self.assertIn('HPM_BUILD_TYPE "flash_xip"', cmake)
        self.assertIn(
            "rv32imac_zicsr_zifencei", cmake
        )
        self.assertIn('RV_ABI "ilp32"', cmake)
        self.assertIn("find_package(hpm-sdk REQUIRED", cmake)
        self.assertIn("-DBOARD=hpm5301evklite", readme)
        self.assertIn("0x80003000", readme)

    def test_bringup_links_vendor_neutral_irq_primitives(self):
        """Application includes YiCore IRQ code without copying SDK startup."""

        cmake = (self.application / "CMakeLists.txt").read_text(
            encoding="utf-8"
        )
        irq_source = (
            self.repo_root / "arch" / "riscv" / "yi_riscv_irq.c"
        ).read_text(encoding="utf-8")

        self.assertIn("arch/riscv/yi_riscv_irq.c", cmake)
        self.assertIn("csrrc", irq_source)
        self.assertIn("csrs mstatus", irq_source)
        self.assertNotIn("hpm_", irq_source.lower())


if __name__ == "__main__":
    unittest.main()
