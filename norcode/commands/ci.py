"""Modular CI command.

This pulls the high-traffic CI command out of the legacy bootstrap dispatcher
without changing the underlying pipeline logic yet.
"""

from __future__ import annotations

import json
import sys

from norcode.bootstrap_ci import run_ci_bootstrap_lane as run_bootstrap_ci_lane
from norcode.ci_pipeline import run_ci_pipeline as run_ci_pipeline_module
from norcode.commands.base import CommandModule
from norcode.migrations import migrate_names as run_migrate_names
from norcode.ir_tools import update_ir_snapshots
from norcode.parity_tools import (
    run_selfhost_parity_progress,
    run_selfhost_parser_core_checks,
    run_selfhost_parser_suite_all_consistency_check,
    run_selfhost_parser_suite_consistency_check,
    run_selfhost_parser_suite_subset_consistency_check,
    sync_selfhost_parser_m2_fixture,
    update_selfhost_parser_fixtures,
)
from norcode.testing_support import run_all_tests


def register_arguments(parser) -> None:
    parser.add_argument("--json", action="store_true", help="Skriv CI-resultat som JSON")
    parser.add_argument("--check-names", action="store_true", help="Inkluder sjekk for navnemigrering (legacy -> Norscode)")
    parser.add_argument("--parity-suite", choices=["m1", "m2", "all"], default="all", help="Velg parity-scope i CI")
    parser.add_argument("--bootstrap-lane", action="store_true", help="Kjør bare fysisk selfhost bootstrap gate + workflow-policy")
    parser.add_argument("--bootstrap-output-dir", default="build/selfhost-bootstrap-gate", help="Output-katalog for --bootstrap-lane artefakter")
    parser.add_argument(
        "--require-selfhost-ready",
        action="store_true",
        help="Feil hvis selfhost parity eller whole-compile ikke er fullført/ready",
    )


def run(args) -> int:
    if args.bootstrap_lane:
        payload = run_bootstrap_lane(
            json_output=args.json,
            check_names=args.check_names,
            output_dir=args.bootstrap_output_dir,
        )
    else:
        payload = run_ci_pipeline_module(
            json_output=args.json,
            check_names=args.check_names,
            parity_suite=args.parity_suite,
            require_selfhost_ready=args.require_selfhost_ready,
            argv=sys.argv[1:],
            update_ir_snapshots_fn=update_ir_snapshots,
            update_selfhost_parser_fixtures_fn=update_selfhost_parser_fixtures,
            run_all_tests_fn=run_all_tests,
            run_selfhost_parser_core_checks_fn=run_selfhost_parser_core_checks,
            run_selfhost_parser_suite_consistency_check_fn=run_selfhost_parser_suite_consistency_check,
            run_selfhost_parser_suite_subset_consistency_check_fn=run_selfhost_parser_suite_subset_consistency_check,
            run_selfhost_parser_suite_all_consistency_check_fn=run_selfhost_parser_suite_all_consistency_check,
            sync_selfhost_parser_m2_fixture_fn=sync_selfhost_parser_m2_fixture,
            run_selfhost_parity_progress_fn=run_selfhost_parity_progress,
            migrate_names_fn=run_migrate_names,
        )

    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    elif not args.bootstrap_lane:
        # The non-bootstrap lane prints progress from the pipeline helper.
        pass
    return 0 if payload.get("ok") else 1


def run_bootstrap_lane(*, json_output: bool = False, check_names: bool = False, output_dir: str = "build/selfhost-bootstrap-gate") -> dict:
    """Run the explicit bootstrap lane used by CI."""
    return run_bootstrap_ci_lane(
        json_output=json_output,
        check_names=check_names,
        output_dir=output_dir,
        argv=sys.argv[1:],
        migrate_names_fn=run_migrate_names,
    )


CI_COMMAND = CommandModule(
    name="ci",
    help="Kjør lokal CI-sekvens (snapshot, parity, test)",
    register_arguments=register_arguments,
    run=run,
    bootstrap_only=True,
)
