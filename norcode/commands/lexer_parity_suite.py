"""Lexer parity suite command.

Runs lexer fixture/check workflows over many source files.  This is the CI-ready
gate for token regression testing and future Norscode-native lexer parity.
"""

from __future__ import annotations

import json
from pathlib import Path

from norcode.commands.base import CommandModule
from norcode.lexer_parity_service import tokens_from_current_lexer_file


DEFAULT_GLOBS = ("tests/*.no", "examples/*.no")



def register_arguments(parser) -> None:
    parser.add_argument("files", nargs="*", help="Valgfrie .no-filer. Hvis tomt brukes tests/*.no og examples/*.no")
    parser.add_argument("--write", action="store_true", help="Skriv/oppdater token fixtures")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def _discover_files(explicit_files: list[str]) -> list[Path]:
    if explicit_files:
        return [Path(item).expanduser().resolve() for item in explicit_files]

    found: list[Path] = []
    root = Path.cwd()
    for pattern in DEFAULT_GLOBS:
        found.extend(sorted(root.glob(pattern)))
    return [item.resolve() for item in found if item.is_file()]



def _fixture_path(source_path: Path) -> Path:
    return source_path.with_suffix(".tokens.json")



def run(args) -> int:
    files = _discover_files(args.files)
    results: list[dict[str, object]] = []
    passed = 0

    for source_path in files:
        parity = tokens_from_current_lexer_file(str(source_path))
        fixture_path = _fixture_path(source_path)
        expected_text = json.dumps(parity.tokens, ensure_ascii=False, indent=2) + "\n"
        existing_text = fixture_path.read_text(encoding="utf-8") if fixture_path.exists() else None
        up_to_date = existing_text == expected_text
        written = False

        if parity.validation_ok and args.write and not up_to_date:
            fixture_path.write_text(expected_text, encoding="utf-8")
            up_to_date = True
            written = True

        ok = parity.validation_ok and up_to_date
        if ok:
            passed += 1

        results.append(
            {
                "source": str(source_path),
                "fixture": str(fixture_path),
                "ok": ok,
                "written": written,
                "up_to_date": up_to_date,
                "token_count": len(parity.tokens),
                "validation_ok": parity.validation_ok,
                "validation_errors": parity.validation_errors,
            }
        )

    summary = {
        "ok": passed == len(files),
        "passed": passed,
        "total": len(files),
        "results": results,
    }

    if args.json:
        print(json.dumps(summary, ensure_ascii=False, indent=2))
    else:
        print(f"Lexer parity suite: {passed}/{len(files)} OK")
        for item in results:
            if item["ok"]:
                continue
            print(f"- FEIL: {item['source']} -> {item['fixture']}")
            if not item["validation_ok"]:
                for error in item["validation_errors"]:
                    print(f"  validation: {error}")
            elif not item["up_to_date"]:
                print("  fixture mangler/utdatert")
        if args.write:
            written_count = sum(1 for item in results if item["written"])
            print(f"Fixtures skrevet/oppdatert: {written_count}")

    return 0 if summary["ok"] else 1


LEXER_PARITY_SUITE_COMMAND = CommandModule(
    name="lexer-parity-suite",
    help="Kjør lexer parity over flere filer",
    register_arguments=register_arguments,
    run=run,
)
