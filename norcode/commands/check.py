"""Check command module.

The actual semantic analysis still runs through the legacy Python compiler
pipeline.  This module prepares the CLI for future self-hosted routing.
"""

from __future__ import annotations

from norcode.commands.base import CommandModule



def register_arguments(parser) -> None:
    parser.add_argument("file")


CHECK_COMMAND = CommandModule(
    name="check",
    help="Parser og valider en .no-fil uten å bygge",
    register_arguments=register_arguments,
)
