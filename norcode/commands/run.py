"""Run command module.

This is the first command moved to the modular dispatch contract.  It still
calls the legacy compiler implementation, but the command handler now lives
outside `main.py`.
"""

from __future__ import annotations

from norcode.commands.base import CommandModule



def register_arguments(parser) -> None:
    parser.add_argument("file")



def run(args) -> int:
    from main import run_program

    run_program(args.file)
    return 0


RUN_COMMAND = CommandModule(
    name="run",
    help="Bygg og kjør en .no-fil",
    register_arguments=register_arguments,
    run=run,
)
