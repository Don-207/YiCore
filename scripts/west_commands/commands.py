"""Provide Zephyr-style west boards and west build commands for YiCore.

Author: Don
Date: 2026-08-02
Version: 1.0.0
"""

from __future__ import annotations

import sys
from pathlib import Path

from west.commands import WestCommand


YICORE_ROOT = Path(__file__).resolve().parents[2]
SCRIPTS_ROOT = YICORE_ROOT / "scripts"
if str(SCRIPTS_ROOT) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_ROOT))

from yi_cli import YiCliError, build_app, build_product, list_boards  # noqa: E402
from yi_runner import (  # noqa: E402
    RunnerError,
    default_runner,
    load_board,
    run_runner,
    runner_command,
)
from yi_sysbuild import SysbuildError, load_plan  # noqa: E402
from yi_test import TestManifestError, discover_tests  # noqa: E402


class Boards(WestCommand):
    """List buildable YiCore boards using the standard west command name."""

    def __init__(self) -> None:
        """Initialize the read-only boards command."""

        super().__init__(
            "boards",
            "list boards supported by YiCore",
            "List boards that have a YiCore platform build adapter.",
        )

    def do_add_parser(self, parser_adder):
        """Register the boards command parser without extra arguments."""

        return parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )

    def do_run(self, args, unknown_args) -> None:
        """Print stable board identifiers and display names."""

        del args, unknown_args
        for summary in list_boards(YICORE_ROOT):
            self.inf(summary)


class Build(WestCommand):
    """Configure and compile a YiCore product or thin application."""

    def __init__(self) -> None:
        """Initialize the Zephyr-compatible build command."""

        super().__init__(
            "build",
            "configure and build a YiCore application",
            "Configure and build a YiCore application with CMake and Ninja.",
            accepts_unknown_args=False,
        )

    def do_add_parser(self, parser_adder):
        """Register the supported west build argument subset."""

        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )
        parser.add_argument("source_dir", nargs="?", type=Path, default=Path.cwd())
        parser.add_argument("-b", "--board")
        parser.add_argument("-d", "--build-dir", type=Path)
        parser.add_argument(
            "-p",
            "--pristine",
            nargs="?",
            const="always",
            default="auto",
            choices=("auto", "always", "never"),
        )
        parser.add_argument("--toolchain", type=Path)
        parser.add_argument("--generator", default="Ninja")
        parser.add_argument(
            "--image",
            choices=("application", "bootloader", "test"),
            default="application",
        )
        return parser

    def do_run(self, args, unknown_args) -> None:
        """Dispatch to the shared YiCore build implementation."""

        del unknown_args
        source_dir = args.source_dir.resolve()
        product_cmake = (
            source_dir / "firmware" / "projects" / "gcc" / "CMakeLists.txt"
        )
        try:
            if product_cmake.is_file():
                result = build_product(
                    source_dir,
                    args.image,
                    args.build_dir,
                    args.pristine,
                    args.generator,
                    args.board,
                )
            else:
                if args.board is None:
                    raise YiCliError("thin application build requires --board")
                result = build_app(
                    YICORE_ROOT,
                    source_dir,
                    args.board,
                    args.build_dir,
                    args.pristine,
                    args.toolchain,
                    args.generator,
                )
        except YiCliError as error:
            self.die(str(error))
        self.inf(str(result))


class _RunnerCommand(WestCommand):
    """Share argument handling for flash and debug runner commands."""

    action = ""

    def do_add_parser(self, parser_adder):
        """Register common board runner arguments."""

        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )
        parser.add_argument("-b", "--board", required=True)
        parser.add_argument("-d", "--build-dir", type=Path)
        parser.add_argument("--board-root", type=Path)
        parser.add_argument("-r", "--runner")
        parser.add_argument("--dry-run", action="store_true")
        return parser

    def do_run(self, args, unknown_args) -> None:
        """Resolve board metadata and execute its selected runner."""

        del unknown_args
        product_root = Path.cwd()
        board_root = args.board_root or (
            product_root / "boards"
            if (product_root / "boards").is_dir()
            else YICORE_ROOT / "boards"
        )
        build_dir = args.build_dir or product_root / "build" / "application"
        try:
            board = load_board(board_root, args.board)
            command = runner_command(
                self.action, args.runner or default_runner(board), board,
                build_dir.resolve(),
            )
            self.inf(" ".join(str(item) for item in command))
            if not args.dry_run:
                run_runner(command)
        except RunnerError as error:
            self.die(str(error))


class Flash(_RunnerCommand):
    """Program a YiCore image with the selected board runner."""

    action = "flash"

    def __init__(self) -> None:
        """Initialize the flash command."""

        super().__init__("flash", "flash a YiCore image", "Program a built YiCore image.")


class Debug(_RunnerCommand):
    """Start a one-shot debug server for a YiCore image."""

    action = "debug"

    def __init__(self) -> None:
        """Initialize the debug command."""

        super().__init__("debug", "debug a YiCore image", "Start a board debug session.")


class DebugServer(_RunnerCommand):
    """Start a persistent debug server for a YiCore board."""

    action = "debugserver"

    def __init__(self) -> None:
        """Initialize the debug-server command."""

        super().__init__(
            "debugserver", "start a YiCore debug server",
            "Start a board debug server without a debugger client.",
        )


class Test(WestCommand):
    """Discover Twister-style YiCore test scenarios."""

    def __init__(self) -> None:
        """Initialize the test discovery command."""

        super().__init__("test", "discover YiCore tests", "List testcase.yaml scenarios.")

    def do_add_parser(self, parser_adder):
        """Register test discovery arguments."""

        parser = parser_adder.add_parser(self.name, help=self.help)
        parser.add_argument("root", nargs="?", type=Path, default=Path.cwd())
        parser.add_argument("-p", "--platform")
        return parser

    def do_run(self, args, unknown_args) -> None:
        """Print scenarios matching an optional platform."""

        del unknown_args
        try:
            scenarios = discover_tests(args.root.resolve())
        except TestManifestError as error:
            self.die(str(error))
        for scenario in scenarios:
            allowed = scenario["platform_allow"]
            if args.platform and allowed and args.platform not in allowed:
                continue
            self.inf(f"{scenario['name']}\t{scenario['path']}")


class Sysbuild(WestCommand):
    """Inspect a bare-metal multi-image build graph."""

    def __init__(self) -> None:
        """Initialize the sysbuild planning command."""

        super().__init__("sysbuild", "plan YiCore images", "Validate and print sysbuild.yml.")

    def do_add_parser(self, parser_adder):
        """Register the product root argument."""

        parser = parser_adder.add_parser(self.name, help=self.help)
        parser.add_argument("product_root", nargs="?", type=Path, default=Path.cwd())
        return parser

    def do_run(self, args, unknown_args) -> None:
        """Print images in dependency order."""

        del unknown_args
        try:
            plan = load_plan(args.product_root.resolve())
        except SysbuildError as error:
            self.die(str(error))
        for image in plan:
            self.inf(f"{image['name']}\t{image['path']}")
