"""Format a `.no` file."""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.formatting import format_program_file


def register_arguments(parser) -> None:
    parser.add_argument("file", help="Kildefil å formatere")
    parser.add_argument("--check", action="store_true", help="Feil hvis filen ikke er formatert")
    parser.add_argument("--diff", action="store_true", help="Vis diff uten å skrive filen")
    parser.add_argument("--json", action="store_true", help="Skriv format-resultat som JSON")


def run(args) -> int:
    result = format_program_file(args.file, check=args.check, diff=args.diff)
    if args.json:
        payload = {
            "mode": "single",
            "result": result,
            "summary": {
                "source": result["source"],
                "changed": result["changed"],
                "written": result["written"],
            },
        }
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        if result["changed"]:
            if args.check:
                print(f"Uformatert: {result['source']}")
            elif not args.diff:
                print(f"Formatert: {result['source']}")
        else:
            print(f"Allerede formatert: {result['source']}")
    if args.check and result["changed"]:
        return 1
    return 0


FORMAT_COMMAND = CommandModule(
    name="format",
    help="Formater en .no-fil",
    register_arguments=register_arguments,
    run=run,
)
