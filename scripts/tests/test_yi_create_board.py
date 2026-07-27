"""YiCore create-board tests.

Author: Don
Date: 2026-07-27
Version: 1.0.0
"""

import json
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_create_board import BoardCreationError, create_board  # noqa: E402
from yi_create_project import (  # noqa: E402
    load_supported_boards,
    load_supported_targets,
    resolve_board,
    resolve_target,
)


class BoardCreationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.yicore = SCRIPTS_DIR.parent
        cls.target = resolve_target(
            load_supported_targets(cls.yicore), model="stm32f103xe"
        )
        cls.source_board = resolve_board(
            load_supported_boards(cls.yicore),
            cls.target,
            "fire-mini-stm32f103",
        )

    def test_board_is_copied_with_local_manifest(self):
        with tempfile.TemporaryDirectory() as temporary:
            output_root = Path(temporary)
            board = create_board(
                self.yicore,
                "product-a-stm32f103",
                self.target,
                self.source_board,
                "Product A",
                "Product A controller board",
                output_root,
            )

            self.assertEqual(board, output_root / "product-a-stm32f103")
            self.assertTrue((board / "board.dts").is_file())
            self.assertTrue((board / "board-gpios.dtsi").is_file())
            manifest = json.loads(
                (board / "board.json").read_text(encoding="utf-8")
            )
            self.assertEqual(manifest["id"], board.name)
            self.assertEqual(manifest["name"], "Product A")
            self.assertEqual(manifest["model"], "stm32f103xe")
            self.assertEqual(
                manifest["description"], "Product A controller board"
            )

    def test_existing_destination_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            output_root = Path(temporary)
            (output_root / "existing-board").mkdir()
            with self.assertRaisesRegex(
                BoardCreationError, "destination already exists"
            ):
                create_board(
                    self.yicore,
                    "existing-board",
                    self.target,
                    self.source_board,
                    output_root=output_root,
                )

    def test_reference_board_must_match_target(self):
        other_target = {
            "vendor": "st",
            "series": "stm32f4",
            "model": "stm32f407xx",
        }
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(
                BoardCreationError, "does not support"
            ):
                create_board(
                    self.yicore,
                    "wrong-target-board",
                    other_target,
                    self.source_board,
                    output_root=Path(temporary),
                )

    def test_invalid_board_id_is_rejected(self):
        with self.assertRaisesRegex(
            ValueError, "board id"
        ):
            create_board(
                self.yicore,
                "../unsafe",
                self.target,
                self.source_board,
            )


if __name__ == "__main__":
    unittest.main()
