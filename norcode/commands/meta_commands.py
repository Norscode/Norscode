"""Meta commands like REPL and command overview."""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.repl import run_repl


def register_repl_arguments(parser) -> None:
    return None


def run_repl_command(args) -> int:
    run_repl()
    return 0


def register_commands_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv kommandooversikt som JSON")


def run_commands(args) -> int:
    from norcode.commands.registry import command_overview

    payload = {
        "prog": "norcode",
        "commands": command_overview(),
    }
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"CLI: {payload['prog']}")
        for row in payload["commands"]:
            print(f"- {row['name']}: {row['help']}")
    return 0


REPL_COMMAND = CommandModule(
    name="repl",
    help="Start en enkel interaktiv Norscode-REPL",
    register_arguments=register_repl_arguments,
    run=run_repl_command,
)

COMMANDS_COMMAND = CommandModule(
    name="commands",
    help="Vis stabil kommandooversikt",
    register_arguments=register_commands_arguments,
    run=run_commands,
)
