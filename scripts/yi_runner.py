#!/usr/bin/env python3
"""Create and execute Zephyr-style board runner commands.

Author: Don
Date: 2026-08-02
Version: 1.0.0
"""

from __future__ import annotations

import json
import subprocess
from pathlib import Path


class RunnerError(ValueError):
    """Report invalid runner metadata, artifacts, or commands."""


def load_board(board_root: Path, board: str) -> dict:
    """Load one board manifest used to select a debug runner."""

    manifest = board_root / board / "board.json"
    if not manifest.is_file():
        raise RunnerError(f"unknown board: {board}")
    return json.loads(manifest.read_text(encoding="utf-8"))


def default_runner(board: dict) -> str:
    """Return the preferred runner for a board vendor."""

    return {
        "st": "openocd",
        "hpmicro": "openocd",
        "wch": "wch-link",
    }.get(board.get("vendor"), "openocd")


def find_artifact(build_dir: Path, suffixes: tuple[str, ...]) -> Path:
    """Find the single preferred firmware artifact below a build directory."""

    for suffix in suffixes:
        matches = sorted(build_dir.rglob(f"*{suffix}"))
        if matches:
            return matches[0].resolve()
    raise RunnerError(f"no firmware artifact found in {build_dir}")


def runner_command(action: str, runner: str, board: dict,
                   build_dir: Path) -> list[str]:
    """Build a flash, debug, or debug-server command without executing it."""

    if action not in {"flash", "debug", "debugserver"}:
        raise RunnerError(f"unsupported runner action: {action}")
    part = str(board.get("part", board.get("model", "")))
    if runner == "jlink":
        server = ["JLinkGDBServerCL.exe", "-device", part, "-if", "SWD"]
        if action == "flash":
            image = find_artifact(build_dir, (".hex", ".bin", ".elf"))
            script = build_dir / "yicore-jlink-flash.jlink"
            script.write_text(
                f"loadfile {image}\nr\ng\nexit\n", encoding="utf-8"
            )
            return ["JLink.exe", "-device", part, "-if", "SWD",
                    "-CommanderScript", str(script.resolve())]
        return server + (["-singlerun"] if action == "debug" else [])
    if runner == "pyocd":
        if action == "flash":
            image = find_artifact(build_dir, (".hex", ".elf", ".bin"))
            return ["pyocd", "flash", "-t", part, str(image)]
        return ["pyocd", "gdbserver", "-t", part]
    if runner == "wch-link":
        if action != "flash":
            raise RunnerError("WCH-LinkUtility runner currently supports flash only")
        image = find_artifact(build_dir, (".hex", ".bin"))
        return ["WCH-LinkUtility.exe", "-File", str(image), "-Program"]
    if runner == "openocd":
        target = board.get("openocd_target")
        if not target:
            target = {
                "st": f"target/{board.get('series', 'stm32f1')}x.cfg",
                "hpmicro": "target/hpm5301.cfg",
                "wch": "target/wch-riscv.cfg",
            }.get(board.get("vendor"), "target/cortex_m.cfg")
        command = ["openocd", "-f", str(target)]
        if action == "flash":
            image = find_artifact(build_dir, (".elf", ".hex", ".bin"))
            return command + ["-c", f"program {image} verify reset exit"]
        if action == "debug":
            return command + ["-c", "init; reset halt"]
        return command
    raise RunnerError(f"unknown runner: {runner}")


def run_runner(command: list[str]) -> None:
    """Execute an already validated runner command."""

    try:
        subprocess.run(command, check=True)
    except (OSError, subprocess.CalledProcessError) as error:
        raise RunnerError(f"runner failed: {command[0]}") from error
