"""Modular selfhost bootstrap gate command."""

from __future__ import annotations

import json

from norcode.bootstrap_ci import run_selfhost_bootstrap_gate
from norcode.commands.base import CommandModule


def register_arguments(parser) -> None:
    parser.add_argument("--output-dir", default="build/selfhost-bootstrap-gate", help="Output-katalog for whole-compile artefakter")
    parser.add_argument("--no-determinism", action="store_true", help="Hopp over dobbelkompilering/digest-sjekk")
    parser.add_argument("--json", action="store_true", help="Skriv gate-resultat som JSON")


def run(args) -> int:
    payload = run_selfhost_bootstrap_gate(
        output_dir=args.output_dir,
        verify_deterministic=not args.no_determinism,
    )
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        whole = payload["selfhost_whole_compile"]
        native = payload["native_bootstrap_verify"]
        print(f"Selfhost bootstrap gate: {'OK' if payload['ok'] else 'FEIL'}")
        print(f"Whole compile: {whole.get('passed')}/{whole.get('total')} OK")
        print(f"Manifest: {whole.get('manifest')}")
        if payload.get("determinism_check", {}).get("enabled"):
            print(
                "Determinisme: "
                f"{'OK' if payload['determinism_check'].get('ok') else 'FEIL'} "
                f"({payload['determinism_check'].get('digest')})"
            )
        execution = payload.get("selfhost_execution_smoke", {})
        print(
            "Selfhost execution: "
            f"{'OK' if execution.get('ok') else 'FEIL'} "
            f"(return={execution.get('result')})"
        )
        package_hosting = payload.get("package_hosting_smoke", {})
        print(
            "Package hosting: "
            f"{'OK' if package_hosting.get('ok') else 'FEIL'} "
            f"({package_hosting.get('url')})"
        )
        if package_hosting.get("skip_reason"):
            print(f"Package hosting hoppet over: {package_hosting.get('skip_reason')}")
        print(f"Native bootstrap: {'OK' if native.get('ok') else 'FEIL'}")
        if native.get("skip_reason"):
            print(f"Native run hoppet over: {native.get('skip_reason')}")
    return 0 if payload.get("ok") else 1


SELFHOST_BOOTSTRAP_GATE_COMMAND = CommandModule(
    name="selfhost-bootstrap-gate",
    help="Kjør whole selfhost compile + native bootstrap gate",
    register_arguments=register_arguments,
    run=run,
    bootstrap_only=True,
    experimental=True,
)
