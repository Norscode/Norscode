"""Lexer parity check command.

Compares the current lexer token stream with an existing token fixture.  This is
used as a regression gate today and becomes the comparison point for a future
Norscode-native lexer.
"""

from __future__ import annotations

import json
from pathlib import Path

from norcode.commands.base import CommandModule
from norcode.lexer_parity_service import tokens_from_current_lexer_file



def register_arguments(parser) -> None:
    parser.add_argument("file", help="Norscode source file")
    parser.add_argument("--fixture", help="Token fixture path, default: <source>.tokens.json")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def run(args) -> int:
    parity = tokens_from_current_lexer_file(args.file)
    fixture_path = Path(args.fixture).expanduser().resolve() if args.fixture else Path(args.file).expanduser().resolve().with_suffix(".tokens.json")

    if not fixture_path.exists():
        result = {
            "ok": False,
            "source": str(parity.source_path),
            "fixture": str(fixture_path),
            "error": "fixture mangler",
        }
        if args.json:
            print(json.dumps(result, ensure_ascii=False, indent=2))
        else:
            print(f"Lexer fixture mangler: {fixture_path}")
        return 1

    expected = json.loads(fixture_path.read_text(encoding="utf-8"))
    actual = parity.tokens
    ok = expected == actual

    result = {
        "ok": ok,
        "source": str(parity.source_path),
        "fixture": str(fixture_path),
        "expected_count": len(expected) if isinstance(expected, list) else None,
        "actual_count": len(actual),
    }

    if not ok:
        result["error"] = "token stream avviker fra fixture"

    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    elif ok:
        print(f"Lexer parity OK: {fixture_path}")
    else:
        print(f"Lexer parity FEIL: {fixture_path}")
        print(f"Forventet tokens: {result['expected_count']}")
        print(f"Faktiske tokens: {result['actual_count']}")

    return 0 if ok else 1


LEXER_PARITY_CHECK_COMMAND = CommandModule(
    name="lexer-parity-check",
    help="Sjekk lexer token-stream mot fixture",
    register_arguments=register_arguments,
    run=run,
)
