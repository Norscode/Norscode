"""AST validate command module.

Validates `.nast.json` or selfhost AST JSON files against the stable
`norscode-ast-v1` contract.
"""

from __future__ import annotations

import json
from pathlib import Path

from norcode.ast_validator import validate_ast_payload
from norcode.commands.base import CommandModule



def register_arguments(parser) -> None:
    parser.add_argument("file", help="AST JSON file to validate")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def run(args) -> int:
    path = Path(args.file).expanduser().resolve()
    payload = json.loads(path.read_text(encoding="utf-8"))
    result = validate_ast_payload(payload)

    if args.json:
        print(json.dumps({"ok": result.ok, "errors": result.errors, "file": str(path)}, ensure_ascii=False, indent=2))
    elif result.ok:
        print(f"AST OK: {path}")
    else:
        print(f"AST ugyldig: {path}")
        for error in result.errors:
            print(f"- {error}")

    return 0 if result.ok else 1


AST_VALIDATE_COMMAND = CommandModule(
    name="ast-validate",
    help="Valider AST JSON mot norscode-ast-v1",
    register_arguments=register_arguments,
    run=run,
)
