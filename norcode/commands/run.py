"""Run command module.

In the modular CLI, `run` is now bytecode-first by default and routes through
`compiler_core` instead of directly orchestrating bytecode compilation.
"""

from __future__ import annotations

from norcode.commands.base import CommandModule
from norcode.compiler_core import run_source
from norcode.compiler_service import run_program_file
from norcode.runtime_service import RuntimeOptions



def register_arguments(parser) -> None:
    parser.add_argument("file")
    parser.add_argument("--legacy", action="store_true", help="Bruk legacy Python/C-kjørevei")
    parser.add_argument("--trace", action="store_true", help="Vis VM-sporing")
    parser.add_argument("--max-steps", type=int, default=5_000_000, help="Maks VM-steg før kjøring avbrytes")



def run(args) -> int:
    if args.legacy:
        run_program_file(args.file)
        return 0

    options = RuntimeOptions(trace=args.trace, max_steps=args.max_steps)
    run_source(args.file, options=options)
    return 0


RUN_COMMAND = CommandModule(
    name="run",
    help="Bygg og kjør en .no-fil",
    register_arguments=register_arguments,
    run=run,
)
