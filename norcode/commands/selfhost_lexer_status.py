"""Selfhost lexer readiness command.

Reports whether the Norscode-native lexer contains the required M1 scanning
building blocks needed before parity/runtime integration.
"""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.selfhost_lexer_service import check_selfhost_lexer



def register_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv status som JSON")



def run(args) -> int:
    status = check_selfhost_lexer()

    result = {
        "ok": status.ok,
        "exists": status.exists,
        "path": str(status.path),
        "missing_functions": status.missing_functions,
        "missing_token_markers": status.missing_token_markers,
    }

    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    elif status.ok:
        print(f"Selfhost lexer M1 OK: {status.path}")
    else:
        print(f"Selfhost lexer M1 MANGLER DELER: {status.path}")

        hvis_funksjoner = len(status.missing_functions) > 0
        hvis_tokens = len(status.missing_token_markers) > 0

        if hvis_funksjoner:
            print("Manglende funksjoner:")
            for item in status.missing_functions:
                print(f"- {item}")

        if hvis_tokens:
            print("Manglende token-markers:")
            for item in status.missing_token_markers:
                print(f"- {item}")

    return 0 if status.ok else 1


SELFHOST_LEXER_STATUS_COMMAND = CommandModule(
    name="selfhost-lexer-status",
    help="Sjekk status for Norscode-native lexer M1",
    register_arguments=register_arguments,
    run=run,
)
