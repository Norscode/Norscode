"""Selfhost lexer compile check command.

Compiles the Norscode-native lexer through compiler_core without executing it.
This is the first smoke gate before runtime parity can pass.
"""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.compiler_core import compile_source
from norcode.selfhost_lexer_service import DEFAULT_SELFHOST_LEXER, check_selfhost_lexer



def register_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def run(args) -> int:
    status = check_selfhost_lexer()
    errors: list[str] = []
    bytecode_functions: list[str] = []

    if not status.ok:
        for item in status.missing_functions:
            errors.append(f"missing function: {item}")
        for item in status.missing_token_markers:
            errors.append(f"missing token marker: {item}")
    else:
        try:
            compile_result = compile_source(str(DEFAULT_SELFHOST_LEXER))
            functions = compile_result.bytecode.get("functions", {})
            if isinstance(functions, dict):
                bytecode_functions = sorted(str(name) for name in functions.keys())
        except Exception as exc:
            errors.append(str(exc))

    ok = status.ok and not errors
    payload = {
        "ok": ok,
        "lexer_path": str(status.path),
        "readiness_ok": status.ok,
        "bytecode_functions": bytecode_functions,
        "errors": errors,
    }

    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    elif ok:
        print(f"Selfhost lexer compile OK: {status.path}")
        print("Funksjoner i bytecode:")
        for name in bytecode_functions:
            print(f"- {name}")
    else:
        print(f"Selfhost lexer compile FEIL: {status.path}")
        for error in errors:
            print(f"- {error}")

    return 0 if ok else 1


SELFHOST_LEXER_COMPILE_CHECK_COMMAND = CommandModule(
    name="selfhost-lexer-compile-check",
    help="Kompiler selfhost lexer som smoke-test",
    register_arguments=register_arguments,
    run=run,
)
