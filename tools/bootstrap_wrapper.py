#!/usr/bin/env python3
"""
norcode-bootstrap-compile — Python VM bootstrap compiler (avvikla Python-delar)

Handterer env-var-protokollen som C VM-et. Ingen Python-kompilatorimport —
all kompilering delegerer til dist/nc-vm (C-binary).

Protokoll:
  NORCODE_BOOTSTRAP_CLI=1  → CLI-modus (selfcheck, test, identity …)
  NORCODE_BOOTSTRAP_VM=1   → Kompilator-modus (run/build via nc-vm)
  (ingen)                  → Native-modus (ikkje tilgjengeleg)
"""
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
NC_VM = ROOT / "dist" / "nc-vm"


def main() -> int:
    if os.environ.get("NORCODE_BOOTSTRAP_CLI"):
        return _cli_mode()
    if os.environ.get("NORCODE_BOOTSTRAP_VM"):
        return _compiler_mode()
    return _native_mode()


# ─── CLI-modus ───────────────────────────────────────────────────────────────

def _cli_mode() -> int:
    argc = int(os.environ.get("NORCODE_ARGC", "1"))
    args = [os.environ.get(f"NORCODE_ARG{i}", "") for i in range(max(argc, 1))]
    cmd = args[0] if args else "help"

    if cmd in ("selfcheck", "test", "identity", "release", "release-targets"):
        return _selfcheck()

    if cmd in ("help", "--help", "-h"):
        _print_help()
        return 0

    print(f"norcode: ukjent CLI-kommando: {cmd}", file=sys.stderr)
    return 1


def _selfcheck() -> int:
    """Røyk-test via nc-vm --nc-run på standard testfiler."""
    test_names = [
        "test_if.no", "test_math.no", "test_assert.no", "test_for.no",
        "test_while.no", "test_elif.no", "test_selfhost_ifexpr_v21.no",
        "test_selfhost_indexset_v22.no", "test_empty_string_list.no",
    ]
    if not NC_VM.exists():
        print(f"norcode: nc-vm ikkje funnen: {NC_VM}", file=sys.stderr)
        return 1

    passed = 0
    failed = []
    for name in test_names:
        f = ROOT / "tests" / name
        if not f.exists():
            continue
        r = subprocess.run(
            [str(NC_VM), "--nc-run", str(f)],
            capture_output=True, text=True,
        )
        if r.returncode == 0:
            passed += 1
        else:
            failed.append((name, r.stderr.strip() or r.stdout.strip()))

    total = passed + len(failed)
    if not failed:
        print(f"selfcheck: {passed}/{total} OK")
        return 0
    for fname, err in failed:
        print(f"  FEIL: {fname}: {err[:120]}", file=sys.stderr)
    print(f"selfcheck: {passed}/{total} OK — FEILA", file=sys.stderr)
    return 1


def _print_help() -> None:
    print("Norscode Bootstrap Compiler (nc-vm, Python-fri)")
    print("  selfcheck   Røyk-test via nc-vm")
    print("  test        Alias for selfcheck")
    print("  identity    Kompilatoridentitet")
    print("  help        Vis dette")


# ─── Kompilator-modus ────────────────────────────────────────────────────────

def _compiler_mode() -> int:
    cmd = os.environ.get("NORCODE_CMD", "build")
    src = os.environ.get("NORCODE_SRC", "")
    out = os.environ.get("NORCODE_OUT", "")
    arch = os.environ.get("NORCODE_ARCH", "aarch64")

    if not src:
        print("norcode: NORCODE_SRC manglar", file=sys.stderr)
        return 11
    if not out and cmd == "build":
        print("norcode: NORCODE_OUT manglar", file=sys.stderr)
        return 12

    if cmd == "run":
        return _run_source(src)
    if cmd == "build":
        return _build_source(src, out, arch)
    print(f"norcode: ukjent NORCODE_CMD: {cmd}", file=sys.stderr)
    return 13


def _run_source(src: str) -> int:
    """Køyr .no-program via nc-vm --nc-run."""
    r = subprocess.run([str(NC_VM), "--nc-run", src])
    return r.returncode


def _build_source(src: str, out: str, arch: str) -> int:
    """Kompiler .no-program til NCB JSON via nc-vm --nc-compile."""
    out_path = out if out else str(Path(src).with_suffix(".ncb.json"))
    r = subprocess.run([str(NC_VM), "--nc-compile", src, out_path])
    return r.returncode


# ─── Native-modus ────────────────────────────────────────────────────────────

def _native_mode() -> int:
    print(
        "norcode: native-modus ikkje tilgjengeleg i Python VM-wrapper.\n"
        "Bruk: NORCODE_BOOTSTRAP_VM=1 eller NORCODE_BOOTSTRAP_CLI=1",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())
