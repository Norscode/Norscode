"""Modular dependency and registry commands."""

from __future__ import annotations

import json

from norcode.commands.base import CommandModule
from norcode.package_registry import (
    add_dependency as package_add_dependency,
    generate_lockfile as package_generate_lockfile,
    list_registry_packages as package_list_registry_packages,
    registry_mirror as package_registry_mirror,
    registry_sign as package_registry_sign,
    registry_sync as package_registry_sync,
    update_dependencies as package_update_dependencies,
    verify_lockfile as package_verify_lockfile,
    _render_git_dependency,
    _render_url_dependency,
)


def _render_registry_packages() -> None:
    config_path, entries = package_list_registry_packages()
    print(f"Prosjektkonfig: {config_path}")
    if not entries:
        print("Registry: ingen pakker funnet")
        return
    print(f"Registry: {len(entries)} pakker")
    for name, meta in sorted(entries.items(), key=lambda item: item[0]):
        desc = meta.get("description")
        desc_text = f" - {desc}" if isinstance(desc, str) and desc.strip() else ""
        version_text = f" @ {meta.get('version')}" if isinstance(meta.get("version"), str) else ""
        source_text = f" [{meta.get('source')}]" if isinstance(meta.get("source"), str) else ""
        if meta.get("kind") == "path":
            target = str(meta["path"])
        elif meta.get("kind") == "git":
            target = _render_git_dependency(meta["git"], meta.get("ref"))
        elif meta.get("kind") == "url":
            target = _render_url_dependency(meta["url"])
        else:
            target = "<ukjent>"
        print(f"  {name}{version_text} => {target}{source_text}{desc_text}")


def register_add_arguments(parser) -> None:
    parser.add_argument("package", nargs="?", help="Pakkenavn eller pakkesti")
    parser.add_argument("path", nargs="?", help="Valgfri pakkesti (hvis package er navn)")
    parser.add_argument("--name", help="Overstyr dependency-navn")
    parser.add_argument("--list", action="store_true", help="Vis tilgjengelige pakker i registry")
    parser.add_argument("--git", help="Direkte Git-kilde (f.eks. https://github.com/org/repo.git)")
    parser.add_argument("--ref", help="Git ref (tag/branch/commit) brukt sammen med --git")
    parser.add_argument("--url", help="Direkte URL-kilde (f.eks. tarball/zip)")
    parser.add_argument("--fetch", action="store_true", help="Last ned/cach ekstern Git/URL-kilde til lokal mappe")
    parser.add_argument("--refresh", action="store_true", help="Tving ny nedlasting ved --fetch")
    parser.add_argument("--pin", action="store_true", help="Krev låst versjon/ref for ekstern kilde")
    parser.add_argument("--sha256", help="Forventet SHA256 for URL-arkiv ved --fetch")
    parser.add_argument("--allow-untrusted", action="store_true", help="Overstyr trusted host-policy for denne kommandoen")


def run_add(args) -> int:
    if args.list:
        _render_registry_packages()
        return 0

    if not args.package:
        raise RuntimeError("Mangler pakkenavn. Bruk: add <pakke> eller add --list")

    config_path, dep_name, dep_value, package_name, dep_kind, changed = package_add_dependency(
        args.package,
        package_path=args.path,
        dep_name_override=args.name,
        git_url=args.git,
        git_ref=args.ref,
        tarball_url=args.url,
        fetch=args.fetch,
        refresh=args.refresh,
        pin=args.pin,
        expected_sha256=args.sha256,
        allow_untrusted=args.allow_untrusted,
    )
    print(f"Konfig: {config_path}")
    print(f"Pakke: {package_name}")
    print(f"Kilde: {dep_kind}")
    print(f"Dependency: {dep_name} = \"{dep_value}\"")
    print("Status: oppdatert" if changed else "Status: uendret")
    return 0


def register_lock_arguments(parser) -> None:
    parser.add_argument("--check", action="store_true", help="Feil hvis lockfile er manglende/utdatert")
    parser.add_argument("--verify", action="store_true", help="Verifiser path-digests i eksisterende lockfile")
    parser.add_argument("--json", action="store_true", help="Skriv lock-resultat som JSON")


def run_lock(args) -> int:
    if args.verify:
        lock_path, ok, results = package_verify_lockfile()
        if args.json:
            print(json.dumps({"lockfile": str(lock_path), "ok": ok, "verify": True, "results": results}, ensure_ascii=False, indent=2))
        else:
            print(f"Lockfile: {lock_path}")
            print("Verify:")
            for row in results:
                print(f"  {row['name']}: {row['status']}")
        return 0 if ok else 1

    lock_path, ok, status = package_generate_lockfile(check_only=args.check)
    if args.json:
        print(json.dumps({"lockfile": str(lock_path), "ok": ok, "status": status, "check": args.check}, ensure_ascii=False, indent=2))
    else:
        print(f"Lockfile: {lock_path}")
        print(f"Status: {status}")
    return 0 if (ok or not args.check) else 1


def register_update_arguments(parser) -> None:
    parser.add_argument("package", nargs="?", help="Valgfri dependency å oppdatere")
    parser.add_argument("--check", action="store_true", help="Feil hvis en dependency ville blitt oppdatert")
    parser.add_argument("--json", action="store_true", help="Skriv update-resultat som JSON")
    parser.add_argument("--pin", action="store_true", help="Krev låst ref for registry git-kilder")
    parser.add_argument("--fetch", action="store_true", help="Materialiser registry git/url-kilder til lokal cache")
    parser.add_argument("--refresh", action="store_true", help="Tving ny nedlasting ved --fetch")
    parser.add_argument("--lock", action="store_true", help="Regenerer lockfile etter oppdatering")
    parser.add_argument("--allow-untrusted", action="store_true", help="Overstyr trusted host-policy for denne kommandoen")


def run_update(args) -> int:
    payload = package_update_dependencies(
        package=args.package,
        check_only=args.check,
        pin=args.pin,
        fetch=args.fetch,
        refresh=args.refresh,
        with_lock=args.lock,
        allow_untrusted=args.allow_untrusted,
    )
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"Konfig: {payload['config']}")
        print(f"Target: {payload['target']}")
        print(f"Updated: {payload['updated']}")
        print(f"Unchanged: {payload['unchanged']}")
        print(f"Skipped: {payload['skipped']}")
        for item in payload["items"]:
            name = item["name"]
            status = item["status"]
            if status == "updated":
                print(f"  {name}: oppdatert -> {item['to']}")
            elif status == "unchanged":
                print(f"  {name}: uendret")
            else:
                print(f"  {name}: hoppet over ({item.get('reason', 'ukjent')})")
        if payload.get("lock"):
            print(f"Lockfile: {payload['lock']['path']} ({payload['lock']['status']})")
    if args.check and payload["updated"] > 0:
        return 1
    return 0


def register_registry_sign_arguments(parser) -> None:
    parser.add_argument("--write-config", action="store_true", help="Skriv hash til [security].trusted_registry_sha256")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def run_registry_sign(args) -> int:
    payload = package_registry_sign(write_config=args.write_config)
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"Registry: {payload['registry']}")
        print(f"SHA256: {payload['sha256']}")
        if payload["written_to_config"]:
            print(f"Konfig: {payload['config']}")
            print("Status: oppdatert" if payload["config_changed"] else "Status: uendret")
    return 0


def register_registry_sync_arguments(parser) -> None:
    parser.add_argument("--source", help="Overstyr registry source for denne kjøringen")
    parser.add_argument("--allow-untrusted", action="store_true", help="Overstyr trusted host-policy for denne kommandoen")
    parser.add_argument("--require-all", action="store_true", help="Feil hvis en eneste source feiler")
    parser.add_argument("--no-fallback", action="store_true", help="Ikke bruk eksisterende cache ved source-feil")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def run_registry_sync(args) -> int:
    payload = package_registry_sync(
        source_override=args.source,
        allow_untrusted=args.allow_untrusted,
        require_all=args.require_all,
        fallback_to_cache=not args.no_fallback,
    )
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"Cache: {payload['cache']}")
        print(f"Kilder: {len(payload['sources'])}")
        for src in payload["sources"]:
            print(f"  - {src}")
        print(f"Pakker i cache: {payload['count']}")
        if payload.get("failed_sources"):
            print("Feilede kilder:")
            for row in payload["failed_sources"]:
                print(f"  - {row['source']}: {row['error']}")
        if payload.get("stale_fallback_used"):
            print("Fallback: bruker eksisterende cache")
    return 0


def register_registry_mirror_arguments(parser) -> None:
    parser.add_argument("--output", help="Output-fil for mirror (default: build/registry_mirror.json)")
    parser.add_argument("--json", action="store_true", help="Skriv resultat som JSON")


def run_registry_mirror(args) -> int:
    payload = package_registry_mirror(output_file=args.output)
    if args.json:
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"Mirror: {payload['output']}")
        print(f"Pakker: {payload['count']}")
    return 0


ADD_COMMAND = CommandModule(
    name="add",
    help="Legg til pakkeavhengighet i norcode.toml",
    register_arguments=register_add_arguments,
    run=run_add,
)

LOCK_COMMAND = CommandModule(
    name="lock",
    help="Generer dependency lockfile (norcode.lock)",
    register_arguments=register_lock_arguments,
    run=run_lock,
)

UPDATE_COMMAND = CommandModule(
    name="update",
    help="Oppdater dependencies fra registry",
    register_arguments=register_update_arguments,
    run=run_update,
)

REGISTRY_SIGN_COMMAND = CommandModule(
    name="registry-sign",
    help="Beregn/pinn SHA256 for packages/registry.toml",
    register_arguments=register_registry_sign_arguments,
    run=run_registry_sign,
)

REGISTRY_SYNC_COMMAND = CommandModule(
    name="registry-sync",
    help="Synkroniser remote registry-indeks til lokal cache",
    register_arguments=register_registry_sync_arguments,
    run=run_registry_sync,
)

REGISTRY_MIRROR_COMMAND = CommandModule(
    name="registry-mirror",
    help="Bygg distribuerbar registry-speilfil fra lokale+remote entries",
    register_arguments=register_registry_mirror_arguments,
    run=run_registry_mirror,
)
