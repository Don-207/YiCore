#!/usr/bin/env python3
"""Validate YiCore architecture, vendor package and SoC backend declarations.

Author: Don
Date: 2026-07-28
Version: 1.0.0
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

from yi_vendor_verify import load_packages


class PlatformRegistryError(ValueError):
    """Report an invalid or inconsistent platform declaration."""


_REQUIRED_FIELDS = (
    "id",
    "arch",
    "vendor",
    "family",
    "backend",
    "vendor_package",
    "status",
)
_VALID_STATUS = {"reserved", "ready"}


def load_platforms(registry: Path) -> list[dict[str, str]]:
    """Load and validate platform registry syntax and safe paths.

    Args:
        registry: Platform JSON registry.
    Returns:
        Validated platform records.
    Raises:
        PlatformRegistryError: A record is malformed or duplicated.
    """

    try:
        raw_platforms: Any = json.loads(
            registry.read_text(encoding="utf-8")
        )
    except (OSError, json.JSONDecodeError) as error:
        raise PlatformRegistryError(
            f"cannot load platform registry: {registry}"
        ) from error
    if not isinstance(raw_platforms, list) or not raw_platforms:
        raise PlatformRegistryError(
            "platform registry must contain at least one platform"
        )

    platforms: list[dict[str, str]] = []
    seen_ids: set[str] = set()
    for index, raw_platform in enumerate(raw_platforms, 1):
        if not isinstance(raw_platform, dict):
            raise PlatformRegistryError(
                f"platform {index} must be an object"
            )
        for field in _REQUIRED_FIELDS:
            value = raw_platform.get(field)
            if not isinstance(value, str) or not value:
                raise PlatformRegistryError(
                    f"platform {index} missing {field}"
                )
        platform = {
            field: raw_platform[field] for field in _REQUIRED_FIELDS
        }
        if platform["id"] in seen_ids:
            raise PlatformRegistryError(
                f"duplicate platform id: {platform['id']}"
            )
        seen_ids.add(platform["id"])
        if platform["status"] not in _VALID_STATUS:
            raise PlatformRegistryError(
                f"{platform['id']} has invalid status"
            )
        backend = Path(platform["backend"])
        if backend.is_absolute() or ".." in backend.parts:
            raise PlatformRegistryError(
                f"{platform['id']} backend must stay below the repository"
            )
        platforms.append(platform)
    return platforms


def verify_platforms(
    repo_root: Path,
    platforms: list[dict[str, str]],
    packages: list[dict[str, Any]],
) -> list[str]:
    """Check platform backends and their vendor-package readiness.

    Args:
        repo_root: YiCore repository root.
        platforms: Validated platform declarations.
        packages: Validated vendor package declarations.
    Returns:
        Human-readable consistency failures.
    Side effects:
        Reads backend directories only.
    """

    failures: list[str] = []
    package_by_id = {package["id"]: package for package in packages}
    for platform in platforms:
        package = package_by_id.get(platform["vendor_package"])
        if package is None:
            failures.append(
                f"{platform['id']}: unknown vendor package "
                f"{platform['vendor_package']}"
            )
            continue
        backend = repo_root / platform["backend"]
        if not backend.is_dir():
            failures.append(
                f"{platform['id']}: missing backend {platform['backend']}"
            )
        if platform["status"] == "ready" and package["status"] != "ready":
            failures.append(
                f"{platform['id']}: ready platform requires ready package"
            )
        adapter = (
            repo_root
            / "cmake"
            / "platforms"
            / f"{platform['vendor']}-{platform['family']}.cmake"
        )
        if platform["status"] == "ready" and not adapter.is_file():
            failures.append(
                f"{platform['id']}: missing build adapter "
                f"{adapter.name}"
            )
    return failures


def main() -> int:
    """Validate all declared YiCore platform integration boundaries."""

    parser = argparse.ArgumentParser(
        description="Validate YiCore platform declarations"
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="YiCore repository root",
    )
    args = parser.parse_args()
    repo_root = args.repo_root.resolve()

    try:
        platforms = load_platforms(
            repo_root / "scripts" / "yi_platforms.json"
        )
        packages = load_packages(
            repo_root / "scripts" / "yi_vendor_packages.json"
        )
        failures = verify_platforms(repo_root, platforms, packages)
    except (PlatformRegistryError, ValueError) as error:
        parser.error(str(error))

    for platform in platforms:
        print(
            f"{platform['id']}: {platform['status']} "
            f"({platform['arch']}/{platform['vendor']}/"
            f"{platform['family']})"
        )
    if failures:
        for failure in failures:
            print(f"ERROR: {failure}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
