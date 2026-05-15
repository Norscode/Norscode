"""Run command module.

This command now routes through the compiler service facade instead of
importing `main.py` directly.
"""

from __future__ import annotations

from norcode.commands.base import CommandModule
from norcode.compiler_service import run_program_file



def register_arguments(parser) -> None:
    parser.add_argument("file")



def run(args) -> int:
    run_program_file(args.file)
    return 0


RUN_COMMAND = CommandModule(
    name="run",
    help="Bygg og kjør en .no-fil",
    register_arguments=register_arguments,
    run=run,
)
