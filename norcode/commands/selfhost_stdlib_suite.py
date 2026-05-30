"""Selfhost stdlib/compiler suite command.

AVVIKLA: erstatta av nc-vm --nc-run (ingen Python nødvendig).
"""

from __future__ import annotations

import json
import subprocess
from pathlib import Path

from norcode.commands.base import CommandModule


DEFAULT_STDLIB_SUITE = (
    "selfhost/tests/stdlib_compiler_tests.no",
)


def register_arguments(parser) -> None:
    parser.add_argument(
        "files",
        nargs="*",
        help="Valgfrie selfhost stdlib/compiler-tester. Hvis tomt brukes selfhost/tests/stdlib_compiler_tests.no",
    )
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def _discover_files(explicit_files: list[str]) -> list[Path]:
    if explicit_files:
        return [Path(item).expanduser().resolve() for item in explicit_files]
    return [Path(item).expanduser().resolve() for item in DEFAULT_STDLIB_SUITE]


def _run_stdlib_suite_file(source_path: Path) -> dict[str, object]:
    completed = subprocess.run(
        ["./bin/nc", "run", str(source_path)],
        text=True,
        capture_output=True,
        check=False,
    )
    ok = completed.returncode == 0
    return {
        "source": str(source_path),
        "ok": ok,
        "returncode": completed.returncode,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
    }


def run(args) -> int:
    files = _discover_files(getattr(args, "files", []))
    results = [_run_stdlib_suite_file(source_path) for source_path in files]
    passed = sum(1 for result in results if result["ok"])
    ok = passed == len(results)
    summary = {
        "ok": ok,
        "passed": passed,
        "total": len(results),
        "results": results,
    }

    if getattr(args, "json", False):
        print(json.dumps(summary, ensure_ascii=False, indent=2))
    else:
        for result in results:
            status = "OK" if result["ok"] else "FEIL"
            print(f"Stdlib suite {status}: {result['source']}")
            if not result["ok"] and result["stderr"]:
                for line in str(result["stderr"]).splitlines():
                    print(f"- {line}")
        print(f"Selfhost stdlib suite: {passed}/{len(results)} OK")

    return 0 if ok else 1


SELFHOST_STDLIB_SUITE_COMMAND = CommandModule(
    name="selfhost-stdlib-suite",
    help="Kjør minimal selfhost stdlib/compiler suite",
    register_arguments=register_arguments,
    run=run,
)

