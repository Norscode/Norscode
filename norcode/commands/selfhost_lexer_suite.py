"""Selfhost lexer suite command.

Runs the full phase-1 lexer QA flow:

- selfhost lexer readiness
- Python lexer fixture presence/check
- selfhost lexer runtime parity against fixtures

This command is intentionally CI-oriented.  It gives one pass/fail gate for the
first selfhost compiler component.
"""

from __future__ import annotations

import json
from pathlib import Path

from norcode.commands.base import CommandModule
from norcode.lexer_parity_service import tokens_from_current_lexer_file
from norcode.selfhost_lexer_runner import run_selfhost_lexer
from norcode.selfhost_lexer_service import check_selfhost_lexer


DEFAULT_GLOBS = ("tests/*.no", "examples/*.no")



def register_arguments(parser) -> None:
    parser.add_argument("files", nargs="*", help="Valgfrie .no-filer. Hvis tomt brukes tests/*.no og examples/*.no")
    parser.add_argument("--write-fixtures", action="store_true", help="Skriv/oppdater Python lexer token fixtures først")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")



def _discover_files(explicit_files: list[str]) -> list[Path]:
    if explicit_files:
        return [Path(item).expanduser().resolve() for item in explicit_files]

    root = Path.cwd()
    found: list[Path] = []
    for pattern in DEFAULT_GLOBS:
        found.extend(sorted(root.glob(pattern)))
    return [item.resolve() for item in found if item.is_file()]



def _fixture_path(source_path: Path) -> Path:
    return source_path.with_suffix(".tokens.json")



def _ensure_fixture(source_path: Path, write: bool) -> tuple[bool, bool, int]:
    parity = tokens_from_current_lexer_file(str(source_path))
    fixture_path = _fixture_path(source_path)
    expected_text = json.dumps(parity.tokens, ensure_ascii=False, indent=2) + "\n"
    existing_text = fixture_path.read_text(encoding="utf-8") if fixture_path.exists() else None
    up_to_date = existing_text == expected_text
    written = False

    if write and not up_to_date:
        fixture_path.write_text(expected_text, encoding="utf-8")
        up_to_date = True
        written = True

    return up_to_date, written, len(parity.tokens)



def run(args) -> int:
    readiness = check_selfhost_lexer()
    files = _discover_files(args.files)
    results: list[dict[str, object]] = []

    passed = 0
    for source_path in files:
        fixture_path = _fixture_path(source_path)
        fixture_ok, fixture_written, expected_count = _ensure_fixture(source_path, args.write_fixtures)

        runtime_result = run_selfhost_lexer(str(source_path)) if readiness.ok and fixture_ok else None
        actual_tokens = runtime_result.tokens if runtime_result else []
        expected_tokens = json.loads(fixture_path.read_text(encoding="utf-8")) if fixture_path.exists() else []
        parity_ok = bool(runtime_result and runtime_result.ok and expected_tokens == actual_tokens)

        if readiness.ok and fixture_ok and parity_ok:
            passed += 1

        results.append(
            {
                "source": str(source_path),
                "fixture": str(fixture_path),
                "fixture_ok": fixture_ok,
                "fixture_written": fixture_written,
                "runtime_ok": bool(runtime_result and runtime_result.ok),
                "parity_ok": parity_ok,
                "expected_count": expected_count,
                "actual_count": len(actual_tokens),
                "runtime_errors": runtime_result.errors if runtime_result else [],
            }
        )

    summary = {
        "ok": readiness.ok and passed == len(files),
        "readiness": {
            "ok": readiness.ok,
            "exists": readiness.exists,
            "path": str(readiness.path),
            "missing_functions": readiness.missing_functions,
            "missing_token_markers": readiness.missing_token_markers,
        },
        "passed": passed,
        "total": len(files),
        "results": results,
    }

    if args.json:
        print(json.dumps(summary, ensure_ascii=False, indent=2))
    else:
        print(f"Selfhost lexer suite: {passed}/{len(files)} OK")
        print(f"Readiness: {'OK' if readiness.ok else 'FEIL'}")
        for item in results:
            if item["parity_ok"]:
                continue
            print(f"- FEIL: {item['source']}")
            if not item["fixture_ok"]:
                print(f"  fixture mangler/utdatert: {item['fixture']}")
            if item["runtime_errors"]:
                for error in item["runtime_errors"]:
                    print(f"  runtime: {error}")

    return 0 if summary["ok"] else 1


SELFHOST_LEXER_SUITE_COMMAND = CommandModule(
    name="selfhost-lexer-suite",
    help="Kjør samlet selfhost lexer QA/parity suite",
    register_arguments=register_arguments,
    run=run,
)
