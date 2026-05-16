"""Parser parity fixture command.

Generates or checks AST v1 fixture files from the current parser.  These
fixtures are the baseline future Norscode-native parsers must match.
"""

from __future__ import annotations

import json
from pathlib import Path

from norcode.commands.base import CommandModule
from norcode.parser_parity_service import ast_payload_from_current_parser



def register_arguments(parser) -> None:
    parser.add_argument("file", help="Norscode source file")
    parser.add_argument("-o", "--output", help="Fixture output path")
    parser.add_argument("--check", action="store_true", help="Feil hvis fixture mangler eller er utdatert")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def run(args) -> int:
    parity = ast_payload_from_current_parser(args.file)
    output_path = Path(args.output).expanduser().resolve() if args.output else Path(args.file).expanduser().resolve().with_suffix(".parity.nast.json")
    expected_text = json.dumps(parity.payload, ensure_ascii=False, indent=2) + "\n"

    existing_text = output_path.read_text(encoding="utf-8") if output_path.exists() else None
    up_to_date = existing_text == expected_text
    written = False

    if parity.validation.ok and not args.check and not up_to_date:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(expected_text, encoding="utf-8")
        written = True
        up_to_date = True

    ok = parity.validation.ok and (up_to_date if args.check else True)
    result = {
        "ok": ok,
        "source": str(parity.source_path),
        "fixture": str(output_path),
        "written": written,
        "up_to_date": up_to_date,
        "errors": parity.validation.errors,
    }

    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    elif not parity.validation.ok:
        print(f"Parser fixture ugyldig: {parity.source_path}")
        for error in parity.validation.errors:
            print(f"- {error}")
    elif args.check and not up_to_date:
        print(f"Parser fixture er utdatert eller mangler: {output_path}")
    elif written:
        print(f"Parser fixture skrevet: {output_path}")
    else:
        print(f"Parser fixture OK: {output_path}")

    return 0 if ok else 1


PARSER_PARITY_FIXTURE_COMMAND = CommandModule(
    name="parser-parity-fixture",
    help="Generer eller sjekk parser parity AST-fixture",
    register_arguments=register_arguments,
    run=run,
)
