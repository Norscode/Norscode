"""Check command module.

The modular CLI now routes semantic analysis through the frontend service
facade instead of directly through the legacy compiler orchestration.
"""

from __future__ import annotations

from norcode.commands.base import CommandModule
from norcode.frontend_service import analyze_source_file



def register_arguments(parser) -> None:
    parser.add_argument("file")
    parser.add_argument("--tokens", action="store_true", help="Vis token-strøm")



def run(args) -> int:
    if args.tokens:
        from norcode.frontend_service import lex_source_file

        for token in lex_source_file(args.file):
            print(f"{token.line}:{token.column} {token.type} {token.value!r}")
        return 0

    result = analyze_source_file(args.file)
    print(f"Kilde: {result.source_path}")
    print(f"Aliaser: {result.alias_map}")
    print("Semantikk: OK")
    print(f"Funksjoner: {result.semantic.function_names}")
    return 0


CHECK_COMMAND = CommandModule(
    name="check",
    help="Parser og valider en .no-fil uten å bygge",
    register_arguments=register_arguments,
    run=run,
)
