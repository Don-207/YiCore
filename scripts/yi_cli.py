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
from yi_vendor_verify import load_packages, verify_packages
from yi_kconfig import (
    ConfigurationError,
    discover_fragments,
    generate_configuration,
)
from yi_toolchain import (
    BuildEnvironment,
    ToolchainResolutionError,
    resolve_build_environment,
)


class YiCliError(ValueError):
    """Report an invalid YiCore command or unavailable build dependency."""


def _load_board_manifest(board_root: Path, board_id: str) -> dict:
    """Load one board manifest from a repository board directory.

    Args:
        board_root: Repository containing boards/<board-id>/board.json.
        board_id: Board identifier to load.
    Returns:
        Parsed board manifest.
    Raises:
        YiCliError: The selected board manifest is absent or malformed.
    """

    manifest_path = board_root / "boards" / board_id / "board.json"
    try:
        return json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise YiCliError(f"invalid board manifest: {manifest_path}") from error


def _select_product_board(product_root: Path, board_id: str | None) -> dict:
    """Select the explicit or sole board declared by a product.

    Args:
        product_root: Product repository root.
        board_id: Optional explicit board identifier.
    Returns:
        Parsed product-local board manifest.
    Raises:
        YiCliError: No board or multiple implicit candidates are present.
    """

    if board_id is not None:
        return _load_board_manifest(product_root, board_id)
    manifests = sorted((product_root / "boards").glob("*/board.json"))
    if len(manifests) != 1:
        raise YiCliError(
            "product build requires --board when it does not contain exactly "
            "one board manifest"
        )
    try:
        return json.loads(manifests[0].read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise YiCliError(f"invalid board manifest: {manifests[0]}") from error


def _workspace_yicore(product_root: Path) -> Path:
    """Locate YiCore in either an in-product or sibling west layout."""

    candidates = (product_root / "YiCore", product_root.parent / "YiCore")
    for candidate in candidates:
        if (candidate / "environments").is_dir():
            return candidate.resolve()
    raise YiCliError(f"YiCore west project not found for {product_root}")


def _append_environment_cmake(
    configure: list[str],
    selected: BuildEnvironment,
    yicore_root: Path,
    product_toolchain: Path | None = None,
) -> None:
    """Append compiler-root and toolchain-file arguments for an environment.

    Args:
        configure: Mutable CMake configure command.
        selected: Resolved chip/compiler environment.
        yicore_root: YiCore repository root.
        product_toolchain: Optional product-local CMake adapter.
    Side effects:
        Adds CMake cache arguments to configure.
    Raises:
        YiCliError: The selected CMake adapter is unavailable.
    """

    if selected.toolchain_id == "arm-gnu-toolchain":
        configure.append(f"-DARM_GCC_ROOT={selected.bin_dir}")
    if selected.cmake_adapter == "sdk":
        return
    if selected.cmake_adapter == "product":
        adapter = product_toolchain
    else:
        adapter = yicore_root / selected.cmake_adapter
    if adapter is None or not adapter.is_file():
        raise YiCliError(
            f"CMake toolchain adapter not found for "
            f"{selected.environment_id}: {adapter}"
        )
    configure.append(f"-DCMAKE_TOOLCHAIN_FILE={adapter.resolve()}")


def _append_kconfig_cmake(
    configure: list[str],
    yicore_root: Path,
    build_dir: Path,
    config_root: Path,
    board_id: str | None,
) -> None:
    """Generate Kconfig outputs and expose them to the CMake build.

    Args:
        configure: Mutable CMake configure command.
        yicore_root: YiCore repository root containing Kconfig.
        build_dir: Selected application build directory.
        config_root: Directory containing prj.conf and optional board fragments.
        board_id: Optional board identifier for boards/<board>.conf.
    Side effects:
        Writes build/yicore configuration outputs and adds CMake arguments.
    Raises:
        YiCliError: Kconfig generation fails.
    """

    try:
        outputs = generate_configuration(
            yicore_root,
            build_dir,
            discover_fragments(config_root, board_id),
        )
    except ConfigurationError as error:
        raise YiCliError(str(error)) from error
    configure.extend(
        (
            f"-DYI_DOTCONFIG={outputs['dotconfig']}",
            f"-DYI_AUTOCONF_HEADER={outputs['autoconf']}",
            f"-DYI_GENERATED_INCLUDE_DIR={outputs['include']}",
            f"-DYI_KCONFIG_CMAKE={outputs['cmake']}",
        )
    )


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

    board_manifest = _load_board_manifest(yicore_root, board)
    try:
        selected_environment = resolve_build_environment(
            yicore_root, board_manifest
        )
    except ToolchainResolutionError as error:
        raise YiCliError(str(error)) from error
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
    ]
    if toolchain is not None:
        configure.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain.resolve()}")
        if selected_environment.toolchain_id == "arm-gnu-toolchain":
            configure.append(
                f"-DARM_GCC_ROOT={selected_environment.bin_dir}"
            )
    else:
        default_adapter = (
            _default_toolchain(yicore_root, board)
            if selected_environment.cmake_adapter == "product"
            else None
        )
        _append_environment_cmake(
            configure,
            selected_environment,
            yicore_root,
            default_adapter,
        )
    _append_kconfig_cmake(
        configure,
        yicore_root,
        resolved_build,
        source_dir,
        board,
    )
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
        subprocess.run(
            configure,
            check=True,
            env=selected_environment.process_environment,
        )
        subprocess.run(
            ["cmake", "--build", str(resolved_build)],
            check=True,
            env=selected_environment.process_environment,
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
    board: str | None = None,
) -> Path:
    """Configure and build one image from a standard YiCore product.

    Args:
        product_root: Product repository containing firmware/projects/gcc.
        image: Firmware image name: application, bootloader, or test.
        build_dir: Optional out-of-tree build directory.
        pristine: One of auto, always, or never.
        generator: CMake generator name.
        board: Optional product-local board identifier.
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
    image_root = product_root / "firmware" / "images" / image
    if not (source_dir / "CMakeLists.txt").is_file():
        raise YiCliError(f"not a YiCore product: {product_root}")
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

    yicore_root = _workspace_yicore(product_root)
    board_manifest = _select_product_board(product_root, board)
    try:
        selected_environment = resolve_build_environment(
            yicore_root, board_manifest
        )
    except ToolchainResolutionError as error:
        raise YiCliError(str(error)) from error
    configure = [
        "cmake",
        "-S",
        str(source_dir),
        "-B",
        str(resolved_build),
        "-G",
        generator,
        f"-DYI_PRODUCT_IMAGE={image}",
        f"-DYIECG_IMAGE={image}",
    ]
    _append_environment_cmake(
        configure,
        selected_environment,
        yicore_root,
        source_dir / "arm-none-eabi.cmake",
    )
    _append_kconfig_cmake(
        configure,
        yicore_root,
        resolved_build,
        image_root,
        board_manifest.get("id", board),
    )
    if generator == "Ninja":
        ninja = _find_ninja()
        if ninja is None:
            raise YiCliError(
                "Ninja not found; install it or select another generator"
            )
        configure.append(f"-DCMAKE_MAKE_PROGRAM={ninja}")

    try:
        subprocess.run(
            configure,
            check=True,
            env=selected_environment.process_environment,
        )
        subprocess.run(
            ["cmake", "--build", str(resolved_build)],
            check=True,
            env=selected_environment.process_environment,
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
        "update", help="update active west workspace projects"
    )
    update_parser.add_argument(
        "--workspace",
        type=Path,
        help="product root containing west.yml (default: current directory)",
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
        "--workspace",
        type=Path,
        help="product root containing west.yml (default: current directory)",
    )
    freeze_parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=Path.cwd() / "west.lock.yml",
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
        default=Path.cwd() / "projects",
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
            workspace = (
                args.workspace.resolve()
                if args.workspace is not None
                else Path.cwd().resolve()
            )
            if not (workspace / "west.yml").is_file():
                raise YiCliError(f"west.yml not found: {workspace}")
            west_arguments = ["update"]
            if args.group_filter:
                west_arguments.extend(
                    ["--group-filter", ",".join(args.group_filter)]
                )
            _run_west(workspace, west_arguments)
            return 0
        if args.command == "manifest":
            workspace = (
                args.workspace.resolve()
                if args.workspace is not None
                else Path.cwd().resolve()
            )
            if not (workspace / "west.yml").is_file():
                raise YiCliError(f"west.yml not found: {workspace}")
            result = _run_west(
                workspace,
                ["manifest", "--freeze", "--active-only"],
                capture_output=True,
            )
            args.output.resolve().write_text(
                result.stdout, encoding="utf-8", newline="\n"
            )
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
                    args.board,
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
