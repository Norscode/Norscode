"""Selfhost bytecode suite command.

Runs the minimal IR/bytecode smoke suite through the runtime so we can gate
the current selfhost bytecode backend without pretending full compiler parity is
done yet.
"""

from __future__ import annotations

import json
import subprocess
from pathlib import Path

from norcode.commands.base import CommandModule


DEFAULT_BYTECODE_SUITE = (
    "selfhost/tests/bytecode_tests.no",
)


def register_arguments(parser) -> None:
    parser.add_argument(
        "files",
        nargs="*",
        help="Valgfrie selfhost bytecode-tester. Hvis tomt brukes selfhost/tests/bytecode_tests.no",
    )
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def _discover_files(explicit_files: list[str]) -> list[Path]:
    if explicit_files:
        return [Path(item).expanduser().resolve() for item in explicit_files]
    return [Path(item).expanduser().resolve() for item in DEFAULT_BYTECODE_SUITE]


def _run_bytecode_suite_file(source_path: Path) -> dict[str, object]:
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
    results = [_run_bytecode_suite_file(source_path) for source_path in files]
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
            print(f"Bytecode suite {status}: {result['source']}")
            if not result["ok"] and result["stderr"]:
                for line in str(result["stderr"]).splitlines():
                    print(f"- {line}")
        print(f"Selfhost bytecode suite: {passed}/{len(results)} OK")

    return 0 if ok else 1


SELFHOST_BYTECODE_SUITE_COMMAND = CommandModule(
    name="selfhost-bytecode-suite",
    help="Kjør minimal selfhost bytecode suite",
    register_arguments=register_arguments,
    run=run,
)
