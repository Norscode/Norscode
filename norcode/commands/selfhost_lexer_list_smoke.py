"""Selfhost lexer token list smoke command.

Runs a minimal TOKEN_FORMAT_V1 list-return smoke test through the runtime ABI.
This isolates BUILD_LIST, dictionary literals, builtin.legg_til, object-in-list
transport and VM list return handling before running the full selfhost lexer.
"""

from __future__ import annotations

import json
from pathlib import Path

from norcode.commands.base import CommandModule
from norcode.runtime_call_service import call_function
from norcode.token_validator import validate_token_stream


DEFAULT_SMOKE_FILE = Path("tests/selfhost_lexer_list_smoke.no")



def register_arguments(parser) -> None:
    parser.add_argument("--file", default=str(DEFAULT_SMOKE_FILE), help="List smoke-test .no-fil")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def run(args) -> int:
    source_path = Path(args.file).expanduser().resolve()
    result = call_function(str(source_path), "start", [])
    tokens = result.value if isinstance(result.value, list) else []
    validation = validate_token_stream(tokens)

    payload = {
        "ok": result.ok and isinstance(result.value, list) and validation.ok,
        "source_path": str(source_path),
        "runtime_ok": result.ok,
        "called_function": result.function_name if result.ok else None,
        "candidate_functions": result.candidate_functions,
        "available_functions": result.available_functions,
        "value": result.value,
        "list_object_ok": isinstance(result.value, list),
        "token_count": len(tokens),
        "validation_ok": validation.ok,
        "validation_errors": validation.errors,
        "errors": result.errors,
    }

    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"Selfhost lexer list smoke: {'OK' if payload['ok'] else 'FEIL'}")
        print(f"Source: {source_path}")
        if result.ok:
            print(f"Called: {result.function_name}")
        if result.candidate_functions:
            print("Candidate functions:")
            for name in result.candidate_functions:
                print(f"- {name}")
        if result.available_functions:
            print("Available functions:")
            for name in result.available_functions:
                print(f"- {name}")
        print(f"List object: {'OK' if isinstance(result.value, list) else 'FEIL'}")
        print(f"Tokens: {len(tokens)}")
        print(f"Validation: {'OK' if validation.ok else 'FEIL'}")
        for error in result.errors:
            print(f"- runtime: {error}")
        for error in validation.errors:
            print(f"- validation: {error}")

    return 0 if payload["ok"] else 1


SELFHOST_LEXER_LIST_SMOKE_COMMAND = CommandModule(
    name="selfhost-lexer-list-smoke",
    help="Kjør minimal token-list smoke-test gjennom runtime ABI",
    register_arguments=register_arguments,
    run=run,
)
