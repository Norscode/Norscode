"""Modular selfhost parity commands.

AVVIKLA: erstatta av nc-vm --nc-run (ingen Python nødvendig).
"""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.ir_tools import SELFHOST_PARSER_EXTENDED_FIXTURE, SELFHOST_PARSER_M1_FIXTURE, SELFHOST_PARSER_M2_FIXTURE
from norcode.parity_tools import (
    run_selfhost_parity_gate,
    run_selfhost_parity_progress,
    run_selfhost_parser_parity,
    run_selfhost_parser_suite_all_consistency_check,
    run_selfhost_parser_suite_consistency_check,
    run_selfhost_parser_suite_subset_consistency_check,
)


def register_selfhost_parity_arguments(parser) -> None:
    parser.add_argument("--suite", choices=["m1", "m2", "all"], default="all", help="Velg parity-suite")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def run_selfhost_parity(args) -> int:
    payload = run_selfhost_parser_parity(suite=args.suite)
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"Suite: {payload['suite']}")
        print(f"OK: {'ja' if payload['ok'] else 'nei'}")
        print(f"Cases: {payload['case_count']}")
        print(
            f"Fordeling: uttrykk={payload['expression_cases']} "
            f"skript={payload['script_cases']} "
            f"linje={payload['line_cases']} feil={payload['error_cases']}"
        )
        print(f"Tid: {payload['duration_ms']} ms")
        for item in payload["results"]:
            status = "OK" if item.get("success") else "FEIL"
            print(
                f"- {status}: {item['source']} "
                f"({item.get('case_count', 0)} cases, "
                f"{item.get('error_cases', 0)} feil)"
            )
            if not item.get("success") and item.get("stderr"):
                print(item["stderr"].rstrip())
    return 0 if payload["ok"] else 1


def register_selfhost_parity_progress_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")
    parser.add_argument("--require-ready", action="store_true", help="Feil hvis progress ikke er ready")
    parser.add_argument("--min-coverage", type=float, help="Krev minimum total dekningsprosent (0-100)")


def run_selfhost_parity_progress_command(args) -> int:
    payload = run_selfhost_parity_progress()
    coverage = payload.get("coverage", {}) if isinstance(payload.get("coverage"), dict) else {}
    total_coverage = coverage.get("total_pct")
    if args.min_coverage is not None:
        threshold = float(args.min_coverage)
        if threshold < 0 or threshold > 100:
            raise RuntimeError("--min-coverage må være mellom 0 og 100")
        payload["min_coverage_required"] = threshold
        payload["min_coverage_ok"] = (
            isinstance(total_coverage, (int, float)) and float(total_coverage) >= threshold
        )
    else:
        payload["min_coverage_required"] = None
        payload["min_coverage_ok"] = True
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"OK: {'ja' if payload.get('ok') else 'nei'}")
        print(f"Klar for full coverage: {'ja' if payload.get('ready') else 'nei'}")
        print(
            "Dekning: "
            f"uttrykk={coverage.get('expression_pct', 0)}% "
            f"skript={coverage.get('script_pct', 0)}% "
            f"total={coverage.get('total_pct', 0)}%"
        )
        if args.min_coverage is not None:
            print(
                "Coverage-gate: "
                f"min={payload.get('min_coverage_required')}% "
                f"status={'OK' if payload.get('min_coverage_ok') else 'FEIL'}"
            )
        print(
            "Cases: "
            f"m1={payload.get('m1', {}).get('case_count', 0)} "
            f"m2={payload.get('m2', {}).get('case_count', 0)} "
            f"utvidet={payload.get('extended', {}).get('case_count', 0)}"
        )
        print(
            "Avvik: "
            f"missing={coverage.get('missing_in_m1_m2_count', 0)} "
            f"extra={coverage.get('extra_in_m1_m2_count', 0)} "
            f"overlap={coverage.get('overlap_count', 0)}"
        )
        consistency = payload.get("consistency", {})
        print(
            "Consistency(all): "
            f"{'OK' if consistency.get('ok') else 'FEIL'} "
            f"({consistency.get('checked_cases', 0)} cases, "
            f"{consistency.get('mismatch_count', 0)} avvik)"
        )
        print(f"Tid: {payload.get('duration_ms', 0)} ms")
        if payload.get("stderr") and not payload.get("ok"):
            print(str(payload.get("stderr", "")).rstrip())
    if not payload.get("ok"):
        return 1
    if args.require_ready and not payload.get("ready"):
        if not args.json:
            print("Gate-feil: progress er ikke ready")
        return 1
    if not payload.get("min_coverage_ok"):
        if not args.json:
            print(
                "Gate-feil: total dekningsprosent under minimum "
                f"({coverage.get('total_pct', 0)} < {payload.get('min_coverage_required')})"
            )
        return 1
    return 0


def register_selfhost_parity_gate_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")
    parser.add_argument("--min-coverage", type=float, help="Krev minimum total dekningsprosent (0-100)")


def run_selfhost_parity_gate_command(args) -> int:
    threshold = None
    if args.min_coverage is not None:
        threshold = float(args.min_coverage)
        if threshold < 0 or threshold > 100:
            raise RuntimeError("--min-coverage må være mellom 0 og 100")
    payload = run_selfhost_parity_gate(min_coverage=threshold)
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"OK: {'ja' if payload.get('ok') else 'nei'}")
        print(f"Klar for gate: {'ja' if payload.get('ready') else 'nei'}")
        if payload.get("coverage_total_pct") is not None:
            print(f"Dekning: {payload['coverage_total_pct']}%")
        if payload.get("min_coverage") is not None:
            print(f"Min coverage: {payload['min_coverage']}%")
    return 0 if payload.get("ok") else 1


def register_selfhost_parity_consistency_arguments(parser) -> None:
    parser.add_argument("--scope", choices=["m1", "m2", "all"], default="m1", help="Velg konsistensscope")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def run_selfhost_parity_consistency(args) -> int:
    if args.scope == "m1":
        payload = run_selfhost_parser_suite_consistency_check(
            SELFHOST_PARSER_M1_FIXTURE,
            SELFHOST_PARSER_EXTENDED_FIXTURE,
        )
    elif args.scope == "m2":
        payload = run_selfhost_parser_suite_subset_consistency_check(
            SELFHOST_PARSER_M2_FIXTURE,
            SELFHOST_PARSER_EXTENDED_FIXTURE,
            "m2",
        )
    else:
        payload = run_selfhost_parser_suite_all_consistency_check(
            SELFHOST_PARSER_M1_FIXTURE,
            SELFHOST_PARSER_M2_FIXTURE,
            SELFHOST_PARSER_EXTENDED_FIXTURE,
        )
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"Scope: {payload.get('scope', 'm1')}")
        print(f"OK: {'ja' if payload.get('success') else 'nei'}")
        print(f"Sjekkede cases: {payload.get('checked_cases', 0)}")
        print(f"Avvik: {payload.get('mismatch_count', 0)}")
        print(f"Tid: {payload.get('duration_ms', 0)} ms")
        checks = payload.get("checks")
        if isinstance(checks, dict):
            m1 = checks.get("m1", {})
            m2 = checks.get("m2", {})
            print(
                f"- m1: {'OK' if m1.get('success') else 'FEIL'} "
                f"({m1.get('checked_cases', 0)} cases, {m1.get('mismatch_count', 0)} avvik)"
            )
            print(
                f"- m2: {'OK' if m2.get('success') else 'FEIL'} "
                f"({m2.get('checked_cases', 0)} cases, {m2.get('mismatch_count', 0)} avvik)"
            )
            print(
                f"- coverage: {checks.get('coverage_checked_cases', 0)} cases, "
                f"{checks.get('coverage_mismatch_count', 0)} avvik"
            )
        if not payload.get("success") and payload.get("stderr"):
            print(payload["stderr"].rstrip())
    return 0 if payload.get("success") else 1


SELFHOST_PARITY_COMMAND = CommandModule(
    name="selfhost-parity",
    help="Kjør selfhost parser parity-suiter",
    register_arguments=register_selfhost_parity_arguments,
    run=run_selfhost_parity,
)

SELFHOST_PARITY_PROGRESS_COMMAND = CommandModule(
    name="selfhost-parity-progress",
    help="Vis fremdrift for M1/M2 dekning mot utvidet parity-suite",
    register_arguments=register_selfhost_parity_progress_arguments,
    run=run_selfhost_parity_progress_command,
)

SELFHOST_PARITY_GATE_COMMAND = CommandModule(
    name="selfhost-parity-gate",
    help="Kjør parity-progress som en eksplisitt gate",
    register_arguments=register_selfhost_parity_gate_arguments,
    run=run_selfhost_parity_gate_command,
)

SELFHOST_PARITY_CONSISTENCY_COMMAND = CommandModule(
    name="selfhost-parity-consistency",
    help="Verifiser at parity-suitene er konsistente med utvidet fixture",
    register_arguments=register_selfhost_parity_consistency_arguments,
    run=run_selfhost_parity_consistency,
)
