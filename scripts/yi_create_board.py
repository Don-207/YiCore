#!/usr/bin/env python3
"""Create a YiCore board from a compatible reference board.

Author: Don
Date: 2026-07-27
Version: 1.0.0
"""

from __future__ import annotations

import json
import shutil
from pathlib import Path

from yi_create_project import (
    _board_matches_target,
    _validate_name,
)


class BoardCreationError(ValueError):
    """Raised when a board cannot be created safely."""


def create_board(
    repo_root: Path,
    board_id: str,
    target: dict[str, str],
    source_board: dict[str, str],
    display_name: str | None = None,
    description: str | None = None,
    output_root: Path | None = None,
) -> Path:
    """Copy a reference board and write a board-local manifest."""

    repo_root = repo_root.resolve()
    board_id = _validate_name(board_id, "board id")
    if not _board_matches_target(source_board, target):
        raise BoardCreationError(
            f"reference board {source_board['id']!r} does not support "
            f"{target['vendor']}/{target['series']}/{target['model']}"
        )

    source = repo_root / "boards" / source_board["id"]
    if not (source / "board.dts").is_file():
        raise BoardCreationError(f"reference board not found: {source}")

    destination_root = (
        output_root.resolve()
        if output_root is not None
        else repo_root / "boards"
    )
    destination = destination_root / board_id
    if destination.exists():
        raise BoardCreationError(
            f"destination already exists: {destination}"
        )

    display_name = display_name or board_id.replace("-", " ").replace(
        "_", " "
    ).title()
    description = description or (
        f"{display_name} based on {target['model']}"
    )
    if not display_name.strip():
        raise BoardCreationError("display name must not be empty")
    if not description.strip():
        raise BoardCreationError("description must not be empty")

    manifest = {
        "id": board_id,
        "name": display_name,
        "vendor": target["vendor"],
        "series": target["series"],
        "model": target["model"],
        "description": description,
    }

    try:
        shutil.copytree(source, destination)
        (destination / "board.json").write_text(
            json.dumps(manifest, indent=2) + "\n",
            encoding="utf-8",
            newline="\n",
        )
    except Exception:
        if destination.exists():
            shutil.rmtree(destination)
        raise

    return destination


if __name__ == "__main__":
    raise SystemExit("use 'yi board create' instead")
