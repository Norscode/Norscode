"""Runtime call command.

Directly invokes a named Norscode function through the runtime ABI.  This is a
low-level execution debugging tool used by selfhost compiler components.
"""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.runtime_call_service import call_function



def register_arguments(parser) -> None:
    parser.add_argument("file", help="Norscode source file to compile")
    parser.add_argument("function", help="Function name to call, for example lex")
    parser.add_argument("args", nargs="*", help="String arguments passed to the function")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def run(args) -> int:
    result = call_function(args.file, args.function, list(args.args))

    payload = {
        "ok": result.ok,
        "source_path": str(result.source_path),
        "requested_function": args.function,
        "called_function": result.function_name if result.ok else None,
        "candidate_functions": result.candidate_functions,
        "available_functions": result.available_functions,
        "args": result.args,
        "value": result.value,
        "errors": result.errors,
    }

    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"Runtime call: {'OK' if result.ok else 'FEIL'}")
        print(f"Source: {result.source_path}")
        print(f"Requested: {args.function}")
        if result.ok:
            print(f"Called: {result.function_name}")
            print(f"Value: {result.value}")
        if result.candidate_functions:
            print("Candidate functions:")
            for name in result.candidate_functions:
                print(f"- {name}")
        if result.available_functions:
            print("Available functions:")
            for name in result.available_functions:
                print(f"- {name}")
        for error in result.errors:
            print(f"- {error}")

    return 0 if result.ok else 1


RUNTIME_CALL_COMMAND = CommandModule(
    name="runtime-call",
    help="Kall en Norscode-funksjon gjennom runtime ABI",
    register_arguments=register_arguments,
    run=run,
)
