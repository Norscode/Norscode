"""AST export + validate command module.

This command is a bridge toward parser parity testing.  It exports a source file
through the current parser pipeline, writes an AST JSON document, and validates
that document against the stable `norscode-ast-v1` contract.
"""

from __future__ import annotations

import json
from pathlib import Path

from norcode.commands.base import CommandModule
from norcode.parser_parity_service import ast_payload_from_current_parser



def register_arguments(parser) -> None:
    parser.add_argument("file", help="Norscode source file")
    parser.add_argument("-o", "--output", help="Output AST JSON file")
    parser.add_argument("--check", action="store_true", help="Valider uten å skrive output")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def run(args) -> int:
    parity = ast_payload_from_current_parser(args.file)

    output_path = Path(args.output).expanduser().resolve() if args.output else Path(args.file).expanduser().resolve().with_suffix(".nast.json")
    if parity.validation.ok and not args.check:
        output_path.write_text(json.dumps(parity.payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    result = {
        "ok": parity.validation.ok,
        "source": str(parity.source_path),
        "output": str(output_path),
        "written": bool(parity.validation.ok and not args.check),
        "errors": parity.validation.errors,
    }

    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    elif parity.validation.ok:
        if args.check:
            print(f"AST export-validert: {parity.source_path}")
        else:
            print(f"AST skrevet og validert: {output_path}")
    else:
        print(f"AST export ugyldig: {parity.source_path}")
        for error in parity.validation.errors:
            print(f"- {error}")

    return 0 if parity.validation.ok else 1


AST_EXPORT_VALIDATE_COMMAND = CommandModule(
    name="ast-export-validate",
    help="Eksporter og valider AST mot norscode-ast-v1",
    register_arguments=register_arguments,
    run=run,
)
