"""Check command module.

The semantic analysis still uses the legacy compiler implementation, but it is
now accessed through the compiler service facade.
"""

from __future__ import annotations

from norcode.commands.base import CommandModule
from norcode.compiler_service import check_program_file



def register_arguments(parser) -> None:
    parser.add_argument("file")



def run(args) -> int:
    source_path, _program, alias_map, analyzer = check_program_file(args.file)
    print(f"Kilde: {source_path}")
    print(f"Aliaser: {alias_map}")
    print("Semantikk: OK")
    print(f"Funksjoner: {list(analyzer.functions.keys())}")
    return 0


CHECK_COMMAND = CommandModule(
    name="check",
    help="Parser og valider en .no-fil uten å bygge",
    register_arguments=register_arguments,
    run=run,
)
