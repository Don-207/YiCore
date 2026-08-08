"""Test YiCore flash and debug runner command generation.

Author: Don
Date: 2026-08-02
Version: 1.0.0
"""

import sys
import tempfile
import unittest
from pathlib import Path


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_runner import RunnerError, runner_command  # noqa: E402


class RunnerCommandTests(unittest.TestCase):
    """Verify runner selection remains deterministic and testable."""

    def test_openocd_flash_uses_elf(self):
        """OpenOCD programs the linked image and verifies it."""

        with tempfile.TemporaryDirectory() as temporary:
            build = Path(temporary)
            (build / "app.elf").write_bytes(b"")
            command = runner_command(
                "flash", "openocd",
                {"vendor": "st", "series": "stm32f1"}, build
            )
        self.assertEqual(command[0], "openocd")
        self.assertIn("verify reset exit", command[-1])

    def test_jlink_debugserver_uses_exact_part(self):
        """J-Link receives the board-owned device identifier."""

        command = runner_command(
            "debugserver", "jlink", {"part": "STM32F103ZE"}, Path("build")
        )
        self.assertIn("STM32F103ZE", command)

    def test_wch_debug_is_rejected_explicitly(self):
        """Unsupported WCH debug behavior never silently changes runner."""

        with self.assertRaisesRegex(RunnerError, "supports flash only"):
            runner_command("debug", "wch-link", {"part": "CH32H417"}, Path("build"))


if __name__ == "__main__":
    unittest.main()
