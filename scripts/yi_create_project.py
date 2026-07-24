#!/usr/bin/env python3
"""Create a buildable YiCore application from a supported MCU target."""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import sys
from pathlib import Path
from typing import Any

from yi_dts_gen import generate


_NAME_RE = re.compile(r"^[A-Za-z][A-Za-z0-9_-]{0,63}$")
_TEMPLATE_NAME = "stm32f103-dts-demo"
_TARGETS_FILE = "yi_supported_targets.json"
_REQUIRED_TARGET_FIELDS = (
    "vendor",
    "vendor_name",
    "series",
    "model",
    "board",
    "template",
    "description",
)


class ProjectCreationError(ValueError):
    """Raised when a project cannot be created safely."""


def _validate_name(value: str, description: str) -> str:
    if not _NAME_RE.fullmatch(value):
        raise ProjectCreationError(
            f"{description} must start with a letter and contain only "
            "letters, digits, '-' or '_' (maximum 64 characters)"
        )
    return value


def _relative_path(source_dir: Path, target: Path, separator: str) -> str:
    try:
        relative = os.path.relpath(target, source_dir)
    except ValueError:
        # Windows cannot express a relative path across drive letters. Keil
        # and the DTS parser both accept absolute paths in that case.
        relative = str(target.resolve())
    if separator == "/":
        return Path(relative).as_posix()
    return relative.replace("/", "\\")


def _project_readme(
    name: str, board: str, target: dict[str, str] | None = None
) -> str:
    if target is None:
        target_summary = f"Board: `{board}`"
    else:
        target_summary = (
            f"Vendor: `{target['vendor_name']}`\n"
            f"MCU: `{target['model']}` (`{target['series']}`)\n"
            f"Board: `{board}`"
        )

    return f"""# {name}

YiCore application target:

{target_summary}

## Build

1. Install Python 3.9 or newer and Keil MDK-ARM with STM32F1 support.
2. Open `MDK-ARM/{name}.uvprojx`.
3. Build the `{name}` target.

The Keil pre-build command generates this application's DeviceTree sources in
`generated/`. Edit `app.dts` to select devices exposed by the board.

The project references shared YiCore sources, the STM32F1 SoC backend, CMSIS,
and STM32Cube HAL from the repository root. Do not copy vendor libraries into
this application.
"""


def load_supported_targets(repo_root: Path) -> list[dict[str, str]]:
    """Load the supported MCU target registry."""

    registry_path = repo_root / "scripts" / _TARGETS_FILE
    try:
        raw_targets = json.loads(registry_path.read_text(encoding="utf-8"))
    except OSError as error:
        raise ProjectCreationError(
            f"target registry not found: {registry_path}"
        ) from error
    except json.JSONDecodeError as error:
        raise ProjectCreationError(
            f"target registry is not valid JSON: {registry_path}"
        ) from error

    if not isinstance(raw_targets, list) or not raw_targets:
        raise ProjectCreationError("target registry must contain targets")

    targets: list[dict[str, str]] = []
    for index, raw_target in enumerate(raw_targets, 1):
        if not isinstance(raw_target, dict):
            raise ProjectCreationError(
                f"target registry entry {index} must be an object"
            )
        target: dict[str, str] = {}
        for field in _REQUIRED_TARGET_FIELDS:
            value = raw_target.get(field)
            if not isinstance(value, str) or not value:
                raise ProjectCreationError(
                    f"target registry entry {index} missing {field}"
                )
            target[field] = value

        _validate_name(target["vendor"], "vendor id")
        _validate_name(target["series"], "chip series")
        _validate_name(target["model"], "chip model")
        _validate_name(target["board"], "board name")
        _validate_name(target["template"], "template name")
        targets.append(target)

    return targets


def _select_option(
    title: str,
    options: list[Any],
    label,
    identity,
) -> Any:
    print(title)
    for index, option in enumerate(options, 1):
        print(f"  {index}. {label(option)}")

    while True:
        answer = input("Select [1]: ").strip()
        if not answer:
            return options[0]
        if answer.isdigit():
            selected = int(answer)
            if 1 <= selected <= len(options):
                return options[selected - 1]
        for option in options:
            if answer.lower() == identity(option).lower():
                return option
        print("Invalid selection, please enter a number or id.")


def select_target_interactive(
    targets: list[dict[str, str]]
) -> dict[str, str]:
    """Prompt for vendor first, then MCU series/model."""

    vendors: list[dict[str, str]] = []
    for target in targets:
        if not any(vendor["vendor"] == target["vendor"] for vendor in vendors):
            vendors.append(target)

    vendor = _select_option(
        "Supported vendors:",
        vendors,
        lambda item: f"{item['vendor_name']} ({item['vendor']})",
        lambda item: item["vendor"],
    )
    vendor_targets = [
        target for target in targets if target["vendor"] == vendor["vendor"]
    ]

    return _select_option(
        f"Supported {vendor['vendor_name']} chip series/models:",
        vendor_targets,
        lambda item: (
            f"{item['model']} ({item['series']}, board: {item['board']})"
        ),
        lambda item: item["model"],
    )


def resolve_target(
    targets: list[dict[str, str]],
    vendor: str | None = None,
    series: str | None = None,
    model: str | None = None,
) -> dict[str, str]:
    """Resolve a target from optional vendor, series, and model filters."""

    matches = targets
    if vendor:
        vendor = _validate_name(vendor, "vendor id").lower()
        matches = [
            target for target in matches if target["vendor"].lower() == vendor
        ]
    if series:
        series = _validate_name(series, "chip series").lower()
        matches = [
            target for target in matches if target["series"].lower() == series
        ]
    if model:
        model = _validate_name(model, "chip model").lower()
        matches = [
            target for target in matches if target["model"].lower() == model
        ]

    if not matches:
        raise ProjectCreationError("no supported target matches selection")
    if len(matches) > 1:
        choices = ", ".join(
            f"{target['vendor']}/{target['series']}/{target['model']}"
            for target in matches
        )
        raise ProjectCreationError(
            f"target selection is ambiguous: {choices}"
        )
    return matches[0]


def create_project(
    repo_root: Path,
    name: str,
    board: str = "fire-mini-stm32f103",
    output_root: Path | None = None,
    target: dict[str, str] | None = None,
) -> Path:
    """Create one project and return its absolute directory path."""

    repo_root = repo_root.resolve()
    name = _validate_name(name, "project name")
    board = _validate_name(board, "board name")
    template_name = target["template"] if target is not None else _TEMPLATE_NAME
    template = repo_root / "examples" / template_name
    board_dts = repo_root / "boards" / board / "board.dts"
    destination_root = (
        output_root.resolve()
        if output_root is not None
        else repo_root / "applications"
    )
    destination = destination_root / name

    if not template.is_dir():
        raise ProjectCreationError(f"template not found: {template}")
    if not board_dts.is_file():
        raise ProjectCreationError(f"board not found: {board_dts}")
    if destination.exists():
        raise ProjectCreationError(f"destination already exists: {destination}")

    try:
        destination.mkdir(parents=True)
        shutil.copytree(template / "Core", destination / "Core")

        mdk_dir = destination / "MDK-ARM"
        mdk_dir.mkdir()
        for filename in ("startup_stm32f103xe.s", "EventRecorderStub.scvd"):
            shutil.copy2(template / "MDK-ARM" / filename, mdk_dir / filename)

        board_include = _relative_path(destination, board_dts, "/")
        template_dts = (template / "app.dts").read_text(encoding="utf-8")
        first_line_end = template_dts.find("\n")
        app_body = template_dts[first_line_end + 1 :]
        app_dts = f'/include/ "{board_include}"\n{app_body}'
        (destination / "app.dts").write_text(
            app_dts, encoding="utf-8", newline="\n"
        )

        project_template = (
            template / "MDK-ARM" / f"{template_name}.uvprojx"
        ).read_text(encoding="utf-8")
        project_text = project_template.replace(template_name, name)

        # Generated files are application-local so parallel projects do not
        # overwrite each other's DeviceTree output.
        project_text = project_text.replace(
            "../../../generated", "../generated"
        ).replace(
            "..\\..\\..\\generated", "..\\generated"
        )

        root_from_mdk_posix = _relative_path(mdk_dir, repo_root, "/")
        root_from_mdk_windows = _relative_path(mdk_dir, repo_root, "\\")
        project_text = project_text.replace(
            "../../..", root_from_mdk_posix
        ).replace(
            "..\\..\\..", root_from_mdk_windows
        )
        (mdk_dir / f"{name}.uvprojx").write_text(
            project_text, encoding="utf-8", newline="\n"
        )

        ioc_text = (template / f"{template_name}.ioc").read_text(
            encoding="utf-8"
        ).replace(template_name, name)
        (destination / f"{name}.ioc").write_text(
            ioc_text, encoding="utf-8", newline="\n"
        )
        (destination / "README.md").write_text(
            _project_readme(name, board, target),
            encoding="utf-8",
            newline="\n",
        )

        generate(
            destination / "app.dts",
            repo_root / "dts" / "bindings",
            destination / "generated",
        )
    except Exception:
        if destination.exists():
            shutil.rmtree(destination)
        raise

    return destination


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Create a YiCore MCU application"
    )
    parser.add_argument(
        "name",
        nargs="?",
        help="project name, for example product-bootloader",
    )
    parser.add_argument(
        "--vendor",
        help="vendor id from scripts/yi_supported_targets.json",
    )
    parser.add_argument(
        "--series",
        help="chip series from scripts/yi_supported_targets.json",
    )
    parser.add_argument(
        "--model",
        help="chip model from scripts/yi_supported_targets.json",
    )
    parser.add_argument(
        "--board",
        help="board directory name below boards/; overrides target board",
    )
    parser.add_argument(
        "--output-root",
        type=Path,
        help="parent directory for the project (default: applications/)",
    )
    args = parser.parse_args()
    repo_root = Path(__file__).resolve().parent.parent

    try:
        if args.name is None:
            if not sys.stdin.isatty():
                parser.error("the following arguments are required: name")
            args.name = input("Project name: ").strip()

        target = None
        board = args.board
        if args.vendor or args.series or args.model:
            target = resolve_target(
                load_supported_targets(repo_root),
                args.vendor,
                args.series,
                args.model,
            )
            board = board or target["board"]
        elif board is None:
            targets = load_supported_targets(repo_root)
            target = (
                select_target_interactive(targets)
                if sys.stdin.isatty()
                else resolve_target(targets)
            )
            board = target["board"]

        destination = create_project(
            repo_root,
            args.name,
            board,
            args.output_root,
            target,
        )
    except (OSError, ProjectCreationError, ValueError) as error:
        parser.error(str(error))

    print(destination)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
