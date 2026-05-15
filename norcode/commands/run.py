"""Run command module.

This command is still executed by the legacy bootstrap runtime.  The goal of
this module is to isolate command metadata and argument registration from the
monolithic argparse setup in `main.py`.
"""

from __future__ import annotations

from norcode.commands.base import CommandModule



def register_arguments(parser) -> None:
    parser.add_argument("file")


RUN_COMMAND = CommandModule(
    name="run",
    help="Bygg og kjør en .no-fil",
    register_arguments=register_arguments,
)
