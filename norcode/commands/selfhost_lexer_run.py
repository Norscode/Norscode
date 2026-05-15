"""Selfhost lexer runner command.

This command is the future execution entrypoint for the Norscode-native lexer.
Initially it reports readiness/integration status until the runtime ABI is
fully connected.
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
        "errors": result.errors,
    }

    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        if result.ready:
            print("Selfhost lexer er klar for runtime-integrasjon")
        else:
            print("Selfhost lexer er ikke klar")

        for item in result.errors:
            print(f"- {item}")

    return 0 if result.ok else 1


SELFHOST_LEXER_RUN_COMMAND = CommandModule(
    name="selfhost-lexer-run",
    help="Kjør readiness/runtime-grense for selfhost lexer",
    register_arguments=register_arguments,
    run=run,
)
