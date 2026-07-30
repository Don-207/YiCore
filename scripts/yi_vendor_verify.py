#!/usr/bin/env python3
"""Validate YiCore vendor package declarations and imported files.

Author: Don
Date: 2026-07-28
Version: 1.0.0
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


class VendorPackageError(ValueError):
    """Report an invalid vendor declaration or incomplete ready package."""


_REQUIRED_FIELDS = (
    "id",
    "vendor",
    "family",
    "package",
    "version",
    "source",
    "path",
    "status",
    "required",
)
_VALID_STATUS = {"pending", "ready"}


def load_packages(registry: Path) -> list[dict[str, Any]]:
    """Load and structurally validate the vendor package registry.

    Args:
        registry: JSON registry path.
    Returns:
        Validated package declarations in registry order.
    Raises:
        VendorPackageError: The registry is malformed or ambiguous.
    """

    try:
        packages = json.loads(registry.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise VendorPackageError(f"cannot load registry: {registry}") from error
    if not isinstance(packages, list) or not packages:
        raise VendorPackageError("registry must contain at least one package")

    seen_ids: set[str] = set()
    for index, package in enumerate(packages, 1):
        if not isinstance(package, dict):
            raise VendorPackageError(f"package {index} must be an object")
        for field in _REQUIRED_FIELDS:
            if field not in package:
                raise VendorPackageError(
                    f"package {index} missing required field: {field}"
                )
        package_id = package["id"]
        if not isinstance(package_id, str) or not package_id:
            raise VendorPackageError(f"package {index} has invalid id")
        if package_id in seen_ids:
            raise VendorPackageError(f"duplicate package id: {package_id}")
        seen_ids.add(package_id)
        if package["status"] not in _VALID_STATUS:
            raise VendorPackageError(
                f"{package_id} has invalid status: {package['status']}"
            )
        required = package["required"]
        if (
            not isinstance(required, list)
            or not required
            or not all(isinstance(item, str) and item for item in required)
        ):
            raise VendorPackageError(
                f"{package_id} must declare required files"
            )
        package_path = Path(package["path"])
        if package_path.is_absolute() or ".." in package_path.parts:
            raise VendorPackageError(
                f"{package_id} path must stay below the repository"
            )
        for item in required:
            required_path = Path(item)
            if required_path.is_absolute() or ".." in required_path.parts:
                raise VendorPackageError(
                    f"{package_id} required path is unsafe: {item}"
                )
    return packages


def verify_packages(
    repo_root: Path,
    packages: list[dict[str, Any]],
    include_pending: bool = False,
) -> list[str]:
    """Check that declared vendor package files exist.

    Args:
        repo_root: YiCore repository root.
        packages: Validated package declarations.
        include_pending: Also check packages not yet declared ready.
    Returns:
        Human-readable missing-file diagnostics.
    Side effects:
        Reads only paths declared by the registry.
    """

    failures: list[str] = []
    for package in packages:
        if package["status"] != "ready" and not include_pending:
            continue
        package_root = repo_root / package["path"]
        if package.get("scope") == "workspace":
            package_root = repo_root.parent / package["path"]
        for relative in package["required"]:
            if not (package_root / relative).is_file():
                failures.append(f"{package['id']}: missing {relative}")
    return failures


def main() -> int:
    """Validate the repository vendor packages and print their readiness."""

    parser = argparse.ArgumentParser(
        description="Validate YiCore vendor package imports"
    )
    parser.add_argument(
        "--registry",
        type=Path,
        default=Path(__file__).with_name("yi_vendor_packages.json"),
        help="vendor package registry",
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="YiCore repository root",
    )
    parser.add_argument(
        "--include-pending",
        action="store_true",
        help="fail when pending package files have not been imported",
    )
    args = parser.parse_args()

    try:
        packages = load_packages(args.registry)
        failures = verify_packages(
            args.repo_root.resolve(), packages, args.include_pending
        )
    except VendorPackageError as error:
        parser.error(str(error))

    for package in packages:
        print(
            f"{package['id']}: {package['status']} "
            f"({package['package']} {package['version']})"
        )
    if failures:
        for failure in failures:
            print(f"ERROR: {failure}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
