"""Registry and argparse coverage for selfhost/runtime modular CLI commands."""

from __future__ import annotations

from norcode.cli_parser import build_parser
from norcode.commands.registry import COMMANDS, command_names


REQUIRED_COMMANDS = {
    "runtime-call",
    "selfhost-lexer-token-smoke",
    "selfhost-lexer-list-smoke",
    "selfhost-lexer-run",
    "selfhost-lexer-suite",
}



def test_selfhost_runtime_commands_are_registered() -> None:
    names = set(command_names())
    assert REQUIRED_COMMANDS <= names



def test_selfhost_runtime_commands_have_run_handlers() -> None:
    command_by_name = {command.name: command for command in COMMANDS}

    for name in REQUIRED_COMMANDS:
        assert command_by_name[name].run is not None



def test_selfhost_runtime_commands_are_parseable_by_modular_cli() -> None:
    parser = build_parser()

    cases = [
        ["runtime-call", "tests/selfhost_lexer_token_smoke.no", "start"],
        ["selfhost-lexer-token-smoke"],
        ["selfhost-lexer-list-smoke"],
        ["selfhost-lexer-run", "tests/test_empty_list_return.no"],
        ["selfhost-lexer-suite", "--skip-runtime"],
    ]

    for argv in cases:
        args = parser.parse_args(argv)
        assert args.command_module.name == argv[0]
        assert args.command_module.run is not None
