"""Test the unified YiCore command-line build behavior.

Author: Don
Date: 2026-07-28
Version: 2.0.0
"""

import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch


SCRIPTS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS_DIR))

from yi_cli import (  # noqa: E402
    YiCliError,
    _create_parser,
    build_app,
    create_board_for_product,
    create_product_in_place,
    list_boards,
    sdk_list,
    sdk_verify,
)
from yi_create_app import create_app  # noqa: E402


class YiCliTests(unittest.TestCase):
    """Verify Zephyr-style argument parsing and CMake dispatch."""

    def test_build_parser_accepts_short_board_and_pristine_options(self):
        """Common west-build style flags map to YiCore build settings."""

        parser = _create_parser()
        args = parser.parse_args(
            ["build", "-p", "always", "-b", "board-a", "app"]
        )
        self.assertEqual(args.command, "build")
        self.assertEqual(args.board, "board-a")
        self.assertEqual(args.pristine, "always")
        self.assertEqual(args.source_dir, Path("app"))

    def test_product_creation_commands_use_nested_zephyr_style(self):
        """Board, product, and image creation are exposed only below yi."""

        parser = _create_parser()
        board = parser.parse_args(
            [
                "board",
                "create",
                "product-a",
                "--from-board",
                "fire-mini-stm32f103",
            ]
        )
        product = parser.parse_args(
            ["product", "create", "--board", "product-a"]
        )
        image = parser.parse_args(["image", "add", "bootloader"])

        self.assertEqual(board.board_command, "create")
        self.assertEqual(board.output_root, Path.cwd() / "boards")
        self.assertEqual(product.product_command, "create")
        self.assertEqual(product.product_root, Path.cwd())
        self.assertEqual(image.image_command, "add")
        self.assertEqual(image.image, "bootloader")

    def test_product_create_parser_accepts_interactive_board_selection(self):
        """Product creation may obtain its board from an interactive list."""

        args = _create_parser().parse_args(["product", "create"])

        self.assertIsNone(args.board)

    def test_product_create_interactive_selects_product_local_board(self):
        """Prompt mode resolves a board registered below the product root."""

        with tempfile.TemporaryDirectory() as temporary:
            product = Path(temporary)
            marker = product / "YiCore" / "scripts" / "yi_cli.py"
            board = product / "boards" / "product-board"
            marker.parent.mkdir(parents=True)
            marker.write_text("# marker\n", encoding="utf-8")
            board.mkdir(parents=True)
            (board / "board.json").write_text(
                '{"id":"product-board","name":"Product Board",'
                '"vendor":"st","series":"stm32f1",'
                '"model":"stm32f103xe","description":"Test board"}\n',
                encoding="utf-8",
            )
            (board / "board.dts").write_text(
                "/dts-v1/;\n/ {};\n", encoding="utf-8"
            )

            with (
                patch("yi_cli.sys.stdin.isatty", return_value=True),
                patch("builtins.input", return_value="product-board"),
                patch(
                    "yi_cli.create_application_in_place",
                    return_value=product,
                ) as create,
            ):
                result = create_product_in_place(
                    SCRIPTS_DIR.parent, product, None
                )

        self.assertEqual(result, product)
        self.assertEqual(create.call_args.args[2]["id"], "product-board")

    def test_product_create_noninteractive_requires_board_argument(self):
        """Automation must provide --board instead of waiting for input."""

        with tempfile.TemporaryDirectory() as temporary:
            product = Path(temporary)
            marker = product / "YiCore" / "scripts" / "yi_cli.py"
            marker.parent.mkdir(parents=True)
            marker.write_text("# marker\n", encoding="utf-8")
            with (
                patch("yi_cli.sys.stdin.isatty", return_value=False),
                self.assertRaisesRegex(YiCliError, "requires --board"),
            ):
                create_product_in_place(
                    SCRIPTS_DIR.parent, product, None
                )

    def test_board_create_parser_accepts_interactive_mode(self):
        """Board creation may omit values that an interactive terminal asks."""

        args = _create_parser().parse_args(["board", "create"])

        self.assertIsNone(args.board_id)
        self.assertIsNone(args.from_board)

    def test_board_create_interactive_prompts_for_missing_values(self):
        """Prompt mode selects a target and reference board before creation."""

        parser = _create_parser()
        with tempfile.TemporaryDirectory() as temporary:
            output_root = Path(temporary) / "boards"
            args = parser.parse_args(
                [
                    "board",
                    "create",
                    "--output-root",
                    str(output_root),
                ]
            )
            answers = [
                "st",
                "stm32f103xe",
                "fire-mini-stm32f103",
                "prompt-board",
                "",
                "",
            ]
            with (
                patch("yi_cli.sys.stdin.isatty", return_value=True),
                patch("builtins.input", side_effect=answers),
            ):
                result = create_board_for_product(
                    SCRIPTS_DIR.parent, args
                )

        self.assertEqual(result.name, "prompt-board")
        self.assertEqual(result.parent, output_root)

    def test_board_create_noninteractive_requires_key_arguments(self):
        """Automation fails clearly instead of waiting for hidden prompts."""

        args = _create_parser().parse_args(["board", "create"])
        with (
            patch("yi_cli.sys.stdin.isatty", return_value=False),
            self.assertRaisesRegex(YiCliError, "requires board_id"),
        ):
            create_board_for_product(SCRIPTS_DIR.parent, args)

    def test_board_create_parameter_mode_does_not_prompt(self):
        """Complete command-line arguments preserve automation behavior."""

        parser = _create_parser()
        with tempfile.TemporaryDirectory() as temporary:
            args = parser.parse_args(
                [
                    "board",
                    "create",
                    "parameter-board",
                    "--from-board",
                    "fire-mini-stm32f103",
                    "--output-root",
                    str(Path(temporary) / "boards"),
                ]
            )
            with patch(
                "builtins.input",
                side_effect=AssertionError("unexpected prompt"),
            ):
                result = create_board_for_product(
                    SCRIPTS_DIR.parent, args
                )

        self.assertEqual(result.name, "parameter-board")

    def test_build_invokes_configure_then_compile(self):
        """A valid app produces CMake configure and build commands."""

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            yicore = root / "YiCore"
            board = yicore / "boards" / "board-a"
            toolchain = (
                yicore
                / "scripts"
                / "templates"
                / "mcu-a"
                / "gcc"
                / "arm-none-eabi.cmake"
            )
            board.mkdir(parents=True)
            toolchain.parent.mkdir(parents=True)
            (board / "board.json").write_text(
                '{"model": "mcu-a"}\n', encoding="utf-8"
            )
            toolchain.write_text("# toolchain\n", encoding="utf-8")
            app = create_app("Demo", root)
            ninja = root / "ninja.exe"
            ninja.write_text("", encoding="utf-8")

            with (
                patch("yi_cli._find_ninja", return_value=ninja),
                patch("yi_cli.subprocess.run") as run,
            ):
                result = build_app(yicore, app, "board-a")

            self.assertEqual(result, app / "build")
            self.assertEqual(run.call_count, 2)
            configure = run.call_args_list[0].args[0]
            compile_command = run.call_args_list[1].args[0]
            self.assertIn("-DBOARD=board-a", configure)
            self.assertEqual(
                compile_command, ["cmake", "--build", str(app / "build")]
            )

    def test_build_rejects_non_application(self):
        """The CLI does not configure an arbitrary source directory."""

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with self.assertRaisesRegex(
                YiCliError, "not a YiCore application"
            ):
                build_app(root, root / "missing", "board-a")

    def test_boards_lists_only_platforms_with_build_adapters(self):
        """Reserved families are excluded from buildable board discovery."""

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            ready = root / "boards" / "ready"
            reserved = root / "boards" / "reserved"
            adapters = root / "cmake" / "platforms"
            ready.mkdir(parents=True)
            reserved.mkdir(parents=True)
            adapters.mkdir(parents=True)
            (ready / "board.json").write_text(
                '{"id":"ready","name":"Ready Board",'
                '"vendor":"st","series":"stm32f1"}\n',
                encoding="utf-8",
            )
            (reserved / "board.json").write_text(
                '{"id":"reserved","name":"Reserved Board",'
                '"vendor":"gigadevice","series":"gd32f30x"}\n',
                encoding="utf-8",
            )
            (adapters / "st-stm32f1.cmake").write_text(
                "# ready\n", encoding="utf-8"
            )
            self.assertEqual(
                list_boards(root), ["ready\tReady Board"]
            )

    def test_sdk_commands_expose_registry_state(self):
        """SDK listing and ready-package verification use one registry."""

        yicore = SCRIPTS_DIR.parent
        summaries = sdk_list(yicore)
        self.assertTrue(
            any(line.startswith("st-stm32f1\tready") for line in summaries)
        )
        self.assertTrue(
            any(
                line.startswith("gigadevice-gd32f30x\tpending")
                for line in summaries
            )
        )
        self.assertEqual(sdk_verify(yicore), [])
        self.assertTrue(sdk_verify(yicore, include_pending=True))


if __name__ == "__main__":
    unittest.main()
