"""Release preparation command."""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.release import prepare_release


def register_arguments(parser) -> None:
    parser.add_argument("--bump", choices=["major", "minor", "patch"], default="patch", help="Type semver-bump")
    parser.add_argument("--version", help="Sett eksakt versjon (overstyrer --bump)")
    parser.add_argument("--date", help="Release-dato (YYYY-MM-DD)")
    parser.add_argument("--dry-run", action="store_true", help="Vis endringer uten å skrive filer")
    parser.add_argument("--json", action="store_true", help="Skriv release-resultat som JSON")


def run(args) -> int:
    payload = prepare_release(
        version=getattr(args, "version", None),
        bump=getattr(args, "bump", "patch"),
        dry_run=getattr(args, "dry_run", False),
        release_date=getattr(args, "date", None),
    )
    if getattr(args, "json", False):
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        mode = "DRY-RUN" if payload["dry_run"] else "SKREVET"
        print(f"Release: {mode}")
        print(f"Versjon: {payload['old_version']} -> {payload['new_version']}")
        print(f"Dato: {payload['release_date']}")
        print(f"Pyproject: {'oppdatert' if payload['changed_pyproject'] else 'uendret'} ({payload['pyproject']})")
        print(f"Changelog: {'oppdatert' if payload['changed_changelog'] else 'uendret'} ({payload['changelog']})")
    return 0


RELEASE_COMMAND = CommandModule(
    name="release",
    help="Forbered release (versjonsbump + changelog)",
    register_arguments=register_arguments,
    run=run,
)
