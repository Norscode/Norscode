"""Migrate-names command — AVVIKLA. Berre tilgjengeleg via --legacy-python-fallback."""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.migrations import migrate_names


def register_arguments(parser) -> None:
    parser.add_argument("--apply", action="store_true", help="Utfør migrering (default er dry-run)")
    parser.add_argument("--cleanup", action="store_true", help="Fjern legacy-filer etter vellykket migrering")
    parser.add_argument("--check", action="store_true", help="Feil hvis migrering/cleanup fortsatt gjenstår")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def run(args) -> int:
    payload = migrate_names(apply_changes=args.apply, cleanup_legacy=args.cleanup)
    if getattr(args, "json", False):
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        mode = "apply+cleanup" if args.apply and args.cleanup else ("apply" if args.apply else ("dry-run+cleanup" if args.cleanup else "dry-run"))
        print(f"Prosjekt: {payload['project_dir']}")
        print(f"Modus: {mode}")
        for action in payload["actions"]:
            reason = action.get("reason")
            if reason:
                print(
                    f"  {action['kind']}: {action['legacy']} -> {action['primary']} "
                    f"[{action['status']}: {reason}]"
                )
            else:
                print(
                    f"  {action['kind']}: {action['legacy']} -> {action['primary']} "
                    f"[{action['status']}]"
                )
        print(
            "Oppsummert: "
            f"copied={payload['copied']} planned={payload['planned']} "
            f"removed={payload['removed']} planned_remove={payload['planned_remove']} "
            f"skipped={payload['skipped']}"
        )
    if getattr(args, "check", False) and payload["needs_migration"]:
        return 1
    return 0


MIGRATE_NAMES_COMMAND = CommandModule(
    name="migrate-names",
    help="Migrer legacy navn (norsklang*) til Norscode-navn",
    register_arguments=register_arguments,
    run=run,
)
