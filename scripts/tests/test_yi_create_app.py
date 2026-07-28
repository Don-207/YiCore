"""Test thin YiCore application creation.

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

from yi_create_app import AppCreationError, create_app  # noqa: E402


class ThinApplicationTests(unittest.TestCase):
    """Verify the board-independent application layout."""

    def test_create_thin_application(self):
        """Creation writes only app-owned source and configuration."""

        with tempfile.TemporaryDirectory() as temporary:
            app = create_app("heart-monitor", Path(temporary))
            self.assertTrue((app / "CMakeLists.txt").is_file())
            self.assertTrue((app / "app.conf").is_file())
            self.assertTrue((app / "app.overlay").is_file())
            self.assertTrue((app / "VERSION").is_file())
            self.assertTrue((app / "src" / "main.c").is_file())
            self.assertFalse((app / "Core").exists())
            self.assertFalse((app / "MDK-ARM").exists())
            self.assertFalse(any(app.rglob("*.ioc")))
            cmake = (app / "CMakeLists.txt").read_text(encoding="utf-8")
            self.assertIn("YiCoreApplication.cmake", cmake)
            self.assertNotIn("BOARD", cmake)

    def test_unsafe_application_name_is_rejected(self):
        """Names cannot escape the selected application root."""

        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(
                AppCreationError, "application name"
            ):
                create_app("../unsafe", Path(temporary))

    def test_existing_application_is_preserved(self):
        """Creation never overwrites an existing application."""

        with tempfile.TemporaryDirectory() as temporary:
            existing = Path(temporary) / "existing"
            existing.mkdir()
            marker = existing / "user.txt"
            marker.write_text("keep", encoding="utf-8")
            with self.assertRaisesRegex(
                AppCreationError, "already exists"
            ):
                create_app("existing", Path(temporary))
            self.assertEqual(marker.read_text(encoding="utf-8"), "keep")


if __name__ == "__main__":
    unittest.main()
