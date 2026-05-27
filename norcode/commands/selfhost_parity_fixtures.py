"""Modular commands for selfhost parity fixture maintenance."""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.parity_tools import sync_selfhost_parser_m2_fixture, update_selfhost_parser_fixtures


def register_update_selfhost_parity_fixtures_arguments(parser) -> None:
    parser.add_argument("--suite", choices=["m1", "m2", "extended", "all"], default="all", help="Velg fixtures å oppdatere")
    parser.add_argument("--check", action="store_true", help="Feil hvis parity-fixtures er utdaterte (skriv ikke)")
    parser.add_argument("--no-sync-m2", action="store_true", help="Hopp over automatisk M2-sync (core minus M1)")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def run_update_selfhost_parity_fixtures(args) -> int:
    payload = update_selfhost_parser_fixtures(
        check_only=args.check,
        suite=args.suite,
        sync_m2=(not args.no_sync_m2),
    )
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"Suite: {payload['suite']}")
        print(f"Cases: {payload['cases']}")
        print(f"Avvik: {payload['updated']}")
        if payload.get("m2_sync") is not None:
            m2_sync = payload["m2_sync"]
            print(
                f"- M2 sync: {m2_sync.get('m2_cases', 0)} cases, "
                f"{m2_sync.get('updated', 0)} oppdateringer ({m2_sync.get('fixture')})"
            )
        for row in payload["fixtures"]:
            print(
                f"- {row['label']}: {row['cases']} cases, "
                f"{row['updated']} oppdateringer ({row['fixture']})"
            )
        if args.check:
            print("Status: check-only")
        else:
            print("Status: skrevet")
    if args.check and payload["updated"] > 0:
        return 1
    return 0


def register_sync_selfhost_parity_m2_arguments(parser) -> None:
    parser.add_argument("--check", action="store_true", help="Feil hvis M2-fixture er ute av synk")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def run_sync_selfhost_parity_m2(args) -> int:
    payload = sync_selfhost_parser_m2_fixture(check_only=args.check)
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"OK: {'ja' if payload.get('ok') else 'nei'}")
        print(f"M2 fixture: {payload['fixture']}")
        print(f"M1 cases: {payload['m1_cases']}")
        print(f"Core cases: {payload['core_cases']}")
        print(f"M2 cases (beregnet): {payload['m2_cases']}")
        print(f"Avvik: {payload['updated']}")
        print(f"M1-mangler i core: {payload['missing_m1_from_core_count']}")
        if args.check:
            print("Status: check-only")
        else:
            print("Status: synkronisert")
    if args.check and not payload.get("ok"):
        return 1
    return 0


UPDATE_SELFHOST_PARITY_FIXTURES_COMMAND = CommandModule(
    name="update-selfhost-parity-fixtures",
    help="Regenerer selfhost parser parity-forventninger",
    register_arguments=register_update_selfhost_parity_fixtures_arguments,
    run=run_update_selfhost_parity_fixtures,
)

SYNC_SELFHOST_PARITY_M2_COMMAND = CommandModule(
    name="sync-selfhost-parity-m2",
    help="Synkroniser M2-fixture som core minus M1",
    register_arguments=register_sync_selfhost_parity_m2_arguments,
    run=run_sync_selfhost_parity_m2,
)
