#!/usr/bin/env python3
"""Create or extend a standard YiCore product firmware layout.

Author: Don
Date: 2026-07-28
Version: 1.0.0
"""

from __future__ import annotations

import re
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

from yi_create_project import (
    ProjectCreationError,
    _relative_path,
    create_project,
)
from yi_build_info_gen import generate as generate_build_info
from yi_dts_gen import generate


def _hpm5301_main() -> str:
    """Return the minimum HPM5301 product application entry."""

    return """/**
 * @file main.c
 * @brief Start an HPM5301 YiCore product application.
 * @author Don
 * @date 2026-07-29
 * @version 1.0.0
 */

#include <stdint.h>
#include <stdio.h>

#include "board.h"
#include "yi_riscv_irq.h"

/** Product heartbeat interval in milliseconds. */
#define YI_PRODUCT_HEARTBEAT_MS (1000U)

/**
 * @brief Initialize the HPM5301 board and run product processing.
 * @return This function does not return during normal operation.
 * @note Runs in machine mode after the official HPM SDK reset path.
 */
int main(void)
{
    /** Saved global interrupt state used to validate critical sections. */
    uint32_t irq_key;
    /** Monotonic product heartbeat counter. */
    uint32_t heartbeat = 0U;

    board_init();
    irq_key = yi_riscv_irq_lock();
    yi_riscv_memory_barrier();
    yi_riscv_irq_unlock(irq_key);

    printf("HPM5301 YiCore product ready\\r\\n");
    for (;;) {
        printf("heartbeat %lu\\r\\n", (unsigned long)heartbeat);
        heartbeat++;
        board_delay_ms(YI_PRODUCT_HEARTBEAT_MS);
    }
}
"""


def _hpm5301_cmake(name: str, sdk_board: str) -> str:
    """Return the official-SDK CMake entry for one HPM5301 product."""

    return f"""# File: CMakeLists.txt
# Function: Build the {name} HPM5301 product application.
# Author: Don
# Date: 2026-07-29
# Version: 1.0.0

cmake_minimum_required(VERSION 3.20)

get_filename_component(
    YI_PRODUCT_ROOT "${{CMAKE_CURRENT_LIST_DIR}}/../../.." ABSOLUTE
)
set(
    HPM_SDK_BASE
    "${{YI_PRODUCT_ROOT}}/modules/hal/hpmicro/hpm_sdk"
    CACHE PATH
    "Pinned official HPM SDK root"
)
set(ENV{{HPM_SDK_BASE}} "${{HPM_SDK_BASE}}")
set(BOARD "{sdk_board}" CACHE STRING "Official HPM SDK board backend")
set(HPM_BUILD_TYPE "flash_xip" CACHE STRING "HPM image memory layout")
set(APP_NAME "{name}" CACHE STRING "Output image base name")
set(STACK_SIZE "0x1000" CACHE STRING "Application stack size in bytes")
set(HEAP_SIZE "0x1000" CACHE STRING "Application heap size in bytes")
set(
    RV_ARCH
    "rv32imac_zicsr_zifencei"
    CACHE STRING
    "Portable HPM5301 ISA baseline"
)
set(RV_ABI "ilp32" CACHE STRING "HPM5301 integer ABI")

find_package(hpm-sdk REQUIRED HINTS "${{HPM_SDK_BASE}}")
project({name} LANGUAGES C ASM)

set(YICORE_ROOT "${{YI_PRODUCT_ROOT}}/YiCore")
sdk_app_src(
    "${{YI_PRODUCT_ROOT}}/firmware/images/application/main.c"
    "${{YICORE_ROOT}}/arch/riscv/yi_riscv_irq.c"
)
sdk_app_inc("${{YICORE_ROOT}}/arch/riscv")
"""


def _hpm5301_product_readme(name: str, board_id: str) -> str:
    """Return build guidance for a generated HPM5301 product."""

    return f"""# {name}

HPM5301 YiCore product generated from `{board_id}`. The first build uses the
official HPM5301EVKLite SDK board backend and Flash XIP layout.

```powershell
.\\YiCore\\yi.cmd update
$env:GNURISCV_TOOLCHAIN_PATH = `
  'D:\\toolchains\\rv32imac_zicsr_zifencei_multilib_b_ext-win'
$env:Path = 'C:\\Qt\\Tools\\Ninja;' + $env:Path
cmake -S firmware\\projects\\gcc -B build -G Ninja
cmake --build build --parallel
```

Replace the official SDK board backend with a product-owned HPM SDK board
directory before changing oscillator, flash, console, or pin assignments.
"""


def _create_hpm5301_product(
    repo_root: Path,
    name: str,
    board: dict[str, str],
    output_root: Path,
    board_root: Path | None,
) -> Path:
    """Create a standalone HPM5301 product using the official SDK backend."""

    destination = output_root.resolve() / name
    if destination.exists():
        raise ProjectCreationError(f"destination already exists: {destination}")

    board_source = (
        board_root.resolve() if board_root else repo_root
    ) / "boards" / board["id"]
    try:
        application = destination / "firmware" / "images" / "application"
        gcc = destination / "firmware" / "projects" / "gcc"
        application.mkdir(parents=True)
        gcc.mkdir(parents=True)
        shutil.copytree(
            board_source,
            destination / "boards" / board["id"],
        )
        (application / "main.c").write_text(
            _hpm5301_main(), encoding="utf-8", newline="\n"
        )
        (application / "VERSION").write_text(
            "0.1.0\n", encoding="utf-8", newline="\n"
        )
        (gcc / "CMakeLists.txt").write_text(
            _hpm5301_cmake(name, "hpm5301evklite"),
            encoding="utf-8",
            newline="\n",
        )
        (destination / "README.md").write_text(
            _hpm5301_product_readme(name, board["id"]),
            encoding="utf-8",
            newline="\n",
        )
        (destination / "west.yml").write_text(
            _west_manifest(
                destination.name,
                [
                    "-hal-st",
                    "+hal-hpmicro",
                    "-serial",
                    "-debug",
                    "-bootloader",
                ],
                _current_yicore_revision(repo_root),
            ),
            encoding="utf-8",
            newline="\n",
        )
        subprocess.run(
            [
                "git", "clone", "--local", "--no-hardlinks",
                str(repo_root), str(destination / "YiCore"),
            ],
            check=True,
        )
        subprocess.run(
            [
                "git", "-C", str(destination / "YiCore"),
                "remote", "set-url", "origin",
                "https://github.com/Don-207/YiCore.git",
            ],
            check=True,
        )
        (destination / ".gitignore").write_text(
            _west_gitignore(),
            encoding="utf-8",
            newline="\n",
        )
    except Exception:
        if destination.exists():
            shutil.rmtree(destination)
        raise
    return destination


def _image_main(image: str) -> str:
    """Return a documented minimal entry source for one optional image."""
    purpose = "bootloader" if image == "bootloader" else "board-test"
    return f"""/**
 * @file main.c
 * @brief Provide the {purpose} image entry.
 * @author Don
 * @date 2026-07-28
 * @version 1.0.0
 */

#include "main.h"
#include "yi_device.h"
#include "yi_system.h"

/**
 * @brief Initialize shared devices and run the {purpose} image.
 * @return This function does not return during normal operation.
 */
int main(void)
{{
    if ((yi_system_init() != 0) || (yi_device_init_all() != 0))
    {{
        Error_Handler();
    }}

    for (;;)
    {{
        /* Add product-specific {purpose} processing here. */
    }}
}}

/** @brief Enter the interrupt-locked fail-safe state. */
void Error_Handler(void)
{{
    yi_system_irq_lock();
    for (;;)
    {{
    }}
}}
"""


def add_image(product_root: Path, image: str) -> Path:
    """Add an optional image to an existing standard product."""
    product_root = product_root.resolve()
    firmware = product_root / "firmware"
    if not (firmware / "images" / "application").is_dir():
        raise ProjectCreationError(
            f"not a YiCore product root: {product_root}"
        )
    destination = firmware / "images" / image
    if destination.exists():
        raise ProjectCreationError(f"image already exists: {destination}")

    application = firmware / "images" / "application"
    destination.mkdir(parents=True)
    west_manifest = product_root / "west.yml"
    original_west = (
        west_manifest.read_text(encoding="utf-8")
        if west_manifest.is_file()
        else None
    )
    try:
        (destination / "main.c").write_text(
            _image_main(image), encoding="utf-8", newline="\n"
        )
        (destination / "VERSION").write_text(
            "0.1.0\n", encoding="utf-8", newline="\n"
        )
        for filename in (
            "app.dts",
            "app-gpios.dtsi",
            "app-pinctrl.dtsi",
            "app-devices.dtsi",
        ):
            shutil.copy2(application / filename, destination / filename)

        yicore = product_root / "YiCore"
        generate(
            destination / "app.dts",
            yicore / "dts" / "bindings",
            destination / "generated",
        )
        generate_build_info(
            image,
            "0.1.0",
            destination / "generated" / "yi_build_info.c",
        )

        project = (
            firmware
            / "projects"
            / "keil"
            / f"{product_root.name}.uvprojx"
        )
        if not project.is_file():
            raise ProjectCreationError("Keil project not found")
        tree = ET.parse(project)
        targets = tree.getroot().find("Targets")
        if targets is None or len(list(targets)) != 1:
            raise ProjectCreationError("Keil application target not found")
        image_target = list(targets)[0]
        product_name = project.stem
        for node in image_target.iter():
            if node.text:
                node.text = node.text.replace("application", image)
        target_name = image_target.find("TargetName")
        if target_name is not None:
            target_name.text = f"{product_name}-{image}"
        output_name = image_target.find(
            ".//TargetCommonOption/OutputName"
        )
        if output_name is not None:
            output_name.text = f"{product_name}-{image}"
        output_directory = image_target.find(
            ".//TargetCommonOption/OutputDirectory"
        )
        if output_directory is not None:
            output_directory.text = f".\\{product_name}-{image}\\"
        listing_path = image_target.find(
            ".//TargetCommonOption/ListingPath"
        )
        if listing_path is not None:
            listing_path.text = f".\\{product_name}-{image}\\"
        ET.indent(tree, space="  ")
        image_project = project.with_name(
            f"{product_name}-{image}.uvprojx"
        )
        image_project.write_text(
            '<?xml version="1.0" encoding="UTF-8" standalone="no" ?>\n'
            + ET.tostring(tree.getroot(), encoding="unicode")
            + "\n",
            encoding="utf-8",
            newline="\n",
        )
        if image == "bootloader" and original_west is not None:
            west_manifest.write_text(
                original_west.replace(
                    "    - -bootloader", "    - +bootloader"
                ),
                encoding="utf-8",
                newline="\n",
            )

    except Exception:
        shutil.rmtree(destination)
        if original_west is not None:
            west_manifest.write_text(
                original_west, encoding="utf-8", newline="\n"
            )
        raise
    return destination


def _west_manifest(
    product_directory: str,
    groups: list[str],
    yicore_revision: str,
) -> str:
    """Return a west manifest selecting modules for one generated product."""

    filters = "\n".join(f"    - {group}" for group in groups)
    return f"""# File: west.yml
# Function: Select product workspace modules managed by west.
# Author: Don
# Date: 2026-07-30
# Version: 1.0.0

manifest:
  version: "1.2"
  group-filter:
{filters}
  projects:
    - name: YiCore
      url: https://github.com/Don-207/YiCore.git
      revision: {yicore_revision}
      path: YiCore
      import:
        file: yi-modules.yml
        path-prefix: {product_directory}
  self:
    path: {product_directory}
"""


def _current_yicore_revision(repo_root: Path) -> str:
    """Return the exact YiCore commit pinned by a generated west manifest."""

    return subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=repo_root,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    ).stdout.strip()


def _west_gitignore() -> str:
    """Return ignore rules for repositories materialized by west."""

    return (
        "/YiCore/\n"
        "/modules/\n"
        "/bootloader/\n"
        "/build/\n"
    )


def create_product(
    repo_root: Path,
    name: str,
    board: dict[str, str],
    target: dict[str, str],
    output_root: Path,
    board_root: Path | None = None,
) -> Path:
    """Create a standalone product repository with one application image."""
    if target["vendor"] == "hpmicro" and target["model"] == "hpm5301":
        return _create_hpm5301_product(
            repo_root, name, board, output_root, board_root
        )

    destination = output_root.resolve() / name
    if destination.exists():
        raise ProjectCreationError(f"destination already exists: {destination}")

    with tempfile.TemporaryDirectory() as temporary:
        staging_root = Path(temporary)
        legacy = create_project(
            repo_root,
            name,
            board["id"],
            staging_root,
            target,
            board_root,
        )
        destination.mkdir(parents=True)
        try:
            common = destination / "firmware" / "common"
            application = destination / "firmware" / "images" / "application"
            keil = destination / "firmware" / "projects" / "keil"
            gcc = destination / "firmware" / "projects" / "gcc"
            linker = destination / "firmware" / "linker" / "gcc"
            for directory in (common, application, keil, gcc, linker):
                directory.mkdir(parents=True, exist_ok=True)
            (destination / "west.yml").write_text(
                _west_manifest(
                    destination.name,
                    [
                        "+hal-st",
                        "-hal-hpmicro",
                        "+serial",
                        "+debug",
                        "-bootloader",
                    ],
                    _current_yicore_revision(repo_root),
                ),
                encoding="utf-8",
                newline="\n",
            )

            shutil.move(str(legacy / "Core"), str(common / "Core"))
            shutil.move(
                str(common / "Core" / "Src" / "main.c"),
                str(application / "main.c"),
            )
            cubemx = common / "cubemx"
            cubemx.mkdir()
            shutil.move(
                str(legacy / f"{name}.ioc"),
                str(cubemx / f"{name}.ioc"),
            )
            for filename in (
                "VERSION",
                "README.md",
                "app.dts",
                "app-gpios.dtsi",
                "app-pinctrl.dtsi",
                "app-devices.dtsi",
                "generated",
            ):
                shutil.move(str(legacy / filename), str(application / filename))
            for source in (legacy / "MDK-ARM").iterdir():
                shutil.move(str(source), str(keil / source.name))

            board_source = (
                board_root.resolve() if board_root else repo_root
            ) / "boards" / board["id"]
            board_destination = destination / "boards" / board["id"]
            shutil.copytree(board_source, board_destination)
            product_board_dts = board_destination / "board.dts"
            product_board_dts.write_text(
                product_board_dts.read_text(encoding="utf-8").replace(
                    '../../dts/', '../../YiCore/dts/'
                ),
                encoding="utf-8",
                newline="\n",
            )
            board_include = _relative_path(
                application, board_destination / "board.dts", "/"
            )
            app_dts = application / "app.dts"
            dts_text = app_dts.read_text(encoding="utf-8")
            first_newline = dts_text.find("\n")
            app_dts.write_text(
                f'/include/ "{board_include}"'
                + dts_text[first_newline:],
                encoding="utf-8",
                newline="\n",
            )

            project = keil / f"{name}.uvprojx"
            project_text = project.read_text(encoding="utf-8")
            old_mdk = legacy / "MDK-ARM"
            for separator in ("/", "\\"):
                old_root = _relative_path(old_mdk, repo_root, separator)
                new_root = "../../../YiCore" if separator == "/" else (
                    "..\\..\\..\\YiCore"
                )
                project_text = project_text.replace(old_root, new_root)
            replacements = {
                "../Core/Src/main.c": "../../images/application/main.c",
                "..\\Core\\Src\\main.c": "..\\..\\images\\application\\main.c",
                "../Core": "../../common/Core",
                "..\\Core": "..\\..\\common\\Core",
                "../generated": "../../images/application/generated",
                "..\\generated": "..\\..\\images\\application\\generated",
                "../app.dts": "../../images/application/app.dts",
                "..\\app.dts": "..\\..\\images\\application\\app.dts",
                "../VERSION": "../../images/application/VERSION",
                "..\\VERSION": "..\\..\\images\\application\\VERSION",
            }
            for old, new in replacements.items():
                project_text = project_text.replace(old, new)
            project.write_text(
                project_text, encoding="utf-8", newline="\n"
            )

            template_root = (
                repo_root / "scripts" / "templates" / target["model"]
            )
            shutil.copytree(template_root / "gcc", gcc, dirs_exist_ok=True)
            shutil.copytree(
                template_root / "linker", linker, dirs_exist_ok=True
            )
            flash_size = board.get("flash_size")
            ram_size = board.get("ram_size")
            if isinstance(flash_size, int) and isinstance(ram_size, int):
                for linker_file in linker.glob("*.ld"):
                    linker_text = linker_file.read_text(encoding="utf-8")
                    linker_text = re.sub(
                        r"(FLASH\s+\(rx\).*?LENGTH\s*=\s*)\S+",
                        rf"\g<1>{flash_size}",
                        linker_text,
                    )
                    linker_text = re.sub(
                        r"(RAM\s+\(xrw\).*?LENGTH\s*=\s*)\S+",
                        rf"\g<1>{ram_size}",
                        linker_text,
                    )
                    linker_file.write_text(
                        linker_text, encoding="utf-8", newline="\n"
                    )
            subprocess.run(
                [
                    "git",
                    "clone",
                    "--local",
                    "--no-hardlinks",
                    str(repo_root),
                    str(destination / "YiCore"),
                ],
                check=True,
            )
            subprocess.run(
                [
                    "git",
                    "-C",
                    str(destination / "YiCore"),
                    "remote",
                    "set-url",
                    "origin",
                    "https://github.com/Don-207/YiCore.git",
                ],
                check=True,
            )
            (destination / ".gitignore").write_text(
                _west_gitignore(),
                encoding="utf-8",
                newline="\n",
            )
        except Exception:
            shutil.rmtree(destination)
            raise
    return destination


def create_application_in_place(
    repo_root: Path,
    product_root: Path,
    board: dict[str, str],
    target: dict[str, str],
    board_root: Path | None = None,
) -> Path:
    """Populate an existing product repository with its application image."""
    product_root = product_root.resolve()
    application = product_root / "firmware" / "images" / "application"
    if (application / "main.c").is_file():
        raise ProjectCreationError(
            f"application already exists: {application}"
        )

    with tempfile.TemporaryDirectory() as temporary:
        generated = create_product(
            repo_root,
            product_root.name,
            board,
            target,
            Path(temporary),
            board_root,
        )
        shutil.copytree(
            generated / "boards",
            product_root / "boards",
            dirs_exist_ok=True,
        )
        shutil.copytree(
            generated / "firmware",
            product_root / "firmware",
            dirs_exist_ok=True,
        )
        generated_west = generated / "west.yml"
        product_west = product_root / "west.yml"
        if generated_west.is_file() and not product_west.exists():
            shutil.copy2(generated_west, product_west)
    return product_root


if __name__ == "__main__":
    raise SystemExit("use 'yi product create' or 'yi image add' instead")
