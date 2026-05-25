from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.quality_suites import run_stress_suite


def register_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv stress-resultat som JSON")


def run(args) -> int:
    payload = run_stress_suite()
    if getattr(args, "json", False):
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"OK: {'ja' if payload['ok'] else 'nei'}")
        for row in payload["cases"]:
            status = "OK" if row.get("ok") else "FEIL"
            print(f"- {status}: {row['name']} ({row.get('ok_count', 0)}/{row.get('requests', 0)} svar)")
    return 0 if payload["ok"] else 1


STRESS_COMMAND = CommandModule(
    name="stress",
    help="Kjør produksjonsnære stresstester av serveradapteren",
    register_arguments=register_arguments,
    run=run,
)
