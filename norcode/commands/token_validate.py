"""Validate token streams against TOKEN_FORMAT_V1."""

from __future__ import annotations

import json
from pathlib import Path

from norcode.commands.base import CommandModule
from norcode.token_validator import validate_token_stream



def register_arguments(parser) -> None:
    parser.add_argument("file", help="JSON token file")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def run(args) -> int:
    token_path = Path(args.file).expanduser().resolve()

    try:
        payload = json.loads(token_path.read_text(encoding="utf-8"))
    except Exception as exc:
        result = {
            "ok": False,
            "file": str(token_path),
            "errors": [f"kunne ikke lese JSON: {exc}"],
        }
        if args.json:
            print(json.dumps(result, ensure_ascii=False, indent=2))
        else:
            print(f"Token validation FEIL: {token_path}")
            print(result["errors"][0])
        return 1

    validation = validate_token_stream(payload)

    result = {
        "ok": validation.ok,
        "file": str(token_path),
        "errors": validation.errors,
    }

    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    elif validation.ok:
        print(f"Token validation OK: {token_path}")
    else:
        print(f"Token validation FEIL: {token_path}")
        for error in validation.errors:
            print(f"- {error}")

    return 0 if validation.ok else 1


TOKEN_VALIDATE_COMMAND = CommandModule(
    name="token-validate",
    help="Valider token-stream mot TOKEN_FORMAT_V1",
    register_arguments=register_arguments,
    run=run,
)
