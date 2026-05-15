"""Static command registry used during CLI migration.

Today the authoritative argparse implementation still lives in `main.py`.
This registry is the first step toward a modular command system where each
command can later own:

- argument registration
- execution handler
- documentation metadata
- selfhost/runtime compatibility metadata

The registry intentionally starts as metadata-only so it can be introduced
without changing runtime behavior.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class CommandSpec:
    name: str
    help: str
    bootstrap_only: bool = False
    experimental: bool = False


COMMANDS: tuple[CommandSpec, ...] = (
    CommandSpec("run", "Bygg og kjør en .no-fil"),
    CommandSpec("repl", "Start en enkel interaktiv Norscode-REPL"),
    CommandSpec("check", "Parser og valider en .no-fil uten å bygge"),
    CommandSpec("build", "Generer C og bygg kjørbar fil", bootstrap_only=True),
    CommandSpec("test", "Kjør én testfil eller alle i tests/"),
    CommandSpec("ci", "Kjør lokal CI-sekvens (snapshot, parity, test)", bootstrap_only=True),
    CommandSpec("selfhost-chain-run", "Kjør full selfhost-kjede", experimental=True),
    CommandSpec("bytecode-run", "Kjør bytecode-backenden"),
    CommandSpec("serve", "Start en lokal webserver for en Norscode-app"),
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
