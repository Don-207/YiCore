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
    load_workspace_manifest,
    merge_manifests,
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

    def test_product_manifest_extends_and_overrides_framework(self):
        """Product dependencies extend standard modules by stable name."""

        framework = {
            "version": "1.0",
            "projects": [
                {
                    "name": "standard",
                    "url": "framework-url",
                    "revision": "framework-revision",
                    "path": "modules/lib/standard",
                }
            ],
        }
        product = {
            "version": "1.0",
            "projects": [
                {
                    "name": "standard",
                    "url": "product-url",
                    "revision": "product-revision",
                    "path": "modules/lib/standard",
                },
                {
                    "name": "private",
                    "url": "private-url",
                    "revision": "private-revision",
                    "path": "modules/lib/private",
                },
            ],
        }

        resolved = merge_manifests(framework, product)

        self.assertEqual(
            [project["name"] for project in resolved["projects"]],
            ["standard", "private"],
        )
        self.assertEqual(
            resolved["projects"][0]["revision"], "product-revision"
        )

    def test_framework_override_cannot_change_install_path(self):
        """A product cannot relocate a standard dependency implicitly."""

        framework = {
            "projects": [
                {
                    "name": "standard",
                    "url": "framework-url",
                    "revision": "framework-revision",
                    "path": "modules/lib/standard",
                }
            ]
        }
        product = {
            "projects": [
                {
                    "name": "standard",
                    "url": "product-url",
                    "revision": "product-revision",
                    "path": "modules/other/standard",
                }
            ]
        }

        with self.assertRaisesRegex(ManifestError, "must keep path"):
            merge_manifests(framework, product)

    def test_missing_product_manifest_uses_framework_modules(self):
        """A new workspace needs no product dependency declarations."""

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            framework_path = root / "yi-modules.yml"
            framework_path.write_text(
                "manifest:\n"
                '  version: "1.0"\n'
                "  projects:\n"
                "    - name: standard\n"
                "      url: framework-url\n"
                "      revision: framework-revision\n"
                "      path: modules/lib/standard\n",
                encoding="utf-8",
            )

            resolved = load_workspace_manifest(
                framework_path,
                root / "yi-manifest.yml",
            )

            self.assertEqual(
                [project["name"] for project in resolved["projects"]],
                ["standard"],
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

    def test_update_existing_project_runs_git_in_checkout(self):
        """Updating an existing module never checks out the workspace root."""

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
                        "submodules": False,
                    }
                ]
            }
            update_workspace(root, manifest)
            real_run = subprocess.run
            with mock.patch(
                "yi_manifest.subprocess.run", wraps=real_run
            ) as run:
                update_workspace(root, manifest)
            run.assert_any_call(
                [
                    "git",
                    "-C",
                    str(root / "modules" / "local"),
                    "checkout",
                    "--detach",
                    "FETCH_HEAD",
                ],
                check=True,
            )

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

    def test_update_can_skip_nested_submodules(self):
        """A manifest project may opt out of unnecessary nested checkouts."""

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
                        "submodules": False,
                    }
                ]
            }
            real_run = subprocess.run
            with mock.patch(
                "yi_manifest.subprocess.run", wraps=real_run
            ) as run:
                update_workspace(root, manifest)
            self.assertNotIn(
                mock.call(
                    [
                        "git",
                        "submodule",
                        "update",
                        "--init",
                        "--recursive",
                    ],
                    cwd=root / "modules" / "local",
                    check=True,
                ),
                run.mock_calls,
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
