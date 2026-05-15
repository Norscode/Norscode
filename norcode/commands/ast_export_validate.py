"""AST export + validate command module.

This command is a bridge toward parser parity testing.  It exports a source file
through the current parser pipeline, writes an AST JSON document, and validates
that document against the stable `norscode-ast-v1` contract.
"""

from __future__ import annotations

import json
from pathlib import Path

from norcode.ast_validator import validate_ast_payload
from norcode.commands.base import CommandModule
from norcode.parser_service import parse_file



def register_arguments(parser) -> None:
    parser.add_argument("file", help="Norscode source file")
    parser.add_argument("-o", "--output", help="Output AST JSON file")
    parser.add_argument("--check", action="store_true", help="Valider uten å skrive output")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def _minimal_payload_from_program(parse_result) -> dict:
    program = parse_result.program
    functions = []
    for fn in getattr(program, "functions", []) or []:
        functions.append(
            {
                "type": "function",
                "name": getattr(fn, "name", ""),
                "module_name": getattr(fn, "module_name", None) or "__main__",
                "params": [
                    {"name": getattr(param, "name", ""), "type": str(getattr(param, "type", ""))}
                    for param in getattr(fn, "params", []) or []
                ],
                "return_type": str(getattr(fn, "return_type", "")),
                "body": {"type": "block", "statements": []},
            }
        )
    imports = [
        {"module_name": getattr(item, "module_name", ""), "alias": getattr(item, "alias", None)}
        for item in getattr(program, "imports", []) or []
    ]
    return {
        "format": "norscode-ast-v1",
        "alias_map": {},
        "imports": imports,
        "functions": functions,
    }



def run(args) -> int:
    parse_result = parse_file(args.file)
    payload = _minimal_payload_from_program(parse_result)
    validation = validate_ast_payload(payload)

    output_path = Path(args.output).expanduser().resolve() if args.output else Path(args.file).expanduser().resolve().with_suffix(".nast.json")
    if validation.ok and not args.check:
        output_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    result = {
        "ok": validation.ok,
        "source": str(parse_result.source_path),
        "output": str(output_path),
        "written": bool(validation.ok and not args.check),
        "errors": validation.errors,
    }

    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    elif validation.ok:
        if args.check:
            print(f"AST export-validert: {parse_result.source_path}")
        else:
            print(f"AST skrevet og validert: {output_path}")
    else:
        print(f"AST export ugyldig: {parse_result.source_path}")
        for error in validation.errors:
            print(f"- {error}")

    return 0 if validation.ok else 1


AST_EXPORT_VALIDATE_COMMAND = CommandModule(
    name="ast-export-validate",
    help="Eksporter og valider AST mot norscode-ast-v1",
    register_arguments=register_arguments,
    run=run,
)
