from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.quality_suites import run_security_suite


def register_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv sikkerhetsresultat som JSON")


def run(args) -> int:
    payload = run_security_suite()
    if getattr(args, "json", False):
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"OK: {'ja' if payload['ok'] else 'nei'}")
        for row in payload["checks"]:
            status = "OK" if row.get("ok") else "FEIL"
            print(f"- {status}: {row['name']} ({row.get('duration_ms', 0)} ms)")
    return 0 if payload["ok"] else 1


SECURITY_COMMAND = CommandModule(
    name="security",
    help="Kjør auth- og input-sikkerhetstester",
    register_arguments=register_arguments,
    run=run,
)
