"""Test test-manifest and multi-image workflow metadata.

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

from yi_sysbuild import SysbuildError, load_plan  # noqa: E402
from yi_test import discover_tests  # noqa: E402


class WorkflowMetadataTests(unittest.TestCase):
    """Verify Zephyr-style workflow manifests remain deterministic."""

    def test_twister_style_scenario_is_discovered(self):
        """Discover platform and build-only constraints from testcase.yaml."""

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            case = root / "tests" / "event"
            case.mkdir(parents=True)
            (case / "testcase.yaml").write_text(
                "tests:\n  core.event:\n    platform_allow: [yi-hpm5301]\n"
                "    build_only: true\n    tags: [core, event]\n",
                encoding="utf-8",
            )
            scenarios = discover_tests(root)
        self.assertEqual(scenarios[0]["name"], "core.event")
        self.assertTrue(scenarios[0]["build_only"])

    def test_sysbuild_orders_bootloader_before_application(self):
        """Honor explicit image dependencies without an RTOS coordinator."""

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for image in ("bootloader", "application"):
                (root / "firmware" / "images" / image).mkdir(parents=True)
            (root / "sysbuild.yml").write_text(
                "images:\n  bootloader: {}\n"
                "  application:\n    depends_on: bootloader\n",
                encoding="utf-8",
            )
            plan = load_plan(root)
        self.assertEqual([item["name"] for item in plan], ["bootloader", "application"])

    def test_sysbuild_rejects_missing_image_directory(self):
        """Never claim an image exists only because it is named in YAML."""

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "sysbuild.yml").write_text(
                "images:\n  application: {}\n", encoding="utf-8"
            )
            with self.assertRaisesRegex(SysbuildError, "image directory"):
                load_plan(root)


if __name__ == "__main__":
    unittest.main()
