"""Registry and argparse coverage for selfhost/runtime modular CLI commands."""

from __future__ import annotations

from norcode.cli_parser import build_parser
from norcode.commands.registry import COMMANDS, command_names


BOOTSTRAP_NATIVE_COMMANDS = {
    "bootstrap-compiler-verify",
    "ci",
    "native-build",
    "native-run",
    "registry-host",
    "selfhost-bootstrap-gate",
    "selfhost-compile-all",
}

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
    assert BOOTSTRAP_NATIVE_COMMANDS <= names



def test_selfhost_runtime_commands_have_run_handlers() -> None:
    command_by_name = {command.name: command for command in COMMANDS}

    for name in REQUIRED_COMMANDS:
        assert command_by_name[name].run is not None



def test_selfhost_runtime_commands_are_parseable_by_modular_cli() -> None:
    parser = build_parser()

    cases = [
        ["runtime-call", "tests/selfhost_lexer_token_smoke.no", "start"],
        ["ci", "--bootstrap-lane", "--bootstrap-output-dir", "build/selfhost-bootstrap-gate-test"],
        ["native-build", "tests/test_empty_int_list.no"],
        ["native-run", "tests/test_empty_int_list.no"],
        ["bootstrap-compiler-verify"],
        ["selfhost-bootstrap-gate", "--output-dir", "build/selfhost-bootstrap-gate-test", "--no-determinism"],
        ["registry-host", "--once"],
        ["selfhost-compile-all", "--root", "std", "--output-dir", "build/selfhost-whole-test"],
        ["selfhost-lexer-token-smoke"],
        ["selfhost-lexer-list-smoke"],
        ["selfhost-lexer-run", "tests/test_empty_list_return.no"],
        ["selfhost-lexer-suite", "--skip-runtime"],
    ]

    for argv in cases:
        args = parser.parse_args(argv)
        assert args.command_module.name == argv[0]
        if argv[0] in BOOTSTRAP_NATIVE_COMMANDS:
            assert args.command_module.bootstrap_only is True
        else:
            assert args.command_module.run is not None
