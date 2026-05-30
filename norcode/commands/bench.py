"""Bench command — AVVIKLA. Berre tilgjengeleg via --legacy-python-fallback."""
from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.quality_suites import run_benchmark_suite


def register_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv benchmark-resultat som JSON")


def run(args) -> int:
    payload = run_benchmark_suite()
    if getattr(args, "json", False):
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"OK: {'ja' if payload['ok'] else 'nei'}")
        print(f"Tid: {payload['total_duration_ms']} ms")
        print(f"Topptid: {payload['max_duration_ms']} ms")
        print(f"Budget-avvik: {payload['budget_exceeded_count']}")
        for row in payload["benchmarks"]:
            status = "OK" if row["ok"] and row["within_budget"] else "FEIL"
            print(f"- {status}: {row['name']} ({row['duration_ms']} ms, budsjett {row['budget_ms']} ms)")
    return 0 if payload["ok"] else 1


BENCH_COMMAND = CommandModule(
    name="bench",
    help="Kjør faste ytelsesmålinger",
    register_arguments=register_arguments,
    run=run,
)
