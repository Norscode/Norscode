"""Selfhost lexer runner command.

This command is the execution entrypoint for the Norscode-native lexer through
the runtime ABI.  It reports function discovery and token validation diagnostics
so execution failures can be debugged directly from the CLI.
"""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.selfhost_lexer_runner import run_selfhost_lexer



def register_arguments(parser) -> None:
    parser.add_argument("file", help="Norscode source file")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def run(args) -> int:
    result = run_selfhost_lexer(args.file)

    payload = {
        "ok": result.ok,
        "ready": result.ready,
        "lexer_path": str(result.lexer_path),
        "source_path": str(result.source_path) if result.source_path else None,
        "token_count": len(result.tokens),
        "called_function": result.called_function,
        "available_functions": result.available_functions,
        "errors": result.errors,
        "validation_errors": result.validation_errors,
    }

    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        if result.ok:
            print("Selfhost lexer runtime OK")
        elif result.ready:
            print("Selfhost lexer er klar, men runtime-kjøring feilet")
        else:
            print("Selfhost lexer er ikke klar")

        print(f"Lexer: {result.lexer_path}")
        if result.called_function:
            print(f"Kalt funksjon: {result.called_function}")
        if result.available_functions:
            print("Tilgjengelige funksjoner:")
            for name in result.available_functions:
                print(f"- {name}")
        print(f"Tokens: {len(result.tokens)}")

        for item in result.errors:
            print(f"- runtime: {item}")
        for item in result.validation_errors:
            print(f"- validation: {item}")

    return 0 if result.ok else 1


SELFHOST_LEXER_RUN_COMMAND = CommandModule(
    name="selfhost-lexer-run",
    help="Kjør selfhost lexer gjennom runtime ABI",
    register_arguments=register_arguments,
    run=run,
)
