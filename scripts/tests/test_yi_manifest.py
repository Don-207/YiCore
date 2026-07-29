"""Test reproducible YiCore workspace manifests.

Author: Don
Date: 2026-07-28
Version: 1.0.0
"""

import subprocess
import sys
import tempfile
import unittest
from unittest import mock
from pathlib import Path


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_manifest import (  # noqa: E402
    ManifestError,
    freeze_manifest,
    load_manifest,
    update_workspace,
    write_manifest,
)


class ManifestTests(unittest.TestCase):
    """Verify local Git workspace update and freeze behavior."""

    def _create_origin(self, root: Path) -> tuple[Path, str]:
        """Create a local one-commit Git origin for deterministic tests."""

        origin = root / "origin"
        origin.mkdir()
        subprocess.run(["git", "init", "-q"], cwd=origin, check=True)
        subprocess.run(
            ["git", "config", "user.email", "test@example.com"],
            cwd=origin,
            check=True,
        )
        subprocess.run(
            ["git", "config", "user.name", "Yi Test"],
            cwd=origin,
            check=True,
        )
        (origin / "README.md").write_text("test\n", encoding="utf-8")
        subprocess.run(["git", "add", "README.md"], cwd=origin, check=True)
        subprocess.run(
            ["git", "commit", "-q", "-m", "initial"],
            cwd=origin,
            check=True,
        )
        revision = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=origin,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
        ).stdout.strip()
        return origin, revision

    def test_update_and_freeze_local_project(self):
        """A manifest clones an exact revision and freezes the same SHA."""

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            origin, revision = self._create_origin(root)
            manifest_path = root / "yi-manifest.yml"
            manifest_path.write_text(
                "manifest:\n"
                '  version: "1.0"\n'
                "  projects:\n"
                "    - name: local\n"
                f"      url: {origin.as_posix()}\n"
                f"      revision: {revision}\n"
                "      path: modules/local\n",
                encoding="utf-8",
            )
            manifest = load_manifest(manifest_path)
            self.assertEqual(update_workspace(root, manifest), ["local"])
            frozen = freeze_manifest(root, manifest)
            self.assertEqual(
                frozen["manifest"]["projects"][0]["revision"], revision
            )
            lock = root / "yi-manifest.lock.yml"
            write_manifest(frozen, lock)
            self.assertEqual(
                load_manifest(lock)["projects"][0]["revision"], revision
            )

    def test_update_refuses_dirty_project(self):
        """Workspace updates preserve uncommitted user changes."""

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            origin, revision = self._create_origin(root)
            manifest = {
                "projects": [
                    {
                        "name": "local",
                        "url": origin.as_posix(),
                        "revision": revision,
                        "path": "modules/local",
                    }
                ]
            }
            update_workspace(root, manifest)
            checkout = root / "modules" / "local"
            (checkout / "README.md").write_text(
                "dirty\n", encoding="utf-8"
            )
            with self.assertRaisesRegex(
                ManifestError, "dirty project"
            ):
                update_workspace(root, manifest)

    def test_update_initializes_nested_submodules(self):
        """A checked-out manifest project initializes recursive submodules."""

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            origin, revision = self._create_origin(root)
            manifest = {
                "projects": [
                    {
                        "name": "local",
                        "url": origin.as_posix(),
                        "revision": revision,
                        "path": "modules/local",
                    }
                ]
            }
            real_run = subprocess.run
            with mock.patch(
                "yi_manifest.subprocess.run", wraps=real_run
            ) as run:
                update_workspace(root, manifest)
            run.assert_any_call(
                [
                    "git",
                    "submodule",
                    "update",
                    "--init",
                    "--recursive",
                ],
                cwd=root / "modules" / "local",
                check=True,
            )

    def test_optional_project_is_skipped_by_default(self):
        """Optional SDK repositories require explicit all-project updates."""

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            manifest = {
                "projects": [
                    {
                        "name": "optional",
                        "url": "unused",
                        "revision": "unused",
                        "path": "modules/optional",
                        "optional": True,
                    }
                ]
            }
            self.assertEqual(update_workspace(root, manifest), [])
            self.assertFalse((root / "modules" / "optional").exists())


if __name__ == "__main__":
    unittest.main()
