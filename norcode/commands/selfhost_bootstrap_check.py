"""Selfhost bootstrap check command.

Runs the deterministic bootstrap suite and compares its output hash against a
golden snapshot.
"""

from __future__ import annotations

import json
import subprocess
from pathlib import Path

from norcode.commands.base import CommandModule


BOOTSTRAP_SUITE = Path("selfhost/tests/bootstrap_tests.no")
BOOTSTRAP_GOLDEN = Path("selfhost/bootstrap.hash")


def register_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")
    parser.add_argument("--golden", default=str(BOOTSTRAP_GOLDEN), help="Sti til golden hash-fil")


def _run_bootstrap_suite() -> dict[str, object]:
    completed = subprocess.run(
        ["./bin/nc", "run", str(BOOTSTRAP_SUITE)],
        text=True,
        capture_output=True,
        check=False,
    )
    stdout = completed.stdout
    hash_line = ""
    marker = "Return: "
    marker_pos = stdout.find(marker)
    if marker_pos != -1:
        hash_line = stdout[marker_pos + len(marker) :].rstrip("\n")
    return {
        "ok": completed.returncode == 0 and hash_line != "",
        "returncode": completed.returncode,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
        "hash": hash_line,
    }


def run(args) -> int:
    first = _run_bootstrap_suite()
    second = _run_bootstrap_suite()
    golden_path = Path(getattr(args, "golden", str(BOOTSTRAP_GOLDEN))).expanduser().resolve()
    golden = golden_path.read_text(encoding="utf-8").strip() if golden_path.exists() else ""
    deterministic = first["hash"] == second["hash"]
    matches_golden = first["hash"] == golden and first["hash"] != ""
    ok = bool(first["ok"] and second["ok"] and deterministic and matches_golden)

    summary = {
        "ok": ok,
        "deterministic": deterministic,
        "matches_golden": matches_golden,
        "hash": first["hash"],
        "golden": golden,
        "first": first,
        "second": second,
        "golden_path": str(golden_path),
    }

    if getattr(args, "json", False):
        print(json.dumps(summary, ensure_ascii=False, indent=2))
    else:
        status = "OK" if ok else "FEIL"
        print(f"Bootstrap check {status}")
        print(f"- hash: {first['hash']}")
        print(f"- deterministic: {deterministic}")
        print(f"- matches_golden: {matches_golden}")
        if not ok:
            print(f"- golden_path: {golden_path}")
            if first["stderr"]:
                for line in str(first["stderr"]).splitlines():
                    print(f"  {line}")
    return 0 if ok else 1


SELFHOST_BOOTSTRAP_CHECK_COMMAND = CommandModule(
    name="selfhost-check",
    help="Sjekk deterministisk selfhost bootstrap-kjede mot golden hash",
    register_arguments=register_arguments,
    run=run,
)
