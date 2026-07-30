#!/usr/bin/env python3
"""Provide the unified YiCore command-line interface.

Author: Don
Date: 2026-07-28
Version: 2.0.0
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

from yi_create_app import AppCreationError, create_app
from yi_create_board import BoardCreationError, create_board
from yi_create_product import add_image, create_application_in_place
from yi_create_project import (
    ProjectCreationError,
    load_supported_boards,
    load_supported_targets,
    resolve_board,
    resolve_target,
    select_board_any_interactive,
    select_board_interactive,
    select_target_interactive,
)
from yi_manifest import (
    ManifestError,
    freeze_manifest,
    load_workspace_manifest,
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


def _run_west(
    workspace: Path,
    arguments: list[str],
    capture_output: bool = False,
) -> subprocess.CompletedProcess[str]:
    """Run west from its Python module, initializing a local workspace first.

    Args:
        workspace: Product repository containing west.yml.
        arguments: West arguments after the executable name.
        capture_output: Capture standard output for manifest generation.
    Returns:
        Completed west process.
    Side effects:
        May create west metadata beside the product and update repositories.
    Raises:
        YiCliError: west is unavailable, initialization fails, or west fails.
    """

    west = [sys.executable, "-m", "west"]
    try:
        topdir = subprocess.run(
            [*west, "topdir"],
            cwd=workspace,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )
        if topdir.returncode != 0:
            subprocess.run(
                [*west, "init", "-l", "."],
                cwd=workspace,
                check=True,
            )
        return subprocess.run(
            [*west, *arguments],
            cwd=workspace,
            check=True,
            text=True,
            stdout=subprocess.PIPE if capture_output else None,
        )
    except (OSError, subprocess.CalledProcessError) as error:
        raise YiCliError(
            "west failed; install it with 'python -m pip install west'"
        ) from error


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


def build_product(
    product_root: Path,
    image: str = "application",
    build_dir: Path | None = None,
    pristine: str = "auto",
    generator: str = "Ninja",
) -> Path:
    """Configure and build one image from a standard YiCore product.

    Args:
        product_root: Product repository containing firmware/projects/gcc.
        image: Firmware image name: application, bootloader, or test.
        build_dir: Optional out-of-tree build directory.
        pristine: One of auto, always, or never.
        generator: CMake generator name.
    Returns:
        Absolute build directory containing the selected image artifacts.
    Side effects:
        May remove the selected build directory and invokes CMake twice.
    Raises:
        YiCliError: Product layout, image, generator, or build is invalid.
    """

    product_root = product_root.resolve()
    if image not in {"application", "bootloader", "test"}:
        raise YiCliError(f"invalid product image: {image}")
    if pristine not in {"auto", "always", "never"}:
        raise YiCliError(f"invalid pristine mode: {pristine}")

    source_dir = product_root / "firmware" / "projects" / "gcc"
    toolchain = source_dir / "arm-none-eabi.cmake"
    image_root = product_root / "firmware" / "images" / image
    if not (source_dir / "CMakeLists.txt").is_file():
        raise YiCliError(f"not a YiCore product: {product_root}")
    if not toolchain.is_file():
        raise YiCliError(f"product GCC toolchain not found: {toolchain}")
    if not image_root.is_dir():
        raise YiCliError(
            f"product image {image!r} does not exist; "
            f"run 'yi image add {image}' first"
        )

    resolved_build = (
        build_dir.resolve()
        if build_dir is not None
        else product_root / "build" / image
    )
    if pristine == "always" and resolved_build.exists():
        shutil.rmtree(resolved_build)

    configure = [
        "cmake",
        "-S",
        str(source_dir),
        "-B",
        str(resolved_build),
        "-G",
        generator,
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
        f"-DYI_PRODUCT_IMAGE={image}",
        f"-DYIECG_IMAGE={image}",
    ]
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
        raise YiCliError(f"product {image} build failed") from error
    return resolved_build


def _create_parser() -> argparse.ArgumentParser:
    """Create the root parser and its Zephyr-style subcommands."""

    parser = argparse.ArgumentParser(prog="yi")
    commands = parser.add_subparsers(dest="command", required=True)

    commands.add_parser("boards", help="list supported build boards")

    board_parser = commands.add_parser("board", help="board operations")
    board_commands = board_parser.add_subparsers(
        dest="board_command", required=True
    )
    board_create = board_commands.add_parser(
        "create", help="create a product-local board"
    )
    board_create.add_argument("board_id", nargs="?")
    board_create.add_argument("--display-name")
    board_create.add_argument("--description")
    board_create.add_argument("--vendor")
    board_create.add_argument("--series")
    board_create.add_argument("--model")
    board_create.add_argument("--from-board")
    board_create.add_argument(
        "--output-root",
        type=Path,
        default=Path.cwd() / "boards",
    )

    product_parser = commands.add_parser(
        "product", help="product operations"
    )
    product_commands = product_parser.add_subparsers(
        dest="product_command", required=True
    )
    product_create = product_commands.add_parser(
        "create", help="create the application layout in a product root"
    )
    product_create.add_argument("--board")
    product_create.add_argument(
        "--product-root", type=Path, default=Path.cwd()
    )

    image_parser = commands.add_parser("image", help="firmware image operations")
    image_commands = image_parser.add_subparsers(
        dest="image_command", required=True
    )
    image_add = image_commands.add_parser(
        "add", help="add an optional product image"
    )
    image_add.add_argument("image", choices=("bootloader", "test"))
    image_add.add_argument(
        "--product-root", type=Path, default=Path.cwd()
    )

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
        help="legacy manifest mode: also update optional projects",
    )
    update_parser.add_argument(
        "--group-filter",
        action="append",
        default=[],
        help="temporary west groups, for example +bootloader,-debug",
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
        "build", help="configure and build a product or thin application"
    )
    build_parser.add_argument("source_dir", type=Path, nargs="?")
    build_parser.add_argument("-b", "--board")
    build_parser.add_argument(
        "--image",
        choices=("application", "bootloader", "test"),
        default="application",
    )
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


def create_board_for_product(yicore_root: Path, args: argparse.Namespace) -> Path:
    """Create a product-local board through the unified command.

    Args:
        yicore_root: YiCore repository root containing reference boards.
        args: Parsed board-create arguments.
    Returns:
        Newly created board directory.
    Side effects:
        Copies and customizes a reference board below the output root.
    """

    targets = load_supported_targets(yicore_root)
    boards = load_supported_boards(yicore_root)
    interactive = args.board_id is None or args.from_board is None
    if interactive and not sys.stdin.isatty():
        raise YiCliError(
            "board create requires board_id and --from-board when stdin "
            "is not interactive"
        )

    if args.from_board is not None:
        source_board = resolve_board(boards, board_id=args.from_board)
        target = resolve_target(
            targets,
            args.vendor or source_board["vendor"],
            args.series or source_board["series"],
            args.model or source_board["model"],
        )
        resolve_board(boards, target, source_board["id"])
    else:
        target = (
            resolve_target(
                targets, args.vendor, args.series, args.model
            )
            if any((args.vendor, args.series, args.model))
            else select_target_interactive(targets)
        )
        source_board = select_board_interactive(boards, target)

    board_id = args.board_id
    if board_id is None:
        board_id = input("New board id: ").strip()
        if not board_id:
            raise YiCliError("board id must not be empty")

    display_name = args.display_name
    if display_name is None and interactive:
        answer = input(
            f"Display name [{board_id.replace('-', ' ').title()}]: "
        ).strip()
        display_name = answer or None

    description = args.description
    if description is None and interactive:
        answer = input("Description [automatic]: ").strip()
        description = answer or None

    destination = create_board(
        yicore_root,
        board_id,
        target,
        source_board,
        display_name,
        description,
        args.output_root,
    )
    board_dts = destination / "board.dts"
    board_dts.write_text(
        board_dts.read_text(encoding="utf-8").replace(
            "../../dts/", "../../YiCore/dts/"
        ),
        encoding="utf-8",
        newline="\n",
    )
    return destination


def create_product_in_place(
    yicore_root: Path, product_root: Path, board_id: str | None
) -> Path:
    """Create the standard firmware layout in an existing product root.

    Args:
        yicore_root: YiCore repository root.
        product_root: Existing independent product repository.
        board_id: Product-local board identifier, or None to prompt.
    Returns:
        Absolute product root.
    Side effects:
        Generates application, build, linker, and project files.
    """

    product_root = product_root.resolve()
    if not (product_root / "YiCore" / "scripts" / "yi_cli.py").is_file():
        raise YiCliError(
            f"YiCore west project not found below product root: {product_root}"
        )
    if board_id is None and not sys.stdin.isatty():
        raise YiCliError(
            "product create requires --board when stdin is not interactive"
        )
    board_root = product_root
    boards = load_supported_boards(board_root)
    if board_id is None:
        board_id = select_board_any_interactive(boards)["id"]
    board = resolve_board(boards, board_id=board_id)
    targets = load_supported_targets(yicore_root)
    target = resolve_target(
        targets, board["vendor"], board["series"], board["model"]
    )
    return create_application_in_place(
        yicore_root, product_root, board, target, board_root
    )


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
        if args.command == "board":
            result = create_board_for_product(yicore_root, args)
        elif args.command == "product":
            result = create_product_in_place(
                yicore_root, args.product_root, args.board
            )
        elif args.command == "image":
            result = add_image(args.product_root, args.image)
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
            if (workspace / "west.yml").is_file():
                west_arguments = ["update"]
                if args.group_filter:
                    west_arguments.extend(
                        ["--group-filter", ",".join(args.group_filter)]
                    )
                _run_west(workspace, west_arguments)
            else:
                manifest = load_workspace_manifest(
                    yicore_root / "yi-modules.yml",
                    manifest_path,
                )
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
            if (workspace / "west.yml").is_file():
                result = _run_west(
                    workspace,
                    ["manifest", "--freeze", "--active-only"],
                    capture_output=True,
                )
                args.output.resolve().write_text(
                    result.stdout, encoding="utf-8", newline="\n"
                )
            else:
                manifest = load_workspace_manifest(
                    yicore_root / "yi-modules.yml",
                    manifest_path,
                )
                document = freeze_manifest(workspace, manifest)
                write_manifest(document, args.output.resolve())
            print(args.output.resolve())
            return 0
        elif args.command == "app":
            result = create_app(args.name, args.output_root)
        elif args.command == "build":
            source_dir = args.source_dir or Path.cwd()
            product_cmake = (
                source_dir.resolve()
                / "firmware"
                / "projects"
                / "gcc"
                / "CMakeLists.txt"
            )
            if args.source_dir is None or product_cmake.is_file():
                result = build_product(
                    source_dir,
                    args.image,
                    args.build_dir,
                    args.pristine,
                    args.generator,
                )
            else:
                if args.board is None:
                    raise YiCliError(
                        "thin application build requires --board"
                    )
                result = build_app(
                    yicore_root,
                    source_dir,
                    args.board,
                    args.build_dir,
                    args.pristine,
                    args.toolchain,
                    args.generator,
                )
    except (
        AppCreationError,
        BoardCreationError,
        ManifestError,
        ProjectCreationError,
        YiCliError,
        OSError,
        ValueError,
    ) as error:
        parser.error(str(error))
    print(result)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
