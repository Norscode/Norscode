"""Registry coverage for selfhost/runtime modular CLI commands."""

from __future__ import annotations

from norcode.commands.registry import COMMANDS, command_names


REQUIRED_COMMANDS = {
    "runtime-call",
    "selfhost-lexer-token-smoke",
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
