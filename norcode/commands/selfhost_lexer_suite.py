"""Selfhost lexer suite command.

Runs the full phase-1 lexer QA flow:

- selfhost lexer readiness
- selfhost lexer compile smoke-check
- Python lexer fixture presence/check
- selfhost lexer runtime parity against fixtures

This command is intentionally CI-oriented.  It gives one pass/fail gate for the
first selfhost compiler component.
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from norcode.commands.base import CommandModule
from norcode.compiler_core import compile_source
from norcode.lexer_parity_service import tokens_from_current_lexer_file
from norcode.selfhost_lexer_runner import run_selfhost_lexer
from norcode.selfhost_lexer_service import DEFAULT_SELFHOST_LEXER, check_selfhost_lexer


DEFAULT_GLOBS = ("tests/*.no", "examples/*.no")



def register_arguments(parser) -> None:
    parser.add_argument("files", nargs="*", help="Valgfrie .no-filer. Hvis tomt brukes tests/*.no og examples/*.no")
    parser.add_argument("--write-fixtures", action="store_true", help="Skriv/oppdater Python lexer token fixtures først")
    parser.add_argument("--skip-runtime", action="store_true", help="Kjør bare readiness + compile + fixture checks")
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



def _compile_selfhost_lexer() -> tuple[bool, list[str], list[str]]:
    errors: list[str] = []
    function_names: list[str] = []
    try:
        compile_result = compile_source(str(DEFAULT_SELFHOST_LEXER))
        functions = compile_result.bytecode.get("functions", {})
        if isinstance(functions, dict):
            function_names = sorted(str(name) for name in functions.keys())
    except Exception as exc:
        errors.append(str(exc))
    return not errors, function_names, errors



def _first_token_diff(expected: Any, actual: Any) -> dict[str, Any] | None:
    if not isinstance(expected, list) or not isinstance(actual, list):
        return {"index": 0, "expected": expected, "actual": actual}
    limit = min(len(expected), len(actual))
    for index in range(limit):
        if expected[index] != actual[index]:
            return {"index": index, "expected": expected[index], "actual": actual[index]}
    if len(expected) != len(actual):
        return {
            "index": limit,
            "expected": expected[limit] if limit < len(expected) else None,
            "actual": actual[limit] if limit < len(actual) else None,
        }
    return None



def _failure_stage(
    *,
    readiness_ok: bool,
    compile_ok: bool,
    fixture_ok: bool,
    runtime_ok: bool,
    validation_ok: bool,
    parity_ok: bool,
    skip_runtime: bool,
) -> str:
    if not readiness_ok:
        return "readiness"
    if not compile_ok:
        return "compile"
    if not fixture_ok:
        return "fixture"
    if skip_runtime:
        return "ok"
    if not runtime_ok:
        return "runtime"
    if not validation_ok:
        return "validation"
    if not parity_ok:
        return "parity"
    return "ok"



def run(args) -> int:
    readiness = check_selfhost_lexer()
    compile_ok, compile_functions, compile_errors = _compile_selfhost_lexer() if readiness.ok else (False, [], [])
    files = _discover_files(args.files)
    results: list[dict[str, object]] = []

    passed = 0
    stage_counts: dict[str, int] = {
        "ok": 0,
        "readiness": 0,
        "compile": 0,
        "fixture": 0,
        "runtime": 0,
        "validation": 0,
        "parity": 0,
    }

    for source_path in files:
        fixture_path = _fixture_path(source_path)
        fixture_ok, fixture_written, expected_count = _ensure_fixture(source_path, args.write_fixtures)

        runtime_result = None
        if readiness.ok and compile_ok and fixture_ok and not args.skip_runtime:
            runtime_result = run_selfhost_lexer(str(source_path))

        actual_tokens = runtime_result.tokens if runtime_result else []
        expected_tokens = json.loads(fixture_path.read_text(encoding="utf-8")) if fixture_path.exists() else []
        runtime_errors = runtime_result.errors if runtime_result else []
        validation_errors = runtime_result.validation_errors if runtime_result else []
        runtime_ok = bool(runtime_result and not runtime_errors)
        validation_ok = bool(runtime_result and not validation_errors)
        parity_ok = bool(runtime_result and runtime_result.ok and expected_tokens == actual_tokens)
        file_ok = fixture_ok if args.skip_runtime else parity_ok
        stage = _failure_stage(
            readiness_ok=readiness.ok,
            compile_ok=compile_ok,
            fixture_ok=fixture_ok,
            runtime_ok=runtime_ok,
            validation_ok=validation_ok,
            parity_ok=parity_ok,
            skip_runtime=bool(args.skip_runtime),
        )
        stage_counts[stage] = stage_counts.get(stage, 0) + 1

        if readiness.ok and compile_ok and file_ok:
            passed += 1

        results.append(
            {
                "source": str(source_path),
                "fixture": str(fixture_path),
                "stage": stage,
                "fixture_ok": fixture_ok,
                "fixture_written": fixture_written,
                "runtime_ok": runtime_ok,
                "validation_ok": validation_ok,
                "runtime_skipped": bool(args.skip_runtime),
                "parity_ok": parity_ok,
                "expected_count": expected_count,
                "actual_count": len(actual_tokens),
                "called_function": runtime_result.called_function if runtime_result else None,
                "candidate_functions": runtime_result.candidate_functions if runtime_result else [],
                "available_functions": runtime_result.available_functions if runtime_result else [],
                "first_diff": _first_token_diff(expected_tokens, actual_tokens) if validation_ok and not parity_ok else None,
                "runtime_errors": runtime_errors,
                "validation_errors": validation_errors,
            }
        )

    summary = {
        "ok": readiness.ok and compile_ok and passed == len(files),
        "readiness": {
            "ok": readiness.ok,
            "exists": readiness.exists,
            "path": str(readiness.path),
            "missing_functions": readiness.missing_functions,
            "missing_token_markers": readiness.missing_token_markers,
        },
        "compile": {
            "ok": compile_ok,
            "functions": compile_functions,
            "errors": compile_errors,
        },
        "runtime_skipped": bool(args.skip_runtime),
        "passed": passed,
        "total": len(files),
        "stage_counts": stage_counts,
        "results": results,
    }

    if args.json:
        print(json.dumps(summary, ensure_ascii=False, indent=2))
    else:
        print(f"Selfhost lexer suite: {passed}/{len(files)} OK")
        print(f"Readiness: {'OK' if readiness.ok else 'FEIL'}")
        print(f"Compile: {'OK' if compile_ok else 'FEIL'}")
        print("Stages:")
        for stage_name in ("ok", "readiness", "compile", "fixture", "runtime", "validation", "parity"):
            print(f"  {stage_name}: {stage_counts.get(stage_name, 0)}")
        for error in compile_errors:
            print(f"  compile: {error}")
        for item in results:
            if item["stage"] == "ok":
                continue
            print(f"- FEIL [{item['stage']}]: {item['source']}")
            if item["called_function"]:
                print(f"  called_function: {item['called_function']}")
            if item["candidate_functions"]:
                print("  candidate_functions:")
                for name in item["candidate_functions"]:
                    print(f"    - {name}")
            if item["available_functions"]:
                print("  available_functions:")
                for name in item["available_functions"]:
                    print(f"    - {name}")
            if not item["fixture_ok"]:
                print(f"  fixture mangler/utdatert: {item['fixture']}")
            if item["runtime_errors"]:
                for error in item["runtime_errors"]:
                    print(f"  runtime: {error}")
            if item["validation_errors"]:
                for error in item["validation_errors"]:
                    print(f"  validation: {error}")
            if item["first_diff"]:
                print(f"  first diff: {item['first_diff']}")

    return 0 if summary["ok"] else 1


SELFHOST_LEXER_SUITE_COMMAND = CommandModule(
    name="selfhost-lexer-suite",
    help="Kjør samlet selfhost lexer QA/parity suite",
    register_arguments=register_arguments,
    run=run,
)
