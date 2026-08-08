"""Test board-driven YiCore compiler environment resolution.

Author: Don
Date: 2026-08-01
Version: 1.0.0
"""

import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
YICORE_ROOT = SCRIPTS_DIR.parent
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_toolchain import (  # noqa: E402
    ToolchainResolutionError,
    resolve_build_environment,
)


class ToolchainResolutionTests(unittest.TestCase):
    """Verify all ready MCU families select their pinned compiler package."""

    def _compiler(
        self, home: Path, toolchain: str, version: str, prefix: str
    ) -> Path:
        """Create a placeholder compiler at the installed-package location."""

        suffix = ".exe" if os.name == "nt" else ""
        compiler = home / toolchain / version / "bin" / f"{prefix}-gcc{suffix}"
        compiler.parent.mkdir(parents=True, exist_ok=True)
        compiler.write_text("", encoding="utf-8")
        return compiler

    def test_stm32_selects_arm_gnu_root(self):
        """STM32 models select the pinned Arm GNU installation and CMake root."""

        with tempfile.TemporaryDirectory() as temporary:
            home = Path(temporary)
            compiler = self._compiler(
                home, "arm-gnu-toolchain", "15.3.rel1", "arm-none-eabi"
            )
            selected = resolve_build_environment(
                YICORE_ROOT,
                {"vendor": "st", "model": "stm32f103xe"},
                home,
            )
        self.assertEqual(selected.compiler, compiler)
        self.assertEqual(selected.cmake_adapter, "product")

    def test_hpm5301_exports_official_sdk_root(self):
        """HPM5301 exports GNURISCV_TOOLCHAIN_PATH for the official SDK."""

        with tempfile.TemporaryDirectory() as temporary:
            home = Path(temporary)
            self._compiler(
                home,
                "hpmicro-riscv-gcc",
                "13.2.0",
                "riscv32-unknown-elf",
            )
            with patch.dict("os.environ", {}, clear=True):
                selected = resolve_build_environment(
                    YICORE_ROOT,
                    {"vendor": "hpmicro", "model": "hpm5301"},
                    home,
                )
        self.assertEqual(
            selected.process_environment["GNURISCV_TOOLCHAIN_PATH"],
            str(selected.root),
        )
        self.assertEqual(selected.cmake_adapter, "sdk")

    def test_ch32h417_exports_wch_root_and_adapter(self):
        """CH32H417 selects WCH GCC and its YiCore CMake adapter."""

        with tempfile.TemporaryDirectory() as temporary:
            home = Path(temporary)
            self._compiler(
                home,
                "wch-riscv-gcc",
                "15.2.0",
                "riscv32-wch-elf",
            )
            selected = resolve_build_environment(
                YICORE_ROOT,
                {"vendor": "wch", "model": "ch32h417"},
                home,
            )
        self.assertEqual(
            selected.process_environment["WCH_RISCV_TOOLCHAIN_PATH"],
            str(selected.root),
        )
        self.assertEqual(
            selected.cmake_adapter, "cmake/toolchains/wch-riscv-gcc.cmake"
        )

    def test_missing_compiler_is_rejected(self):
        """A declared environment never silently falls back to PATH."""

        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(
                ToolchainResolutionError, "compiler not found"
            ):
                resolve_build_environment(
                    YICORE_ROOT,
                    {"vendor": "hpmicro", "model": "hpm5301"},
                    Path(temporary),
                )


if __name__ == "__main__":
    unittest.main()
