"""Fuzz command — AVVIKLA. Berre tilgjengeleg via --legacy-python-fallback."""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.fuzz import run_negative_suite


def register_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv fuzz-resultat som JSON")


def run(args) -> int:
    payload = run_negative_suite()
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"OK: {'ja' if payload['ok'] else 'nei'}")
        print(f"Parser-feil fanget: {len(payload['parser_cases']) - payload['parser_failures']}/{len(payload['parser_cases'])}")
        print(f"Runtime-feil fanget: {len(payload['runtime_cases']) - payload['runtime_failures']}/{len(payload['runtime_cases'])}")
        for row in payload["parser_cases"]:
            status = "OK" if row["ok"] else "FEIL"
            print(f"- {status}: parser/{row['name']} ({row['duration_ms']} ms)")
        for row in payload["runtime_cases"]:
            status = "OK" if row["ok"] else "FEIL"
            print(f"- {status}: runtime/{row['name']} ({row['duration_ms']} ms)")
    if not payload["ok"]:
        return 1
    return 0


FUZZ_COMMAND = CommandModule(
    name="fuzz",
    help="Kjør negativ parser- og runtime-korpus",
    register_arguments=register_arguments,
    run=run,
)
