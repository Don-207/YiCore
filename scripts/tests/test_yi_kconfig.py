"""Test Zephyr-style YiCore Kconfig artifact generation.

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

from yi_kconfig import discover_fragments, generate_configuration  # noqa: E402


class KconfigGenerationTests(unittest.TestCase):
    """Verify fragment ordering and standard generated outputs."""

    def test_board_fragment_is_overridden_by_prj_conf(self):
        """Merge board defaults first and application selections last."""

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            yicore = root / "YiCore"
            app = root / "app"
            (app / "boards").mkdir(parents=True)
            yicore.mkdir()
            (yicore / "Kconfig").write_text(
                'mainmenu "Test"\n\nconfig FEATURE\n    bool "Feature"\n',
                encoding="utf-8",
            )
            (app / "boards" / "board-a.conf").write_text(
                "CONFIG_FEATURE=y\n", encoding="utf-8"
            )
            (app / "prj.conf").write_text(
                "# CONFIG_FEATURE is not set\n", encoding="utf-8"
            )

            outputs = generate_configuration(
                yicore,
                root / "build",
                discover_fragments(app, "board-a"),
            )

            self.assertIn("# CONFIG_FEATURE is not set", outputs["dotconfig"].read_text())
            self.assertNotIn("CONFIG_FEATURE", outputs["autoconf"].read_text())
            self.assertTrue(outputs["cmake"].is_file())

    def test_default_symbol_is_exported_to_header_and_cmake(self):
        """Enabled defaults are visible to C sources and CMake logic."""

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            yicore = root / "YiCore"
            yicore.mkdir()
            (yicore / "Kconfig").write_text(
                "config BARE_METAL\n    bool\n    default y\n",
                encoding="utf-8",
            )

            outputs = generate_configuration(yicore, root / "build")

            self.assertIn("#define CONFIG_BARE_METAL 1", outputs["autoconf"].read_text())
            self.assertIn('set(CONFIG_BARE_METAL "y")', outputs["cmake"].read_text())


if __name__ == "__main__":
    unittest.main()
