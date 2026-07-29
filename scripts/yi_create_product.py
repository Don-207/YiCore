#!/usr/bin/env python3
"""Create or extend a standard YiCore product firmware layout.

Author: Don
Date: 2026-07-28
Version: 1.0.0
"""

from __future__ import annotations

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

        if image == "bootloader":
            mcuboot = yicore / "third_party" / "mcuboot"
            if not (mcuboot / "scripts" / "imgtool.py").is_file():
                source = (
                    Path(__file__).resolve().parent.parent
                    / "third_party"
                    / "mcuboot"
                )
                subprocess.run(
                    [
                        "git", "-C", str(yicore), "submodule", "init",
                        "third_party/mcuboot",
                    ],
                    check=True,
                )
                subprocess.run(
                    [
                        "git", "clone", "--local", "--no-hardlinks",
                        str(source), str(mcuboot),
                    ],
                    check=True,
                )
                subprocess.run(
                    [
                        "git", "-C", str(mcuboot), "remote", "set-url",
                        "origin", "https://github.com/mcu-tools/mcuboot.git",
                    ],
                    check=True,
                )
    except Exception:
        shutil.rmtree(destination)
        raise
    return destination


def create_product(
    repo_root: Path,
    name: str,
    board: dict[str, str],
    target: dict[str, str],
    output_root: Path,
    board_root: Path | None = None,
) -> Path:
    """Create a standalone product repository with one application image."""
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
                    "submodule",
                    "init",
                    "third_party/lwrb",
                ],
                check=True,
            )
            subprocess.run(
                [
                    "git",
                    "clone",
                    "--local",
                    "--no-hardlinks",
                    str(repo_root / "third_party" / "lwrb"),
                    str(
                        destination
                        / "YiCore"
                        / "third_party"
                        / "lwrb"
                    ),
                ],
                check=True,
            )
            subprocess.run(
                [
                    "git",
                    "-C",
                    str(
                        destination
                        / "YiCore"
                        / "third_party"
                        / "lwrb"
                    ),
                    "remote",
                    "set-url",
                    "origin",
                    "https://github.com/MaJerle/lwrb.git",
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
            (destination / ".gitmodules").write_text(
                '[submodule "YiCore"]\n'
                "\tpath = YiCore\n"
                "\turl = https://github.com/Don-207/YiCore.git\n"
                "\tshallow = true\n",
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
    return product_root


if __name__ == "__main__":
    raise SystemExit("use 'yi product create' or 'yi image add' instead")
