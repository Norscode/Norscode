"""Selfhost bootstrap build command.

Runs the deterministic bootstrap suite and reports the resulting hash.
"""

from __future__ import annotations

import json
import subprocess
from pathlib import Path

from norcode.commands.base import CommandModule


def register_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def _run_bootstrap_suite() -> dict[str, object]:
    completed = subprocess.run(
        ["./bin/nc", "run", "selfhost/tests/bootstrap_tests.no"],
        text=True,
        capture_output=True,
        check=False,
    )
    stdout = completed.stdout
    marker = "Return: "
    marker_pos = stdout.find(marker)
    hash_text = ""
    if marker_pos != -1:
        hash_text = stdout[marker_pos + len(marker) :].rstrip("\n")
    return {
        "ok": completed.returncode == 0 and hash_text != "",
        "returncode": completed.returncode,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
        "hash": hash_text,
    }


def run(args) -> int:
    result = _run_bootstrap_suite()
    summary = {
        "suite": "selfhost/tests/bootstrap_tests.no",
        "ok": result["ok"],
        "hash": result["hash"],
        "returncode": result["returncode"],
    }

    if getattr(args, "json", False):
        print(json.dumps(summary, ensure_ascii=False, indent=2))
    else:
        status = "OK" if result["ok"] else "FEIL"
        print(f"Selfhost build {status}: {summary['suite']}")
        print(f"- hash: {summary['hash']}")

    return 0 if result["ok"] else 1


SELFHOST_BOOTSTRAP_BUILD_COMMAND = CommandModule(
    name="selfhost-build",
    help="Bygg selfhost bootstrap-kjeden med Python-first compiler",
    register_arguments=register_arguments,
    run=run,
)
