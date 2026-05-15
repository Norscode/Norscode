"""Lexer parity fixture command.

Creates or checks token-stream fixtures from the current Python lexer.  Future
Norscode-native lexers must match these fixtures.
"""

from __future__ import annotations

import json
from pathlib import Path

from norcode.commands.base import CommandModule
from norcode.lexer_parity_service import tokens_from_current_lexer_file



def register_arguments(parser) -> None:
    parser.add_argument("file", help="Norscode source file")
    parser.add_argument("-o", "--output", help="Fixture output path")
    parser.add_argument("--check", action="store_true", help="Feil hvis fixture mangler eller er utdatert")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def run(args) -> int:
    parity = tokens_from_current_lexer_file(args.file)

    output_path = Path(args.output).expanduser().resolve() if args.output else Path(args.file).expanduser().resolve().with_suffix(".tokens.json")

    expected_text = json.dumps(parity.tokens, ensure_ascii=False, indent=2) + "\n"
    existing_text = output_path.read_text(encoding="utf-8") if output_path.exists() else None

    up_to_date = existing_text == expected_text
    written = False

    if not args.check and not up_to_date:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(expected_text, encoding="utf-8")
        written = True
        up_to_date = True

    ok = up_to_date if args.check else True

    result = {
        "ok": ok,
        "source": str(parity.source_path),
        "fixture": str(output_path),
        "written": written,
        "up_to_date": up_to_date,
        "token_count": len(parity.tokens),
    }

    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    elif args.check and not up_to_date:
        print(f"Lexer fixture er utdatert eller mangler: {output_path}")
    elif written:
        print(f"Lexer fixture skrevet: {output_path}")
    else:
        print(f"Lexer fixture OK: {output_path}")

    return 0 if ok else 1


LEXER_PARITY_FIXTURE_COMMAND = CommandModule(
    name="lexer-parity-fixture",
    help="Generer eller sjekk lexer parity fixture",
    register_arguments=register_arguments,
    run=run,
)
