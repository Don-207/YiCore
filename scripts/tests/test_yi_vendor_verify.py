"""Test YiCore vendor package validation.

Author: Don
Date: 2026-07-28
Version: 1.0.0
"""

import json
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_vendor_verify import (  # noqa: E402
    VendorPackageError,
    load_packages,
    verify_packages,
)


class VendorPackageTests(unittest.TestCase):
    """Verify readiness and safe-path enforcement for vendor packages."""

    def test_current_ready_packages_are_complete(self):
        """The repository must contain every file of each ready package."""

        packages = load_packages(
            SCRIPTS_DIR / "yi_vendor_packages.json"
        )
        failures = verify_packages(SCRIPTS_DIR.parent, packages)
        self.assertEqual(failures, [])

    def test_pending_gd_package_is_registered_but_not_ready(self):
        """GD32 is visible without falsely advertising a buildable port."""

        packages = load_packages(
            SCRIPTS_DIR / "yi_vendor_packages.json"
        )
        gd_package = next(
            package
            for package in packages
            if package["family"] == "gd32f30x"
        )
        self.assertEqual(gd_package["status"], "pending")
        self.assertEqual(gd_package["version"], "3.0.3")

    def test_pending_hpmicro_package_uses_external_module(self):
        """HPM SDK is registered at the workspace module boundary."""

        packages = load_packages(
            SCRIPTS_DIR / "yi_vendor_packages.json"
        )
        hpm_package = next(
            package
            for package in packages
            if package["family"] == "hpm5300"
        )
        self.assertEqual(hpm_package["status"], "pending")
        self.assertEqual(
            hpm_package["path"], "modules/hal/hpmicro/hpm_sdk"
        )

    def test_ready_package_reports_missing_files(self):
        """A ready declaration must fail verification when files are absent."""

        package = {
            "id": "test-sdk",
            "status": "ready",
            "path": "vendor/test",
            "required": ["include/device.h"],
        }
        with tempfile.TemporaryDirectory() as temporary:
            failures = verify_packages(Path(temporary), [package])
        self.assertEqual(
            failures, ["test-sdk: missing include/device.h"]
        )

    def test_registry_rejects_parent_path_escape(self):
        """Registry paths cannot escape the repository root."""

        package = {
            "id": "unsafe",
            "vendor": "test",
            "family": "test",
            "package": "test",
            "version": "1.0",
            "source": "https://example.invalid",
            "path": "../outside",
            "status": "pending",
            "required": ["device.h"],
        }
        with tempfile.TemporaryDirectory() as temporary:
            registry = Path(temporary) / "registry.json"
            registry.write_text(json.dumps([package]), encoding="utf-8")
            with self.assertRaisesRegex(
                VendorPackageError, "stay below"
            ):
                load_packages(registry)


if __name__ == "__main__":
    unittest.main()
