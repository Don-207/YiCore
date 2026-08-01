#!/usr/bin/env python3
"""Create a buildable YiCore application from a supported MCU target.

Author: Don
Date: 2026-07-26
Version: 1.0.0
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any

from yi_build_info_gen import generate as generate_build_info
from yi_dts_bindings import load_bindings
from yi_dts_gen import generate
from yi_dts_parser import parse_file


_NAME_RE = re.compile(r"^[A-Za-z][A-Za-z0-9_-]{0,63}$")
_TEMPLATE_NAME = "stm32f103-dts-demo"
_TARGETS_FILE = "yi_supported_targets.json"
_REQUIRED_TARGET_FIELDS = (
    "vendor",
    "vendor_name",
    "series",
    "model",
    "template",
    "description",
)
_OPTIONAL_BOARD_STRING_FIELDS = (
    "part",
    "keil_device",
    "cube_cpn",
    "cube_name",
    "cube_package",
    "cube_user_name",
)
_OPTIONAL_BOARD_SIZE_FIELDS = (
    "flash_size",
    "ram_size",
)
_REQUIRED_BOARD_FIELDS = (
    "id",
    "name",
    "vendor",
    "series",
    "model",
    "description",
)

# Optional leaf drivers copied from the reference Keil project.  Platform,
# bus and common subsystem sources intentionally remain in every project;
# only standalone device drivers are selected from the resolved DeviceTree.
_OPTIONAL_DRIVER_PATHS = {
    "led": (
        "drivers/led/yi_led.c",
        "drivers/led",
    ),
    "w25q64": (
        "drivers/flash/w25q64/yi_w25q64.c",
        "drivers/flash/w25q64",
    ),
    "at24c02": (
        "drivers/eeprom/at24c02/yi_at24c02.c",
        "drivers/eeprom/at24c02",
    ),
    "ads7830": (
        "drivers/adc/ads7830/yi_ads7830.c",
        "drivers/adc/ads7830",
    ),
    "ads1298": (
        "drivers/adc/ads1298/yi_ads1298.c",
        "drivers/adc/ads1298",
    ),
    "max31856": (
        "drivers/sensor/max31856/yi_max31856.c",
        "drivers/sensor/max31856",
    ),
    "tsys01": (
        "drivers/sensor/tsys01/yi_tsys01.c",
        "drivers/sensor/tsys01",
    ),
}


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


def _normalize_project_path(value: str) -> str:
    return value.replace("\\", "/").lower().rstrip("/")


def _select_optional_drivers(
    dts: Path, bindings_dir: Path
) -> set[str]:
    tree = parse_file(dts)
    bindings = load_bindings(bindings_dir)
    selected: set[str] = set()

    # Keep every optional device type exposed by the selected board, including
    # nodes currently marked disabled.  Applications commonly enable those
    # nodes after project creation; pruning them here would make the next
    # DeviceTree pre-build generate headers for sources absent from uvprojx.
    for node in tree.root.walk():
        compatible_value = node.properties.get("compatible")
        if compatible_value is None:
            continue
        compatibles = (
            compatible_value
            if isinstance(compatible_value, list)
            else [compatible_value]
        )
        binding = next(
            (
                bindings[compatible]
                for compatible in compatibles
                if isinstance(compatible, str) and compatible in bindings
            ),
            None,
        )
        if (
            binding is not None
            and binding.driver in _OPTIONAL_DRIVER_PATHS
        ):
            selected.add(binding.driver)
    return selected


def _prune_optional_drivers(
    project_text: str, selected_drivers: set[str]
) -> str:
    """Remove unused leaf drivers and their private include directories."""

    root = ET.fromstring(project_text)
    unused = set(_OPTIONAL_DRIVER_PATHS) - selected_drivers
    unused_sources = {
        _normalize_project_path(_OPTIONAL_DRIVER_PATHS[name][0])
        for name in unused
    }
    unused_includes = {
        _normalize_project_path(_OPTIONAL_DRIVER_PATHS[name][1])
        for name in unused
    }

    for files in root.findall(".//Files"):
        for file_node in list(files.findall("File")):
            path_node = file_node.find("FilePath")
            if path_node is None or not path_node.text:
                continue
            normalized = _normalize_project_path(path_node.text)
            if any(normalized.endswith(path) for path in unused_sources):
                files.remove(file_node)

    for include_node in root.findall(".//IncludePath"):
        entries = (include_node.text or "").split(";")
        kept = [
            entry for entry in entries
            if not any(
                _normalize_project_path(entry).endswith(path)
                for path in unused_includes
            )
        ]
        include_node.text = ";".join(kept)

    ET.indent(root, space="  ")
    return (
        '<?xml version="1.0" encoding="UTF-8" standalone="no" ?>\n'
        + ET.tostring(root, encoding="unicode")
        + "\n"
    )


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
`generated/`.

- Keep the shared board files under `boards/{board}/` unchanged.
- Edit `app-gpios.dtsi` for application-specific GPIO assignments.
- Edit `app-pinctrl.dtsi` for application-specific peripheral pinmux.
- Edit `app-devices.dtsi` for application-specific device and bus properties.
- Edit `app.dts` to enable or disable devices exposed by the board.

Create a new board directory only when the physical PCB wiring changes.

The project references shared YiCore sources, the STM32F1 SoC backend, CMSIS,
and STM32Cube HAL from the repository root. Do not copy vendor libraries into
this application.
"""


def _app_pinctrl_template() -> str:
    return """/*
 * Application-local peripheral pinmux overrides.
 *
 * Keep reusable PCB wiring in the selected board. Override a labeled board
 * pinmux node here only when this application intentionally assigns it
 * differently. MCU alternate-function remap settings must match the selected
 * peripheral pins.
 *
 * Example:
 *
 * &usart1_tx_pin {
 *     speed = "medium";
 * };
 */
"""


def _app_gpios_template() -> str:
    return """/*
 * Application-local GPIO overrides.
 *
 * Put project-specific LEDs, keys, chip selects and software-bus GPIO
 * assignments here. When changing port, update the clock phandle as well.
 *
 * Example:
 *
 * &soft_i2c0_scl_gpio {
 *     port = "GPIOB";
 *     pin = <8>;
 *     clocks = <&clk_gpiob>;
 * };
 */
"""


def _app_devices_template() -> str:
    return """/*
 * Application-local device and bus overrides.
 *
 * Put project-specific baud rates, bus frequencies, addresses and similar
 * properties here. Keep app.dts focused on enabling and disabling devices.
 *
 * Example:
 *
 * &usart1 {
 *     current-speed = <115200>;
 * };
 */
"""


def _app_dts_template(board_include: str) -> str:
    return f"""/include/ "{board_include}"
/include/ "app-gpios.dtsi"
/include/ "app-pinctrl.dtsi"
/include/ "app-devices.dtsi"

/*
 * Application device selection.
 *
 * The selected board supplies all available labels. Enable only labels that
 * exist in that board; keep board-independent parameter overrides in the
 * application-local .dtsi files above.
 */
"""


def _main_source_template() -> str:
    return """/**
 * @file main.c
 * @brief YiCore application entry.
 * @author Don
 * @date 2026-07-28
 * @version 1.0.0
 */

#include "main.h"

#include "yi_device.h"
#include "yi_system.h"

/**
 * @brief Initialize YiCore devices and run the application loop.
 * @return This function does not return during normal operation.
 */
int main(void)
{
    if((yi_system_init() != 0) || (yi_device_init_all() != 0))
    {
        Error_Handler();
    }

    while(1)
    {
    }
}

/** @brief Enter the interrupt-locked fail-safe state. */
void Error_Handler(void)
{
    yi_system_irq_lock();
    while(1)
    {
    }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief Handle a HAL full-assert failure.
 * @param file Source file that raised the assertion.
 * @param line Source line that raised the assertion.
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
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
        _validate_name(target["template"], "template name")
        targets.append(target)

    return targets


def load_supported_boards(repo_root: Path) -> list[dict[str, str]]:
    """Discover and validate boards from boards/*/board.json manifests."""

    boards: list[dict[str, str]] = []
    seen_ids: set[str] = set()
    manifests = sorted((repo_root / "boards").glob("*/board.json"))
    if not manifests:
        raise ProjectCreationError(
            f"no board manifests found below: {repo_root / 'boards'}"
        )

    for manifest_path in manifests:
        try:
            raw_board = json.loads(manifest_path.read_text(encoding="utf-8"))
        except OSError as error:
            raise ProjectCreationError(
                f"cannot read board manifest: {manifest_path}"
            ) from error
        except json.JSONDecodeError as error:
            raise ProjectCreationError(
                f"board manifest is not valid JSON: {manifest_path}"
            ) from error

        if not isinstance(raw_board, dict):
            raise ProjectCreationError(
                f"board manifest must be an object: {manifest_path}"
            )
        board: dict[str, str] = {}
        for field in _REQUIRED_BOARD_FIELDS:
            value = raw_board.get(field)
            if not isinstance(value, str) or not value:
                raise ProjectCreationError(
                    f"board manifest missing {field}: {manifest_path}"
                )
            board[field] = value
        for field in _OPTIONAL_BOARD_STRING_FIELDS:
            value = raw_board.get(field)
            if value is not None:
                if not isinstance(value, str) or not value:
                    raise ProjectCreationError(
                        f"board manifest invalid {field}: {manifest_path}"
                    )
                board[field] = value
        for field in _OPTIONAL_BOARD_SIZE_FIELDS:
            value = raw_board.get(field)
            if value is not None:
                if not isinstance(value, int) or value <= 0:
                    raise ProjectCreationError(
                        f"board manifest invalid {field}: {manifest_path}"
                    )
                board[field] = value

        _validate_name(board["id"], "board id")
        _validate_name(board["vendor"], "vendor id")
        _validate_name(board["series"], "chip series")
        _validate_name(board["model"], "chip model")
        if board["id"] != manifest_path.parent.name:
            raise ProjectCreationError(
                f"board id {board['id']!r} must match directory "
                f"{manifest_path.parent.name!r}"
            )
        normalized_id = board["id"].lower()
        if normalized_id in seen_ids:
            raise ProjectCreationError(
                f"duplicate board id: {board['id']}"
            )
        seen_ids.add(normalized_id)

        board_dts = manifest_path.parent / "board.dts"
        if not board_dts.is_file():
            raise ProjectCreationError(
                f"registered board not found: {board_dts}"
            )
        boards.append(board)

    return boards


def _board_matches_target(
    board: dict[str, str], target: dict[str, str]
) -> bool:
    return all(
        board[field].lower() == target[field].lower()
        for field in ("vendor", "series", "model")
    )


def resolve_board(
    boards: list[dict[str, str]],
    target: dict[str, str] | None = None,
    board_id: str | None = None,
) -> dict[str, str]:
    """Resolve a board, optionally constrained to one MCU target."""

    matches = boards
    if target is not None:
        matches = [
            board for board in matches
            if _board_matches_target(board, target)
        ]
    if board_id:
        board_id = _validate_name(board_id, "board id").lower()
        matches = [
            board for board in matches if board["id"].lower() == board_id
        ]

    if not matches:
        if board_id and target is not None:
            raise ProjectCreationError(
                f"board {board_id!r} does not support "
                f"{target['vendor']}/{target['series']}/{target['model']}"
            )
        raise ProjectCreationError("no supported board matches selection")
    if len(matches) > 1:
        choices = ", ".join(board["id"] for board in matches)
        raise ProjectCreationError(
            f"board selection is ambiguous: {choices}; use --board"
        )
    return matches[0]


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
        lambda item: f"{item['model']} ({item['series']})",
        lambda item: item["model"],
    )


def select_board_interactive(
    boards: list[dict[str, str]], target: dict[str, str]
) -> dict[str, str]:
    """Prompt for a board compatible with the selected MCU target."""

    compatible = [
        board for board in boards if _board_matches_target(board, target)
    ]
    if not compatible:
        raise ProjectCreationError(
            "selected MCU has no registered boards"
        )
    return _select_option(
        f"Supported boards for {target['model']}:",
        compatible,
        lambda item: f"{item['name']} ({item['id']})",
        lambda item: item["id"],
    )


def select_board_any_interactive(
    boards: list[dict[str, str]]
) -> dict[str, str]:
    """Prompt for any registered product board.

    Args:
        boards: Validated board manifests available in the product.
    Returns:
        Board manifest selected by number or board identifier.
    Side effects:
        Writes choices to stdout and reads a selection from stdin.
    """

    if not boards:
        raise ProjectCreationError("product has no registered boards")
    return _select_option(
        "Product boards:",
        boards,
        lambda item: (
            f"{item['name']} ({item['id']}, {item['model']})"
        ),
        lambda item: item["id"],
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
    board_root: Path | None = None,
) -> Path:
    """Create one project and return its absolute directory path.

    Args:
        repo_root: YiCore root providing templates and framework sources.
        name: Project identifier.
        board: Selected board identifier.
        output_root: Optional parent directory for the generated project.
        target: Resolved MCU target metadata.
        board_root: Optional product root providing a local boards directory.
    Returns:
        Absolute generated project directory.
    Side effects:
        Creates and populates the project directory.
    """

    repo_root = repo_root.resolve()
    name = _validate_name(name, "project name")
    board = _validate_name(board, "board name")
    template_name = target["template"] if target is not None else _TEMPLATE_NAME
    template = repo_root / "examples" / template_name
    resolved_board_root = (
        board_root.resolve() if board_root is not None else repo_root
    )
    board_dts = resolved_board_root / "boards" / board / "board.dts"
    destination_root = (
        output_root.resolve()
        if output_root is not None
        else repo_root / "projects"
    )
    destination = destination_root / name

    if not template.is_dir():
        raise ProjectCreationError(f"template not found: {template}")
    if not board_dts.is_file():
        raise ProjectCreationError(f"board not found: {board_dts}")
    board_entry = resolve_board(
        load_supported_boards(resolved_board_root), target, board
    )
    if destination.exists():
        raise ProjectCreationError(f"destination already exists: {destination}")

    try:
        destination.mkdir(parents=True)
        shutil.copytree(template / "Core", destination / "Core")
        shutil.copy2(template / "VERSION", destination / "VERSION")
        (destination / "Core" / "Src" / "main.c").write_text(
            _main_source_template(), encoding="utf-8", newline="\n"
        )

        mdk_dir = destination / "MDK-ARM"
        mdk_dir.mkdir()
        for filename in ("startup_stm32f103xe.s", "EventRecorderStub.scvd"):
            shutil.copy2(template / "MDK-ARM" / filename, mdk_dir / filename)

        board_include = _relative_path(destination, board_dts, "/")
        (destination / "app.dts").write_text(
            _app_dts_template(board_include),
            encoding="utf-8",
            newline="\n",
        )
        (destination / "app-gpios.dtsi").write_text(
            _app_gpios_template(), encoding="utf-8", newline="\n"
        )
        (destination / "app-pinctrl.dtsi").write_text(
            _app_pinctrl_template(), encoding="utf-8", newline="\n"
        )
        (destination / "app-devices.dtsi").write_text(
            _app_devices_template(), encoding="utf-8", newline="\n"
        )

        project_template = (
            template / "MDK-ARM" / f"{template_name}.uvprojx"
        ).read_text(encoding="utf-8")
        project_text = project_template.replace(template_name, name)
        if board_entry.get("keil_device"):
            project_text = re.sub(
                r"<Device>STM32F103[A-Z0-9]+</Device>",
                f"<Device>{board_entry['keil_device']}</Device>",
                project_text,
                count=1,
            )
        flash_size = board_entry.get("flash_size")
        ram_size = board_entry.get("ram_size")
        if isinstance(flash_size, int) and isinstance(ram_size, int):
            flash_end = 0x08000000 + flash_size - 1
            ram_end = 0x20000000 + ram_size - 1
            project_text = re.sub(
                r"<Cpu>.*?</Cpu>",
                (
                    f"<Cpu>IRAM(0x20000000-0x{ram_end:X}) "
                    f"IROM(0x8000000-0x{flash_end:X})  CLOCK(8000000) "
                    'CPUTYPE("Cortex-M3") TZ</Cpu>'
                ),
                project_text,
                count=1,
            )
            project_text = re.sub(
                r"(<IRAM>.*?<Size>)0x[0-9a-fA-F]+(</Size>)",
                rf"\g<1>0x{ram_size:X}\g<2>",
                project_text,
                count=1,
                flags=re.DOTALL,
            )
            project_text = re.sub(
                r"(<IROM>.*?<Size>)0x[0-9a-fA-F]+(</Size>)",
                rf"\g<1>0x{flash_size:X}\g<2>",
                project_text,
                count=1,
                flags=re.DOTALL,
            )

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
        selected_drivers = _select_optional_drivers(
            destination / "app.dts",
            repo_root / "dts" / "bindings",
        )
        project_text = _prune_optional_drivers(
            project_text, selected_drivers
        )
        (mdk_dir / f"{name}.uvprojx").write_text(
            project_text, encoding="utf-8", newline="\n"
        )

        ioc_text = (template / f"{template_name}.ioc").read_text(
            encoding="utf-8"
        ).replace(template_name, name)
        if board_entry:
            ioc_replacements = {
                "Mcu.CPN": board_entry.get("cube_cpn"),
                "Mcu.Name": board_entry.get("cube_name"),
                "Mcu.Package": board_entry.get("cube_package"),
                "Mcu.UserName": board_entry.get("cube_user_name"),
                "ProjectManager.DeviceId": board_entry.get("cube_user_name"),
            }
            for key, value in ioc_replacements.items():
                if value:
                    ioc_text = re.sub(
                        rf"^{re.escape(key)}=.*$",
                        f"{key}={value}",
                        ioc_text,
                        flags=re.MULTILINE,
                    )
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
        generate_build_info(
            "application",
            (destination / "VERSION").read_text(encoding="utf-8").strip(),
            destination / "generated" / "yi_build_info.c",
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
        help="board id discovered from boards/*/board.json",
    )
    parser.add_argument(
        "--output-root",
        type=Path,
        help="parent directory for the project (default: projects/)",
    )
    args = parser.parse_args()
    repo_root = Path(__file__).resolve().parent.parent

    try:
        if args.name is None:
            if not sys.stdin.isatty():
                parser.error("the following arguments are required: name")
            args.name = input("Project name: ").strip()

        targets = load_supported_targets(repo_root)
        boards = load_supported_boards(repo_root)
        target = None
        board_entry = None
        if args.vendor or args.series or args.model:
            target = resolve_target(
                targets,
                args.vendor,
                args.series,
                args.model,
            )
        elif args.board:
            board_entry = resolve_board(boards, board_id=args.board)
            target = resolve_target(
                targets,
                board_entry["vendor"],
                board_entry["series"],
                board_entry["model"],
            )
        else:
            target = (
                select_target_interactive(targets)
                if sys.stdin.isatty()
                else resolve_target(targets)
            )

        if board_entry is None:
            board_entry = (
                resolve_board(boards, target, args.board)
                if args.board or not sys.stdin.isatty()
                else select_board_interactive(boards, target)
            )

        destination = create_project(
            repo_root,
            args.name,
            board_entry["id"],
            args.output_root,
            target,
        )
    except (OSError, ProjectCreationError, ValueError) as error:
        parser.error(str(error))

    print(destination)
    return 0


if __name__ == "__main__":
    raise SystemExit("use 'yi app create' or 'yi product create' instead")
