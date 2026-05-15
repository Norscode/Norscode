"""Static command registry used during CLI migration.

Today the authoritative argparse implementation still lives in `main.py`.
This registry is the first step toward a modular command system where each
command can later own:

- argument registration
- execution handler
- documentation metadata
- selfhost/runtime compatibility metadata
"""

from __future__ import annotations

from norcode.commands.base import CommandModule
from norcode.commands.bytecode_run import BYTECODE_RUN_COMMAND
from norcode.commands.check import CHECK_COMMAND
from norcode.commands.run import RUN_COMMAND
from norcode.commands.test import TEST_COMMAND


COMMANDS: tuple[CommandModule, ...] = (
    RUN_COMMAND,
    CHECK_COMMAND,
    TEST_COMMAND,
    BYTECODE_RUN_COMMAND,
    CommandModule(
        name="repl",
        help="Start en enkel interaktiv Norscode-REPL",
        register_arguments=lambda parser: None,
    ),
    CommandModule(
        name="build",
        help="Generer C og bygg kjørbar fil",
        register_arguments=lambda parser: parser.add_argument("file"),
        bootstrap_only=True,
    ),
    CommandModule(
        name="ci",
        help="Kjør lokal CI-sekvens (snapshot, parity, test)",
        register_arguments=lambda parser: None,
        bootstrap_only=True,
    ),
    CommandModule(
        name="selfhost-chain-run",
        help="Kjør full selfhost-kjede",
        register_arguments=lambda parser: parser.add_argument("file"),
        experimental=True,
    ),
    CommandModule(
        name="serve",
        help="Start en lokal webserver for en Norscode-app",
        register_arguments=lambda parser: parser.add_argument("file"),
    ),
)



def command_names() -> list[str]:
    return sorted(command.name for command in COMMANDS)



def command_overview() -> list[dict[str, object]]:
    return [
        {
            "name": command.name,
            "help": command.help,
            "bootstrap_only": command.bootstrap_only,
            "experimental": command.experimental,
        }
        for command in sorted(COMMANDS, key=lambda item: item.name)
    ]
