#!/usr/bin/env python3
"""Resolve YiCore chip environments to installed compiler directories.

Author: Don
Date: 2026-08-01
Version: 1.0.0
"""

from __future__ import annotations

import json
import os
from dataclasses import dataclass
from pathlib import Path


class ToolchainResolutionError(ValueError):
    """Report missing chip metadata or an unavailable compiler installation."""


@dataclass(frozen=True)
class BuildEnvironment:
    """Describe the compiler and CMake settings selected for one MCU model."""

    environment_id: str
    toolchain_id: str
    version: str
    root: Path
    bin_dir: Path
    compiler: Path
    process_environment: dict[str, str]
    cmake_adapter: str


def default_toolchain_home() -> Path:
    """Return the configured cross-toolchain installation root.

    Returns:
        Absolute root containing toolchain-id/version directories.
    Raises:
        ToolchainResolutionError: YI_TOOLCHAIN_HOME is not configured.
    """

    configured_home = os.environ.get("YI_TOOLCHAIN_HOME")
    if not configured_home:
        raise ToolchainResolutionError(
            "YI_TOOLCHAIN_HOME is not set; expected a directory such as "
            r"D:\toolchains\Yi"
        )
    return Path(configured_home).expanduser().resolve()


def load_chip_environment(
    yicore_root: Path, vendor: str, model: str
) -> dict:
    """Load the declarative compiler environment for an MCU model.

    Args:
        yicore_root: YiCore repository root.
        vendor: Canonical board vendor identifier.
        model: Canonical board MCU model identifier.
    Returns:
        Parsed environment manifest.
    Raises:
        ToolchainResolutionError: The environment is absent or malformed.
    """

    manifest_path = yicore_root / "environments" / vendor / f"{model}.json"
    if not manifest_path.is_file():
        raise ToolchainResolutionError(
            f"no build environment for {vendor}/{model}: {manifest_path}"
        )
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        toolchain = manifest["toolchain"]
        for required_key in ("id", "version", "compiler_prefix"):
            toolchain[required_key]
        manifest["id"]
        manifest["cmake"]["adapter"]
    except (OSError, json.JSONDecodeError, KeyError) as error:
        raise ToolchainResolutionError(
            f"invalid build environment: {manifest_path}"
        ) from error
    return manifest


def resolve_build_environment(
    yicore_root: Path,
    board: dict,
    toolchain_home: Path | None = None,
) -> BuildEnvironment:
    """Resolve and validate the compiler selected by board metadata.

    Args:
        yicore_root: YiCore repository root.
        board: Parsed board.json object containing vendor and model.
        toolchain_home: Optional explicit installation root for tests or tools.
    Returns:
        Validated process and CMake environment.
    Raises:
        ToolchainResolutionError: Board identity or compiler files are missing.
    """

    try:
        vendor = board["vendor"]
        model = board["model"]
    except KeyError as error:
        raise ToolchainResolutionError(
            "board manifest must contain vendor and model"
        ) from error
    manifest = load_chip_environment(yicore_root, vendor, model)
    toolchain = manifest["toolchain"]
    resolved_home = (
        toolchain_home.resolve()
        if toolchain_home is not None
        else default_toolchain_home()
    )
    toolchain_root = resolved_home / toolchain["id"] / toolchain["version"]
    compiler_suffix = ".exe" if os.name == "nt" else ""
    bin_dir = toolchain_root / "bin"
    compiler = bin_dir / f"{toolchain['compiler_prefix']}-gcc{compiler_suffix}"
    if not compiler.is_file():
        raise ToolchainResolutionError(
            f"compiler not found for {vendor}/{model}: {compiler}"
        )
    process_environment = os.environ.copy()
    for variable_name, location in manifest.get("environment", {}).items():
        if location != "root":
            raise ToolchainResolutionError(
                f"unsupported environment location {location!r} in "
                f"{manifest['id']}"
            )
        process_environment[variable_name] = str(toolchain_root)
    return BuildEnvironment(
        environment_id=manifest["id"],
        toolchain_id=toolchain["id"],
        version=toolchain["version"],
        root=toolchain_root,
        bin_dir=bin_dir,
        compiler=compiler,
        process_environment=process_environment,
        cmake_adapter=manifest["cmake"]["adapter"],
    )
