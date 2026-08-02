#!/usr/bin/env python3
"""Create a thin, board-independent YiCore application.

Author: Don
Date: 2026-07-28
Version: 1.0.0
"""

from __future__ import annotations

import re
import shutil
from pathlib import Path


class AppCreationError(ValueError):
    """Report an unsafe name or application destination."""


_APP_NAME_RE = re.compile(r"^[A-Za-z][A-Za-z0-9_-]{0,63}$")


def _cmake_template(name: str) -> str:
    """Return the board-independent application CMake entry."""

    return f"""# File: CMakeLists.txt
# Function: Build the {name} YiCore application.
# Author: Don
# Date: 2026-07-28
# Version: 1.0.0

cmake_minimum_required(VERSION 3.20)
project({name} LANGUAGES C ASM)

if(NOT DEFINED YICORE_ROOT)
    message(FATAL_ERROR "Set YICORE_ROOT to the YiCore repository")
endif()

include("${{YICORE_ROOT}}/cmake/YiCoreApplication.cmake")
yi_application(
    NAME {name}
    SOURCES src/main.c
    DTS "${{CMAKE_CURRENT_LIST_DIR}}/app.overlay"
    CONF "${{CMAKE_CURRENT_LIST_DIR}}/app.conf"
    VERSION_FILE "${{CMAKE_CURRENT_LIST_DIR}}/VERSION"
)
"""


def _main_template() -> str:
    """Return the minimal portable application entry source."""

    return """/**
 * @file main.c
 * @brief Provide the board-independent YiCore application entry.
 * @author Don
 * @date 2026-07-28
 * @version 1.0.0
 */

#include "yi_device.h"
#include "yi_poll.h"
#include "yi_system.h"

/**
 * @brief Initialize YiCore and run application processing.
 * @return This function does not return during normal operation.
 */
int main(void)
{
    if((yi_system_init() != 0) || (yi_device_init_all() != 0))
    {
        yi_system_irq_lock();
        for(;;)
        {
        }
    }

    for(;;)
    {
        (void)yi_poll();
        /* Add one non-blocking application processing step here. */
        yi_idle();
    }
}
"""


def _readme_template(name: str) -> str:
    """Return concise build guidance for a thin application."""

    return f"""# {name}

This is a board-independent YiCore application. It owns application source,
configuration and DeviceTree overrides only; startup files, linker scripts,
SoC drivers and vendor libraries are selected by the build-time board.

Configure and build:

```text
cmake -S . -B build -DYICORE_ROOT=<YiCore> -DBOARD=<board> \
  -DCMAKE_TOOLCHAIN_FILE=<toolchain>
cmake --build build
```

Use `app.conf` for feature selection and `app.overlay` for application-local
hardware overrides. Do not copy vendor or board implementation files here.
"""


def create_app(name: str, output_root: Path) -> Path:
    """Create one board-independent application directory.

    Args:
        name: Application identifier used by CMake.
        output_root: Parent directory for the application.
    Returns:
        Absolute destination path.
    Side effects:
        Creates a new directory tree and removes it if creation fails.
    Raises:
        AppCreationError: The name or destination is invalid.
    """

    if not _APP_NAME_RE.fullmatch(name):
        raise AppCreationError(
            "application name must start with a letter and contain only "
            "letters, digits, '-' or '_'"
        )
    destination = output_root.resolve() / name
    if destination.exists():
        raise AppCreationError(
            f"application already exists: {destination}"
        )

    try:
        source_dir = destination / "src"
        source_dir.mkdir(parents=True)
        (destination / "CMakeLists.txt").write_text(
            _cmake_template(name), encoding="utf-8", newline="\n"
        )
        (destination / "app.conf").write_text(
            "# Application feature configuration.\n",
            encoding="utf-8",
            newline="\n",
        )
        (destination / "app.overlay").write_text(
            "/* Application-local DeviceTree overrides. */\n",
            encoding="utf-8",
            newline="\n",
        )
        (destination / "VERSION").write_text(
            "0.1.0\n", encoding="utf-8", newline="\n"
        )
        (source_dir / "main.c").write_text(
            _main_template(), encoding="utf-8", newline="\n"
        )
        (destination / "README.md").write_text(
            _readme_template(name), encoding="utf-8", newline="\n"
        )
    except Exception:
        if destination.exists():
            shutil.rmtree(destination)
        raise
    return destination


if __name__ == "__main__":
    raise SystemExit("use 'yi app create' instead")
