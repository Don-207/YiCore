#!/usr/bin/env python3
"""Provide the unified YiCore command-line interface.

Author: Don
Date: 2026-07-28
Version: 1.0.0
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
from pathlib import Path

from yi_create_app import AppCreationError, create_app
from yi_manifest import (
    ManifestError,
    freeze_manifest,
    load_manifest,
    update_workspace,
    write_manifest,
)
from yi_vendor_verify import load_packages, verify_packages


class YiCliError(ValueError):
    """Report an invalid YiCore command or unavailable build dependency."""


def _default_toolchain(yicore_root: Path, board: str) -> Path:
    """Resolve the built-in GCC toolchain for a supported board.

    Args:
        yicore_root: YiCore repository root.
        board: Board identifier selected by the user.
    Returns:
        Toolchain file for the board's registered MCU model.
    Raises:
        YiCliError: Board metadata or its toolchain is unavailable.
    """

    board_manifest = yicore_root / "boards" / board / "board.json"
    if not board_manifest.is_file():
        raise YiCliError(f"unknown board: {board}")
    try:
        board_data = json.loads(board_manifest.read_text(encoding="utf-8"))
        model = board_data["model"]
    except (OSError, json.JSONDecodeError, KeyError) as error:
        raise YiCliError(
            f"invalid board manifest: {board_manifest}"
        ) from error
    toolchain = (
        yicore_root
        / "scripts"
        / "templates"
        / model
        / "gcc"
        / "arm-none-eabi.cmake"
    )
    if not toolchain.is_file():
        raise YiCliError(f"board has no GCC toolchain: {board}")
    return toolchain


def _find_ninja() -> Path | None:
    """Find Ninja on PATH or in common Windows tool installations."""

    executable = shutil.which("ninja")
    if executable:
        return Path(executable)
    candidates = (
        Path(r"C:\Qt\Tools\Ninja\ninja.exe"),
        Path(r"C:\Program Files\CMake\bin\ninja.exe"),
    )
    return next((path for path in candidates if path.is_file()), None)


def build_app(
    yicore_root: Path,
    source_dir: Path,
    board: str,
    build_dir: Path | None = None,
    pristine: str = "auto",
    toolchain: Path | None = None,
    generator: str = "Ninja",
) -> Path:
    """Configure and build a thin YiCore application.

    Args:
        yicore_root: YiCore repository root.
        source_dir: Thin application directory.
        board: Build-time board identifier.
        build_dir: Optional out-of-tree build directory.
        pristine: One of auto, always or never.
        toolchain: Optional explicit CMake toolchain file.
        generator: CMake generator name.
    Returns:
        Absolute build directory.
    Side effects:
        May remove the selected build directory for pristine builds and invokes
        CMake configure/build subprocesses.
    Raises:
        YiCliError: Inputs are invalid or CMake fails.
    """

    source_dir = source_dir.resolve()
    yicore_root = yicore_root.resolve()
    if not (source_dir / "CMakeLists.txt").is_file():
        raise YiCliError(f"not a YiCore application: {source_dir}")
    if pristine not in {"auto", "always", "never"}:
        raise YiCliError(f"invalid pristine mode: {pristine}")
    resolved_build = (
        build_dir.resolve()
        if build_dir is not None
        else source_dir / "build"
    )
    if pristine == "always" and resolved_build.exists():
        shutil.rmtree(resolved_build)

    resolved_toolchain = (
        toolchain.resolve()
        if toolchain is not None
        else _default_toolchain(yicore_root, board)
    )
    configure = [
        "cmake",
        "-S",
        str(source_dir),
        "-B",
        str(resolved_build),
        "-G",
        generator,
        f"-DYICORE_ROOT={yicore_root}",
        f"-DBOARD={board}",
        f"-DCMAKE_TOOLCHAIN_FILE={resolved_toolchain}",
    ]
    workspace_root = yicore_root.parent
    external_st = workspace_root / "modules" / "hal" / "st"
    external_cmsis = workspace_root / "modules" / "lib" / "cmsis"
    if external_st.is_dir():
        configure.append(f"-DYI_HAL_ST_ROOT={external_st}")
    if (external_cmsis / "Include").is_dir():
        configure.append(f"-DYI_CMSIS_ROOT={external_cmsis}")
    elif (external_cmsis / "CMSIS" / "Core" / "Include").is_dir():
        configure.append(
            f"-DYI_CMSIS_ROOT={external_cmsis / 'CMSIS' / 'Core'}"
        )
    if generator == "Ninja":
        ninja = _find_ninja()
        if ninja is None:
            raise YiCliError(
                "Ninja not found; install it or select another generator"
            )
        configure.append(f"-DCMAKE_MAKE_PROGRAM={ninja}")

    try:
        subprocess.run(configure, check=True)
        subprocess.run(
            ["cmake", "--build", str(resolved_build)], check=True
        )
    except (OSError, subprocess.CalledProcessError) as error:
        raise YiCliError("application build failed") from error
    return resolved_build


def _create_parser() -> argparse.ArgumentParser:
    """Create the root parser and its Zephyr-style subcommands."""

    parser = argparse.ArgumentParser(prog="yi")
    commands = parser.add_subparsers(dest="command", required=True)

    commands.add_parser("boards", help="list supported build boards")

    sdk_parser = commands.add_parser("sdk", help="vendor SDK operations")
    sdk_commands = sdk_parser.add_subparsers(
        dest="sdk_command", required=True
    )
    sdk_commands.add_parser("list", help="list registered SDK packages")
    verify_parser = sdk_commands.add_parser(
        "verify", help="verify imported SDK packages"
    )
    verify_parser.add_argument(
        "--all",
        action="store_true",
        help="also require pending SDK packages",
    )

    update_parser = commands.add_parser(
        "update", help="update workspace projects from a manifest"
    )
    update_parser.add_argument(
        "-m",
        "--manifest",
        type=Path,
        default=Path.cwd() / "yi-manifest.yml",
    )
    update_parser.add_argument(
        "--workspace",
        type=Path,
        help="workspace root (default: manifest parent)",
    )
    update_parser.add_argument(
        "--all",
        action="store_true",
        help="also update optional projects",
    )

    manifest_parser = commands.add_parser(
        "manifest", help="workspace manifest operations"
    )
    manifest_commands = manifest_parser.add_subparsers(
        dest="manifest_command", required=True
    )
    freeze_parser = manifest_commands.add_parser(
        "freeze", help="write checked-out revisions as commit SHAs"
    )
    freeze_parser.add_argument(
        "-m",
        "--manifest",
        type=Path,
        default=Path.cwd() / "yi-manifest.yml",
    )
    freeze_parser.add_argument(
        "--workspace",
        type=Path,
        help="workspace root (default: manifest parent)",
    )
    freeze_parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=Path.cwd() / "yi-manifest.lock.yml",
    )

    app_parser = commands.add_parser("app", help="application operations")
    app_commands = app_parser.add_subparsers(
        dest="app_command", required=True
    )
    create_parser = app_commands.add_parser(
        "create", help="create a thin application"
    )
    create_parser.add_argument("name")
    create_parser.add_argument(
        "--output-root",
        type=Path,
        default=Path.cwd() / "applications",
    )

    build_parser = commands.add_parser(
        "build", help="configure and build an application"
    )
    build_parser.add_argument("source_dir", type=Path)
    build_parser.add_argument("-b", "--board", required=True)
    build_parser.add_argument("-d", "--build-dir", type=Path)
    build_parser.add_argument(
        "-p",
        "--pristine",
        nargs="?",
        const="always",
        default="auto",
        choices=("auto", "always", "never"),
    )
    build_parser.add_argument("--toolchain", type=Path)
    build_parser.add_argument("--generator", default="Ninja")
    return parser


def list_boards(yicore_root: Path) -> list[str]:
    """List board identifiers that have a ready platform adapter.

    Args:
        yicore_root: YiCore repository root.
    Returns:
        Sorted human-readable board summary lines.
    Side effects:
        Reads board manifests and platform adapter paths.
    """

    summaries: list[str] = []
    for manifest in sorted((yicore_root / "boards").glob("*/board.json")):
        try:
            board = json.loads(manifest.read_text(encoding="utf-8"))
            board_id = board["id"]
            board_name = board["name"]
            adapter = (
                yicore_root
                / "cmake"
                / "platforms"
                / f"{board['vendor']}-{board['series']}.cmake"
            )
        except (OSError, json.JSONDecodeError, KeyError):
            continue
        if adapter.is_file():
            summaries.append(f"{board_id}\t{board_name}")
    return summaries


def sdk_list(yicore_root: Path) -> list[str]:
    """List registered vendor SDK packages and readiness.

    Args:
        yicore_root: YiCore repository root.
    Returns:
        Registry summaries in declared order.
    """

    packages = load_packages(
        yicore_root / "scripts" / "yi_vendor_packages.json"
    )
    return [
        f"{package['id']}\t{package['status']}\t"
        f"{package['package']} {package['version']}"
        for package in packages
    ]


def sdk_verify(yicore_root: Path, include_pending: bool = False) -> list[str]:
    """Verify vendor SDK imports selected by their readiness state.

    Args:
        yicore_root: YiCore repository root.
        include_pending: Require files for pending packages too.
    Returns:
        Missing-file diagnostics; an empty list indicates success.
    """

    packages = load_packages(
        yicore_root / "scripts" / "yi_vendor_packages.json"
    )
    return verify_packages(yicore_root, packages, include_pending)


def main() -> int:
    """Dispatch the selected YiCore command."""

    parser = _create_parser()
    args = parser.parse_args()
    yicore_root = Path(__file__).resolve().parent.parent
    try:
        if args.command == "boards":
            for summary in list_boards(yicore_root):
                print(summary)
            return 0
        if args.command == "sdk":
            if args.sdk_command == "list":
                for summary in sdk_list(yicore_root):
                    print(summary)
                return 0
            failures = sdk_verify(yicore_root, args.all)
            if failures:
                raise YiCliError("\n".join(failures))
            print("vendor SDK verification passed")
            return 0
        if args.command == "update":
            manifest_path = args.manifest.resolve()
            workspace = (
                args.workspace.resolve()
                if args.workspace is not None
                else manifest_path.parent
            )
            manifest = load_manifest(manifest_path)
            for project_name in update_workspace(
                workspace, manifest, args.all
            ):
                print(f"updated {project_name}")
            return 0
        if args.command == "manifest":
            manifest_path = args.manifest.resolve()
            workspace = (
                args.workspace.resolve()
                if args.workspace is not None
                else manifest_path.parent
            )
            manifest = load_manifest(manifest_path)
            document = freeze_manifest(workspace, manifest)
            write_manifest(document, args.output.resolve())
            print(args.output.resolve())
            return 0
        if args.command == "app":
            result = create_app(args.name, args.output_root)
        else:
            result = build_app(
                yicore_root,
                args.source_dir,
                args.board,
                args.build_dir,
                args.pristine,
                args.toolchain,
                args.generator,
            )
    except (AppCreationError, ManifestError, YiCliError) as error:
        parser.error(str(error))
    print(result)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
