"""Argparse construction for the modular Norscode CLI.

This module is introduced as a migration bridge.  The authoritative CLI is still
assembled in `main.py`, but new command modules should be wired through this
builder first.  Once command handlers have moved out of `main.py`, `norcode.cli`
can call this parser directly.
"""

from __future__ import annotations

import argparse
from collections.abc import Iterable

from norcode.commands.base import CommandModule
from norcode.commands.registry import COMMANDS



def build_parser(commands: Iterable[CommandModule] = COMMANDS) -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="norcode", description="Norscode CLI")
    subparsers = parser.add_subparsers(dest="cmd")

    for command in sorted(commands, key=lambda item: item.name):
        subparser = subparsers.add_parser(command.name, help=command.help)
        command.register_arguments(subparser)
        subparser.set_defaults(command_module=command)

    return parser



def parser_command_overview(parser: argparse.ArgumentParser) -> list[dict[str, str]]:
    overview: list[dict[str, str]] = []
    for action in parser._actions:
        choices = getattr(action, "choices", None)
        if not choices:
            continue
        for name, subparser in choices.items():
            overview.append({"name": str(name), "help": getattr(subparser, "description", None) or ""})
    return sorted(overview, key=lambda row: row["name"])
