"""legacy_main.py — AVVIKLA Python-fallback entrypoint.

Berre tilgjengeleg via bin/nc --legacy-python-fallback.
Alle primærkommandoar er Python-fri via nc-vm.
"""
from __future__ import annotations

import sys


_AVVIKLA = {
    "serve":        "norcode.server_runtime",
    "smoke":        "norcode.quality_suites",
    "bench":        "norcode.quality_suites",
    "migrate-names": "norcode.migrations",
    "diagnose":     "norcode.diagnostics",
    "stress":       "norcode.quality_suites",
    "fuzz":         "norcode.fuzz",
    "add":          "norcode.package_registry",
    "update":       "norcode.package_registry",
    "lock":         "norcode.package_registry",
    "packages":     "norcode.package_registry",
    "scaffold-api": "norcode.scaffold",
}

_ERSTATNING = {
    "smoke":   "nc test",
    "bench":   "nc test",
    "diagnose": "nc doctor",
}


def main() -> None:
    if len(sys.argv) < 2:
        print("norcode (legacy): bruk bin/nc <kommando>")
        sys.exit(1)

    cmd = sys.argv[1]

    if cmd in ("--help", "-h", "help"):
        print("norcode – Norscode CLI (pip-installert, avvikla)\n")
        print("Primærbruk: bin/nc <kommando>  (Python-fri via nc-vm)")
        print("\nLegacy-kommandoar (avvikla):")
        for k in sorted(_AVVIKLA):
            print(f"  {k}")
        sys.exit(0)

    if cmd == "test":
        import os, subprocess
        root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        result = subprocess.run(["sh", os.path.join(root, "tools", "nc_test.sh")] + sys.argv[2:])
        sys.exit(result.returncode)

    if cmd in _AVVIKLA:
        erstatning = _ERSTATNING.get(cmd)
        if erstatning:
            print(
                f"nc {cmd}: avvikla. Bruk '{erstatning}' i staden.",
                file=sys.stderr,
            )
        else:
            print(
                f"nc {cmd}: avvikla – berre tilgjengeleg via --legacy-python-fallback.",
                file=sys.stderr,
            )
        _run_legacy_cmd(cmd, sys.argv[2:])
        return

    print(
        f"norcode (legacy): ukjend kommando '{cmd}'. "
        "Alle primærkommandoar er Python-fri via bin/nc.",
        file=sys.stderr,
    )
    sys.exit(2)



def _run_legacy_cmd(cmd: str, args: list[str]) -> None:
    """Køyr avvikla Python-kommando direkte utan cli_parser."""
    import os
    sys.argv = ["nc", cmd] + args

    if cmd == "serve":
        try:
            from norcode.server_runtime import serve_program
        except ImportError:
            print("nc serve: Python-runtime ikkje tilgjengeleg (avvikla)", file=sys.stderr)
            sys.exit(2)
        host = "127.0.0.1"
        port = 8000
        source = "app.no"
        for i, a in enumerate(args):
            if a == "--host" and i + 1 < len(args):
                host = args[i + 1]
            if a == "--port" and i + 1 < len(args):
                port = int(args[i + 1])
            if not a.startswith("-"):
                source = a
        serve_program(source_file=source, host=host, port=port)
        return

    if cmd in ("smoke", "bench", "stress"):
        from norcode.quality_suites import run_benchmark_suite, run_smoke_suite
        if cmd == "smoke":
            r = run_smoke_suite()
        else:
            r = run_benchmark_suite()
        print("OK" if r.get("ok") else "FEIL")
        sys.exit(0 if r.get("ok") else 1)

    if cmd == "migrate-names":
        from norcode.migrations import migrate_names
        apply = "--apply" in args
        cleanup = "--cleanup" in args
        r = migrate_names(apply_changes=apply, cleanup_legacy=cleanup)
        print(f"Oppsummert: copied={r['copied']} planned={r['planned']} skipped={r['skipped']}")
        sys.exit(1 if ("--check" in args and r.get("needs_migration")) else 0)

    if cmd == "diagnose":
        from norcode.diagnostics import run_diagnostics
        import json
        r = run_diagnostics()
        if "--json" in args:
            print(json.dumps(r, ensure_ascii=False, indent=2))
        else:
            print(f"Status: {r.get('status', 'ukjend')}")
        return

    if cmd == "fuzz":
        from norcode.fuzz import run_fuzz_suite
        r = run_fuzz_suite()
        sys.exit(0 if r else 1)

    if cmd in ("add", "update", "lock", "packages"):
        from norcode.package_registry import (
            add_dependency, update_dependencies, generate_lockfile,
            list_registry_packages,
        )
        if cmd == "add" and args:
            add_dependency(args[0])
        elif cmd == "update":
            update_dependencies()
        elif cmd == "lock":
            generate_lockfile()
        elif cmd == "packages":
            _, pkgs = list_registry_packages()
            for name in sorted(pkgs or {}):
                print(name)
        return

    if cmd == "scaffold-api":
        from norcode.scaffold import scaffold_api
        scaffold_api(args[0] if args else ".")
        return

    print(f"nc {cmd}: ikkje implementert i legacy-fallback", file=sys.stderr)
    sys.exit(2)


if __name__ == "__main__":
    main()
