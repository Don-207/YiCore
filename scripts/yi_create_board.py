#!/usr/bin/env python3
"""Create a YiCore board from a compatible reference board.

Author: Don
Date: 2026-07-27
Version: 1.0.0
"""

from __future__ import annotations

import argparse
import json
import shutil
import sys
from pathlib import Path

from yi_create_project import (
    ProjectCreationError,
    _board_matches_target,
    _validate_name,
    load_supported_boards,
    load_supported_targets,
    resolve_board,
    resolve_target,
    select_board_interactive,
    select_target_interactive,
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


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Create a YiCore board from a compatible reference board"
    )
    parser.add_argument(
        "board_id",
        nargs="?",
        help="new board directory id, for example product-a-stm32f103",
    )
    parser.add_argument("--display-name", help="human-readable board name")
    parser.add_argument("--description", help="board description")
    parser.add_argument("--vendor", help="vendor id")
    parser.add_argument("--series", help="MCU series")
    parser.add_argument("--model", help="MCU model")
    parser.add_argument(
        "--from-board",
        help="compatible reference board id; selected interactively if omitted",
    )
    parser.add_argument(
        "--output-root",
        type=Path,
        help="parent directory for the board (default: boards/)",
    )
    args = parser.parse_args()
    repo_root = Path(__file__).resolve().parent.parent

    try:
        if args.board_id is None:
            if not sys.stdin.isatty():
                parser.error("the following arguments are required: board_id")
            args.board_id = input("Board id: ").strip()

        targets = load_supported_targets(repo_root)
        boards = load_supported_boards(repo_root)
        target = None
        source_board = None

        if args.vendor or args.series or args.model:
            target = resolve_target(
                targets, args.vendor, args.series, args.model
            )
        elif args.from_board:
            source_board = resolve_board(
                boards, board_id=args.from_board
            )
            target = resolve_target(
                targets,
                source_board["vendor"],
                source_board["series"],
                source_board["model"],
            )
        else:
            target = (
                select_target_interactive(targets)
                if sys.stdin.isatty()
                else resolve_target(targets)
            )

        if source_board is None:
            source_board = (
                resolve_board(boards, target, args.from_board)
                if args.from_board or not sys.stdin.isatty()
                else select_board_interactive(boards, target)
            )

        destination = create_board(
            repo_root,
            args.board_id,
            target,
            source_board,
            args.display_name,
            args.description,
            args.output_root,
        )
    except (OSError, BoardCreationError, ProjectCreationError) as error:
        parser.error(str(error))

    print(destination)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
