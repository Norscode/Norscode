"""Selfhost lexer parity command.

Runs the Norscode-native lexer through the runtime ABI and compares its token
stream against the existing Python lexer token fixture.
"""

from __future__ import annotations

import json
from pathlib import Path

from norcode.commands.base import CommandModule
from norcode.selfhost_lexer_runner import run_selfhost_lexer



def register_arguments(parser) -> None:
    parser.add_argument("file", help="Norscode source file")
    parser.add_argument("--fixture", help="Token fixture path, default: <source>.tokens.json")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def run(args) -> int:
    source_path = Path(args.file).expanduser().resolve()
    fixture_path = Path(args.fixture).expanduser().resolve() if args.fixture else source_path.with_suffix(".tokens.json")

    runtime_result = run_selfhost_lexer(str(source_path))

    if not fixture_path.exists():
        payload = {
            "ok": False,
            "ready": runtime_result.ready,
            "source": str(source_path),
            "fixture": str(fixture_path),
            "error": "fixture mangler",
            "runtime_errors": runtime_result.errors,
        }
        if args.json:
            print(json.dumps(payload, ensure_ascii=False, indent=2))
        else:
            print(f"Selfhost lexer parity FEIL: fixture mangler {fixture_path}")
            for item in runtime_result.errors:
                print(f"- {item}")
        return 1

    expected = json.loads(fixture_path.read_text(encoding="utf-8"))
    actual = runtime_result.tokens
    ok = runtime_result.ok and expected == actual

    payload = {
        "ok": ok,
        "ready": runtime_result.ready,
        "source": str(source_path),
        "fixture": str(fixture_path),
        "expected_count": len(expected) if isinstance(expected, list) else None,
        "actual_count": len(actual),
        "runtime_errors": runtime_result.errors,
    }

    if not ok and runtime_result.ok:
        payload["error"] = "selfhost token stream avviker fra fixture"
    elif not runtime_result.ok:
        payload["error"] = "selfhost lexer runtime failed"

    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    elif ok:
        print(f"Selfhost lexer parity OK: {fixture_path}")
    else:
        print(f"Selfhost lexer parity FEIL: {fixture_path}")
        print(f"Forventet tokens: {payload['expected_count']}")
        print(f"Faktiske tokens: {payload['actual_count']}")
        for item in runtime_result.errors:
            print(f"- {item}")

    return 0 if ok else 1


SELFHOST_LEXER_PARITY_COMMAND = CommandModule(
    name="selfhost-lexer-parity",
    help="Sammenlign selfhost lexer mot token fixture",
    register_arguments=register_arguments,
    run=run,
)
